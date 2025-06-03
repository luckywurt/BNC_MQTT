#ifndef STATION_H
#define STATION_H

#include <string>
#include <newmat.h>
#include "pppInclude.h"
#include "bnctime.h"

namespace BNC_PPP {

class t_windUp;
class t_iono;

class t_pppStation {
 public:
  t_pppStation();
  ~t_pppStation();
  void setName(std::string name) {_name = name;}
  void setAntName(std::string antName) {_antName = antName;}
  void setXyzApr(const ColumnVector& xyzApr);
  void setNeuEcc(const ColumnVector& neuEcc);
  void setDClk(double dClk) {_dClk = dClk;}
  void setTideDsplEarth(const ColumnVector& tideDsplEarth) {_tideDsplEarth = tideDsplEarth;}
  void setTideDsplOcean(const ColumnVector& tideDsplOcean) {_tideDsplOcean = tideDsplOcean;}
  void setEpochTime(const bncTime& epochTime) {_epochTime = epochTime;}
  const std::string&  name()      const {return _name;}
  const std::string&  antName()   const {return _antName;}
  const ColumnVector& xyzApr()    const {return _xyzApr;}
  const ColumnVector& ellApr()    const {return _ellApr;}
  const ColumnVector& neuEcc()    const {return _neuEcc;}
  const ColumnVector& xyzEcc()    const {return _xyzEcc;}
  const ColumnVector& tideDsplEarth()  const {return _tideDsplEarth;}
  const ColumnVector& tideDsplOcean()  const {return _tideDsplOcean;}
  const bncTime       epochTime() const {return _epochTime;}
  double dClk() const {return _dClk;}
  double windUp(const bncTime& time, t_prn prn, const ColumnVector& rSat, bool ssr,
                double yaw, const ColumnVector& vSat) const;
  double stec(const t_vTec* vTec, const double signalPropagationTime, const ColumnVector& rSat) const;

 private:
  std::string       _name;
  std::string       _antName;
  ColumnVector      _xyzApr;
  ColumnVector      _ellApr;
  ColumnVector      _neuEcc;
  ColumnVector      _xyzEcc;
  ColumnVector      _tideDsplEarth;
  ColumnVector      _tideDsplOcean;
  double            _dClk;
  mutable t_windUp* _windUp;
  bncTime           _timeCheck;
  ColumnVector      _xyzCheck;
  t_iono*            _iono;
  bncTime           _epochTime;
};

}

#endif
