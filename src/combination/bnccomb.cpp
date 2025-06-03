/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncComb
 *
 * Purpose:    Combinations of Orbit/Clock Corrections
 *
 * Author:     L. Mervart
 *
 * Created:    22-Jan-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <newmatio.h>
#include <iomanip>
#include <sstream>
#include <map>

#include "bnccomb.h"
#include <combination/bncbiassnx.h>
#include "bnccore.h"
#include "upload/bncrtnetdecoder.h"
#include "bncsettings.h"
#include "bncutils.h"
#include "bncantex.h"
#include "t_prn.h"

const double sig0_offAC    = 1000.0;
const double sig0_offACSat =  100.0;
const double sigP_offACSat =   0.01;
const double sig0_clkSat   =  100.0;

const double sigObs        =   0.05;

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncComb::cmbParam::cmbParam(parType type_, int index_, const QString& ac_, const QString& prn_) {

  type   = type_;
  index  = index_;
  AC     = ac_;
  prn    = prn_;
  xx     = 0.0;
  eph    = 0;

  if      (type == offACgnss) {
    epoSpec = true;
    sig0    = sig0_offAC;
    sigP    = sig0;
  }
  else if (type == offACSat) {
    epoSpec = false;
    sig0    = sig0_offACSat;
    sigP    = sigP_offACSat;
  }
  else if (type == clkSat) {
    epoSpec = true;
    sig0    = sig0_clkSat;
    sigP    = sig0;
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncComb::cmbParam::~cmbParam() {
}

// Partial
////////////////////////////////////////////////////////////////////////////
double bncComb::cmbParam::partial(char sys, const QString& AC_, const QString& prn_) {

  if      (type == offACgnss) {
    if (AC == AC_ && prn_[0].toLatin1() == sys) {
      return 1.0;
    }
  }
  else if (type == offACSat) {
    if (AC == AC_ && prn == prn_) {
      return 1.0;
    }
  }
  else if (type == clkSat) {
    if (prn == prn_) {
      return 1.0;
    }
  }

  return 0.0;
}

//
////////////////////////////////////////////////////////////////////////////
QString bncComb::cmbParam::toString(char sys) const {

  QString outStr;

  if      (type == offACgnss) {
    outStr = "AC  Offset " + AC + " " + sys + "  ";
  }
  else if (type == offACSat) {
    outStr = "Sat Offset " + AC + " " + prn.mid(0,3);
  }
  else if (type == clkSat) {
    outStr = "Clk Corr " + prn.mid(0,3);
  }

  return outStr;
}

// Singleton: definition class variable
////////////////////////////////////////////////////////////////////////////
bncComb* bncComb::instance = nullptr;


// Constructor
////////////////////////////////////////////////////////////////////////////
bncComb::bncComb() : _ephUser(true) {

  bncSettings settings;

  _running = true;

  QStringList cmbStreams = settings.value("cmbStreams").toStringList();

  _cmbSampl = settings.value("cmbSampl").toInt();
  if (_cmbSampl <= 0) {
    _cmbSampl = 5;
  }
  _useGps = (Qt::CheckState(settings.value("cmbGps").toInt()) == Qt::Checked) ? true : false;
  if (_useGps) {
    _cmbSysPrn['G'] = t_prn::MAXPRN_GPS;
    _masterMissingEpochs['G'] = 0;
  }
  _useGlo = (Qt::CheckState(settings.value("cmbGlo").toInt()) == Qt::Checked) ? true : false;
  if (_useGlo) {
    _cmbSysPrn['R'] = t_prn::MAXPRN_GLONASS;
    _masterMissingEpochs['R'] = 0;
  }
  _useGal = (Qt::CheckState(settings.value("cmbGal").toInt()) == Qt::Checked) ? true : false;
  if (_useGal) {
    _cmbSysPrn['E'] = t_prn::MAXPRN_GALILEO;
    _masterMissingEpochs['E'] = 0;
  }
  _useBds = (Qt::CheckState(settings.value("cmbBds").toInt()) == Qt::Checked) ? true : false;
  if (_useBds) {
    _cmbSysPrn['C'] = t_prn::MAXPRN_BDS;
    _masterMissingEpochs['C'] = 0;
  }
  _useQzss = (Qt::CheckState(settings.value("cmbQzss").toInt()) == Qt::Checked) ? true : false;
  if (_useQzss) {
    _cmbSysPrn['J'] = t_prn::MAXPRN_QZSS;
    _masterMissingEpochs['J'] = 0;
  }
  _useSbas = (Qt::CheckState(settings.value("cmbSbas").toInt()) == Qt::Checked) ? true : false;
  if (_useSbas) {
    _cmbSysPrn['S'] = t_prn::MAXPRN_SBAS;
    _masterMissingEpochs['S'] = 0;
  }
  _useNavic = (Qt::CheckState(settings.value("cmbNavic").toInt()) == Qt::Checked) ? true : false;
  if (_useNavic) {
    _cmbSysPrn['I'] = t_prn::MAXPRN_NavIC;
    _masterMissingEpochs['I'] = 0;
  }

  if (cmbStreams.size() >= 1 && !cmbStreams[0].isEmpty()) {
    QListIterator<QString> it(cmbStreams);
    while (it.hasNext()) {
      QStringList hlp = it.next().split(" ");
      cmbAC* newAC = new cmbAC();
      newAC->mountPoint   = hlp[0];
      newAC->name         = hlp[1];
      newAC->weightFactor = hlp[2].toDouble();
      newAC->excludeSats  = hlp[3].split(QRegExp("[ ,]"), Qt::SkipEmptyParts);
      newAC->isAPC        = bool(newAC->mountPoint.mid(0,4) == "SSRA");
      QMapIterator<char, unsigned> itSys(_cmbSysPrn);
      // init
      while (itSys.hasNext()) {
        itSys.next();
        char sys = itSys.key();
        if (!_masterOrbitAC.contains(sys) &&
            !newAC->excludeSats.contains(QString(sys), Qt::CaseSensitive)) {
          _masterOrbitAC[sys] = newAC->name;
          _masterIsAPC[sys]   = newAC->isAPC;
        }
      }
      _ACs.append(newAC);
    }
  }

  QString ssrFormat;
  _ssrCorr = 0;
  QListIterator<QString> it(settings.value("uploadMountpointsOut").toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(",");
    if (hlp.size() > 7) {
      ssrFormat = hlp[7];
    }
  }
  if      (ssrFormat == "IGS-SSR") {
    _ssrCorr = new SsrCorrIgs();
  }
  else if (ssrFormat == "RTCM-SSR") {
    _ssrCorr = new SsrCorrRtcm();
  }
  else { // default
    _ssrCorr = new SsrCorrIgs();
  }

  _rtnetDecoder = new bncRtnetDecoder();

  connect(this, SIGNAL(newMessage(QByteArray,bool)), BNC_CORE, SLOT(slotMessage(const QByteArray,bool)));
  connect(BNC_CORE, SIGNAL(providerIDChanged(QString)), this, SLOT(slotProviderIDChanged(QString)));
  connect(BNC_CORE, SIGNAL(newOrbCorrections(QList<t_orbCorr>)), this, SLOT(slotNewOrbCorrections(QList<t_orbCorr>)));
  connect(BNC_CORE, SIGNAL(newClkCorrections(QList<t_clkCorr>)), this, SLOT(slotNewClkCorrections(QList<t_clkCorr>)));
  connect(BNC_CORE, SIGNAL(newCodeBiases(QList<t_satCodeBias>)), this, SLOT(slotNewCodeBiases(QList<t_satCodeBias>)));

  // Combination Method
  // ------------------
  if (settings.value("cmbMethod").toString() == "Single-Epoch") {
    _method = singleEpoch;
  }
  else {
    _method = filter;
  }

  // Initialize Parameters (model: Clk_Corr = AC_Offset + Sat_Offset + Clk)
  // ----------------------------------------------------------------------
  if (_method == filter) {
    // SYSTEM
    QMapIterator<char, unsigned> itSys(_cmbSysPrn);
    while (itSys.hasNext()) {
      itSys.next();
      int nextPar = 0;
      char sys = itSys.key();
      int maxPrn = itSys.value();
      // AC
      QListIterator<cmbAC*> itAc(_ACs);
      while (itAc.hasNext()) {
        cmbAC* AC = itAc.next();
        _params[sys].push_back(new cmbParam(cmbParam::offACgnss, ++nextPar, AC->name, ""));
        for (int iGnss = 1; iGnss <= maxPrn; iGnss++) {
          int flag = t_corrSSR::getSsrNavTypeFlag(sys, iGnss);
          QString prn = QString("%1%2_%3").arg(sys).arg(iGnss, 2, 10, QChar('0')).arg(flag);
          _params[sys].push_back(new cmbParam(cmbParam::offACSat, ++nextPar, AC->name, prn));
        }
      }
      for (int iGnss = 1; iGnss <= maxPrn; iGnss++) {
        int flag = t_corrSSR::getSsrNavTypeFlag(sys, iGnss);
        QString prn = QString("%1%2_%3").arg(sys).arg(iGnss, 2, 10, QChar('0')).arg(flag);
        _params[sys].push_back(new cmbParam(cmbParam::clkSat, ++nextPar, "", prn));
      }
      // Initialize Variance-Covariance Matrix
      // -------------------------------------
      _QQ[sys].ReSize(_params[sys].size());
      _QQ[sys] = 0.0;
      for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
        cmbParam* pp = _params[sys][iPar-1];
        _QQ[sys](iPar,iPar) = pp->sig0 * pp->sig0;
      }
    }
  }

  // ANTEX File
  // ----------
  _antex = 0;
  QString antexFileName = settings.value("uploadAntexFile").toString();
  if (!antexFileName.isEmpty()) {
    _antex = new bncAntex();
    if (_antex->readFile(antexFileName) != success) {
      emit newMessage("bncComb: wrong ANTEX file", true);
      delete _antex;
      _antex = 0;
    }
  }


  // Bias SINEX File
  // ---------------
  _bsx = 0;
  QString bsxFileName = settings.value("cmbBsxFile").toString();
  if (!bsxFileName.isEmpty()) {
    _bsx = new bncBiasSnx();
    if (_bsx->readFile(bsxFileName) != success) {
      emit newMessage("bncComb: wrong Bias SINEX file", true);
      delete _bsx;
      _bsx = 0;
    }
  }
  if (_bsx) {
    _ms = 3.0 * 3600 * 1000.0;
    QTimer::singleShot(_ms, this, SLOT(slotReadBiasSnxFile()));
  }

  // Maximal Residuum
  // ----------------
  _MAX_RES = settings.value("cmbMaxres").toDouble();
  if (_MAX_RES <= 0.0) {
    _MAX_RES = 999.0;
  }

  // Maximal Displacement
  // --------------------
  _MAX_DISPLACEMENT = settings.value("cmbMaxdisplacement").toDouble();
  if (_MAX_DISPLACEMENT <= 0.0) {
    _MAX_DISPLACEMENT = 2.0;
  }

  // Logfile
  // -------
  QString intr = "1 day";
  QString logFileCmb = settings.value("cmbLogpath").toString();
  int l = logFileCmb.length();
  if (logFileCmb.isEmpty()) {
    _logFile = 0;
  }
  else {
    if (l && logFileCmb[l-1] != QDir::separator() ) {
      logFileCmb += QDir::separator();
    }

    logFileCmb = logFileCmb + "COMB00BNC" + "${V3PROD}" + ".CMB";

    _logFile = new bncoutf(logFileCmb, intr, _cmbSampl);
  }

  _newCorr = 0;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncComb::~bncComb() {

  _running = false;
#ifndef WIN32
  sleep(2);
#else
  Sleep(2000);
#endif

  QListIterator<cmbAC*> icAC(_ACs);
  while (icAC.hasNext()) {
    delete icAC.next();
  }
  _ACs.clear();

  delete _rtnetDecoder;

  if (_ssrCorr) {
    delete _ssrCorr;
  }

  if (_newCorr) {
    delete _newCorr;
  }

  delete _antex;
  delete _bsx;
  delete _logFile;

  QMapIterator<char, unsigned> itSys(_cmbSysPrn);
  while (itSys.hasNext()) {
    itSys.next();
    char sys = itSys.key();
    for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
      delete _params[sys][iPar-1];
    }
    _buffer.remove(sys);
  }
  _params.clear();
  _buffer.clear();
  _cmbSysPrn.clear();

  while (!_epoClkData.empty()) {
    delete _epoClkData.front();
    _epoClkData.pop_front();
  }
}

void bncComb::slotReadBiasSnxFile() {
  bncSettings settings;
  QString bsxFileName = settings.value("cmbBsxFile").toString();

  if (bsxFileName.isEmpty()) {
    emit newMessage("bncComb: no Bias SINEX file specified", true);
    return;
  }
  if (!_bsx) {
    _bsx = new bncBiasSnx();
  }
  if (_bsx->readFile(bsxFileName) != success) {
    emit newMessage("bncComb: wrong Bias SINEX file", true);
    delete _bsx;
    _bsx = 0;
  }
  else {
    emit newMessage("bncComb: successfully read Bias SINEX file", true);
  }

  QTimer::singleShot(_ms, this, SLOT(slotReadBiasSnxFile()));
}


// Remember orbit corrections
////////////////////////////////////////////////////////////////////////////
void bncComb::slotNewOrbCorrections(QList<t_orbCorr> orbCorrections) {
  QMutexLocker locker(&_mutex);
  for (int ii = 0; ii < orbCorrections.size(); ii++) {
    t_orbCorr& orbCorr = orbCorrections[ii];
    QString    staID(orbCorr._staID.c_str());
    char       sys = orbCorr._prn.system();

    if (!_cmbSysPrn.contains(sys)){
      continue;
    }

    // Find/Check the AC Name
    // ----------------------
    QString acName;
    QStringList excludeSats;
    QListIterator<cmbAC*> icAC(_ACs);
    while (icAC.hasNext()) {
      cmbAC* AC = icAC.next();
      if (AC->mountPoint == staID) {
        acName      = AC->name;
        excludeSats = AC->excludeSats;
        break;
      }
    }
    if (acName.isEmpty() || excludeSat(orbCorr._prn, excludeSats)) {
      continue;
    }

    // Store the correction
    // --------------------
    QMap<t_prn, t_orbCorr>& storage = _orbCorrections[acName];
    storage[orbCorr._prn] = orbCorr;
  }
}

// Remember Satellite Code Biases
////////////////////////////////////////////////////////////////////////////
void bncComb::slotNewCodeBiases(QList<t_satCodeBias> satCodeBiases) {
  QMutexLocker locker(&_mutex);

  for (int ii = 0; ii < satCodeBiases.size(); ii++) {
    t_satCodeBias& satCodeBias = satCodeBiases[ii];
    QString    staID(satCodeBias._staID.c_str());
    char       sys = satCodeBias._prn.system();

    if (!_cmbSysPrn.contains(sys)){
      continue;
    }

    // Find/Check the AC Name
    // ----------------------
    QString acName;
    QStringList excludeSats;
    QListIterator<cmbAC*> icAC(_ACs);
    while (icAC.hasNext()) {
      cmbAC* AC = icAC.next();
      if (AC->mountPoint == staID) {
        acName      = AC->name;
        excludeSats = AC->excludeSats;
        break;
      }
    }
    if (acName.isEmpty() || excludeSat(satCodeBias._prn, excludeSats)) {
      continue;
    }

    // Store the correction
    // --------------------
    QMap<t_prn, t_satCodeBias>& storage = _satCodeBiases[acName];
    storage[satCodeBias._prn] = satCodeBias;
  }

}

// Process clock corrections
////////////////////////////////////////////////////////////////////////////
void bncComb::slotNewClkCorrections(QList<t_clkCorr> clkCorrections) {
  QMutexLocker locker(&_mutex);
  const double outWait = 1.0 * _cmbSampl;
  int    currentWeek = 0;
  double currentSec  = 0.0;
  currentGPSWeeks(currentWeek, currentSec);
  bncTime currentTime(currentWeek, currentSec);

  // Loop over all observations (possible different epochs)
  // -----------------------------------------------------
  for (int ii = 0; ii < clkCorrections.size(); ii++) {
    t_clkCorr& newClk = clkCorrections[ii];
    char sys = newClk._prn.system();
    if (!_cmbSysPrn.contains(sys)){
      continue;
    }

    // Check Modulo Time
    // -----------------
    int sec = int(nint(newClk._time.gpssec()*10));
    if (sec % (_cmbSampl*10) != 0.0) {
      continue;
    }

    // Find/Check the AC Name
    // ----------------------
    QString acName;
    QStringList excludeSats;
    bool isAPC;
    QString staID(newClk._staID.c_str());
    QListIterator<cmbAC*> icAC(_ACs);
    while (icAC.hasNext()) {
      cmbAC* AC = icAC.next();
      if (AC->mountPoint == staID) {
        acName      = AC->name;
        excludeSats = AC->excludeSats;
        isAPC       = AC->isAPC;
        break;
      }
    }
    if (acName.isEmpty() ||
        isAPC != _masterIsAPC[sys] ||
        excludeSat(newClk._prn, excludeSats)) {
      continue;
    }

    // Check regarding current time
    // ----------------------------
    if (BNC_CORE->mode() != t_bncCore::batchPostProcessing) {
      if (fabs(currentTime - newClk._time) > 60.0) { // future or very old data sets
  #ifdef BNC_DEBUG_CMB
        emit newMessage("bncComb: future or very old data sets: " + acName.toLatin1() + " " + newClk._prn.toString().c_str() +
            QString(" %1 ").arg(newClk._time.timestr().c_str()).toLatin1() + "vs. current time" +
            QString(" %1 ").arg(currentTime.timestr().c_str()).toLatin1(), true);
  #endif
        continue;
      }
    }

    // Check Correction Age
    // --------------------
    if (_resTime.valid() && newClk._time <= _resTime) {
#ifdef BNC_DEBUG_CMB
      emit newMessage("bncComb: old correction: " + acName.toLatin1() + " " + newClk._prn.toString().c_str() +
          QString(" %1 ").arg(newClk._time.timestr().c_str()).toLatin1() + "vs. last processing Epoch" +
          QString(" %1 ").arg(_resTime.timestr().c_str()).toLatin1(), true);
#endif
      continue;
    }

    // Set the last time
    // -----------------
    if (_lastClkCorrTime.undef() || newClk._time > _lastClkCorrTime) {
      _lastClkCorrTime = newClk._time;
    }

    // Find the corresponding data epoch or create a new one
    // -----------------------------------------------------
    epoClkData* epoch = 0;
    deque<epoClkData*>::const_iterator it;
    for (it = _epoClkData.begin(); it != _epoClkData.end(); it++) {
      if (newClk._time == (*it)->_time) {
      epoch = *it;
        break;
      }
    }
    if (epoch == 0) {
      if (_epoClkData.empty() || newClk._time > _epoClkData.back()->_time) {
        epoch = new epoClkData;
        epoch->_time = newClk._time;
        _epoClkData.push_back(epoch);
      }
    }

    // Put data into the epoch
    // -----------------------
    if (epoch != 0) {
      epoch->_clkCorr.push_back(newClk);
    }
  }

  // Make sure the buffer does not grow beyond any limit
  // ---------------------------------------------------
  const unsigned MAX_EPODATA_SIZE = 60.0 / _cmbSampl;
  if (_epoClkData.size() > MAX_EPODATA_SIZE) {
    delete _epoClkData.front();
    _epoClkData.pop_front();
  }

  // Process the oldest epochs
  // ------------------------
  while (_epoClkData.size()) {

    if (!_lastClkCorrTime.valid()) {
      return;
    }

    const vector<t_clkCorr>& clkCorrVec = _epoClkData.front()->_clkCorr;

    bncTime epoTime = _epoClkData.front()->_time;

    if (BNC_CORE->mode() != t_bncCore::batchPostProcessing) {
      if ((currentTime - epoTime) > 60.0) {
        delete _epoClkData.front();
        _epoClkData.pop_front();
        continue;
      }
    }
    // Process the front epoch
    // -----------------------
    if (epoTime < (_lastClkCorrTime - outWait)) {
      _resTime = epoTime;
      processEpoch(_resTime, clkCorrVec);
      delete _epoClkData.front();
      _epoClkData.pop_front();
    }
    else {
      return;
    }
  }
}

// Change the correction so that it refers to last received ephemeris
////////////////////////////////////////////////////////////////////////////
void bncComb::switchToLastEph(t_eph* lastEph, cmbCorr* corr) {

  if (corr->_eph == lastEph) {
    return;
  }

  ColumnVector oldXC(6);
  ColumnVector oldVV(3);
  if (corr->_eph->getCrd(corr->_time, oldXC, oldVV, false) != success) {
    return;
  }

  ColumnVector newXC(6);
  ColumnVector newVV(3);
  if (lastEph->getCrd(corr->_time, newXC, newVV, false) != success) {
    return;
  }

  ColumnVector dX = newXC.Rows(1,3) - oldXC.Rows(1,3);
  ColumnVector dV = newVV           - oldVV;
  double       dC = newXC(4)        - oldXC(4);

  ColumnVector dRAO(3);
  XYZ_to_RSW(newXC.Rows(1,3), newVV, dX, dRAO);

  ColumnVector dDotRAO(3);
  XYZ_to_RSW(newXC.Rows(1,3), newVV, dV, dDotRAO);

  QString msg = "bncComb: switch corr " + corr->_prn.mid(0,3)
    + QString(" %1 -> %2 %3").arg(corr->_iod,3).arg(lastEph->IOD(),3).arg(dC*t_CST::c, 8, 'f', 4);

  emit newMessage(msg.toLatin1(), false);

  corr->_iod = lastEph->IOD();
  corr->_eph = lastEph;

  corr->_orbCorr._xr    += dRAO;
  corr->_orbCorr._dotXr += dDotRAO;
  corr->_clkCorr._dClk  -= dC;
}

// Process Epoch
////////////////////////////////////////////////////////////////////////////
void bncComb::processEpoch(bncTime epoTime, const vector<t_clkCorr>& clkCorrVec) {
  QDateTime now = currentDateAndTimeGPS();
  bncTime   currentTime(now.toString(Qt::ISODate).toStdString());
  for (unsigned ii = 0; ii < clkCorrVec.size(); ii++) {
    const t_clkCorr& clkCorr = clkCorrVec[ii];
    QString    staID(clkCorr._staID.c_str());
    QString    prnStr(clkCorr._prn.toInternalString().c_str());
    char       sys = clkCorr._prn.system();

    // Find/Check the AC Name
    // ----------------------
    QString acName;
    double weigthFactor;
    QListIterator<cmbAC*> icAC(_ACs);
    while (icAC.hasNext()) {
      cmbAC* AC = icAC.next();
      if (AC->mountPoint == staID) {
        acName       = AC->name;
        weigthFactor = AC->weightFactor;
        break;
      }
    }

    // Create new correction
    // ---------------------
    _newCorr                = new cmbCorr();
    _newCorr->_prn          = prnStr;
    _newCorr->_time         = clkCorr._time;
    _newCorr->_iod          = clkCorr._iod;
    _newCorr->_acName       = acName;
    _newCorr->_weightFactor = weigthFactor;
    _newCorr->_clkCorr      = clkCorr;

    // Check orbit correction
    // ----------------------
    if (!_orbCorrections.contains(acName)) {
      delete _newCorr; _newCorr = 0;
      continue;
    }
    else {
      QMap<t_prn, t_orbCorr>& storage = _orbCorrections[acName];
      if (!storage.contains(clkCorr._prn)  ||
           storage[clkCorr._prn]._iod != _newCorr->_iod) {
        delete _newCorr; _newCorr = 0;
        continue;
      }
      else {
        _newCorr->_orbCorr = storage[clkCorr._prn];
      }
    }

    // Check the Ephemeris
    //--------------------
    t_eph* ephLast = _ephUser.ephLast(prnStr);
    t_eph* ephPrev = _ephUser.ephPrev(prnStr);
    if (ephLast == 0) {
#ifdef BNC_DEBUG_CMB
      emit newMessage("bncComb: eph not found for "  + prnStr.mid(0,3).toLatin1(), true);
#endif
      delete _newCorr; _newCorr = 0;
      continue;
    }
    else if (outDatedBcep(ephLast, currentTime)  ||
             ephLast->checkState() == t_eph::bad ||
             ephLast->checkState() == t_eph::outdated ||
             ephLast->checkState() == t_eph::unhealthy) {
#ifdef BNC_DEBUG_CMB
      emit newMessage("bncComb: ephLast not ok (checkState: " +  ephLast->checkStateToString().toLatin1() + ") for "  + prnStr.mid(0,3).toLatin1(), true);
#endif
      delete _newCorr; _newCorr = 0;
      continue;
    }
    else {
      if      (ephLast->IOD() == _newCorr->_iod) {
        _newCorr->_eph = ephLast;
      }
      else if (ephPrev &&
               !outDatedBcep(ephPrev, currentTime) &&  // if not updated in storage
               ephPrev->checkState() == t_eph::ok  &&  // received
               ephPrev->IOD() == _newCorr->_iod) {
        _newCorr->_eph = ephPrev;
        switchToLastEph(ephLast, _newCorr);
      }
      else {
#ifdef BNC_DEBUG_CMB
        emit newMessage("bncComb: eph not found for "  + prnStr.mid(0,3).toLatin1() +
                        QString(" with IOD %1").arg(_newCorr->_iod).toLatin1(), true);
#endif
        delete _newCorr; _newCorr = 0;
        continue;
      }
    }

    // Check satellite code biases
    // ----------------------------
    if (_satCodeBiases.contains(acName)) {
      QMap<t_prn, t_satCodeBias>& storage = _satCodeBiases[acName];
      if (storage.contains(clkCorr._prn)) {
        _newCorr->_satCodeBias = storage[clkCorr._prn];
        QMap<t_frequency::type, double> codeBiasesRefSig;
        for (unsigned ii = 1; ii < cmbRefSig::cIF; ii++) {
          t_frequency::type frqType = cmbRefSig::toFreq(sys, static_cast<cmbRefSig::type>(ii));
          char frqNum = t_frequency::toString(frqType)[1];
          char attrib = cmbRefSig::toAttrib(sys, static_cast<cmbRefSig::type>(ii));
          QString rnxType2ch = QString("%1%2").arg(frqNum).arg(attrib);
          for (unsigned ii = 0; ii < _newCorr->_satCodeBias._bias.size(); ii++) {
            const t_frqCodeBias& bias = _newCorr->_satCodeBias._bias[ii];
            if (rnxType2ch.toStdString() == bias._rnxType2ch) {
              codeBiasesRefSig[frqType] = bias._value;
            }
          }
        }
        if (codeBiasesRefSig.size() == 2) {
          map<t_frequency::type, double> codeCoeff;
          double channel = double(_newCorr->_eph->slotNum());
          cmbRefSig::coeff(sys, cmbRefSig::cIF, channel, codeCoeff);
          map<t_frequency::type, double>::const_iterator it;
          for (it = codeCoeff.begin(); it != codeCoeff.end(); it++) {
            t_frequency::type frqType = it->first;
            _newCorr->_satCodeBiasIF += it->second * codeBiasesRefSig[frqType];
          }
        }
        _newCorr->_satCodeBias._bias.clear();
      }
    }

    // Store correction into the buffer
    // --------------------------------
    QVector<cmbCorr*>& corrs = _buffer[sys].corrs;
    QVectorIterator<cmbCorr*> itCorr(corrs);
    bool available = false;
    while (itCorr.hasNext()) {
      cmbCorr* corr   = itCorr.next();
      QString  prnStr = corr->_prn;
      QString  acName = corr->_acName;
      if (_newCorr->_acName == acName &&
          _newCorr->_prn == prnStr) {
        available = true;
      }
    }

    if (!available) {
      corrs.push_back(_newCorr); _newCorr = 0;
    }
    else {
      delete _newCorr; _newCorr = 0;
      continue;
    }
  }

  // Process Systems of this Epoch
  // ------------------------------
  QMapIterator<char, unsigned> itSys(_cmbSysPrn);
  while (itSys.hasNext()) {
    itSys.next();
    char sys = itSys.key();
    _log.clear();
    QTextStream out(&_log, QIODevice::WriteOnly);
    processSystem(epoTime, sys, out);
    _buffer.remove(sys);
    if (_logFile) {
      _logFile->write(epoTime.gpsw(),epoTime.gpssec(), QString(_log));
    }
  }
}

void bncComb::processSystem(bncTime epoTime, char sys, QTextStream& out) {

  out << "\n"
      << epoTime.datestr().c_str()    << " "
      << epoTime.timestr().c_str()    << " "
      << "Combination: " << sys << "\n"
      << "--------------------------------------" << "\n";

  // Observation Statistics
  // ----------------------
  bool masterPresent = false;
  QListIterator<cmbAC*> icAC(_ACs);
  while (icAC.hasNext()) {
    cmbAC* AC = icAC.next();
    AC->numObs[sys] = 0;
    QVectorIterator<cmbCorr*> itCorr(corrs(sys));
    while (itCorr.hasNext()) {
      cmbCorr* corr = itCorr.next();
      if (corr->_acName == AC->name) {
        AC->numObs[sys] += 1;
        if (AC->name == _masterOrbitAC[sys]) {
          masterPresent = true;
        }
      }
    }
    out << epoTime.datestr().c_str()    << " "
        << epoTime.timestr().c_str()    << " "
        << "Sat Num "         << sys << " "
        << AC->name.toLatin1().data() << ": " << AC->numObs[sys] << "\n";
  }

  // If Master not present, switch to another one
  // --------------------------------------------
  const unsigned switchMasterAfterGap = 1;
  if (masterPresent) {
    _masterMissingEpochs[sys] = 0;
  }
  else {
    ++_masterMissingEpochs[sys];
    if (_masterMissingEpochs[sys] < switchMasterAfterGap) {
      out << "Missing Master, Epoch skipped" << "\n";
      _buffer.remove(sys);
      emit newMessage(_log, false);
      return;
    }
    else {
      _masterMissingEpochs[sys] = 0;
      QListIterator<cmbAC*> icAC(_ACs);
      while (icAC.hasNext()) {
        cmbAC* AC = icAC.next();
        if (AC->numObs[sys] > 0) {
          out <<  epoTime.datestr().c_str()    << " "
              << epoTime.timestr().c_str()     << " "
              << "Switching Master " << sys << " "
              << _masterOrbitAC[sys].toLatin1().data() << " --> "
              << AC->name.toLatin1().data()   << "\n";
          _masterOrbitAC[sys] = AC->name;
          _masterIsAPC[sys]   = AC->isAPC;
          break;
        }
      }
    }
  }

  QMap<QString, cmbCorr*> resCorr;

  // Perform the actual Combination using selected Method
  // ----------------------------------------------------
  t_irc irc;
  ColumnVector dx;
  if (_method == filter) {
    irc = processEpoch_filter(epoTime, sys, out, resCorr, dx);
  }
  else {
    irc = processEpoch_singleEpoch(epoTime, sys, out, resCorr, dx);
  }

  // Update Parameter Values, Print Results
  // --------------------------------------
  if (irc == success) {
    for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
      cmbParam* pp = _params[sys][iPar-1];
      pp->xx += dx(iPar);
      if (pp->type == cmbParam::clkSat) {
        if (resCorr.find(pp->prn) != resCorr.end()) {
          // set clock result
          resCorr[pp->prn]->_dClkResult = pp->xx / t_CST::c;
          // Add Code Biases from SINEX File
          if (_bsx) {
            map<t_frequency::type, double> codeCoeff;
            double channel = double(resCorr[pp->prn]->_eph->slotNum());
            cmbRefSig::coeff(sys, cmbRefSig::cIF, channel, codeCoeff);
            t_frequency::type fType1 = cmbRefSig::toFreq(sys, cmbRefSig::c1);
            t_frequency::type fType2 = cmbRefSig::toFreq(sys, cmbRefSig::c2);
            _bsx->determineSsrSatCodeBiases(pp->prn.mid(0,3), codeCoeff[fType1], codeCoeff[fType2], resCorr[pp->prn]->_satCodeBias);
          }
        }
      }
      out << epoTime.datestr().c_str() << " "
          << epoTime.timestr().c_str() << " ";
      out.setRealNumberNotation(QTextStream::FixedNotation);
      out.setFieldWidth(8);
      out.setRealNumberPrecision(4);
      out << pp->toString(sys) << " "
          << pp->xx << " +- " << sqrt(_QQ[sys](pp->index,pp->index)) << "\n";
      out.setFieldWidth(0);
      out.flush();
    }
    printResults(epoTime, out, resCorr);
    dumpResults(epoTime, resCorr);
  }
}

// Process Epoch - Filter Method
////////////////////////////////////////////////////////////////////////////
t_irc bncComb::processEpoch_filter(bncTime epoTime, char sys, QTextStream& out,
                                   QMap<QString, cmbCorr*>& resCorr,
                                   ColumnVector& dx) {

  // Prediction Step
  // ---------------
  int nPar = _params[sys].size();
  ColumnVector x0(nPar);
  for (int iPar = 1; iPar <= nPar; iPar++) {
    cmbParam* pp  = _params[sys][iPar-1];
    if (pp->epoSpec) {
      pp->xx = 0.0;
      _QQ[sys].Row(iPar)    = 0.0;
      _QQ[sys].Column(iPar) = 0.0;
      _QQ[sys](iPar,iPar) = pp->sig0 * pp->sig0;
    }
    else {
      _QQ[sys](iPar,iPar) += pp->sigP * pp->sigP;
    }
    x0(iPar) = pp->xx;
  }

  // Check Satellite Positions for Outliers
  // --------------------------------------
  if (checkOrbits(epoTime, sys, out) != success) {
    return failure;
  }

  // Update and outlier detection loop
  // ---------------------------------
  SymmetricMatrix QQ_sav = _QQ[sys];
  while (_running) {

    Matrix         AA;
    ColumnVector   ll;
    DiagonalMatrix PP;

    if (createAmat(sys, AA, ll, PP, x0, resCorr) != success) {
      return failure;
    }

    dx.ReSize(nPar); dx = 0.0;
    kalman(AA, ll, PP, _QQ[sys], dx);

    ColumnVector vv = ll - AA * dx;

    int     maxResIndex;
    double  maxRes = vv.maximum_absolute_value1(maxResIndex);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(3);
    out << epoTime.datestr().c_str() << " " << epoTime.timestr().c_str()
        << " Maximum Residuum " << maxRes << ' '
        << corrs(sys)[maxResIndex-1]->_acName << ' ' << corrs(sys)[maxResIndex-1]->_prn.mid(0,3);
    if (maxRes > _MAX_RES) {
      for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
        cmbParam* pp = _params[sys][iPar-1];
        if (pp->type == cmbParam::offACSat            &&
            pp->AC   == corrs(sys)[maxResIndex-1]->_acName &&
            pp->prn  == corrs(sys)[maxResIndex-1]->_prn.mid(0,3)) {
          QQ_sav.Row(iPar)    = 0.0;
          QQ_sav.Column(iPar) = 0.0;
          QQ_sav(iPar,iPar)   = pp->sig0 * pp->sig0;
        }
      }

      out << "  Outlier" << "\n";
      _QQ[sys] = QQ_sav;
      delete corrs(sys)[maxResIndex-1];
      corrs(sys).remove(maxResIndex-1);
    }
    else {
      out << "  OK" << "\n";
      out.setRealNumberNotation(QTextStream::FixedNotation);
      out.setRealNumberPrecision(4);
      for (int ii = 0; ii < corrs(sys).size(); ii++) {
      const cmbCorr* corr = corrs(sys)[ii];
        out << epoTime.datestr().c_str() << ' '
            << epoTime.timestr().c_str() << " "
            << corr->_acName << ' ' << corr->_prn.mid(0,3);
        out.setFieldWidth(10);
        out <<  " res = " << vv[ii] << "\n";
        out.setFieldWidth(0);
      }
      break;
    }
  }

  return success;
}

// Print results
////////////////////////////////////////////////////////////////////////////
void bncComb::printResults(bncTime epoTime, QTextStream& out,
                           const QMap<QString, cmbCorr*>& resCorr) {

  QMapIterator<QString, cmbCorr*> it(resCorr);
  while (it.hasNext()) {
    it.next();
    cmbCorr* corr = it.value();
    const t_eph* eph = corr->_eph;
    if (eph) {
      ColumnVector xc(6);
      ColumnVector vv(3);
      if (eph->getCrd(epoTime, xc, vv, false) != success) {
        continue;
      }
      out << epoTime.datestr().c_str() << " "
          << epoTime.timestr().c_str() << " ";
      out.setFieldWidth(3);
      out << "Full Clock " << corr->_prn.mid(0,3) << " " << corr->_iod << " ";
      out.setFieldWidth(14);
      out << (xc(4) + corr->_dClkResult) * t_CST::c << "\n";
      out.setFieldWidth(0);
      out.flush();
    }
    else {
      out << "bncComb::printResults bug" << "\n";
    }
  }
}

// Send results to RTNet Decoder and directly to PPP Client
////////////////////////////////////////////////////////////////////////////
void bncComb::dumpResults(bncTime epoTime, QMap<QString, cmbCorr*>& resCorr) {

  QList<t_orbCorr> orbCorrections;
  QList<t_clkCorr> clkCorrections;
  QList<t_satCodeBias> satCodeBiasList;

  unsigned year, month, day, hour, minute;
  double   sec;
  epoTime.civil_date(year, month, day);
  epoTime.civil_time(hour, minute, sec);

  QString outLines = QString().asprintf("*  %4d %2d %2d %d %d %12.8f\n",
                                        year, month, day, hour, minute, sec);

  QMutableMapIterator<QString, cmbCorr*> it(resCorr);
  while (it.hasNext()) {
    it.next();
    cmbCorr* corr = it.value();

    // ORBIT
    t_orbCorr orbCorr(corr->_orbCorr);
    orbCorr._staID = "INTERNAL";
    orbCorrections.push_back(orbCorr);

    // CLOCK
    t_clkCorr clkCorr(corr->_clkCorr);
    clkCorr._staID      = "INTERNAL";
    clkCorr._dClk       = corr->_dClkResult;
    clkCorr._dotDClk    = 0.0;
    clkCorr._dotDotDClk = 0.0;
    clkCorrections.push_back(clkCorr);

    ColumnVector xc(6);
    ColumnVector vv(3);
    corr->_eph->setClkCorr(dynamic_cast<const t_clkCorr*>(&clkCorr));
    corr->_eph->setOrbCorr(dynamic_cast<const t_orbCorr*>(&orbCorr));
    if (corr->_eph->getCrd(epoTime, xc, vv, true) != success) {
      delete corr;
      it.remove();
      continue;
    }

    // Correction Phase Center --> CoM
    // -------------------------------
    ColumnVector dx(3);   dx = 0.0;
    ColumnVector apc(3); apc = 0.0;
    ColumnVector com(3); com = 0.0;
    bool masterIsAPC = true;
    if (_antex) {
      double Mjd = epoTime.mjd() + epoTime.daysec()/86400.0;
      char sys = corr->_eph->prn().system();
      masterIsAPC = _masterIsAPC[sys];
      if (_antex->satCoMcorrection(corr->_prn, Mjd, xc.Rows(1,3), dx) != success) {
        dx = 0;
        _log += "antenna not found " + corr->_prn.mid(0,3).toLatin1() + '\n';
      }
    }
    if (masterIsAPC) {
      apc(1) = xc(1);
      apc(2) = xc(2);
      apc(3) = xc(3);
      com(1) = xc(1)-dx(1);
      com(2) = xc(2)-dx(2);
      com(3) = xc(3)-dx(3);
    }
    else {
      com(1) = xc(1);
      com(2) = xc(2);
      com(3) = xc(3);
      apc(1) = xc(1)+dx(1);
      apc(2) = xc(2)+dx(2);
      apc(3) = xc(3)+dx(3);
    }

    outLines += corr->_prn.mid(0,3);
    QString hlp = QString().asprintf(
                  " APC 3 %15.4f %15.4f %15.4f"
                  " Clk 1 %15.4f"
                  " Vel 3 %15.4f %15.4f %15.4f"
                  " CoM 3 %15.4f %15.4f %15.4f",
                  apc(1), apc(2), apc(3),
                  xc(4) *  t_CST::c,
                  vv(1), vv(2), vv(3),
                  com(1), com(2), com(3));
    outLines += hlp;
    hlp.clear();

    // CODE BIASES
    if (corr->_satCodeBias._bias.size()) {
      t_satCodeBias satCodeBias(corr->_satCodeBias);
      satCodeBias._staID = "INTERNAL";
      satCodeBiasList.push_back(satCodeBias);
      hlp = QString().asprintf(" CodeBias %2lu", satCodeBias._bias.size());
      outLines += hlp;
      hlp.clear();
      for (unsigned ii = 0; ii < satCodeBias._bias.size(); ii++) {
        const t_frqCodeBias& frqCodeBias = satCodeBias._bias[ii];
        if (!frqCodeBias._rnxType2ch.empty()) {
          hlp = QString().asprintf(" %s%10.6f", frqCodeBias._rnxType2ch.c_str(), frqCodeBias._value);
          outLines += hlp;
          hlp.clear();
        }
      }
    }
    outLines += "\n";
    delete corr;
    it.remove();
  }

  outLines += "EOE\n"; // End Of Epoch flag
  //cout << outLines.toStdString();

  vector<string> errmsg;
  _rtnetDecoder->Decode(outLines.toLatin1().data(), outLines.length(), errmsg);

  // Send new Corrections to PPP etc.
  // --------------------------------
  if (orbCorrections.size() > 0 && clkCorrections.size() > 0) {
    emit newOrbCorrections(orbCorrections);
    emit newClkCorrections(clkCorrections);
  }
  if (satCodeBiasList.size()) {
    emit newCodeBiases(satCodeBiasList);
  }
}

// Create First Design Matrix and Vector of Measurements
////////////////////////////////////////////////////////////////////////////
t_irc bncComb::createAmat(char sys, Matrix& AA, ColumnVector& ll, DiagonalMatrix& PP,
                          const ColumnVector& x0, QMap<QString, cmbCorr*>& resCorr) {

  unsigned nPar = _params[sys].size();
  unsigned nObs = corrs(sys).size();

  if (nObs == 0) {
    return failure;
  }

  int maxSat = _cmbSysPrn[sys];

  const int nCon = (_method == filter) ? 1 + maxSat : 0;

  AA.ReSize(nObs+nCon, nPar);  AA = 0.0;
  ll.ReSize(nObs+nCon);        ll = 0.0;
  PP.ReSize(nObs+nCon);        PP = 1.0 / (sigObs * sigObs);

  int iObs = 0;
  QVectorIterator<cmbCorr*> itCorr(corrs(sys));
  while (itCorr.hasNext()) {
    cmbCorr* corr = itCorr.next();
    QString  prn  = corr->_prn;

    ++iObs;

    if (corr->_acName == _masterOrbitAC[sys] && resCorr.find(prn) == resCorr.end()) {
      resCorr[prn] = new cmbCorr(*corr);
    }

    for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
      cmbParam* pp = _params[sys][iPar-1];
      AA(iObs, iPar) = pp->partial(sys, corr->_acName, prn);
    }

    ll(iObs) = (corr->_clkCorr._dClk * t_CST::c - corr->_satCodeBiasIF) - DotProduct(AA.Row(iObs), x0);

    PP(iObs, iObs) *= 1.0 / (corr->_weightFactor * corr->_weightFactor);
  }

  // Regularization
  // --------------
  if (_method == filter) {
    const double Ph = 1.e6;
    PP(nObs+1) = Ph;
    for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
      cmbParam* pp = _params[sys][iPar-1];
      if ( AA.Column(iPar).maximum_absolute_value() > 0.0 &&
           pp->type == cmbParam::clkSat ) {
        AA(nObs+1, iPar) = 1.0;
      }
    }
//    if (sys == 'R') {
//      return success;
//    }
    int iCond = 1;
    // GNSS
    for (unsigned iGnss = 1; iGnss <= _cmbSysPrn[sys]; iGnss++) {
      int flag = t_corrSSR::getSsrNavTypeFlag(sys, iGnss);
      QString prn = QString("%1%2_%3").arg(sys).arg(iGnss, 2, 10, QChar('0')).arg(flag);
      ++iCond;
      PP(nObs+iCond) = Ph;
      for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
        cmbParam* pp = _params[sys][iPar-1];
        if ( pp &&
             AA.Column(iPar).maximum_absolute_value() > 0.0 &&
             pp->type == cmbParam::offACSat                 &&
             pp->prn == prn) {
          AA(nObs+iCond, iPar) = 1.0;
        }
      }
    }
  }

  return success;
}

// Process Epoch - Single-Epoch Method
////////////////////////////////////////////////////////////////////////////
t_irc bncComb::processEpoch_singleEpoch(bncTime epoTime, char sys, QTextStream& out,
                                        QMap<QString, cmbCorr*>& resCorr,
                                        ColumnVector& dx) {

  // Check Satellite Positions for Outliers
  // --------------------------------------
  if (checkOrbits(epoTime, sys, out) != success) {
    return failure;
  }

  // Outlier Detection Loop
  // ----------------------
  while (_running) {

    // Remove Satellites that are not in Master
    // ----------------------------------------
    QMutableVectorIterator<cmbCorr*> it(corrs(sys));
    while (it.hasNext()) {
      cmbCorr* corr    = it.next();
      QString  prnStr  = corr->_prn;
      bool foundMaster = false;
      QVectorIterator<cmbCorr*> itHlp(corrs(sys));
      while (itHlp.hasNext()) {
        cmbCorr* corrHlp = itHlp.next();
        QString  prnHlp  = corrHlp->_prn;
        QString  ACHlp   = corrHlp->_acName;
        if (ACHlp == _masterOrbitAC[sys] && prnStr == prnHlp) {
          foundMaster = true;
          break;
        }
      }
      if (!foundMaster) {
        delete corr;
        it.remove();
      }
    }

    // Count Number of Observations per Satellite and per AC
    // -----------------------------------------------------
    QMap<QString, int> numObsPrn;
    QMap<QString, int> numObsAC;
    QVectorIterator<cmbCorr*> itCorr(corrs(sys));
    while (itCorr.hasNext()) {
      cmbCorr* corr = itCorr.next();
      QString  prn  = corr->_prn;
      QString  AC   = corr->_acName;
      if (numObsPrn.find(prn) == numObsPrn.end()) {
        numObsPrn[prn]  = 1;
      }
      else {
        numObsPrn[prn] += 1;
      }
      if (numObsAC.find(AC) == numObsAC.end()) {
        numObsAC[AC]  = 1;
      }
      else {
        numObsAC[AC] += 1;
      }
    }

    // Clean-Up the Parameters
    // -----------------------
    for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
      delete _params[sys][iPar-1];
    }
    _params[sys].clear();

    // Set new Parameters
    // ------------------
    int nextPar = 0;

    QMapIterator<QString, int> itAC(numObsAC);
    while (itAC.hasNext()) {
      itAC.next();
      const QString& AC     = itAC.key();
      int            numObs = itAC.value();
      if (AC != _masterOrbitAC[sys] && numObs > 0) {
        _params[sys].push_back(new cmbParam(cmbParam::offACgnss, ++nextPar, AC, ""));
      }
    }

    QMapIterator<QString, int> itPrn(numObsPrn);
    while (itPrn.hasNext()) {
      itPrn.next();
      const QString& prn    = itPrn.key();
      int            numObs = itPrn.value();
      if (numObs > 0) {
        _params[sys].push_back(new cmbParam(cmbParam::clkSat, ++nextPar, "", prn));
      }
    }

    int nPar = _params[sys].size();
    ColumnVector x0(nPar);
    x0 = 0.0;

    // Create First-Design Matrix
    // --------------------------
    Matrix         AA;
    ColumnVector   ll;
    DiagonalMatrix PP;
    if (createAmat(sys, AA, ll, PP, x0, resCorr) != success) {
      return failure;
    }

    ColumnVector vv;
    try {
      Matrix          ATP = AA.t() * PP;
      SymmetricMatrix NN; NN << ATP * AA;
      ColumnVector    bb = ATP * ll;
      _QQ[sys] = NN.i();
      dx  = _QQ[sys] * bb;
      vv  = ll - AA * dx;
    }
    catch (Exception& exc) {
      out << exc.what() << "\n";
      return failure;
    }

    int     maxResIndex;
    double  maxRes = vv.maximum_absolute_value1(maxResIndex);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(3);
    out << epoTime.datestr().c_str() << " " << epoTime.timestr().c_str()
        << " Maximum Residuum " << maxRes << ' '
        << corrs(sys)[maxResIndex-1]->_acName << ' ' << corrs(sys)[maxResIndex-1]->_prn.mid(0,3);

    if (maxRes > _MAX_RES) {
      out << "  Outlier" << "\n";
      delete corrs(sys)[maxResIndex-1];
      corrs(sys).remove(maxResIndex-1);
    }
    else {
      out << "  OK" << "\n";
      out.setRealNumberNotation(QTextStream::FixedNotation);
      out.setRealNumberPrecision(3);
      for (int ii = 0; ii < vv.Nrows(); ii++) {
        const cmbCorr* corr = corrs(sys)[ii];
        out << epoTime.datestr().c_str() << ' '
            << epoTime.timestr().c_str() << " "
            << corr->_acName << ' ' << corr->_prn.mid(0,3);
        out.setFieldWidth(6);
        out << " res = " << vv[ii] << "\n";
        out.setFieldWidth(0);
      }
      return success;
    }

  }

  return failure;
}

// Check Satellite Positions for Outliers
////////////////////////////////////////////////////////////////////////////
t_irc bncComb::checkOrbits(bncTime epoTime, char sys, QTextStream& out) {

  // Switch to last ephemeris (if possible)
  // --------------------------------------
  QMutableVectorIterator<cmbCorr*> im(corrs(sys));
  while (im.hasNext()) {
    cmbCorr* corr = im.next();
    QString  prn  = corr->_prn;

    t_eph* ephLast = _ephUser.ephLast(prn);
    t_eph* ephPrev = _ephUser.ephPrev(prn);

    if      (ephLast == 0) {
      out << "checkOrbit: missing eph (not found) " << corr->_prn.mid(0,3) << "\n";
      delete corr;
      im.remove();
    }
    else if (corr->_eph == 0) {
      out << "checkOrbit: missing eph (zero) " << corr->_prn.mid(0,3) << "\n";
      delete corr;
      im.remove();
    }
    else {
      if ( corr->_eph == ephLast ||
           corr->_eph == ephPrev ) {
        switchToLastEph(ephLast, corr);
      }
      else {
        out << "checkOrbit: missing eph (deleted) " << corr->_prn.mid(0,3) << "\n";
        delete corr;
        im.remove();
      }
    }
  }

  while (_running) {

    // Compute Mean Corrections for all Satellites
    // -------------------------------------------
    QMap<QString, int>          numCorr;
    QMap<QString, ColumnVector> meanRao;
    QVectorIterator<cmbCorr*> it(corrs(sys));
    while (it.hasNext()) {
      cmbCorr* corr = it.next();
      QString  prn  = corr->_prn;
      if (meanRao.find(prn) == meanRao.end()) {
        meanRao[prn].ReSize(4);
        meanRao[prn].Rows(1,3) = corr->_orbCorr._xr;
        meanRao[prn](4)        = 1;
      }
      else {
        meanRao[prn].Rows(1,3) += corr->_orbCorr._xr;
        meanRao[prn](4)        += 1;
      }
      if (numCorr.find(prn) == numCorr.end()) {
        numCorr[prn] = 1;
      }
      else {
        numCorr[prn] += 1;
      }
    }

    // Compute Differences wrt. Mean, find Maximum
    // -------------------------------------------
    QMap<QString, cmbCorr*> maxDiff;
    it.toFront();
    while (it.hasNext()) {
      cmbCorr* corr = it.next();
      QString  prn  = corr->_prn;
      if (meanRao[prn](4) != 0) {
        meanRao[prn] /= meanRao[prn](4);
        meanRao[prn](4) = 0;
      }
      corr->_diffRao = corr->_orbCorr._xr - meanRao[prn].Rows(1,3);
      if (maxDiff.find(prn) == maxDiff.end()) {
        maxDiff[prn] = corr;
      }
      else {
        double normMax = maxDiff[prn]->_diffRao.NormFrobenius();
        double norm    = corr->_diffRao.NormFrobenius();
        if (norm > normMax) {
          maxDiff[prn] = corr;
        }
      }
    }

    if (_ACs.size() == 1) {
      break;
    }

    // Remove Outliers
    // ---------------
    bool removed = false;
    QMutableVectorIterator<cmbCorr*> im(corrs(sys));
    while (im.hasNext()) {
      cmbCorr* corr = im.next();
      QString  prn  = corr->_prn;
      if      (numCorr[prn] < 2) {
        delete corr;
        im.remove();
      }
      else if (corr == maxDiff[prn]) {
        double norm = corr->_diffRao.NormFrobenius();
        if (norm > (_MAX_DISPLACEMENT)) {
          out << epoTime.datestr().c_str()    << " "
              << epoTime.timestr().c_str()    << " "
              << "Orbit Outlier: "
              << corr->_acName.toLatin1().data() << " "
              << prn.mid(0,3).toLatin1().data()           << " "
              << corr->_iod                     << " "
              << norm                           << "\n";
          delete corr;
          im.remove();
          removed = true;
        }
      }
    }

    if (!removed) {
      break;
    }
  }

  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void bncComb::slotProviderIDChanged(QString mountPoint) {
  QMutexLocker locker(&_mutex);

  QTextStream out(&_log, QIODevice::WriteOnly);

  // Find the AC Name
  // ----------------
  QString acName;
  QListIterator<cmbAC*> icAC(_ACs);
  while (icAC.hasNext()) {
    cmbAC *AC = icAC.next();
    if (AC->mountPoint == mountPoint) {
      acName = AC->name;
      out << "Provider ID changed: AC " << AC->name.toLatin1().data() << " "
          << _resTime.datestr().c_str() << " " << _resTime.timestr().c_str()
          << "\n";
      break;
    }
  }
  if (acName.isEmpty()) {
    return;
  }
  QMapIterator<char, unsigned> itSys(_cmbSysPrn);
  while (itSys.hasNext()) {
    itSys.next();
    char sys = itSys.key();
    // Remove all corrections of the corresponding AC
    // ----------------------------------------------
    QVector<cmbCorr*> &corrVec = _buffer[sys].corrs;
    QMutableVectorIterator<cmbCorr*> it(corrVec);
    while (it.hasNext()) {
      cmbCorr *corr = it.next();
      if (acName == corr->_acName) {
        delete corr;
        it.remove();
      }
    }
    // Reset Satellite Offsets
    // -----------------------
    if (_method == filter) {
      for (int iPar = 1; iPar <= _params[sys].size(); iPar++) {
        cmbParam *pp = _params[sys][iPar - 1];
        if (pp->AC == acName && pp->type == cmbParam::offACSat) {
          pp->xx = 0.0;
          _QQ[sys].Row(iPar) = 0.0;
          _QQ[sys].Column(iPar) = 0.0;
          _QQ[sys](iPar, iPar) = pp->sig0 * pp->sig0;
        }
      }
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool bncComb::excludeSat(const t_prn& prn, const QStringList excludeSats) const {
  QStringListIterator it(excludeSats);
  while (it.hasNext()) {
    string prnStr = it.next().toLatin1().data();
    if (prnStr == prn.toString() ||              // prn
        prnStr == prn.toString().substr(0,1)) {  // sys
      return true;
    }
  }
  return false;
}
