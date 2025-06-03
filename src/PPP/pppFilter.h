#ifndef FILTER_H
#define FILTER_H

#include <vector>
#include <newmat.h>
#include "pppInclude.h"
#include "pppParlist.h"
#include "bnctime.h"
#include "t_prn.h"

namespace BNC_PPP {

class t_pppParlist;
class t_pppObsPool;
class t_pppSatObs;

class t_pppFilter {
 public:
  t_pppFilter();
  ~t_pppFilter();

  t_irc processEpoch(t_pppObsPool* obsPool);

  const ColumnVector&    x() const {return _xFlt;}
  const SymmetricMatrix& Q() const {return _QFlt;}

  int    numSat() const {return _numSat;}
  double HDOP() const {return _dop.H;}
  double HDOV() const {return _dop.V;}
  double PDOP() const {return _dop.P;}
  double GDOP() const {return _dop.G;}
  double trp() const {
    const std::vector<t_pppParam*>& par = _parlist->params();
    for (unsigned ii = 0; ii < par.size(); ++ii) {
      if (par[ii]->type() == t_pppParam::trp) {
        return x()[ii];
      }
    }
    return 0.0;
  };
  double trpStdev() const {
    const std::vector<t_pppParam*>& par = _parlist->params();
    for (unsigned ii = 0; ii < par.size(); ++ii) {
      if (par[ii]->type() == t_pppParam::trp) {
        return sqrt(Q()[ii][ii]);
      }
    }
    return 0.0;
  };

 private:
  class t_slip {
   public:
    t_slip() {
      _slip            = false;
      _obsSlipCounter  = -1;
      _biasJumpCounter = -1;
    }
    bool _slip;
    int  _obsSlipCounter;
    int  _biasJumpCounter;
  };

  class t_dop {
   public:
    t_dop() {reset();}
    void reset() {H = V = P = T = G = 0.0;}
    double H;
    double V;
    double P;
    double T;
    double G;
  };

  t_irc processSystem(const std::vector<t_lc::type>& LCs,
                      const std::vector<t_pppSatObs*>& obsVector,
                      bool pseudoObsIonoAvailable);

  t_irc detectCycleSlips(const std::vector<t_lc::type>& LCs,
                         const std::vector<t_pppSatObs*>& obsVector);

  t_irc resetAmb(t_prn prn, const std::vector<t_pppSatObs*>& obsVector, t_lc::type lc,
                 SymmetricMatrix* QSav = 0, ColumnVector* xSav = 0);

  void cmpDOP(const std::vector<t_pppSatObs*>& obsVector);

  void setStateVectorAndVarCovMatrix(const ColumnVector& xFltOld, const SymmetricMatrix& QFltOld,
                                     bool setNeuNoiseToZero);

  void predictCovCrdPart(const SymmetricMatrix& QFltOld, bool setNeuNoiseToZero);


  bncTime         _epoTime;
  t_pppParlist*   _parlist;
  SymmetricMatrix _QFlt;
  ColumnVector    _xFlt;
  ColumnVector    _x0;
  t_slip          _slips[t_prn::MAXPRN+1];
  int             _numSat;
  t_dop           _dop;
  bncTime         _firstEpoTime;
  bncTime         _lastEpoTimeOK;
};

}

#endif
