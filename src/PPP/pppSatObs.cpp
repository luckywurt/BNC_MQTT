/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppSatObs
 *
 * Purpose:    Satellite observations
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/


#include <iostream>
#include <iomanip>
#include <cmath>
#include <newmatio.h>

#include "pppSatObs.h"
#include "bncconst.h"
#include "pppEphPool.h"
#include "pppStation.h"
#include "bncutils.h"
#include "bncantex.h"
#include "pppObsPool.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;


// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppSatObs::t_pppSatObs(const t_satObs& pppSatObs) {
  _prn        = pppSatObs._prn;
  _time       = pppSatObs._time;
  _outlier    = false;
  _valid      = true;
  _reference  = false;
  _stecSat    = 0.0;
  _signalPriorities = QString::fromStdString(OPT->_signalPriorities);
  for (unsigned ii = 0; ii < t_frequency::max; ii++) {
    _obs[ii] = 0;
  }
  prepareObs(pppSatObs);
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppSatObs::~t_pppSatObs() {
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    delete _obs[iFreq];
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::prepareObs(const t_satObs& pppSatObs) {

  _model.reset();

  std::vector<char> bb = OPT->frqBands(_prn.system());
  char frqNum1 = '0';
  if (bb.size() >= 1) {
    frqNum1 = bb[0];
  }
  char frqNum2 = '0';
  if (bb.size() == 2) {
    frqNum2 = bb[1];
  }

  // Select pseudo-ranges and phase observations
  // -------------------------------------------
  QStringList priorList = _signalPriorities.split(" ", Qt::SkipEmptyParts);
  string preferredAttrib;
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    t_frequency::type frqType = static_cast<t_frequency::type>(iFreq);
    char frqSys = t_frequency::toString(frqType)[0]; //cout << "frqSys: " << frqSys << endl;
    char frqNum = t_frequency::toString(frqType)[1]; //cout << "frqNum: " << frqNum << endl;
    if (frqSys != _prn.system()) {
      continue;
    }
    if (frqNum != frqNum1 &&
        frqNum != frqNum2 ) {
      continue;
    }
    QStringList hlp;
    for (int ii = 0; ii < priorList.size(); ii++) {
      if (priorList[ii].indexOf(":") != -1) {
        hlp = priorList[ii].split(":", Qt::SkipEmptyParts);
        if (hlp.size() == 2 && hlp[0].length() == 1 && hlp[0][0] == frqSys) {
          hlp = hlp[1].split("&", Qt::SkipEmptyParts);
        }
        if (hlp.size() == 2 && hlp[0].indexOf(frqNum) != -1) {
          preferredAttrib = hlp[1].toStdString(); //cout << "preferredAttrib: " << preferredAttrib << endl;
        }
      }
      for (unsigned iPref = 0; iPref < preferredAttrib.length(); iPref++) {
        QString obsType = QString("%1").arg(frqNum) + preferredAttrib[iPref];  //cout << "obstype: " << obsType.toStdString().c_str() << endl;
        if (_obs[iFreq] == 0) {
          for (unsigned ii = 0; ii < pppSatObs._obs.size(); ii++) {
            const t_frqObs* obs = pppSatObs._obs[ii];
            //cout << "observation2char: " << obs->_rnxType2ch << " vs. " << obsType.toStdString().c_str()<< endl;
            if (obs->_rnxType2ch == obsType.toStdString() &&
                obs->_codeValid  && obs->_code &&
                obs->_phaseValid && obs->_phase) {
              _obs[iFreq] = new t_frqObs(*obs); //cout << "================> newObs: " << obs->_rnxType2ch << " obs->_lockTime: " << obs->_lockTime << endl;
            }
          }
        }
      }
    }
  }

  // Used frequency types
  // --------------------
  _fType1 = t_frqBand::toFreq(_prn.system(), frqNum1);
  _fType2 = t_frqBand::toFreq(_prn.system(), frqNum2);

  // Check whether all required frequencies available
  // ------------------------------------------------
  for (unsigned ii = 0; ii < OPT->LCs(_prn.system()).size(); ii++) {
    t_lc::type tLC = OPT->LCs(_prn.system())[ii];
    if (tLC == t_lc::GIM) {continue;}
    if (!isValid(tLC)) {
      _valid = false;
      return;
    }
  }

  // Find GLONASS Channel Number
  // ---------------------------
  if (_prn.system() == 'R') {
    _channel = PPP_CLIENT->ephPool()->getChannel(_prn);
  }
  else {
    _channel = 0;
  }

  // Compute Satellite Coordinates at Time of Transmission
  // -----------------------------------------------------
  _xcSat.ReSize(6); _xcSat = 0.0;
  _vvSat.ReSize(3); _vvSat = 0.0;
  bool totOK  = false;
  ColumnVector satPosOld(6); satPosOld = 0.0;
  t_lc::type tLC = t_lc::dummy;
  if (isValid(t_lc::cIF)) {
    tLC = t_lc::cIF;
  }
  if (tLC == t_lc::dummy && isValid(t_lc::c1)) {
      tLC = t_lc::c1;
  }
  if (tLC == t_lc::dummy && isValid(t_lc::c2)) {
      tLC = t_lc::c2;
  }
  if (tLC == t_lc::dummy) {
    _valid = false;
    return;
  }
  double prange = obsValue(tLC);
  for (int ii = 1; ii <= 10; ii++) {
    bncTime ToT = _time - prange / t_CST::c - _xcSat[3];
    if (PPP_CLIENT->ephPool()->getCrd(_prn, ToT, _xcSat, _vvSat) != success) {
      _valid = false;
      return;
    }
    ColumnVector dx = _xcSat - satPosOld;
    dx[3] *= t_CST::c;
    if (dx.NormFrobenius() < 1.e-4) {
      totOK = true;
      break;
    }
    satPosOld = _xcSat;
  }
  if (totOK) {
    _signalPropagationTime = prange / t_CST::c - _xcSat[3];
    _model._satClkM = _xcSat[3] * t_CST::c;
  }
  else {
    _valid = false;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::lcCoeff(t_lc::type tLC,
                          map<t_frequency::type, double>& codeCoeff,
                          map<t_frequency::type, double>& phaseCoeff,
                          map<t_frequency::type, double>& ionoCoeff) const {

  codeCoeff.clear();
  phaseCoeff.clear();
  ionoCoeff.clear();

  double f1 = t_CST::freq(_fType1, _channel);
  double f2 = t_CST::freq(_fType2, _channel);
  double f1GPS = t_CST::freq(t_frequency::G1, 0);

  switch (tLC) {
  case t_lc::l1:
    phaseCoeff[_fType1] =  1.0;
    ionoCoeff [_fType1] = -1.0 * pow(f1GPS, 2) / pow(f1, 2);
    return;
  case t_lc::l2:
    phaseCoeff[_fType2] =  1.0;
    ionoCoeff [_fType2] = -1.0 * pow(f1GPS, 2) / pow(f2, 2);
    return;
  case t_lc::lIF:
    phaseCoeff[_fType1] =  f1 * f1 / (f1 * f1 - f2 * f2);
    phaseCoeff[_fType2] = -f2 * f2 / (f1 * f1 - f2 * f2);
    return;
  case t_lc::MW:
    phaseCoeff[_fType1] =  f1 / (f1 - f2);
    phaseCoeff[_fType2] = -f2 / (f1 - f2);
    codeCoeff[_fType1]  = -f1 / (f1 + f2);
    codeCoeff[_fType2]  = -f2 / (f1 + f2);
    return;
  case t_lc::CL:
    phaseCoeff[_fType1] =  0.5;
    codeCoeff [_fType1] =  0.5;
    return;
  case t_lc::c1:
    codeCoeff[_fType1] = 1.0;
    ionoCoeff[_fType1] = pow(f1GPS, 2) / pow(f1, 2);
    return;
  case t_lc::c2:
    codeCoeff[_fType2] = 1.0;
    ionoCoeff[_fType2] = pow(f1GPS, 2) / pow(f2, 2);
    return;
  case t_lc::cIF:
    codeCoeff[_fType1] =  f1 * f1 / (f1 * f1 - f2 * f2);
    codeCoeff[_fType2] = -f2 * f2 / (f1 * f1 - f2 * f2);
    return;
  case t_lc::GIM:
  case t_lc::dummy:
  case t_lc::maxLc:
    return;
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool t_pppSatObs::isValid(t_lc::type tLC) const {
  bool valid = true;
  obsValue(tLC, &valid);

  return valid;
}
//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::obsValue(t_lc::type tLC, bool* valid) const {

  double retVal = 0.0;
  if (valid) *valid = true;

  // Pseudo observations
  if (tLC == t_lc::GIM) {
    if (_stecSat == 0.0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      return _stecSat;
    }
  }

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(tLC, codeCoeff, phaseCoeff, ionoCoeff);

  map<t_frequency::type, double>::const_iterator it;

  // Code observations
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++) {
    t_frequency::type tFreq = it->first;
    if (_obs[tFreq] == 0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      retVal += it->second * _obs[tFreq]->_code;
    }
  }
  // Phase observations
  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    t_frequency::type tFreq = it->first;
    if (_obs[tFreq] == 0) {
      if (valid) *valid = false;
      return 0.0;
    }
    else {
      retVal += it->second * _obs[tFreq]->_phase * t_CST::lambda(tFreq, _channel);
    }
  }
  return retVal;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::lambda(t_lc::type tLC) const {

  double f1 = t_CST::freq(_fType1, _channel);
  double f2 = t_CST::freq(_fType2, _channel);

  if      (tLC == t_lc::l1) {
    return t_CST::c / f1;
  }
  else if (tLC == t_lc::l2) {
    return t_CST::c / f2;
  }
  else if (tLC == t_lc::lIF) {
    return t_CST::c / (f1 + f2);
  }
  else if (tLC == t_lc::MW) {
    return t_CST::c / (f1 - f2);
  }
  else if (tLC == t_lc::CL) {
    return t_CST::c / f1 / 2.0;
  }

  return 0.0;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::sigma(t_lc::type tLC) const {

  double retVal = 0.0;
  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(tLC, codeCoeff, phaseCoeff, ionoCoeff);

  if (tLC == t_lc::GIM) {
    retVal = OPT->_sigmaGIM * OPT->_sigmaGIM;
  }

  map<t_frequency::type, double>::const_iterator it;
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++)   {
    retVal += it->second * it->second * OPT->_sigmaC1 * OPT->_sigmaC1;
  }

  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    retVal += it->second * it->second * OPT->_sigmaL1 * OPT->_sigmaL1;
  }

  retVal = sqrt(retVal);

  // De-Weight R
  // -----------
  if (_prn.system() == 'R'&& t_lc::includesCode(tLC)) {
    retVal *= 5.0;
  }

  // Elevation-Dependent Weighting
  // -----------------------------
  double cEle = 1.0;
  if ( (OPT->_eleWgtCode  && t_lc::includesCode(tLC)) ||
       (OPT->_eleWgtPhase && t_lc::includesPhase(tLC)) ) {
    double eleD = eleSat()*180.0/M_PI;
    double hlp  = fabs(90.0 - eleD);
    cEle = (1.0 + hlp*hlp*hlp*0.000004);
  }

  return cEle * retVal;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::maxRes(t_lc::type tLC) const {
  double retVal = 0.0;

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  lcCoeff(tLC, codeCoeff, phaseCoeff, ionoCoeff);

  map<t_frequency::type, double>::const_iterator it;
  for (it = codeCoeff.begin(); it != codeCoeff.end(); it++)   {
    retVal += it->second * it->second * OPT->_maxResC1 * OPT->_maxResC1;
  }
  for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
    retVal += it->second * it->second * OPT->_maxResL1 * OPT->_maxResL1;
  }
  if (tLC == t_lc::GIM) {
    retVal = OPT->_maxResGIM * OPT->_maxResGIM + OPT->_maxResGIM * OPT->_maxResGIM;
  }

  retVal = sqrt(retVal);

  return retVal;
}


//
////////////////////////////////////////////////////////////////////////////
t_irc t_pppSatObs::cmpModel(const t_pppStation* station) {

  // Reset all model values
  // ----------------------
  _model.reset();

  // Topocentric Satellite Position
  // ------------------------------
  ColumnVector rSat = _xcSat.Rows(1,3);
  ColumnVector rRec = station->xyzApr();
  ColumnVector rhoV = rSat - rRec;
  _model._rho = rhoV.NormFrobenius();

  ColumnVector vSat = _vvSat;

  ColumnVector neu(3);
  xyz2neu(station->ellApr().data(), rhoV.data(), neu.data());

  _model._eleSat = acos(sqrt(neu[0]*neu[0] + neu[1]*neu[1]) / _model._rho);
  if (neu[2] < 0.0) {
    _model._eleSat *= -1.0;
  }
  _model._azSat  = atan2(neu[1], neu[0]);

  // Sun unit vector
  ColumnVector xSun = t_astro::Sun(_time.mjddec());
  xSun /= xSun.norm_Frobenius();

  // Satellite unit vectors sz, sy, sx
  ColumnVector sz = -rSat / rSat.norm_Frobenius();
  ColumnVector sy = crossproduct(sz, xSun);
  ColumnVector sx = crossproduct(sy, sz);

  sx /= sx.norm_Frobenius();
  sy /= sy.norm_Frobenius();

  // LOS unit vector satellite --> receiver
  ColumnVector rho = rRec - rSat;
  rho /= rho.norm_Frobenius();

  // LOS vector in satellite frame
  ColumnVector u(3);
  u(1) = dotproduct(sx, rho);
  u(2) = dotproduct(sy, rho);
  u(3) = dotproduct(sz, rho);

  // Azimuth and elevation in satellite antenna frame
  _model._elTx = atan2(u(3),sqrt(pow(u(2),2)+pow(u(1),2)));
  _model._azTx = atan2(u(2),u(1));


  // Satellite Clocks
  // ----------------
  _model._satClkM = _xcSat[3] * t_CST::c; // satellite system specific

  // Receiver Clocks
  // ---------------
  _model._recClkM = station->dClk() * t_CST::c;

  // Sagnac Effect (correction due to Earth rotation)
  // ------------------------------------------------
  ColumnVector Omega(3);
  Omega[0] = 0.0;
  Omega[1] = 0.0;
  Omega[2] = t_CST::omega / t_CST::c;
  _model._sagnac = DotProduct(Omega, crossproduct(rSat, rRec));

  // Antenna Eccentricity
  // --------------------
  _model._antEcc = -DotProduct(station->xyzEcc(), rhoV) / _model._rho;

  // Antenna Phase Center Offsets and Variations
  // -------------------------------------------
  if (PPP_CLIENT->antex()) {
    for (unsigned ii = 0; ii < t_frequency::max; ii++) {
      t_frequency::type frqType = static_cast<t_frequency::type>(ii);
      string frqStr = t_frequency::toString(frqType);
      if (frqStr[0] != _prn.system()) {continue;}
      bool found;
      QString prn(_prn.toString().c_str());
      _model._antPCO[ii]  = PPP_CLIENT->antex()->rcvCorr(station->antName(), frqType, _model._eleSat, _model._azSat, found);
      _model._antPCO[ii] += PPP_CLIENT->antex()->satCorr(prn, frqType, _model._elTx, _model._azTx, found);
      if (OPT->_isAPC && found) {
        // the PCOs as given in the satellite antenna correction for all frequencies
        // have to be reduced by the PCO of the respective reference frequency
        if      (_prn.system() == 'G') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::G1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'R') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::R1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'E') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::E1, _model._elTx, _model._azTx, found);
        }
        else if (_prn.system() == 'C') {
          _model._antPCO[ii] -= PPP_CLIENT->antex()->satCorr(prn, t_frequency::C2, _model._elTx, _model._azTx, found);
        }
      }
    }
  }

  // Tropospheric Delay
  // ------------------
  _model._tropo  = t_tropo::delay_saast(rRec, _model._eleSat);

  // Code Biases
  // -----------
  const t_satCodeBias* satCodeBias = PPP_CLIENT->obsPool()->satCodeBias(_prn);
  if (satCodeBias) {
    for (unsigned ii = 0; ii < satCodeBias->_bias.size(); ii++) {
      const t_frqCodeBias& bias = satCodeBias->_bias[ii];
      for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
        string frqStr = t_frequency::toString(t_frequency::type(iFreq));
        if (frqStr[0] != _prn.system()) {
          continue;
        }
        const t_frqObs* obs = _obs[iFreq];
        if (obs &&
            obs->_rnxType2ch == bias._rnxType2ch) {
          _model._codeBias[iFreq]  = bias._value;
        }
      }
    }
  }

  // Phase Biases
  // -----------
  const t_satPhaseBias* satPhaseBias = PPP_CLIENT->obsPool()->satPhaseBias(_prn);
  double yaw = 0.0;
  bool ssr = false;
  if (satPhaseBias) {
    double dt = station->epochTime() - satPhaseBias->_time;
    if (satPhaseBias->_updateInt) {
      dt -= (0.5 * ssrUpdateInt[satPhaseBias->_updateInt]);
    }
    yaw = satPhaseBias->_yaw + satPhaseBias->_yawRate * dt;
    ssr = true;
    for (unsigned ii = 0; ii < satPhaseBias->_bias.size(); ii++) {
      const t_frqPhaseBias& bias = satPhaseBias->_bias[ii];
      for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
        string frqStr = t_frequency::toString(t_frequency::type(iFreq));
        if (frqStr[0] != _prn.system()) {
          continue;
        }
        const t_frqObs* obs = _obs[iFreq];
        if (obs &&
            obs->_rnxType2ch == bias._rnxType2ch) {
          _model._phaseBias[iFreq]  = bias._value;
        }
      }
    }
  }

  // Phase Wind-Up
  // -------------
  _model._windUp = station->windUp(_time, _prn, rSat, ssr, yaw, vSat) ;

  // Relativistic effect due to earth gravity
  // ----------------------------------------
  double a = rSat.NormFrobenius() + rRec.NormFrobenius();
  double b = (rSat - rRec).NormFrobenius();
  double gm = 3.986004418e14; // m3/s2
  _model._rel = 2 * gm / t_CST::c / t_CST::c * log((a + b) / (a - b));

  // Tidal Correction
  // ----------------
  _model._tideEarth = -DotProduct(station->tideDsplEarth(), rhoV) / _model._rho;
  _model._tideOcean = -DotProduct(station->tideDsplOcean(), rhoV) / _model._rho;

  // Ionospheric Delay
  // -----------------
  const t_vTec* vTec = PPP_CLIENT->obsPool()->vTec();
  bool vTecUsage = true;
  for (unsigned ii = 0; ii < OPT->LCs(_prn.system()).size(); ii++) {
    t_lc::type tLC = OPT->LCs(_prn.system())[ii];
    if (tLC == t_lc::cIF || tLC == t_lc::lIF) {
      vTecUsage = false;
    }
  }

  if (vTecUsage && vTec) {
    double stec  = station->stec(vTec, _signalPropagationTime, rSat);
    double f1GPS = t_CST::freq(t_frequency::G1, 0);
    for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
      if (OPT->_pseudoObsIono) {
        // For scaling the slant ionospheric delays the trick is to be consistent with units!
        // The conversion of TECU into meters requires the frequency of the signal.
        // Hence, GPS L1 frequency is used for all systems. The same is true for mu_i in lcCoeff().
        _model._ionoCodeDelay[iFreq] = 40.3E16 / pow(f1GPS, 2) * stec;
      }
      else { // PPP-RTK
        t_frequency::type frqType = static_cast<t_frequency::type>(iFreq);
        _model._ionoCodeDelay[iFreq] = 40.3E16 / pow(t_CST::freq(frqType, _channel), 2) * stec;
      }
    }
  }

  // Set Model Set Flag
  // ------------------
  _model._set = true;

  //printModel();

  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::printModel() const {

  LOG.setf(ios::fixed);
  LOG << "\nMODEL for Satellite " << _prn.toString() << (isReference() ? " (Reference Satellite)" : "")

      << "\n======================= " << endl
      << "PPP "
      <<  ((OPT->_pseudoObsIono) ? " with pseudo-observations for STEC" : "")          << endl
      << "RHO           : " << setw(12) << setprecision(3) << _model._rho              << endl
      << "ELE           : " << setw(12) << setprecision(3) << _model._eleSat * RHO_DEG << endl
      << "AZI           : " << setw(12) << setprecision(3) << _model._azSat  * RHO_DEG << endl
      << "SATCLK        : " << setw(12) << setprecision(3) << _model._satClkM          << endl
      << "RECCLK        : " << setw(12) << setprecision(3) << _model._recClkM          << endl
      << "SAGNAC        : " << setw(12) << setprecision(3) << _model._sagnac           << endl
      << "ANTECC        : " << setw(12) << setprecision(3) << _model._antEcc           << endl
      << "TROPO         : " << setw(12) << setprecision(3) << _model._tropo            << endl
      << "WINDUP        : " << setw(12) << setprecision(3) << _model._windUp           << endl
      << "REL           : " << setw(12) << setprecision(3) << _model._rel              << endl
      << "EARTH TIDES   : " << setw(12) << setprecision(3) << _model._tideEarth        << endl
      << "OCEAN TIDES   : " << setw(12) << setprecision(3) << _model._tideOcean        << endl
      << endl
      << "FREQUENCY DEPENDENT CORRECTIONS:" << endl
      << "-------------------------------" << endl;
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    if (_obs[iFreq]) {
      string frqStr = t_frequency::toString(t_frequency::type(iFreq));
      if (_prn.system() == frqStr[0]) {
      LOG << "PCO           : " << frqStr << setw(12) << setprecision(3) << _model._antPCO[iFreq]       << endl
          << "BIAS CODE     : " << frqStr << setw(12) << setprecision(3) << _model._codeBias[iFreq]     << "\t(" << _obs[iFreq]->_rnxType2ch[1] << ") " << endl
          << "BIAS PHASE    : " << frqStr << setw(12) << setprecision(3) << _model._phaseBias[iFreq]    << "\t(" << _obs[iFreq]->_rnxType2ch[1] << ") " << endl
          << "IONO CODEDELAY: " << frqStr << setw(12) << setprecision(3) << _model._ionoCodeDelay[iFreq]<< endl;
      }
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::printObsMinusComputed() const {
  LOG.setf(ios::fixed);
  LOG << "\nOBS-COMP for Satellite " << _prn.toString() << (isReference() ? " (Reference Satellite)" : "") << endl
       << "========================== " << endl;
  char sys = _prn.system();
  for (unsigned ii = 0; ii < OPT->LCs(sys).size(); ii++) {
    t_lc::type tLC = OPT->LCs(sys)[ii];
    LOG << "OBS-CMP " << setw(4) << t_lc::toString(tLC) << ": " << _prn.toString() << " "
        << setw(12) << setprecision(3) << obsValue(tLC) << " "
        << setw(12) << setprecision(3) << cmpValue(tLC) << " "
        << setw(12) << setprecision(3) << obsValue(tLC)  - cmpValue(tLC) << endl;
  }
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::cmpValueForBanc(t_lc::type tLC) const {
  return cmpValue(tLC) - _model._rho - _model._sagnac - _model._recClkM;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::cmpValue(t_lc::type tLC) const {
  double cmpValue;

  if      (!isValid(tLC)) {
    cmpValue =  0.0;
  }
  else if (tLC == t_lc::GIM) {
    cmpValue =  _stecSat;
  }
  else {
    // Non-Dispersive Part
    // -------------------
    double nonDisp = _model._rho
                   + _model._recClkM   - _model._satClkM
                   + _model._sagnac    + _model._antEcc    + _model._tropo
                   + _model._tideEarth + _model._tideOcean + _model._rel;

    // Add Dispersive Part
    // -------------------
    double dispPart = 0.0;
    map<t_frequency::type, double> codeCoeff;
    map<t_frequency::type, double> phaseCoeff;
    map<t_frequency::type, double> ionoCoeff;
    lcCoeff(tLC, codeCoeff, phaseCoeff, ionoCoeff);
    map<t_frequency::type, double>::const_iterator it;
    for (it = codeCoeff.begin(); it != codeCoeff.end(); it++) {
      t_frequency::type tFreq = it->first;
      dispPart += it->second * (_model._antPCO[tFreq] - _model._codeBias[tFreq]);
      if (OPT->PPP_RTK) {
        dispPart += it->second * (_model._ionoCodeDelay[tFreq]);
      }
    }
    for (it = phaseCoeff.begin(); it != phaseCoeff.end(); it++) {
      t_frequency::type tFreq = it->first;
      dispPart += it->second * (_model._antPCO[tFreq] - _model._phaseBias[tFreq] +
                                _model._windUp * t_CST::lambda(tFreq, _channel));
      if (OPT->PPP_RTK) {
        dispPart += it->second * (- _model._ionoCodeDelay[tFreq]);
      }
    }
    cmpValue = nonDisp + dispPart;
  }

  return cmpValue;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppSatObs::setRes(t_lc::type tLC, double res) {
  _res[tLC] = res;
}

//
////////////////////////////////////////////////////////////////////////////
double t_pppSatObs::getRes(t_lc::type tLC) const {
  map<t_lc::type, double>::const_iterator it = _res.find(tLC);
  if (it != _res.end()) {
    return it->second;
  }
  else {
    return 0.0;
  }
}

//
////////////////////////////////////////////////////////////////////////////
bool  t_pppSatObs::setPseudoObsIono(t_frequency::type freq) {
  bool pseudoObsIono = false;
  _stecSat    = _model._ionoCodeDelay[freq];
  if (_stecSat) {
    pseudoObsIono = true;
  }
  return pseudoObsIono;
}
