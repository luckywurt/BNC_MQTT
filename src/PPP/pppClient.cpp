/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppClient
 *
 * Purpose:    PPP Client processing starts here
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <QThreadStorage>

#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>

#include "pppClient.h"
#include "pppEphPool.h"
#include "pppObsPool.h"
#include "bncconst.h"
#include "bncutils.h"
#include "pppStation.h"
#include "bncantex.h"
#include "pppFilter.h"

using namespace BNC_PPP;
using namespace std;

// Global variable holding thread-specific pointers
//////////////////////////////////////////////////////////////////////////////
QThreadStorage<t_pppClient*> CLIENTS;

// Static function returning thread-specific pointer
//////////////////////////////////////////////////////////////////////////////
t_pppClient* t_pppClient::instance() {
  return CLIENTS.localData();
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_pppClient::t_pppClient(const t_pppOptions* opt) {
  _running = true;
  _output   = 0;
  _opt      = new t_pppOptions(*opt);
  _log      = new ostringstream();
  _ephPool  = new t_pppEphPool();
  _obsPool  = new t_pppObsPool();
  _staRover = new t_pppStation();
  _filter   = new t_pppFilter();
  _tides    = new t_tides();
  _antex    = 0;
  if (!_opt->_antexFileName.empty()) {
    _antex = new bncAntex(_opt->_antexFileName.c_str());
  }
  if (!_opt->_blqFileName.empty()) {
    if (_tides->readBlqFile(_opt->_blqFileName.c_str()) == success) {
      //_tides->printAllBlqSets();
    }
  }
  CLIENTS.setLocalData(this);  // CLIENTS takes ownership over "this"
}

// Destructor
//////////////////////////////////////////////////////////////////////////////
t_pppClient::~t_pppClient() {
  _running = false;
  delete _log;
  delete _opt;
  delete _ephPool;
  delete _obsPool;
  delete _staRover;
  if (_antex) {
    delete _antex;
  }
  delete _filter;
  delete _tides;
  clearObs();
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putEphemeris(const t_eph* eph) {
  const t_ephGPS* ephGPS = dynamic_cast<const t_ephGPS*>(eph);
  const t_ephGlo* ephGlo = dynamic_cast<const t_ephGlo*>(eph);
  const t_ephGal* ephGal = dynamic_cast<const t_ephGal*>(eph);
  const t_ephBDS* ephBDS = dynamic_cast<const t_ephBDS*>(eph);
  if      (ephGPS) {
    _ephPool->putEphemeris(new t_ephGPS(*ephGPS));
  }
  else if (ephGlo) {
    _ephPool->putEphemeris(new t_ephGlo(*ephGlo));
  }
  else if (ephGal) {
    _ephPool->putEphemeris(new t_ephGal(*ephGal));
  }
  else if (ephBDS) {
    _ephPool->putEphemeris(new t_ephBDS(*ephBDS));
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putTec(const t_vTec* vTec) {
  _obsPool->putTec(new t_vTec(*vTec));
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putOrbCorrections(const vector<t_orbCorr*>& corr) {
  for (unsigned ii = 0; ii < corr.size(); ii++) {
    _ephPool->putOrbCorrection(new t_orbCorr(*corr[ii]));
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putClkCorrections(const vector<t_clkCorr*>& corr) {
  for (unsigned ii = 0; ii < corr.size(); ii++) {
    _ephPool->putClkCorrection(new t_clkCorr(*corr[ii]));
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putCodeBiases(const vector<t_satCodeBias*>& biases) {
  for (unsigned ii = 0; ii < biases.size(); ii++) {
    _obsPool->putCodeBias(new t_satCodeBias(*biases[ii]));
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::putPhaseBiases(const vector<t_satPhaseBias*>& biases) {
  for (unsigned ii = 0; ii < biases.size(); ii++) {
    _obsPool->putPhaseBias(new t_satPhaseBias(*biases[ii]));
  }
}

//
//////////////////////////////////////////////////////////////////////////////
t_irc t_pppClient::prepareObs(const vector<t_satObs*>& satObs,
                              vector<t_pppSatObs*>& obsVector, bncTime& epoTime) {

  // Default
  // -------
  epoTime.reset();
  clearObs();

  // Create vector of valid observations
  // -----------------------------------
  for (unsigned ii = 0; ii < satObs.size(); ii++) {
    char sys = satObs[ii]->_prn.system();
    if (_opt->useSystem(sys)) {
      t_pppSatObs* pppSatObs = new t_pppSatObs(*satObs[ii]);
      if (pppSatObs->isValid()) {
        obsVector.push_back(pppSatObs);
      }
      else {
        delete pppSatObs;
      }
    }
  }

  // Check whether data are synchronized, compute epoTime
  // ----------------------------------------------------
  const double MAXSYNC = 0.05; // synchronization limit
  double meanDt = 0.0;
  for (unsigned ii = 0; ii < obsVector.size(); ii++) {
    const t_pppSatObs* satObs = obsVector.at(ii);
    if (epoTime.undef()) {
      epoTime = satObs->time();
    }
    else {
      double dt = satObs->time() - epoTime;
      if (fabs(dt) > MAXSYNC) {
        LOG << "t_pppClient::prepareObs asynchronous observations" << endl;
        return failure;
      }
      meanDt += dt;
    }
  }

  if (obsVector.size() > 0) {
    epoTime += meanDt / obsVector.size();
  }

  return success;
}

//
//////////////////////////////////////////////////////////////////////////////
bool t_pppClient::preparePseudoObs(std::vector<t_pppSatObs*>& obsVector) {

  bool pseudoObsIono = false;

  if (_opt->_pseudoObsIono) {
    vector<t_pppSatObs*>::iterator it = obsVector.begin();
    while (it != obsVector.end()) {
      t_pppSatObs* satObs = *it;
      pseudoObsIono = satObs->setPseudoObsIono(t_frequency::G1);
      it++;
    }
  }
/*
  vector<t_pppSatObs*>::iterator it = obsVector.begin();
  while (it != obsVector.end()) {
    t_pppSatObs* satObs = *it;
    satObs->printObsMinusComputed();
    it++;
  }
*/
  return pseudoObsIono;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::useObsWithCodeBiasesOnly(std::vector<t_pppSatObs*>& obsVector) {

  vector<t_pppSatObs*>::iterator it = obsVector.begin();
  while (it != obsVector.end()) {
    t_pppSatObs* pppSatObs = *it;
    bool codeBiasesAvailable = false;
    if (pppSatObs->getCodeBias(pppSatObs->fType1()) &&
        pppSatObs->getCodeBias(pppSatObs->fType2())) {
        codeBiasesAvailable = true;
    }
    if (codeBiasesAvailable) {
      ++it;
    }
    else {
      it = obsVector.erase(it);
      delete pppSatObs;
    }
  }
  return;
}


// Compute the Bancroft position, check for blunders
//////////////////////////////////////////////////////////////////////////////
t_irc t_pppClient::cmpBancroft(const bncTime& epoTime,
                                  vector<t_pppSatObs*>& obsVector,
                                  ColumnVector& xyzc, bool print) {
  t_lc::type tLC = t_lc::dummy;
  int  numBancroft = obsVector.size();

  while (_running) {
    Matrix BB(numBancroft, 4);
    int iObs = -1;
    for (unsigned ii = 0; ii < obsVector.size(); ii++) {
      const t_pppSatObs* satObs = obsVector.at(ii);
      if (tLC == t_lc::dummy) {
        if (satObs->isValid(t_lc::cIF)) {
          tLC = t_lc::cIF;
        }
        else if (satObs->isValid(t_lc::c1)) {
          tLC = t_lc::c1;
        }
        else if (satObs->isValid(t_lc::c2)) {
          tLC = t_lc::c2;
        }
      }
      if ( satObs->isValid(tLC) &&
          (!satObs->modelSet() || satObs->eleSat() >= _opt->_minEle) ) {
        ++iObs;
        BB[iObs][0] = satObs->xc()[0];
        BB[iObs][1] = satObs->xc()[1];
        BB[iObs][2] = satObs->xc()[2];
        BB[iObs][3] = satObs->obsValue(tLC) - satObs->cmpValueForBanc(tLC);
      }
    }
    if (iObs + 1 < _opt->_minObs) {
      LOG << "t_pppClient::cmpBancroft not enough observations: " << iObs + 1 << endl;
      return failure;
    }
    BB = BB.Rows(1,iObs+1);
    if (bancroft(BB, xyzc) != success) {
      return failure;
    }

    xyzc[3] /= t_CST::c;

    // Check Blunders
    // --------------
    const double BLUNDER = 1500.0;
    double   maxRes      = 0.0;
    unsigned maxResIndex = 0;
    for (unsigned ii = 0; ii < obsVector.size(); ii++) {
      const t_pppSatObs* satObs = obsVector.at(ii);
      if (satObs->isValid() &&
          (!satObs->modelSet() || satObs->eleSat() >= _opt->_minEle) ) {
        ColumnVector rr = satObs->xc().Rows(1,3) - xyzc.Rows(1,3);
        double res = rr.NormFrobenius() - satObs->obsValue(tLC)
                   - (satObs->xc()[3] - xyzc[3]) * t_CST::c;
        if (fabs(res) > maxRes) {
          maxRes = fabs(res);
          maxResIndex = ii;
        }
      }
    }
    if (maxRes < BLUNDER) {
      if (print) {
        LOG.setf(ios::fixed);
        LOG << "\nPPP of Epoch ";
        if (!_epoTimeRover.undef()) LOG << string(_epoTimeRover);
        LOG << "\n---------------------------------------------------------------\n";
        LOG << string(epoTime) << " BANCROFT: "
            << setw(14) << setprecision(3) << xyzc[0] << ' '
            << setw(14) << setprecision(3) << xyzc[1] << ' '
            << setw(14) << setprecision(3) << xyzc[2] << ' '
            << setw(14) << setprecision(3) << xyzc[3] * t_CST::c << endl << endl;
      }
      break;
    }
    else {
      t_pppSatObs* satObs = obsVector.at(maxResIndex);
      LOG << "t_pppClient::cmpBancroft Outlier " << satObs->prn().toString()
          << " " << maxRes << endl;
      delete satObs;
      obsVector.erase(obsVector.begin() + maxResIndex);
    }
  }

  return success;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::initOutput(t_output* output) {
  _output = output;
  _output->_numSat = 0;
  _output->_hDop   = 0.0;
  _output->_error  = false;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::clearObs() {
  for (unsigned ii = 0; ii < _obsRover.size(); ii++) {
    delete _obsRover.at(ii);
  }
  _obsRover.clear();
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::finish(t_irc irc, int ind) {
#ifdef BNC_DEBUG_PPP
  LOG << "t_pppClient::finish(" << ind << "): " << irc << endl;
#endif

  clearObs();

  _output->_epoTime = _epoTimeRover;

  if (irc == success) {
    _output->_xyzRover[0] = _staRover->xyzApr()[0] + _filter->x()[0];
    _output->_xyzRover[1] = _staRover->xyzApr()[1] + _filter->x()[1];
    _output->_xyzRover[2] = _staRover->xyzApr()[2] + _filter->x()[2];

    xyz2neu(_staRover->ellApr().data(), _filter->x().data(), _output->_neu);

    copy(&_filter->Q().data()[0], &_filter->Q().data()[6], _output->_covMatrix);

    _output->_trp0     = t_tropo::delay_saast(_staRover->xyzApr(), M_PI/2.0);
    _output->_trp      = _filter->trp();
    _output->_trpStdev = _filter->trpStdev();

    _output->_numSat   = _filter->numSat();
    _output->_hDop     = _filter->HDOP();
    _output->_error = false;
  }
  else {
    _output->_error = true;
  }

  _output->_log = _log->str();
  delete _log; _log = new ostringstream();

  return;
}

//
//////////////////////////////////////////////////////////////////////////////
t_irc t_pppClient::cmpModel(t_pppStation* station, const ColumnVector& xyzc,
                               vector<t_pppSatObs*>& obsVector) {
  bncTime time;
  time = _epoTimeRover;
  station->setName(_opt->_roverName);
  station->setAntName(_opt->_antNameRover);
  station->setEpochTime(time);

  if (_opt->xyzAprRoverSet()) {
    station->setXyzApr(_opt->_xyzAprRover);
  }
  else {
    station->setXyzApr(xyzc.Rows(1,3));
  }
  station->setNeuEcc(_opt->_neuEccRover);

  // Receiver Clock
  // --------------
  station->setDClk(xyzc[3]);

  // Tides
  // -----
  station->setTideDsplEarth(_tides->earth(time, station->xyzApr()));
  station->setTideDsplOcean(_tides->ocean(time, station->xyzApr(), station->name()));

  // Observation model
  // -----------------
  vector<t_pppSatObs*>::iterator it = obsVector.begin();
  while (it != obsVector.end()) {
    t_pppSatObs* satObs = *it;
    t_irc modelSetup;
    if (satObs->isValid()) {
      modelSetup = satObs->cmpModel(station);
    }
    if (satObs->isValid() &&
        satObs->eleSat() >= _opt->_minEle &&
        modelSetup == success) {
      ++it;
    }
    else {
      delete satObs;
      it = obsVector.erase(it);
    }
  }

  return success;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::processEpoch(const vector<t_satObs*>& satObs, t_output* output) {

  try {
    initOutput(output);

    // Prepare Observations of the Rover
    // ---------------------------------
    if (prepareObs(satObs, _obsRover, _epoTimeRover) != success) {
      return finish(failure, 1);
    }

    for (int iter = 1; iter <= 2; iter++) {
      ColumnVector xyzc(4); xyzc = 0.0;
      bool print = (iter == 2);
      if (cmpBancroft(_epoTimeRover, _obsRover, xyzc, print) != success) {
        return finish(failure, 2);
      }
       if (cmpModel(_staRover, xyzc, _obsRover) != success) {
        return finish(failure, 3);
      }
    }
    // use observations only if satellite code biases are available
    /* ------------------------------------------------------------
    if (!_opt->_corrMount.empty() {
        useObsWithCodeBiasesOnly(_obsRover);
    }*/

    if (int(_obsRover.size()) < _opt->_minObs) {
      LOG << "t_pppClient::processEpoch not enough observations" << endl;
      return finish(failure, 4);
    }

    // Prepare Pseudo Observations of the Rover
    // ----------------------------------------
    _pseudoObsIono = preparePseudoObs(_obsRover);

    // Store last epoch of data
    // ------------------------
    _obsPool->putEpoch(_epoTimeRover, _obsRover, _pseudoObsIono);

    // Process Epoch in Filter
    // -----------------------
    if (_filter->processEpoch(_obsPool) != success) {
      LOG << "t_pppFilter::processEpoch() != success" << endl;
      return finish(failure, 5);
    }
  }
  catch (Exception& exc) {
    LOG << exc.what() << endl;
    return finish(failure, 6);
  }
  catch (t_except msg) {
    LOG << msg.what() << endl;
    return finish(failure, 7);
  }
  catch (const char* msg) {
    LOG << msg << endl;
    return finish(failure, 8);
  }
  catch (...) {
    LOG << "unknown exception" << endl;
    return finish(failure, 9);
  }
  return finish(success, 0);
}

//
////////////////////////////////////////////////////////////////////////////
double lorentz(const ColumnVector& aa, const ColumnVector& bb) {
  return aa[0]*bb[0] +  aa[1]*bb[1] +  aa[2]*bb[2] -  aa[3]*bb[3];
}

//
////////////////////////////////////////////////////////////////////////////
t_irc t_pppClient::bancroft(const Matrix& BBpass, ColumnVector& pos) {

  if (pos.Nrows() != 4) {
    pos.ReSize(4);
  }
  pos = 0.0;

  for (int iter = 1; iter <= 2; iter++) {
    Matrix BB = BBpass;
    int mm = BB.Nrows();
    for (int ii = 1; ii <= mm; ii++) {
      double xx = BB(ii,1);
      double yy = BB(ii,2);
      double traveltime = 0.072;
      if (iter > 1) {
        double zz  = BB(ii,3);
        double rho = sqrt( (xx-pos(1)) * (xx-pos(1)) +
                           (yy-pos(2)) * (yy-pos(2)) +
                           (zz-pos(3)) * (zz-pos(3)) );
        traveltime = rho / t_CST::c;
      }
      double angle = traveltime * t_CST::omega;
      double cosa  = cos(angle);
      double sina  = sin(angle);
      BB(ii,1) =  cosa * xx + sina * yy;
      BB(ii,2) = -sina * xx + cosa * yy;
    }

    Matrix BBB;
    if (mm > 4) {
      SymmetricMatrix hlp; hlp << BB.t() * BB;
      BBB = hlp.i() * BB.t();
    }
    else {
      BBB = BB.i();
    }
    ColumnVector ee(mm); ee = 1.0;
    ColumnVector alpha(mm); alpha = 0.0;
    for (int ii = 1; ii <= mm; ii++) {
      alpha(ii) = lorentz(BB.Row(ii).t(),BB.Row(ii).t())/2.0;
    }
    ColumnVector BBBe     = BBB * ee;
    ColumnVector BBBalpha = BBB * alpha;
    double aa = lorentz(BBBe, BBBe);
    double bb = lorentz(BBBe, BBBalpha)-1;
    double cc = lorentz(BBBalpha, BBBalpha);
    double root = sqrt(bb*bb-aa*cc);

    Matrix hlpPos(4,2);
    hlpPos.Column(1) = (-bb-root)/aa * BBBe + BBBalpha;
    hlpPos.Column(2) = (-bb+root)/aa * BBBe + BBBalpha;

    ColumnVector omc(2);
    for (int pp = 1; pp <= 2; pp++) {
      hlpPos(4,pp)      = -hlpPos(4,pp);
      omc(pp) = BB(1,4) -
                sqrt( (BB(1,1)-hlpPos(1,pp)) * (BB(1,1)-hlpPos(1,pp)) +
                      (BB(1,2)-hlpPos(2,pp)) * (BB(1,2)-hlpPos(2,pp)) +
                      (BB(1,3)-hlpPos(3,pp)) * (BB(1,3)-hlpPos(3,pp)) ) -
                hlpPos(4,pp);
    }
    if ( fabs(omc(1)) > fabs(omc(2)) ) {
      pos = hlpPos.Column(2);
    }
    else {
      pos = hlpPos.Column(1);
    }
  }
  if (pos.size() != 4 ||
      std::isnan(pos(1)) ||
      std::isnan(pos(2)) ||
      std::isnan(pos(3)) ||
      std::isnan(pos(4)))  {
    return failure;
  }

  return success;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppClient::reset() {

  // to delete old orbit and clock corrections
  delete _ephPool;
  _ephPool  = new t_pppEphPool();

  // to delete old code biases
  delete _obsPool;
  _obsPool  = new t_pppObsPool();

  // to delete all parameters
  delete _filter;
  _filter   = new t_pppFilter();

}
