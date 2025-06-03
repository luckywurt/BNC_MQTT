/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppObsPool
 *
 * Purpose:    Buffer with observations
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include "pppObsPool.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_epoch::t_epoch(const bncTime& epoTime, vector<t_pppSatObs*>& obsVector,
                               bool pseudoObsIono) {
  _epoTime        = epoTime;
  _pseudoObsIono  = pseudoObsIono;
  for (unsigned ii = 0; ii < obsVector.size(); ii++) {
    _obsVector.push_back(obsVector[ii]);
  }
  obsVector.clear();
}

// Destructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_epoch::~t_epoch() {
  for (unsigned ii = 0; ii < _obsVector.size(); ii++) {
    delete _obsVector[ii];
  }
}

// Constructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::t_pppObsPool() {
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    _satCodeBiases[ii] = 0;
  }
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    _satPhaseBiases[ii] = 0;
  }
  _vTec = 0;
}

// Destructor
/////////////////////////////////////////////////////////////////////////////
t_pppObsPool::~t_pppObsPool() {
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    delete _satCodeBiases[ii];
  }
  for (unsigned ii = 0; ii <= t_prn::MAXPRN; ii++) {
    delete _satPhaseBiases[ii];
  }
  delete _vTec;
  while (_epochs.size() > 0) {
    delete _epochs.front();
    _epochs.pop_front();
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putCodeBias(t_satCodeBias* satCodeBias) {
  int iPrn = satCodeBias->_prn.toInt();
  delete _satCodeBiases[iPrn];
  _satCodeBiases[iPrn] = satCodeBias;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putPhaseBias(t_satPhaseBias* satPhaseBias) {
  int iPrn = satPhaseBias->_prn.toInt();
  delete _satPhaseBiases[iPrn];
  _satPhaseBiases[iPrn] = satPhaseBias;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putTec(t_vTec* vTec) {
  delete _vTec;
  _vTec = new t_vTec(*vTec);
  delete vTec;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppObsPool::putEpoch(const bncTime& epoTime, vector<t_pppSatObs*>& obsVector,
                            bool pseudoObsIono) {
  const unsigned MAXSIZE = 2;
  _epochs.push_back(new t_epoch(epoTime, obsVector, pseudoObsIono));

  if (_epochs.size() > MAXSIZE) {
    delete _epochs.front();
    _epochs.pop_front();
  }
}
