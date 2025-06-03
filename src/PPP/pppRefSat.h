/*
 * pppRefSat.h
 *
 *  Created on: Feb 27, 2020
 *      Author: stuerze
 */

#ifndef PPPREFSAT_H_
#define PPPREFSAT_H_

#include <vector>
#include <QMap>
#include "t_prn.h"

namespace BNC_PPP {

class t_pppRefSat {
public:
  t_pppRefSat() {
    _prn = t_prn();
    _stecValue = 0.0;
  };
  t_pppRefSat(t_prn prn, double stecValue) {
    _prn = prn;
    _stecValue = stecValue;
  }
  ~t_pppRefSat() {};
  t_prn    prn() {return _prn;}
  void     setPrn(t_prn prn) {_prn = prn;}
  double   stecValue() {return _stecValue;}
  void     setStecValue(double stecValue) {_stecValue = stecValue;}
private:
  t_prn    _prn;
  double   _stecValue;
};

class t_refSatellites {
 public:
  t_refSatellites(std::vector<char> systems) {
    for (unsigned iSys = 0; iSys < systems.size(); iSys++) {
      char sys = systems[iSys];
      _refSatMap[sys] = new t_pppRefSat();
    }
  }
  ~t_refSatellites() {
    QMapIterator<char, t_pppRefSat*> it(_refSatMap);
    while (it.hasNext()) {
      it.next();
      delete it.value();
    }
    _refSatMap.clear();
  }
  const QMap<char, t_pppRefSat*>& refSatMap() const {return _refSatMap;}

 private:
  QMap<char, t_pppRefSat*> _refSatMap;

};


}


#endif /* PPPREFSAT_H_ */
