/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppEphPool
 *
 * Purpose:    Buffer with satellite ephemerides
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>
#include "pppEphPool.h"
#include "pppInclude.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::putEphemeris(t_eph* eph) {
  if (eph && (eph->checkState() != t_eph::bad ||
              eph->checkState() != t_eph::unhealthy ||
              eph->checkState() != t_eph::outdated))  {
    _satEphPool[eph->prn().toInt()].putEphemeris(_maxQueueSize, eph);
  }
  else {
    delete eph;
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::putOrbCorrection(t_orbCorr* corr) {
  if (corr) {
    _satEphPool[corr->_prn.toInt()].putOrbCorrection(corr);
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::putClkCorrection(t_clkCorr* corr) {
  if (corr) {
    _satEphPool[corr->_prn.toInt()].putClkCorrection(corr);
  }
}

//
/////////////////////////////////////////////////////////////////////////////
t_irc t_pppEphPool::getCrd(const t_prn& prn, const bncTime& tt,
                             ColumnVector& xc, ColumnVector& vv) const {
  return _satEphPool[prn.toInt()].getCrd(tt, xc, vv);
}

//
/////////////////////////////////////////////////////////////////////////////
int t_pppEphPool::getChannel(const t_prn& prn) const {
  return _satEphPool[prn.toInt()].getChannel();
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::t_satEphPool::putEphemeris(unsigned maxQueueSize, t_eph* eph) {
  if (_ephs.empty() || eph->isNewerThan(_ephs.front())) {
    _ephs.push_front(eph);
    if (maxQueueSize > 0 && _ephs.size() > maxQueueSize) {
      delete _ephs.back();
      _ephs.pop_back();
    }
  }
  else {
    delete eph;
  }
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::t_satEphPool::putOrbCorrection(t_orbCorr* corr) {
  if (_ephs.empty()) {
    return;
  }

  for (unsigned ii = 0; ii < _ephs.size(); ii++) {
    t_eph* eph = _ephs[ii];
    if (eph->IOD() == corr->_iod) {
      eph->setOrbCorr(corr);
    }
  }
  delete corr;
}

//
/////////////////////////////////////////////////////////////////////////////
void t_pppEphPool::t_satEphPool::putClkCorrection(t_clkCorr* corr) {
  if (_ephs.empty()) {
    return;
  }

  for (unsigned ii = 0; ii < _ephs.size(); ii++) {
    t_eph* eph = _ephs[ii];
    if (eph->IOD() == corr->_iod) {
      eph->setClkCorr(corr);
    }
  }
  delete corr;
}

//
/////////////////////////////////////////////////////////////////////////////
t_irc t_pppEphPool::t_satEphPool::getCrd(const bncTime& tt, ColumnVector& xc,
                                           ColumnVector& vv) const {
  if (_ephs.empty()) {
    return failure;
  }

  for (unsigned ii = 0; ii < _ephs.size(); ii++) {
    const t_eph* eph = _ephs[ii];
    t_irc irc = eph->getCrd(tt, xc, vv, OPT->useOrbClkCorr());
    if (irc == success) {
        if (outDatedBcep(eph, tt)) {
          continue;
      }
      return irc;
    }
  }
  return failure;
}

//
/////////////////////////////////////////////////////////////////////////////
int t_pppEphPool::t_satEphPool::getChannel() const {
  if (!_ephs.empty()) {
    return _ephs[0]->slotNum();
  }
  return 0;
}
