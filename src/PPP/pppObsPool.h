#ifndef OBSPOOL_H
#define OBSPOOL_H

#include <vector>
#include <deque>
#include <iostream>
#include "pppSatObs.h"
#include "bnctime.h"

namespace BNC_PPP {

class t_pppObsPool {
 public:

  class t_epoch {
   public:
    t_epoch(const bncTime& epoTime, std::vector<t_pppSatObs*>& obsVector,
            bool pseudoObsIono);
    ~t_epoch();
          std::vector<t_pppSatObs*>& obsVector() {return _obsVector;}
    const std::vector<t_pppSatObs*>& obsVector() const {return _obsVector;}
    const bncTime& epoTime() const {return _epoTime;}
    bool pseudoObsIono() const {return _pseudoObsIono;}
   private:
    bncTime                   _epoTime;
    bool                      _pseudoObsIono;
    std::vector<t_pppSatObs*> _obsVector;
  };

  t_pppObsPool();
  ~t_pppObsPool();
  void putCodeBias(t_satCodeBias* satCodeBias);
  void putPhaseBias(t_satPhaseBias* satPhaseBias);
  void putTec(t_vTec* _vTec);

  void putEpoch(const bncTime& epoTime, std::vector<t_pppSatObs*>& obsVector,
                bool pseudoObsIono);

  const t_satCodeBias* satCodeBias(const t_prn& prn) const {
    return _satCodeBiases[prn.toInt()];
  }
  const t_satPhaseBias* satPhaseBias(const t_prn& prn) const {
    return _satPhaseBiases[prn.toInt()];
  }
  const t_vTec* vTec() const {return _vTec;}

  t_epoch* lastEpoch() {
    if (_epochs.size()) {
      return _epochs.back();
    }
    else {
      return 0;
    }
  }

 private:
  t_satCodeBias*           _satCodeBiases[t_prn::MAXPRN+1];
  t_satPhaseBias*          _satPhaseBiases[t_prn::MAXPRN+1];
  t_vTec*                  _vTec;
  std::deque<t_epoch*>     _epochs;
};

}

#endif
