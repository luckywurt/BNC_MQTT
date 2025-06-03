#ifndef EPHPOOL_H
#define EPHPOOL_H

#include <deque>
#include "pppInclude.h"
#include "bnctime.h"
#include "ephemeris.h"

namespace BNC_PPP {

class t_pppEphPool {
 public:
  t_pppEphPool(unsigned maxQueueSize = 3) {
    _maxQueueSize = maxQueueSize;
  }
  ~t_pppEphPool() {};

  void putEphemeris(t_eph* eph);
  void putOrbCorrection(t_orbCorr* corr);
  void putClkCorrection(t_clkCorr* corr);

  t_irc getCrd(const t_prn& prn, const bncTime& tt,
                    ColumnVector& xc, ColumnVector& vv) const;

  int getChannel(const t_prn& prn) const;

  std::deque<t_eph*>& ephs(t_prn prn) {
    return _satEphPool[prn]._ephs;
  }

 private:

  class t_satEphPool {
   public:
    t_satEphPool() {};
    ~t_satEphPool() {
      if (_ephs.empty()) return;
      for (unsigned ii = 0; ii < _ephs.size(); ii++) {
        delete _ephs[ii];
      }
    }
    void putEphemeris(unsigned maxQueueSize, t_eph* eph);
    void putOrbCorrection(t_orbCorr* corr);
    void putClkCorrection(t_clkCorr* corr);
    t_irc getCrd(const bncTime& tt,
                      ColumnVector& xc, ColumnVector& vv) const;
    int getChannel() const;
    std::deque<t_eph*> _ephs;
  };

  t_satEphPool _satEphPool[t_prn::MAXPRN+1];
  unsigned     _maxQueueSize;
};

}

#endif
