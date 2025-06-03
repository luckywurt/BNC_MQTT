/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppFilter
 *
 * Purpose:    Filter Adjustment
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
#include <newmat.h>
#include <newmatio.h>
#include <newmatap.h>

#include "pppFilter.h"
#include "bncutils.h"
#include "pppParlist.h"
#include "pppObsPool.h"
#include "pppStation.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppFilter::t_pppFilter() {
  _parlist = 0;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppFilter::~t_pppFilter() {
  delete _parlist;
}

// Process Single Epoch
////////////////////////////////////////////////////////////////////////////
t_irc t_pppFilter::processEpoch(t_pppObsPool* obsPool) {

  _numSat = 0;
  const double maxSolGap = 60.0;
  bool setNeuNoiseToZero = false;

  if (!_parlist) {
    _parlist = new t_pppParlist();
  }

  // Vector of all Observations
  // --------------------------
  t_pppObsPool::t_epoch *epoch = obsPool->lastEpoch();
  if (!epoch) {
    return failure;
  }
  vector<t_pppSatObs*> &allObs = epoch->obsVector();

  // Time of the Epoch
  // -----------------
  _epoTime = epoch->epoTime();

  if (!_firstEpoTime.valid() ||
      !_lastEpoTimeOK.valid()||
      (maxSolGap > 0.0 && _epoTime - _lastEpoTimeOK > maxSolGap)) {
    _firstEpoTime = _epoTime;
  }

  string epoTimeStr = string(_epoTime);

  // Set Parameters
  // --------------
  if (_parlist->set(_epoTime, allObs) != success) {
    return failure;
  }

  // Status Vector, Variance-Covariance Matrix
  // -----------------------------------------
  ColumnVector    xFltOld = _xFlt;
  SymmetricMatrix QFltOld = _QFlt;
  setStateVectorAndVarCovMatrix(xFltOld, QFltOld, setNeuNoiseToZero);

  // Process Satellite Systems separately
  // ------------------------------------
  for (unsigned iSys = 0; iSys < OPT->systems().size(); iSys++) {
    char sys = OPT->systems()[iSys];
    int num = 0;
    vector<t_pppSatObs*> obsVector;
    for (unsigned jj = 0; jj < allObs.size(); jj++) {
      if (allObs[jj]->prn().system() == sys) {
        obsVector.push_back(allObs[jj]);
        num++;
      }
    }
    LOG << epoTimeStr << " SATNUM " << sys << ' ' << right << setw(2) << num << endl;
    if (processSystem(OPT->LCs(sys), obsVector, epoch->pseudoObsIono()) != success) {
      LOG << "t_pppFilter::processSystem() !=  success: " << sys << endl;
      return failure;
    }
  }

  // close epoch processing
  // ----------------------
  cmpDOP(allObs);
  _parlist->printResult(_epoTime, _QFlt, _xFlt);
  _lastEpoTimeOK = _epoTime; // remember time of last successful epoch processing
  return success;
}

// Process Selected LCs
////////////////////////////////////////////////////////////////////////////
t_irc t_pppFilter::processSystem(const vector<t_lc::type> &LCs,
                                 const vector<t_pppSatObs*> &obsVector,
                                 bool pseudoObsIonoAvailable) {
  LOG.setf(ios::fixed);

  // Detect Cycle Slips
  // ------------------
  if (detectCycleSlips(LCs, obsVector) != success) {
    return failure;
  }

  ColumnVector    xSav = _xFlt;
  SymmetricMatrix QSav = _QFlt;
  string          epoTimeStr = string(_epoTime);
  const vector<t_pppParam*> &params = _parlist->params();
  unsigned        nPar = _parlist->nPar();

  unsigned usedLCs = LCs.size();
  if (OPT->_pseudoObsIono && !pseudoObsIonoAvailable) {
    usedLCs -= 1;  // GIM not used
  }
  // max Obs
  unsigned maxObs = obsVector.size() * usedLCs;

  // Outlier Detection Loop
  // ----------------------
  for (unsigned iOutlier = 0; iOutlier < maxObs; iOutlier++) {

    if (iOutlier > 0) {
      _xFlt = xSav;
      _QFlt = QSav;
    }

    // First-Design Matrix, Terms Observed-Computed, Weight Matrix
    // -----------------------------------------------------------
    Matrix AA(maxObs, nPar);   AA = 0.0;
    ColumnVector ll(maxObs);   ll = 0.0;
    DiagonalMatrix PP(maxObs); PP = 0.0;

    int iObs = -1;
    vector<t_pppSatObs*> usedObs;
    vector<t_lc::type> usedTypes;

    // Real Observations
    // =================
    int nSat = 0;
    for (unsigned ii = 0; ii < obsVector.size(); ii++) {
      t_pppSatObs *obs = obsVector[ii];
      if (iOutlier == 0) {
        obs->resetOutlier();
      }
      if (!obs->outlier()) {
        nSat++;
        for (unsigned jj = 0; jj < usedLCs; jj++) {
          const t_lc::type tLC = LCs[jj];
          if (tLC == t_lc::GIM) {
            continue;
          }
          ++iObs;
          usedObs.push_back(obs);
          usedTypes.push_back(tLC);
          for (unsigned iPar = 0; iPar < nPar; iPar++) {
            const t_pppParam *par = params[iPar];
            AA[iObs][iPar] = par->partial(_epoTime, obs, tLC);
          }

          ll[iObs] = obs->obsValue(tLC) - obs->cmpValue(tLC) - DotProduct(_x0, AA.Row(iObs + 1));
          PP[iObs] = 1.0 / (obs->sigma(tLC) * obs->sigma(tLC));
        }
      }
    }

    // Check number of observations
    // ----------------------------
    if (!nSat) {
      return failure;
    }

    // Pseudo Obs Iono
    // ================
    if (OPT->_pseudoObsIono && pseudoObsIonoAvailable) {
      for (unsigned ii = 0; ii < obsVector.size(); ii++) {
        t_pppSatObs *obs = obsVector[ii];
        if (!obs->outlier()) {
          for (unsigned jj = 0; jj < usedLCs; jj++) {
            const t_lc::type tLC = LCs[jj];
            if (tLC == t_lc::GIM) {
              ++iObs;
            } else {
              continue;
            }
            usedObs.push_back(obs);
            usedTypes.push_back(tLC);
            for (unsigned iPar = 0; iPar < nPar; iPar++) {
              const t_pppParam *par = params[iPar];
              AA[iObs][iPar] = par->partial(_epoTime, obs, tLC);
            }
            ll[iObs] = obs->obsValue(tLC) - obs->cmpValue(tLC) - DotProduct(_x0, AA.Row(iObs + 1));
            PP[iObs] = 1.0 / (obs->sigma(tLC) * obs->sigma(tLC));
          }
        }
      }
    }

    // Truncate matrices
    // -----------------
    AA = AA.Rows(1, iObs + 1);
    ll = ll.Rows(1, iObs + 1);
    PP = PP.SymSubMatrix(1, iObs + 1);

    // Kalman update step
    // ------------------
    kalman(AA, ll, PP, _QFlt, _xFlt);

    // Check Residuals
    // ---------------
    ColumnVector vv = AA * _xFlt - ll;
    double maxOutlier = 0.0;
    int maxOutlierIndex = -1;
    t_lc::type maxOutlierLC = t_lc::dummy;
    for (unsigned ii = 0; ii < usedObs.size(); ii++) {
      const t_lc::type tLC = usedTypes[ii];
      double res = fabs(vv[ii]);
      if (res > usedObs[ii]->maxRes(tLC)) {
        if (res > fabs(maxOutlier)) {
          maxOutlier = vv[ii];
          maxOutlierIndex = ii;
          maxOutlierLC = tLC;
        }
      }
    }

    // Mark outlier or break outlier detection loop
    // --------------------------------------------
    if (maxOutlierIndex > -1) {
      t_pppSatObs *obs = usedObs[maxOutlierIndex];
      t_pppParam *par = 0;
      LOG << epoTimeStr << " Outlier " << t_lc::toString(maxOutlierLC) << ' '
          << obs->prn().toString() << ' ' << setw(8) << setprecision(4)
          << maxOutlier << endl;
      for (unsigned iPar = 0; iPar < nPar; iPar++) {
        t_pppParam *hlp = params[iPar];
        if (hlp->type() == t_pppParam::amb &&
            hlp->prn()  == obs->prn() &&
            hlp->tLC()  == usedTypes[maxOutlierIndex]) {
          par = hlp;
        }
      }
      if (par) {
        resetAmb(obs->prn(), obsVector, maxOutlierLC, &QSav, &xSav);
      }
      else {
        obs->setOutlier();
      }
    }
    // Print Residuals
    // ---------------
    else {
        for (unsigned jj = 0; jj < LCs.size(); jj++) {
          for (unsigned ii = 0; ii < usedObs.size(); ii++) {
            const t_lc::type tLC = usedTypes[ii];
            t_pppSatObs *obs = usedObs[ii];
            if (tLC == LCs[jj]) {
              obs->setRes(tLC, vv[ii]);
              LOG << epoTimeStr << " RES " << left << setw(3)
                  << t_lc::toString(tLC) << right << ' '
                  << obs->prn().toString() << ' '
                  << setw(8) << setprecision(4) << vv[ii] << endl;
          }
        }
      }
      break;
    }
  }
  return success;
}

// Cycle-Slip Detection
////////////////////////////////////////////////////////////////////////////
t_irc t_pppFilter::detectCycleSlips(const vector<t_lc::type> &LCs,
                                    const vector<t_pppSatObs*> &obsVector) {

  double SLIP = 150.0;
  double fac = 1.0;
  if (_lastEpoTimeOK.valid()) {
    fac = _epoTime - _lastEpoTimeOK;
    if (fac > 60.0 || fac < 1.0) {
      fac = 1.0;
    }
  }
  SLIP *= fac;
  string epoTimeStr = string(_epoTime);
  const vector<t_pppParam*> &params = _parlist->params();

  for (unsigned ii = 0; ii < LCs.size(); ii++) {
    const t_lc::type &tLC = LCs[ii];
    if (t_lc::includesPhase(tLC)) {
      for (unsigned iObs = 0; iObs < obsVector.size(); iObs++) {
        const t_pppSatObs *obs = obsVector[iObs];

        // Check set Slips and Jump Counters
        // ---------------------------------
        bool slip = false;

        if (obs->slip()) {
          LOG << epoTimeStr << " Cycle slip set (obs) " << obs->prn().toString() << endl;
          slip = true;
        }

        if (_slips[obs->prn()]._obsSlipCounter != -1 &&
            _slips[obs->prn()]._obsSlipCounter != obs->slipCounter()) {
          LOG << epoTimeStr  << " Cycle slip set (obsSlipCounter) " << obs->prn().toString()  << endl;
          slip = true;
        }
        _slips[obs->prn()]._obsSlipCounter = obs->slipCounter();

        if (_slips[obs->prn()]._biasJumpCounter != -1 &&
            _slips[obs->prn()]._biasJumpCounter != obs->biasJumpCounter()) {
          LOG << epoTimeStr  << " Cycle slip set (biasJumpCounter) " << obs->prn().toString() << endl;
          slip = true;
        }
        _slips[obs->prn()]._biasJumpCounter = obs->biasJumpCounter();

        // Slip Set
        // --------
        if (slip) {
          resetAmb(obs->prn(), obsVector, tLC);
        }

        // Check Pre-Fit Residuals
        /* -----------------------
        else {
          ColumnVector AA(params.size());
          for (unsigned iPar = 0; iPar < params.size(); iPar++) {
            const t_pppParam* par = params[iPar];
            AA[iPar] = par->partial(_epoTime, obs, tLC);
          }
          double ll = obs->obsValue(tLC) - obs->cmpValue(tLC) - DotProduct(_x0, AA);
          double vv = DotProduct(AA, _xFlt) - ll;

          if (fabs(vv) > SLIP) {
            LOG << epoTimeStr << " cycle slip detected " << t_lc::toString(tLC) << ' '
                << obs->prn().toString() << ' ' << setw(8) << setprecision(4) << vv << endl;
            resetAmb(obs->prn(), obsVector, tLC);
          }
        }*/
      }
    }
  }
  return success;
}

// Reset Ambiguity Parameter (cycle slip)
////////////////////////////////////////////////////////////////////////////
t_irc t_pppFilter::resetAmb(const t_prn prn, const vector<t_pppSatObs*> &obsVector, t_lc::type lc,
                            SymmetricMatrix *QSav, ColumnVector *xSav) {

  t_irc irc = failure;
  vector<t_pppParam*>& params = _parlist->params();
  unsigned nPar = params.size();
  for (unsigned iPar = 0; iPar < nPar; iPar++) {
    t_pppParam *par = params[iPar];
    if (par->type() == t_pppParam::amb && par->prn() == prn) {
      int     ind    = par->indexNew();
      double  eleSat = par->ambEleSat();
      bncTime firstObsTime;
      bncTime lastObsTime = par->lastObsTime();
      (par->firstObsTime().undef()) ?
        firstObsTime = lastObsTime : firstObsTime = par->firstObsTime();
      t_lc::type tLC = par->tLC();
      if (tLC != lc) {continue;}
      LOG << string(_epoTime) << " RESET " << par->toString() << endl;
      delete par; par = new t_pppParam(t_pppParam::amb, prn, tLC, &obsVector);
      par->setIndex(ind);
      par->setFirstObsTime(firstObsTime);
      par->setLastObsTime(lastObsTime);
      par->setAmbEleSat(eleSat);
      params[iPar] = par;
      for (unsigned ii = 0; ii < nPar; ii++) {
        _QFlt(ii+1, ind+1) = 0.0;
        if (QSav) {
          (*QSav)(ii+1, ind+1) = 0.0;
        }
      }
      _QFlt(ind+1,ind+1) = par->sigma0() * par->sigma0();
      if (QSav) {
        (*QSav)(ind+1,ind+1) = _QFlt(ind+1,ind+1);
      }
      _xFlt[ind] = 0.0;
      if (xSav) {
        (*xSav)[ind] = _xFlt[ind];
      }
      _x0[ind] = par->x0();
      irc = success;
    }
  }

  return irc;
}

// Compute various DOP Values
////////////////////////////////////////////////////////////////////////////
void t_pppFilter::cmpDOP(const vector<t_pppSatObs*> &obsVector) {

  _dop.reset();

  try {
    const unsigned numPar = 4;
    Matrix AA(obsVector.size(), numPar);
    t_pppParam* parX = 0;
    t_pppParam* parY = 0;
    t_pppParam* parZ = 0;
    _numSat = 0;
    for (unsigned ii = 0; ii < obsVector.size(); ii++) {
      t_pppSatObs *obs = obsVector[ii];
      if (obs->isValid() && !obs->outlier()) {
        ++_numSat;
        for (unsigned iPar = 0; iPar < numPar; iPar++) {
          t_pppParam* par = _parlist->params()[iPar];
          AA[_numSat - 1][iPar] = par->partial(_epoTime, obs, t_lc::c1);
          if      (par->type() == t_pppParam::crdX) {
            parX = par;
          }
          else if (par->type() == t_pppParam::crdY) {
            parY = par;
          }
          else if (par->type() == t_pppParam::crdZ) {
            parZ = par;
          }
        }
      }
    }
    if (_numSat < 4) {
      return;
    }
    AA = AA.Rows(1, _numSat);
    SymmetricMatrix NN; NN << AA.t() * AA;
    SymmetricMatrix QQ = NN.i();
    SymmetricMatrix QQxyz = QQ.SymSubMatrix(1,3);

    ColumnVector xyz(3), neu(3);
    SymmetricMatrix QQneu(3);
    const t_pppStation *sta = PPP_CLIENT->staRover();
    xyz[0] = _xFlt[parX->indexNew()];
    xyz[1] = _xFlt[parY->indexNew()];
    xyz[2] = _xFlt[parZ->indexNew()];
    xyz2neu(sta->ellApr().data(), xyz.data(), neu.data());
    covariXYZ_NEU(QQxyz, sta->ellApr().data(), QQneu);

    _dop.H = sqrt(QQneu(1, 1) + QQneu(2, 2));
    _dop.V = sqrt(QQneu(3, 3));
    _dop.P = sqrt(QQ(1, 1) + QQ(2, 2) + QQ(3, 3));
    _dop.T = sqrt(QQ(4, 4));
    _dop.G = sqrt(QQ(1, 1) + QQ(2, 2) + QQ(3, 3) + QQ(4, 4));
  }
  catch (...) {
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppFilter::predictCovCrdPart(const SymmetricMatrix &QFltOld, bool setNeuNoiseToZero) {

  const vector<t_pppParam*>& params = _parlist->params();

  if (params.size() < 3) {
    return;
  }

  bool first = (params[0]->indexOld() < 0);

  SymmetricMatrix Qneu(3); Qneu = 0.0;
  for (unsigned ii = 0; ii < 3; ii++) {
    const t_pppParam *par = params[ii];
    if (first) {
      Qneu[ii][ii] = par->sigma0() * par->sigma0();
    }
    else {
      Qneu[ii][ii] = par->noise() * par->noise();
    }
  }

  const t_pppStation *sta = PPP_CLIENT->staRover();
  SymmetricMatrix Qxyz(3);
  covariNEU_XYZ(Qneu, sta->ellApr().data(), Qxyz);

  if (first) {
    _QFlt.SymSubMatrix(1, 3) = Qxyz;
  }
  else {
    double dt = _epoTime - _firstEpoTime;
    if (dt < OPT->_seedingTime || setNeuNoiseToZero) {
      _QFlt.SymSubMatrix(1, 3) = QFltOld.SymSubMatrix(1, 3);
    }
    else {
      _QFlt.SymSubMatrix(1, 3) = QFltOld.SymSubMatrix(1, 3) + Qxyz;
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppFilter::setStateVectorAndVarCovMatrix(const ColumnVector &xFltOld,
                                                const SymmetricMatrix &QFltOld,
                                                bool setNeuNoiseToZero) {

  const vector<t_pppParam*>& params = _parlist->params();
  unsigned nPar = params.size();

  _QFlt.ReSize(nPar); _QFlt = 0.0;
  _xFlt.ReSize(nPar); _xFlt = 0.0;
  _x0.ReSize(nPar);   _x0   = 0.0;

  for (unsigned ii = 0; ii < nPar; ii++) {
    t_pppParam *par1 = params[ii];
    if (QFltOld.size() == 0) {
      par1->resetIndex();
    }
    _x0[ii] = par1->x0();
    int iOld = par1->indexOld();
    if (iOld < 0) {
      _QFlt[ii][ii] = par1->sigma0() * par1->sigma0(); // new parameter
    } else {
      _QFlt[ii][ii] = QFltOld[iOld][iOld] + par1->noise() * par1->noise();
      _xFlt[ii]     = xFltOld[iOld];
      for (unsigned jj = 0; jj < ii; jj++) {
        t_pppParam *par2 = params[jj];
        int jOld = par2->indexOld();
        if (jOld >= 0) {
          _QFlt[ii][jj] = QFltOld(iOld + 1, jOld + 1);
        }
      }
    }
  }
  predictCovCrdPart(QFltOld, setNeuNoiseToZero);
}
