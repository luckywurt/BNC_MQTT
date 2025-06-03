/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncRtnetUploadCaster
 *
 * Purpose:    Connection to NTRIP Caster
 *
 * Author:     L. Mervart
 *
 * Created:    29-Mar-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <math.h>
#include "bncrtnetuploadcaster.h"
#include "bncsettings.h"
#include "bncephuser.h"
#include "bncclockrinex.h"
#include "bncbiassinex.h"
#include "bncsp3.h"
#include "gnss.h"
#include "bncutils.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncRtnetUploadCaster::bncRtnetUploadCaster(const QString& mountpoint,
    const QString& outHost, int outPort,
    const QString& ntripVersion,
    const QString& userName, const QString& password,
    const QString& crdTrafo, const QString& ssrFormat, bool CoM, const QString& sp3FileName,
    const QString& rnxFileName, const QString& bsxFileName, int PID, int SID, int IOD, int iRow) :
    bncUploadCaster(mountpoint, outHost, outPort, ntripVersion, userName, password, iRow, 0) {

  if (!mountpoint.isEmpty()) {
    _casterID += mountpoint;
  }
  if (!outHost.isEmpty()) {
    _casterID +=  " " + outHost;
    if (outPort) {
      _casterID += ":" +  QString("%1").arg(outPort, 10);
    }
  }
  if (!crdTrafo.isEmpty()) {
    _casterID += " " + crdTrafo;
  }
  if (!sp3FileName.isEmpty()) {
    _casterID += " " + sp3FileName;
  }
  if (!rnxFileName.isEmpty()) {
    _casterID += " " + rnxFileName;
  }

  if (!bsxFileName.isEmpty()) {
    _casterID += " " + bsxFileName;
  }

  _crdTrafoStr = crdTrafo;

  _ssrFormat = ssrFormat;

  _ssrCorr = 0;
  if      (_ssrFormat == "IGS-SSR") {
    _ssrCorr = new SsrCorrIgs();
  }
  else if (_ssrFormat == "RTCM-SSR") {
    _ssrCorr = new SsrCorrRtcm();
  }

  _CoM = CoM;
  _PID = PID;
  _SID = SID;
  _IOD = IOD;
  _phaseBiasInformationDecoded = false;

  // Member that receives the ephemeris
  // ----------------------------------
  _ephUser = new bncEphUser(true);

  bncSettings settings;
  QString intr = settings.value("uploadIntr").toString();
  QStringList hlp = settings.value("cmbStreams").toStringList();
  _samplRtcmEphCorr = settings.value("uploadSamplRtcmEphCorr").toDouble();
  if (hlp.size() > 1) { // combination stream upload
    _samplRtcmClkCorr = settings.value("cmbSampl").toInt();
  }
  else { // single stream upload or sp3 file generation
    _samplRtcmClkCorr = 5; // default
  }
  _samplRtcmVtec = 60.0;
  _samplRtcmCrs  = 60.0;
  int samplClkRnx = settings.value("uploadSamplClkRnx").toInt();
  int samplSp3    = settings.value("uploadSamplSp3").toString().split("sec").first().toInt();
  int samplBiaSnx = settings.value("uploadSamplBiaSnx").toInt();

  if (_samplRtcmEphCorr == 0.0) {
    _usedEph = 0;
  }
  else {
    _usedEph = new QMap<QString, const t_eph*>;
  }

  // RINEX writer
  // ------------
  if (!rnxFileName.isEmpty()) {
    _rnx = new bncClockRinex(rnxFileName, intr, samplClkRnx);
  }
  else {
    _rnx = 0;
  }

  // SP3 writer
  // ----------
  if (!sp3FileName.isEmpty()) {
    _sp3 = new bncSP3(sp3FileName, intr, samplSp3);
  }
  else {
    _sp3 = 0;
  }

  // SINEX writer
  // ------------
  if (!bsxFileName.isEmpty()) {
    _bsx = new bncBiasSinex(bsxFileName, intr, samplBiaSnx);
  }
  else {
    _bsx = 0;
  }


  // Set Transformation Parameters
  // -----------------------------
  // Transformation Parameters from ITRF2014 to ETRF2000
  // http://etrs89.ign.fr/pub/EUREF-TN-1-Mar-04-2024.pdf
  if (_crdTrafoStr == "ETRF2000") {
    _dx  =  0.0552;
    _dy  =  0.0527;
    _dz  = -0.0836;

    _dxr =  0.0001;
    _dyr =  0.0001;
    _dzr = -0.0019;

    _ox  =  0.002106;
    _oy  =  0.012740;
    _oz  = -0.020592;

    _oxr =  0.000081;
    _oyr =  0.000490;
    _ozr = -0.000792;

    _sc  =  2.67;
    _scr =  0.11;

    _t0  =  2015.0;
  }
  // Transformation Parameters from ITRF2014 to GDA2020 (Ryan Ruddick, GA)
  else if (_crdTrafoStr == "GDA2020") {
    _dx  = 0.0;
    _dy  = 0.0;
    _dz  = 0.0;

    _dxr = 0.0;
    _dyr = 0.0;
    _dzr = 0.0;

    _ox  = 0.0;
    _oy  = 0.0;
    _oz  = 0.0;

    _oxr = 0.00150379;
    _oyr = 0.00118346;
    _ozr = 0.00120716;

    _sc  = 0.0;
    _scr = 0.0;

    _t0  = 2020.0;
  }
  // Transformation Parameters from IGb14 to SIRGAS2000 (Thanks to Sonia Costa, BRA)
  // June 29 2020: TX:-0.0027 m  TY:-0.0025 m  TZ:-0.0042 m  SCL:1.20 (ppb) no rotations and no rates.*/
  else if (_crdTrafoStr == "SIRGAS2000") {
    _dx  = -0.0027;
    _dy  = -0.0025;
    _dz  = -0.0042;

    _dxr =  0.0;
    _dyr =  0.0;
    _dzr =  0.0;

    _ox  =  0.0;
    _oy  =  0.0;
    _oz  =  0.0;

    _oxr =  0.0;
    _oyr =  0.0;
    _ozr =  0.0;

    _sc  =  1.2;
    _scr =  0.0;
    _t0  =  2000.0;
  }
  // Transformation Parameters from ITRF2014 to DREF91
  else if (_crdTrafoStr == "DREF91") {
    _dx  =  0.0547;
    _dy  =  0.0522;
    _dz  = -0.0741;

    _dxr =  0.0001;
    _dyr =  0.0001;
    _dzr = -0.0019;
    // ERTF200  + rotation parameters (ETRF2000 => DREF91)
    _ox  =  0.001701 + 0.000658;
    _oy  =  0.010290 - 0.000208;
    _oz  = -0.016632 + 0.000755;

    _oxr =  0.000081;
    _oyr =  0.000490;
    _ozr = -0.000729;

    _sc  =  2.12;
    _scr =  0.11;

    _t0  =  2010.0;
  }
  else if (_crdTrafoStr == "Custom") {
    _dx = settings.value("trafo_dx").toDouble();
    _dy = settings.value("trafo_dy").toDouble();
    _dz = settings.value("trafo_dz").toDouble();
    _dxr = settings.value("trafo_dxr").toDouble();
    _dyr = settings.value("trafo_dyr").toDouble();
    _dzr = settings.value("trafo_dzr").toDouble();
    _ox = settings.value("trafo_ox").toDouble();
    _oy = settings.value("trafo_oy").toDouble();
    _oz = settings.value("trafo_oz").toDouble();
    _oxr = settings.value("trafo_oxr").toDouble();
    _oyr = settings.value("trafo_oyr").toDouble();
    _ozr = settings.value("trafo_ozr").toDouble();
    _sc = settings.value("trafo_sc").toDouble();
    _scr = settings.value("trafo_scr").toDouble();
    _t0 = settings.value("trafo_t0").toDouble();
  }
    // TODO: the following lines can be deleted if all parameters are updated regarding ITRF2020
  if (_crdTrafoStr == "ETRF2000" ||
      _crdTrafoStr == "GDA2020"  ||
      _crdTrafoStr == "DREF91"   ||
      _crdTrafoStr == "SIRGAS2000") {
       // Transformation Parameters from ITRF2020 to ITRF2014
       // from ITRF web site: https://itrf.ign.fr/en/solutions/transformations
   _dx14  = -0.0014;
   _dy14  = -0.0009;
   _dz14  =  0.0014;
   _dxr14 =  0.0;
   _dyr14 = -0.0001;
   _dzr14 = -0.0002;
   _ox14  =  0.0;
   _oy14  =  0.0;
   _oz14  =  0.0;
   _oxr14 =  0.0;
   _oyr14 =  0.0;
   _ozr14 =  0.0;
   _sc14  = -0.42;
   _scr14 =  0.0;
   _t014  =  2015.0;
 }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncRtnetUploadCaster::~bncRtnetUploadCaster() {
  if (isRunning()) {
    wait();
  }
  delete _rnx;
  delete _sp3;
  delete _ephUser;
  delete _usedEph;
  delete _ssrCorr;
}

//
////////////////////////////////////////////////////////////////////////////
void bncRtnetUploadCaster::decodeRtnetStream(char* buffer, int bufLen) {

  QMutexLocker locker(&_mutex);

  // Append to internal buffer
  // -------------------------
  _rtnetStreamBuffer.append(QByteArray(buffer, bufLen));

  // Select buffer part that contains last epoch
  // -------------------------------------------
  QStringList lines;
  int iEpoBeg = _rtnetStreamBuffer.lastIndexOf('*');   // begin of last epoch
  if (iEpoBeg == -1) {
    _rtnetStreamBuffer.clear();
    return;
  }
  _rtnetStreamBuffer = _rtnetStreamBuffer.mid(iEpoBeg);

  int iEpoEnd = _rtnetStreamBuffer.lastIndexOf("EOE"); // end   of last epoch
  if (iEpoEnd == -1) {
    return;
  }
  else {
    lines = _rtnetStreamBuffer.left(iEpoEnd).split('\n', Qt::SkipEmptyParts);
    _rtnetStreamBuffer = _rtnetStreamBuffer.mid(iEpoEnd+3);
  }

  if (lines.size() < 2) {
      emit(newMessage(
            "bncRtnetUploadCaster: less than 2 lines to decode " + _casterID.toLatin1(), false));
    return;
  }

  // Keep the last unfinished line in buffer
  // ---------------------------------------
  int iLastEOL = _rtnetStreamBuffer.lastIndexOf('\n');
  if (iLastEOL != -1) {
    _rtnetStreamBuffer = _rtnetStreamBuffer.mid(iLastEOL+1);
  }

  decodeRtnetEpoch(lines);
}

//
////////////////////////////////////////////////////////////////////////////
void bncRtnetUploadCaster::decodeRtnetEpoch(QStringList epochLines) {

  // Read first line (with epoch time)
  // ---------------------------------
  QTextStream in(epochLines[0].toLatin1());
  QString hlp;
  int year, month, day, hour, min;
  double sec;
  in >> hlp >> year >> month >> day >> hour >> min >> sec;
  bncTime epoTime;
  epoTime.set(year, month, day, hour, min, sec);//cout <<  epoTime.timestr().c_str() << endl;

  emit(newMessage(
      "bncRtnetUploadCaster: decode " + QByteArray(epoTime.datestr().c_str())
          + " " + QByteArray(epoTime.timestr().c_str()) + " "
          + _casterID.toLatin1(), false));

  struct SsrCorr::ClockOrbit co;
  memset(&co, 0, sizeof(co));
  co.EpochTime[CLOCKORBIT_SATGPS] = static_cast<int>(epoTime.gpssec());
  double gt = epoTime.gpssec() - gnumleap(year, month, day);
  if      (_ssrFormat == "RTCM-SSR") {
    gt += 3 * 3600;
  }
  co.EpochTime[CLOCKORBIT_SATGLONASS] = static_cast<int>(fmod(gt, 86400.0));
  co.EpochTime[CLOCKORBIT_SATGALILEO] = static_cast<int>(epoTime.gpssec());
  co.EpochTime[CLOCKORBIT_SATQZSS]    = static_cast<int>(epoTime.gpssec());
  co.EpochTime[CLOCKORBIT_SATSBAS]    = static_cast<int>(epoTime.gpssec());
  co.EpochTime[CLOCKORBIT_SATBDS]     = static_cast<int>(epoTime.bdssec());
  co.Supplied[_ssrCorr->COBOFS_CLOCK] = 1;
  co.Supplied[_ssrCorr->COBOFS_ORBIT] = 1;
  (_crdTrafoStr == "IGS20") ?
      co.SatRefDatum = _ssrCorr->DATUM_ITRF :
      co.SatRefDatum = _ssrCorr->DATUM_LOCAL;
  co.SSRIOD        = _IOD;
  co.SSRProviderID = _PID; // 256 .. BKG,  257 ... EUREF
  co.SSRSolutionID = _SID;
  if      (_ssrFormat == "RTCM-SSR") {
    co.EpochTime[CLOCKORBIT_SATBDS] = static_cast<int>(epoTime.bdssec());
  }
  else if (_ssrFormat == "IGS-SSR") {
    co.EpochTime[CLOCKORBIT_SATBDS] = static_cast<int>(epoTime.gpssec());
  }
  co.Supplied[_ssrCorr->COBOFS_CLOCK] = 1;
  co.Supplied[_ssrCorr->COBOFS_ORBIT] = 1;


  t_serviceCrs serviceCrs;
  t_rtcmCrs    rtcmCrs;
  if (_crdTrafoStr == "IGS20") {
    serviceCrs._CE = serviceCrs._coordinateEpoch  = 0;
    int nc = _crdTrafoStr.length();
    if (nc > 31) {nc = 31;}
    for (int i = 0; i < nc; i++) {
      serviceCrs._name[i] =
         rtcmCrs._name[i] = _crdTrafoStr.toStdString()[i];
    }
    serviceCrs._name[nc] = 0;
    rtcmCrs._name[nc] = 0;
    rtcmCrs._anchor      = 0; // global CRS
    rtcmCrs._plateNumber = 0; // unknown
    rtcmCrs._databaseLinks << "ISO.DATUM.979" << "ISO.CRS:980";
  }
  else {
    if (_crdTrafoStr != "Custom") {
      serviceCrs._coordinateEpoch = _t0;
      serviceCrs.setCEFromCoordinateEpoch();
      int nc = _crdTrafoStr.length();
      if (nc > 31) {nc = 31;}
      for (int i = 0; i < nc; i++) {
        serviceCrs._name[i] = _crdTrafoStr.toStdString()[i];
      }
      serviceCrs._name[nc] = 0;
      QString rtcmcrsname = _crdTrafoStr + QString("(%1)").arg(_t0,4,'d',0);
      nc = rtcmcrsname.length();
      if (nc > 31) {nc = 31;}
      for (int i = 0; i < nc; i++) {
        rtcmCrs._name[i] = rtcmcrsname.toStdString()[i];
      }
      rtcmCrs._name[nc] = 0;
      if (_crdTrafoStr == "ETRF2000") {
        rtcmCrs._anchor      = 1; // plate-fixed CRS
        rtcmCrs._plateNumber = 7; // Eurasia
        rtcmCrs._databaseLinks << "ISO.DATUM.187"    << "ISO.CRS:260"   << "ISO.CRS:457"
                               << "EPSG.DATUM:1186"  << "EPSG.CRS:7930" << "EPSG.CRS:7931";
      }
      else if (_crdTrafoStr == "DREF91") {
        rtcmCrs._anchor      = 1; // plate-fixed CRS
        rtcmCrs._plateNumber = 7; // Eurasia
      }
      else if (_crdTrafoStr == "GDA2020") {
        rtcmCrs._anchor      = 1; // plate-fixed CRS
        rtcmCrs._plateNumber = 4; // Australia
        rtcmCrs._databaseLinks << "ISO.DATUM.186"   << "ISO.CRS:404"   << "ISO.CRS:329"
                               << "EPSG.DATUM:1168" << "EPSG.CRS:7842" << "EPSG.CRS:7843";
      }
      else if (_crdTrafoStr == "SIRGAS2000") {
        rtcmCrs._anchor      =  1; // plate-fixed CRS
        rtcmCrs._plateNumber = 12; // S.America
        rtcmCrs._databaseLinks << "ISO.DATUM:169"   << "ISO.CRS:384"   << "ISO.CRS:313"
                               << "EPSG.DATUM:6674" << "EPSG.CRS:4988" << "EPSG.CRS:4989";
      }
    }
  }

  struct SsrCorr::CodeBias bias;
  memset(&bias, 0, sizeof(bias));
  bias.EpochTime[CLOCKORBIT_SATGPS]     = co.EpochTime[CLOCKORBIT_SATGPS];
  bias.EpochTime[CLOCKORBIT_SATGLONASS] = co.EpochTime[CLOCKORBIT_SATGLONASS];
  bias.EpochTime[CLOCKORBIT_SATGALILEO] = co.EpochTime[CLOCKORBIT_SATGALILEO];
  bias.EpochTime[CLOCKORBIT_SATQZSS]    = co.EpochTime[CLOCKORBIT_SATQZSS];
  bias.EpochTime[CLOCKORBIT_SATSBAS]    = co.EpochTime[CLOCKORBIT_SATSBAS];
  bias.EpochTime[CLOCKORBIT_SATBDS]     = co.EpochTime[CLOCKORBIT_SATBDS];
  bias.SSRIOD        = _IOD;
  bias.SSRProviderID = _PID;
  bias.SSRSolutionID = _SID;

  struct SsrCorr::PhaseBias phasebias;
  memset(&phasebias, 0, sizeof(phasebias));
  unsigned int dispersiveBiasConsistenyIndicator = 0;
  unsigned int mwConsistencyIndicator = 0;
  phasebias.EpochTime[CLOCKORBIT_SATGPS]     = co.EpochTime[CLOCKORBIT_SATGPS];
  phasebias.EpochTime[CLOCKORBIT_SATGLONASS] = co.EpochTime[CLOCKORBIT_SATGLONASS];
  phasebias.EpochTime[CLOCKORBIT_SATGALILEO] = co.EpochTime[CLOCKORBIT_SATGALILEO];
  phasebias.EpochTime[CLOCKORBIT_SATQZSS]    = co.EpochTime[CLOCKORBIT_SATQZSS];
  phasebias.EpochTime[CLOCKORBIT_SATSBAS]    = co.EpochTime[CLOCKORBIT_SATSBAS];
  phasebias.EpochTime[CLOCKORBIT_SATBDS]     = co.EpochTime[CLOCKORBIT_SATBDS];
  phasebias.SSRIOD        = _IOD;
  phasebias.SSRProviderID = _PID;
  phasebias.SSRSolutionID = _SID;

  struct SsrCorr::VTEC vtec;
  memset(&vtec, 0, sizeof(vtec));
  vtec.EpochTime = static_cast<int>(epoTime.gpssec());
  vtec.SSRIOD        = _IOD;
  vtec.SSRProviderID = _PID;
  vtec.SSRSolutionID = _SID;

  // Default Update Interval
  // -----------------------
  int clkUpdInd = 2;         // 5 sec
  int ephUpdInd = clkUpdInd; // default

  if (!_samplRtcmEphCorr) {
    _samplRtcmEphCorr = 5.0;
  }

  if (_samplRtcmClkCorr > 5.0 && _samplRtcmEphCorr <= 5.0) { // combined orb and clock
    ephUpdInd = determineUpdateInd(_samplRtcmClkCorr);
  }
  if (_samplRtcmClkCorr > 5.0) {
    clkUpdInd = determineUpdateInd(_samplRtcmClkCorr);
  }
  if (_samplRtcmEphCorr > 5.0) {
    ephUpdInd = determineUpdateInd(_samplRtcmEphCorr);
  }

  co.UpdateInterval = clkUpdInd;
  bias.UpdateInterval = ephUpdInd;
  phasebias.UpdateInterval = ephUpdInd;

  for (int ii = 1; ii < epochLines.size(); ii++) {
    QString key;  // prn or key VTEC, IND (phase bias indicators)
    double rtnUra = 0.0; // [m]
    ColumnVector rtnAPC; rtnAPC.ReSize(3); rtnAPC = 0.0;          // [m, m, m]
    ColumnVector rtnVel; rtnVel.ReSize(3); rtnVel = 0.0;          // [m/s, m/s, m/s]
    ColumnVector rtnCoM; rtnCoM.ReSize(3); rtnCoM = 0.0;          // [m, m, m]
    ColumnVector rtnClk; rtnClk.ReSize(3); rtnClk = 0.0;          // [m, m/s, m/s²]
    ColumnVector rtnClkSig; rtnClkSig.ReSize(3); rtnClkSig = 0.0; // [m, m/s, m/s²]

    QTextStream in(epochLines[ii].toLatin1());

    in >> key;

    // non-satellite specific parameters
    if (key.contains("IND", Qt::CaseSensitive)) {
      in >> dispersiveBiasConsistenyIndicator >> mwConsistencyIndicator;
      continue;
    }
    // non-satellite specific parameters
    if (key.contains("VTEC", Qt::CaseSensitive)) {
      double ui;
      in >> ui >> vtec.NumLayers;
      vtec.UpdateInterval = (unsigned int) determineUpdateInd(ui);
      for (unsigned ll = 0; ll < vtec.NumLayers; ll++) {
        int dummy;
        in >> dummy >> vtec.Layers[ll].Degree >> vtec.Layers[ll].Order
            >> vtec.Layers[ll].Height;
        for (unsigned iDeg = 0; iDeg <= vtec.Layers[ll].Degree; iDeg++) {
          for (unsigned iOrd = 0; iOrd <= vtec.Layers[ll].Order; iOrd++) {
            in >> vtec.Layers[ll].Cosinus[iDeg][iOrd];
          }
        }
        for (unsigned iDeg = 0; iDeg <= vtec.Layers[ll].Degree; iDeg++) {
          for (unsigned iOrd = 0; iOrd <= vtec.Layers[ll].Order; iOrd++) {
            in >> vtec.Layers[ll].Sinus[iDeg][iOrd];
          }
        }
      }
      continue;
    }
    // satellite specific parameters
    t_prn prn;
    char sys = key.mid(0, 1).at(0).toLatin1();
    int  num = key.mid(1, 2).toInt();
    int flag = t_corrSSR::getSsrNavTypeFlag(sys, num);
    if (!num) {
      continue;
    }
    prn.set(sys, num, flag);
    QString prnInternalStr = QString::fromStdString(prn.toInternalString());
    QString prnStr         = QString::fromStdString(prn.toString());

    const t_eph* ephLast = _ephUser->ephLast(prnInternalStr);
    const t_eph* ephPrev = _ephUser->ephPrev(prnInternalStr);
    const t_eph* eph = ephLast;
    if (eph) {

      // Use previous ephemeris if the last one is too recent
      // ----------------------------------------------------
      const int MINAGE = 60; // seconds
      if (ephPrev && eph->receptDateTime().isValid() &&
          eph->receptDateTime().secsTo(currentDateAndTimeGPS()) < MINAGE) {
        eph = ephPrev;
      }

      // Make sure the clock messages refer to same IOD as orbit messages
      // ----------------------------------------------------------------
      if (_usedEph) {
        if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
          (*_usedEph)[prnInternalStr] = eph;
        }
        else {
          eph = 0;
          if (_usedEph->contains(prnInternalStr)) {
            const t_eph* usedEph = _usedEph->value(prnInternalStr);
            if (usedEph == ephLast) {
              eph = ephLast;
            }
            else if (usedEph == ephPrev) {
              eph = ephPrev;
            }
          }
        }
      }
    }

    QDateTime now = currentDateAndTimeGPS();
    bncTime   currentTime(now.toString(Qt::ISODate).toStdString());
    if (eph &&
        !outDatedBcep(eph, currentTime)       &&  // detected from storage because of no update
        eph->checkState() != t_eph::bad       &&
        eph->checkState() != t_eph::unhealthy &&
        eph->checkState() != t_eph::outdated) {   // detected during reception (bncephuser)
      QMap<QString, double> codeBiases;
      QList<phaseBiasSignal> phaseBiasList;
      phaseBiasesSat pbSat;
      _phaseBiasInformationDecoded = false;

      while (true) {
        QString key;
        int numVal = 0;
        in >> key;
        if (in.status() != QTextStream::Ok) {
          break;
        }
        if (key == "APC") {
          in >> numVal;
          rtnAPC.ReSize(3); rtnAPC = 0.0;
          for (int ii = 0; ii < numVal; ii++) {
            in >> rtnAPC[ii];
          }
        }
        else if (key == "Ura") {
          in >> numVal;
          if (numVal == 1)
            in >> rtnUra;
        }
        else if (key == "Clk") {
          in >> numVal;
          rtnClk.ReSize(3); rtnClk = 0.0;
          for (int ii = 0; ii < numVal; ii++) {
            in >> rtnClk[ii];
          }
        }
        else if (key == "ClkSig") {
          in >> numVal;
          rtnClkSig.ReSize(3); rtnClkSig = 0.0;
          for (int ii = 0; ii < numVal; ii++) {
            in >> rtnClkSig[ii];
          }
        }
        else if (key == "Vel") {
          in >> numVal;
          rtnVel.ReSize(3); rtnVel = 0.0;
          for (int ii = 0; ii < numVal; ii++) {
            in >> rtnVel[ii];
          }
        }
        else if (key == "CoM") {
          in >> numVal;
          rtnCoM.ReSize(3); rtnCoM = 0.0;
          for (int ii = 0; ii < numVal; ii++) {
            in >> rtnCoM[ii];
          }
        }
        else if (key == "CodeBias") {
          in >> numVal;
          for (int ii = 0; ii < numVal; ii++) {
            QString type;
            double value;
            in >> type >> value;
            codeBiases[type] = value;
          }
        }
        else if (key == "YawAngle") {
          _phaseBiasInformationDecoded = true;
          in >> numVal >> pbSat.yawAngle;
          if      (pbSat.yawAngle < 0.0) {
            pbSat.yawAngle += (2*M_PI);
          }
          else if (pbSat.yawAngle > 2*M_PI) {
            pbSat.yawAngle -= (2*M_PI);
          }
        }
        else if (key == "YawRate") {
          _phaseBiasInformationDecoded = true;
          in >> numVal >> pbSat.yawRate;
        }
        else if (key == "PhaseBias") {
          _phaseBiasInformationDecoded = true;
          in >> numVal;
          for (int ii = 0; ii < numVal; ii++) {
            phaseBiasSignal pb;
            in >> pb.type >> pb.bias >> pb.integerIndicator
              >> pb.wlIndicator >> pb.discontinuityCounter;
            phaseBiasList.append(pb);
          }
        }
        else {
          in >> numVal;
          for (int ii = 0; ii < numVal; ii++) {
            double dummy;
            in >> dummy;
          }
          emit(newMessage("                      RTNET format error: "
                          +  epochLines[ii].toLatin1(), false));
          break;
        }
      }

      struct SsrCorr::ClockOrbit::SatData* sd = 0;
      if (prn.system() == 'G') {
        sd = co.Sat + co.NumberOfSat[CLOCKORBIT_SATGPS];
        ++co.NumberOfSat[CLOCKORBIT_SATGPS];
      }
      else if (prn.system() == 'R') {
        sd = co.Sat + CLOCKORBIT_NUMGPS + co.NumberOfSat[CLOCKORBIT_SATGLONASS];
        ++co.NumberOfSat[CLOCKORBIT_SATGLONASS];
      }
      else if (prn.system() == 'E') {
        sd = co.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
           + co.NumberOfSat[CLOCKORBIT_SATGALILEO];
        ++co.NumberOfSat[CLOCKORBIT_SATGALILEO];
      }
      else if (prn.system() == 'J') {
        sd = co.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
           + CLOCKORBIT_NUMGALILEO
           + co.NumberOfSat[CLOCKORBIT_SATQZSS];
        ++co.NumberOfSat[CLOCKORBIT_SATQZSS];
      }
      else if (prn.system() == 'S') {
        sd = co.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
            + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
            + co.NumberOfSat[CLOCKORBIT_SATSBAS];
        ++co.NumberOfSat[CLOCKORBIT_SATSBAS];
      }
      else if (prn.system() == 'C') {
        sd = co.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
            + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
            + CLOCKORBIT_NUMSBAS
            + co.NumberOfSat[CLOCKORBIT_SATBDS];
        ++co.NumberOfSat[CLOCKORBIT_SATBDS];
      }
      if (sd) {
        QString outLine;
        t_irc irc = processSatellite(eph, epoTime.gpsw(), epoTime.gpssec(), prnStr, rtnAPC,
                                     rtnUra, rtnClk, rtnVel, rtnCoM, rtnClkSig, sd, outLine);
        if (irc != success) {
          continue;
        }
      }

      // Code Biases
      // -----------
      struct SsrCorr::CodeBias::BiasSat* biasSat = 0;
      if (!codeBiases.isEmpty()) {
        if (prn.system() == 'G') {
          biasSat = bias.Sat + bias.NumberOfSat[CLOCKORBIT_SATGPS];
          ++bias.NumberOfSat[CLOCKORBIT_SATGPS];
        }
        else if (prn.system() == 'R') {
          biasSat = bias.Sat + CLOCKORBIT_NUMGPS
                  + bias.NumberOfSat[CLOCKORBIT_SATGLONASS];
          ++bias.NumberOfSat[CLOCKORBIT_SATGLONASS];
        }
        else if (prn.system() == 'E') {
          biasSat = bias.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                  + bias.NumberOfSat[CLOCKORBIT_SATGALILEO];
          ++bias.NumberOfSat[CLOCKORBIT_SATGALILEO];
        }
        else if (prn.system() == 'J') {
          biasSat = bias.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                  + CLOCKORBIT_NUMGALILEO
                  + bias.NumberOfSat[CLOCKORBIT_SATQZSS];
          ++bias.NumberOfSat[CLOCKORBIT_SATQZSS];
        }
        else if (prn.system() == 'S') {
          biasSat = bias.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                  + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                  + bias.NumberOfSat[CLOCKORBIT_SATSBAS];
          ++bias.NumberOfSat[CLOCKORBIT_SATSBAS];
        }
        else if (prn.system() == 'C') {
          biasSat = bias.Sat + CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                  + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                  + CLOCKORBIT_NUMSBAS
                  + bias.NumberOfSat[CLOCKORBIT_SATBDS];
          ++bias.NumberOfSat[CLOCKORBIT_SATBDS];
        }
      }

      if (biasSat) {
        biasSat->ID = prn.number();
        biasSat->NumberOfCodeBiases = 0;
        QMapIterator<QString, double> it(codeBiases);
        while (it.hasNext()) {
          it.next();
          int ii = biasSat->NumberOfCodeBiases;
          if (ii >= CLOCKORBIT_NUMBIAS)
            break;
          SsrCorr::CodeType type = _ssrCorr->rnxTypeToCodeType(prn.system(), it.key().toStdString());
          if (type != _ssrCorr->RESERVED) {
            biasSat->NumberOfCodeBiases += 1;
            biasSat->Biases[ii].Type = type;
            biasSat->Biases[ii].Bias = it.value();
            if (_bsx) {
              QString obsCode = 'C' + it.key();
              _bsx->write(epoTime.gpsw(), epoTime.gpssec(), prnStr, obsCode, it.value());
            }
          }
        }
      }

      // Phase Biases
      // ------------
      struct SsrCorr::PhaseBias::PhaseBiasSat* phasebiasSat = 0;
      if (prn.system()      == 'G') {
        phasebiasSat = phasebias.Sat
                     + phasebias.NumberOfSat[CLOCKORBIT_SATGPS];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATGPS];
      }
      else if (prn.system() == 'R') {
        phasebiasSat = phasebias.Sat + CLOCKORBIT_NUMGPS
                     + phasebias.NumberOfSat[CLOCKORBIT_SATGLONASS];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATGLONASS];
      }
      else if (prn.system() == 'E') {
        phasebiasSat = phasebias.Sat + CLOCKORBIT_NUMGPS  + CLOCKORBIT_NUMGLONASS
                     + phasebias.NumberOfSat[CLOCKORBIT_SATGALILEO];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATGALILEO];
      }
      else if (prn.system() == 'J') {
        phasebiasSat = phasebias.Sat + CLOCKORBIT_NUMGPS  + CLOCKORBIT_NUMGLONASS
                     + CLOCKORBIT_NUMGALILEO
                     + phasebias.NumberOfSat[CLOCKORBIT_SATQZSS];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATQZSS];
      }
      else if (prn.system() == 'S') {
        phasebiasSat = phasebias.Sat + CLOCKORBIT_NUMGPS  + CLOCKORBIT_NUMGLONASS
                     + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                     + phasebias.NumberOfSat[CLOCKORBIT_SATSBAS];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATSBAS];
      }
      else if (prn.system() == 'C') {
        phasebiasSat = phasebias.Sat + CLOCKORBIT_NUMGPS  + CLOCKORBIT_NUMGLONASS
                     + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                     + CLOCKORBIT_NUMSBAS
                     + phasebias.NumberOfSat[CLOCKORBIT_SATBDS];
        ++phasebias.NumberOfSat[CLOCKORBIT_SATBDS];
      }

      if (phasebiasSat && _phaseBiasInformationDecoded) {
        phasebias.DispersiveBiasConsistencyIndicator = dispersiveBiasConsistenyIndicator;
        phasebias.MWConsistencyIndicator = mwConsistencyIndicator;
        phasebiasSat->ID = prn.number();
        phasebiasSat->NumberOfPhaseBiases = 0;
        phasebiasSat->YawAngle = pbSat.yawAngle;
        phasebiasSat->YawRate = pbSat.yawRate;
        QListIterator<phaseBiasSignal> it(phaseBiasList);
        while (it.hasNext()) {
          const phaseBiasSignal &pbSig = it.next();
          int ii = phasebiasSat->NumberOfPhaseBiases;
          if (ii >= CLOCKORBIT_NUMBIAS)
            break;
          SsrCorr::CodeType type = _ssrCorr->rnxTypeToCodeType(prn.system(), pbSig.type.toStdString());
          if (type != _ssrCorr->RESERVED) {
            phasebiasSat->NumberOfPhaseBiases += 1;
            phasebiasSat->Biases[ii].Type = type;
            phasebiasSat->Biases[ii].Bias = pbSig.bias;
            phasebiasSat->Biases[ii].SignalIntegerIndicator = pbSig.integerIndicator;
            phasebiasSat->Biases[ii].SignalsWideLaneIntegerIndicator = pbSig.wlIndicator;
            phasebiasSat->Biases[ii].SignalDiscontinuityCounter = pbSig.discontinuityCounter;
            if (_bsx) {
              QString obsCode = 'L' + pbSig.type;
              _bsx->write(epoTime.gpsw(), epoTime.gpssec(), prnStr, obsCode, pbSig.bias);
            }
          }
        }
      }
    }
  }

  QByteArray hlpBufferCo;

  // Orbit and Clock Corrections together
  // ------------------------------------
  if (_samplRtcmEphCorr == _samplRtcmClkCorr) {
    if (co.NumberOfSat[CLOCKORBIT_SATGPS] > 0
        || co.NumberOfSat[CLOCKORBIT_SATGLONASS] > 0
        || co.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0
        || co.NumberOfSat[CLOCKORBIT_SATQZSS] > 0
        || co.NumberOfSat[CLOCKORBIT_SATSBAS] > 0
        || co.NumberOfSat[CLOCKORBIT_SATBDS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      int len = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_AUTO, 0, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferCo = QByteArray(obuffer, len);
      }
    }
  }

  // Orbit and Clock Corrections separately
  // --------------------------------------
  else {
    if (co.NumberOfSat[CLOCKORBIT_SATGPS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GPSORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = (co.NumberOfSat[CLOCKORBIT_SATGLONASS] > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATQZSS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATSBAS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATBDS]     > 0   ) ? 1 : 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GPSCLOCK, mmsg, obuffer, sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
    if (co.NumberOfSat[CLOCKORBIT_SATGLONASS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GLONASSORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = (co.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATQZSS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATSBAS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATBDS]     > 0   ) ? 1 : 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GLONASSCLOCK, mmsg, obuffer, sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
    if (co.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GALILEOORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = (co.NumberOfSat[CLOCKORBIT_SATQZSS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATSBAS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATBDS]     > 0   ) ? 1 : 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_GALILEOCLOCK, mmsg, obuffer, sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
    if (co.NumberOfSat[CLOCKORBIT_SATQZSS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_QZSSORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = (co.NumberOfSat[CLOCKORBIT_SATSBAS]    > 0 ||
                  co.NumberOfSat[CLOCKORBIT_SATBDS]     > 0   ) ? 1 : 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_QZSSCLOCK, mmsg, obuffer, sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
    if (co.NumberOfSat[CLOCKORBIT_SATSBAS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_SBASORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = (co.NumberOfSat[CLOCKORBIT_SATBDS] > 0) ? 1 : 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_SBASCLOCK, mmsg, obuffer,
          sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
    if (co.NumberOfSat[CLOCKORBIT_SATBDS] > 0) {
      char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
      if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
        co.UpdateInterval = ephUpdInd;
        int len1 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_BDSORBIT, 1, obuffer, sizeof(obuffer));
        co.UpdateInterval = clkUpdInd;
        if (len1 > 0) {
          hlpBufferCo += QByteArray(obuffer, len1);
        }
      }
      int mmsg = 0;
      int len2 = _ssrCorr->MakeClockOrbit(&co, _ssrCorr->COTYPE_BDSCLOCK, mmsg, obuffer, sizeof(obuffer));
      if (len2 > 0) {
        hlpBufferCo += QByteArray(obuffer, len2);
      }
    }
  }

  // Code Biases
  // -----------
  QByteArray hlpBufferBias;
  if (bias.NumberOfSat[CLOCKORBIT_SATGPS] > 0
      || bias.NumberOfSat[CLOCKORBIT_SATGLONASS] > 0
      || bias.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0
      || bias.NumberOfSat[CLOCKORBIT_SATQZSS] > 0
      || bias.NumberOfSat[CLOCKORBIT_SATSBAS] > 0
      || bias.NumberOfSat[CLOCKORBIT_SATBDS] > 0) {
    char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
    if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
      int len = _ssrCorr->MakeCodeBias(&bias, _ssrCorr->CBTYPE_AUTO, 0, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferBias = QByteArray(obuffer, len);
      }
    }
  }

  // Phase Biases
  // ------------
  QByteArray hlpBufferPhaseBias;
  if ((phasebias.NumberOfSat[CLOCKORBIT_SATGPS] > 0
      || phasebias.NumberOfSat[CLOCKORBIT_SATGLONASS] > 0
      || phasebias.NumberOfSat[CLOCKORBIT_SATGALILEO] > 0
      || phasebias.NumberOfSat[CLOCKORBIT_SATQZSS] > 0
      || phasebias.NumberOfSat[CLOCKORBIT_SATSBAS] > 0
      || phasebias.NumberOfSat[CLOCKORBIT_SATBDS] > 0)
      && (_phaseBiasInformationDecoded)) {
    char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
    if (fmod(epoTime.gpssec(), _samplRtcmEphCorr) == 0.0) {
      int len = _ssrCorr->MakePhaseBias(&phasebias, _ssrCorr->PBTYPE_AUTO, 0, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferPhaseBias = QByteArray(obuffer, len);
      }
    }
  }

  // VTEC
  // ----
  QByteArray hlpBufferVtec;
  if (vtec.NumLayers > 0) {
    char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
    if (fmod(epoTime.gpssec(), _samplRtcmVtec) == 0.0) {
      int len = _ssrCorr->MakeVTEC(&vtec, 0, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferVtec = QByteArray(obuffer, len);
      }
    }
  }

  // Service CRS
  // -----------
  QByteArray hlpBufferServiceCrs;
  if (sizeof(serviceCrs._name) > 0) {
    char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
    if (fmod(epoTime.gpssec(), _samplRtcmCrs) == 0.0) {
      int len = t_crsEncoder::RTCM3(serviceCrs, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferServiceCrs = QByteArray(obuffer, len);
      }
    }
  }

  // RTCM CRS
  // --------
  QByteArray hlpBufferRtcmCrs;
  if (sizeof(rtcmCrs._name) > 0) {
    char obuffer[CLOCKORBIT_BUFFERSIZE] = {'\0'};
    if (fmod(epoTime.gpssec(), _samplRtcmCrs) == 0.0) {
      int len = t_crsEncoder::RTCM3(rtcmCrs, obuffer, sizeof(obuffer));
      if (len > 0) {
        hlpBufferRtcmCrs = QByteArray(obuffer, len);
      }
    }
  }

  // Create OutBuffer
  // ----------------
  _outBuffer += hlpBufferCo + hlpBufferBias + hlpBufferPhaseBias + hlpBufferVtec
      + hlpBufferServiceCrs + hlpBufferRtcmCrs + '\0';
}
//
////////////////////////////////////////////////////////////////////////////
t_irc bncRtnetUploadCaster::processSatellite(const t_eph* eph, int GPSweek,
    double GPSweeks, const QString& prn, const ColumnVector& rtnAPC,
    double rtnUra, const ColumnVector& rtnClk, const ColumnVector& rtnVel,
    const ColumnVector& rtnCoM, const ColumnVector& rtnClkSig,
    struct SsrCorr::ClockOrbit::SatData* sd, QString& outLine) {

  // Broadcast Position and Velocity
  // -------------------------------
  ColumnVector xB(6);
  ColumnVector vB(3);
  t_irc irc = eph->getCrd(bncTime(GPSweek, GPSweeks), xB, vB, false);

  if (irc != success) {
    return irc;
  }

  // Precise Position
  // ----------------
  ColumnVector xP = _CoM ? rtnCoM : rtnAPC;

  if (xP.size() == 0) {
    return failure;
  }

  double dc = 0.0;
  if (_crdTrafoStr != "IGS20") {
    crdTrafo14(GPSweek, xP, dc); // ITRF2020 => ITRF2014
    crdTrafo(GPSweek, xP, dc);   // ITRF2014 to other reference frames
  }

  // Difference in xyz
  // -----------------
  ColumnVector dx = xB.Rows(1, 3) - xP;
  ColumnVector dv = vB - rtnVel;

  // Difference in RSW
  // -----------------
  ColumnVector rsw(3);
  XYZ_to_RSW(xB.Rows(1, 3), vB, dx, rsw);

  ColumnVector dotRsw(3);
  XYZ_to_RSW(xB.Rows(1, 3), vB, dv, dotRsw);

  // Clock Correction
  // ----------------
  double dClkA0 = rtnClk(1) - (xB(4) - dc) * t_CST::c;
  double dClkA1 = 0.0;
  if (rtnClk(2)) {
    dClkA1 = rtnClk(2) - xB(5) * t_CST::c;
  }
  double dClkA2 = 0.0;
  if (rtnClk(3)) {
    dClkA2 = rtnClk(3) - xB(6) * t_CST::c;
  }

  if (sd) {
    sd->ID = prn.mid(1).toInt();
    sd->IOD = eph->IOD();
    sd->Clock.DeltaA0 = dClkA0;
    sd->Clock.DeltaA1 = dClkA1;
    sd->Clock.DeltaA2 = dClkA2;
    sd->UserRangeAccuracy = rtnUra;
    sd->Orbit.DeltaRadial     = rsw(1);
    sd->Orbit.DeltaAlongTrack = rsw(2);
    sd->Orbit.DeltaCrossTrack = rsw(3);
    sd->Orbit.DotDeltaRadial     = dotRsw(1);
    sd->Orbit.DotDeltaAlongTrack = dotRsw(2);
    sd->Orbit.DotDeltaCrossTrack = dotRsw(3);

    if (corrIsOutOfRange(sd)) {
      emit(newMessage(QString("bncRtnetUploadCaster: SSR parameter is out of its defined range").toLatin1(), false));
      return failure;
    }
  }

  outLine = QString().asprintf("%d %.1f %s  %u  %10.3f %8.3f %8.3f  %8.3f %8.3f %8.3f\n", GPSweek,
      GPSweeks, eph->prn().toString().c_str(), eph->IOD(), dClkA0, dClkA1, dClkA2,
      rsw(1), rsw(2), rsw(3));  //fprintf(stderr, "%s\n", outLine.toStdString().c_str());

  // RTNET full clock for RINEX and SP3 file
  // ---------------------------------------
  double relativity = -2.0 * DotProduct(xP, rtnVel) / t_CST::c;
  double clkRnx     = (rtnClk[0] - relativity) / t_CST::c;  // [s]
  double clkRnxRate = rtnClk[1] / t_CST::c;                 // [s/s  = -]
  double clkRnxAcc  = rtnClk[2] / t_CST::c;                 // [s/s² = -/s]

  if (_rnx) {
    double clkRnxSig, clkRnxRateSig, clkRnxAccSig;
    int s = rtnClkSig.size();
    switch (s) {
      case 1:
        clkRnxSig     = rtnClkSig[0] / t_CST::c;    // [s]
        clkRnxRateSig = 0.0;                        // [s/s = -]
        clkRnxAccSig  = 0.0;                        // [s/s² ) -/s]
        break;
      case 2:
        clkRnxSig     = rtnClkSig[0] / t_CST::c;     // [s]
        clkRnxRateSig = rtnClkSig[1] / t_CST::c;     // [s/s = -]
        clkRnxAccSig  = 0.0;                         // [s/s² ) -/s]
        break;
      case 3:
        clkRnxSig     = rtnClkSig[0] / t_CST::c;     // [s]
        clkRnxRateSig = rtnClkSig[1] / t_CST::c;     // [s/s = -]
        clkRnxAccSig  = rtnClkSig[2] / t_CST::c;     // [s/s² ) -/s]
        break;
    }
    _rnx->write(GPSweek, GPSweeks, prn, clkRnx, clkRnxRate, clkRnxAcc,
                clkRnxSig, clkRnxRateSig, clkRnxAccSig);
  }
  if (_sp3) {
    _sp3->write(GPSweek, GPSweeks, prn, rtnCoM, clkRnx, rtnVel, clkRnxRate);
  }
  return success;
}

// Transform Coordinates
////////////////////////////////////////////////////////////////////////////
void bncRtnetUploadCaster::crdTrafo(int GPSWeek, ColumnVector& xyz,
    double& dc) {

  // Current epoch minus 2000.0 in years
  // ------------------------------------
  double dt = (GPSWeek - (1042.0 + 6.0 / 7.0)) / 365.2422 * 7.0 + 2000.0 - _t0;

  ColumnVector dx(3);

  dx(1) = _dx + dt * _dxr;
  dx(2) = _dy + dt * _dyr;
  dx(3) = _dz + dt * _dzr;

  static const double arcSec = 180.0 * 3600.0 / M_PI;

  double ox = (_ox + dt * _oxr) / arcSec;
  double oy = (_oy + dt * _oyr) / arcSec;
  double oz = (_oz + dt * _ozr) / arcSec;

  double sc = 1.0 + _sc * 1e-9 + dt * _scr * 1e-9;

  // Specify approximate center of area
  // ----------------------------------
  ColumnVector meanSta(3);

  if (_crdTrafoStr == "ETRF2000") {
    meanSta(1) = 3661090.0;
    meanSta(2) = 845230.0;
    meanSta(3) = 5136850.0;
  }
  else if (_crdTrafoStr == "GDA2020") {
    meanSta(1) = -4052050.0;
    meanSta(2) = 4212840.0;
    meanSta(3) = -2545110.0;
  }
  else if (_crdTrafoStr == "SIRGAS2000") {
    meanSta(1) = 3740860.0;
    meanSta(2) = -4964290.0;
    meanSta(3) = -1425420.0;
  }
  else if (_crdTrafoStr == "DREF91") {
    meanSta(1) = 3959579.0;
    meanSta(2) = 721719.0;
    meanSta(3) = 4931539.0;
  }
  else if (_crdTrafoStr == "Custom") {
    meanSta(1) = 0.0;
    meanSta(2) = 0.0;
    meanSta(3) = 0.0;
  }

  // Clock correction proportional to topocentric distance to satellites
  // -------------------------------------------------------------------
  double rho = (xyz - meanSta).NormFrobenius();
  dc = rho * (sc - 1.0) / sc / t_CST::c;

  Matrix rMat(3, 3);
  rMat(1, 1) = 1.0;
  rMat(1, 2) = -oz;
  rMat(1, 3) = oy;
  rMat(2, 1) = oz;
  rMat(2, 2) = 1.0;
  rMat(2, 3) = -ox;
  rMat(3, 1) = -oy;
  rMat(3, 2) = ox;
  rMat(3, 3) = 1.0;

  xyz = sc * rMat * xyz + dx;
}

// Transform Coordinates
////////////////////////////////////////////////////////////////////////////
void bncRtnetUploadCaster::crdTrafo14(int GPSWeek, ColumnVector& xyz,
    double& dc) {

  // Current epoch minus 2000.0 in years
  // ------------------------------------
  double dt = (GPSWeek - (1042.0 + 6.0 / 7.0)) / 365.2422 * 7.0 + 2000.0 - _t0;

  ColumnVector dx(3);

  dx(1) = _dx14 + dt * _dxr14;
  dx(2) = _dy14 + dt * _dyr14;
  dx(3) = _dz14 + dt * _dzr14;

  static const double arcSec = 180.0 * 3600.0 / M_PI;

  double ox = (_ox14 + dt * _oxr14) / arcSec;
  double oy = (_oy14 + dt * _oyr14) / arcSec;
  double oz = (_oz14 + dt * _ozr14) / arcSec;

  double sc = 1.0 + _sc14 * 1e-9 + dt * _scr14 * 1e-9;

  // Specify approximate center of area
  // ----------------------------------
  ColumnVector meanSta(3);
  meanSta(1) = 0.0; // TODO
  meanSta(2) = 0.0; // TODO
  meanSta(3) = 0.0; // TODO

  // Clock correction proportional to topocentric distance to satellites
  // -------------------------------------------------------------------
  double rho = (xyz - meanSta).NormFrobenius();
  dc = rho * (sc - 1.0) / sc / t_CST::c;

  Matrix rMat(3, 3);
  rMat(1, 1) = 1.0;
  rMat(1, 2) = -oz;
  rMat(1, 3) = oy;
  rMat(2, 1) = oz;
  rMat(2, 2) = 1.0;
  rMat(2, 3) = -ox;
  rMat(3, 1) = -oy;
  rMat(3, 2) = ox;
  rMat(3, 3) = 1.0;

  xyz = sc * rMat * xyz + dx;
}

// Update Interval
////////////////////////////////////////////////////////////////////////////
int bncRtnetUploadCaster::determineUpdateInd(double samplingRate) {

  if (samplingRate == 10.0) {
    return 3;
  }
  else if (samplingRate == 15.0) {
    return 4;
  }
  else if (samplingRate == 30.0) {
    return 5;
  }
  else if (samplingRate == 60.0) {
    return 6;
  }
  else if (samplingRate == 120.0) {
    return 7;
  }
  else if (samplingRate == 240.0) {
    return 8;
  }
  else if (samplingRate == 300.0) {
    return 9;
  }
  else if (samplingRate == 600.0) {
    return 10;
  }
  else if (samplingRate == 900.0) {
    return 11;
  }
  else if (samplingRate == 1800.0) {
    return 12;
  }
  else if (samplingRate == 3600.0) {
    return 13;
  }
  else if (samplingRate == 7200.0) {
    return 14;
  }
  else if (samplingRate == 10800.0) {
    return 15;
  }
  return 2;  // default
}

// Check corrections
////////////////////////////////////////////////////////////////////////////
bool bncRtnetUploadCaster::corrIsOutOfRange(struct SsrCorr::ClockOrbit::SatData* sd) {

  if (fabs(sd->Clock.DeltaA0) > 209.7151)   {return true;}
  if (fabs(sd->Clock.DeltaA1) > 1.048575)   {return true;}
  if (fabs(sd->Clock.DeltaA2) > 1.34217726) {return true;}

  if (fabs(sd->Orbit.DeltaRadial)     > 209.7151) {return true;}
  if (fabs(sd->Orbit.DeltaAlongTrack) > 209.7148) {return true;}
  if (fabs(sd->Orbit.DeltaCrossTrack) > 209.7148) {return true;}

  if (fabs(sd->Orbit.DotDeltaRadial)     > 1.048575) {return true;}
  if (fabs(sd->Orbit.DotDeltaAlongTrack) > 1.048572) {return true;}
  if (fabs(sd->Orbit.DotDeltaCrossTrack) > 1.048572) {return true;}
  return false;
}
