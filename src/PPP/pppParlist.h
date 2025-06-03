#ifndef PARLIST_H
#define PARLIST_H

#include <vector>
#include <string>
#include "pppInclude.h"
#include "t_prn.h"
#include "bnctime.h"

namespace BNC_PPP {

class t_pppSatObs;

class t_pppParam {
 public:
  enum e_type {crdX, crdY, crdZ, rClkG, rClkR, rClkE, rClkC, trp, ion, amb,
               cBiasG1, cBiasR1, cBiasE1, cBiasC1, pBiasG1, pBiasR1, pBiasE1, pBiasC1,
               cBiasG2, cBiasR2, cBiasE2, cBiasC2, pBiasG2, pBiasR2, pBiasE2, pBiasC2};

  t_pppParam(e_type type, const t_prn& prn, t_lc::type tLC, const std::vector<t_pppSatObs*>* obsVector = 0);
  ~t_pppParam();

  e_type type() const {return _type;}
  double x0()  const {return _x0;}
  double partial(const bncTime& epoTime, const t_pppSatObs* obs,
                 const t_lc::type& tLC) const;
  bool   epoSpec() const {return _epoSpec;}
  bool   isEqual(const t_pppParam* par2) const {
    return (_type == par2->_type && _prn == par2->_prn && _tLC == par2->_tLC);
  }
  void   setIndex(int indexNew) {
    _indexOld = _indexNew;
    _indexNew = indexNew;
  }
  void resetIndex() {
    _indexOld = -1;
  }
  int indexOld() const {return _indexOld;}
  int indexNew() const {return _indexNew;}
  double sigma0() const {return _sigma0;}
  double noise() const {return _noise;}
  t_lc::type tLC() const {return _tLC;}
  t_prn prn() const {return _prn;}
  std::string toString() const;

  const bncTime& lastObsTime() const {return _lastObsTime;}
  void setLastObsTime(const bncTime& epoTime) {_lastObsTime = epoTime;}
  const bncTime& firstObsTime() const {return _firstObsTime;}
  void setFirstObsTime(const bncTime& epoTime) {_firstObsTime = epoTime;}

  bool     ambResetCandidate() const   {return _ambInfo && _ambInfo->_resetCandidate;}
  void     setAmbResetCandidate()      {if (_ambInfo) _ambInfo->_resetCandidate = true;}
  double   ambEleSat() const           {return _ambInfo ? _ambInfo->_eleSat : 0.0;}
  void     setAmbEleSat(double eleSat) {if (_ambInfo) _ambInfo->_eleSat = eleSat;}
  unsigned ambNumEpo() const           {return _ambInfo ? _ambInfo->_numEpo : 0;}
  void     stepAmbNumEpo()             {if (_ambInfo) _ambInfo->_numEpo += 1;}

  static bool sortFunction(const t_pppParam* p1, const t_pppParam* p2) {
    if      (p1->_type != p2->_type) {
      return p1->_type < p2->_type;
    }
    else if (p1->_tLC != p2->_tLC) {
      return p1->_tLC < p2->_tLC;
    }
    else if (p1->_prn != p2->_prn) {
      return p1->_prn < p2->_prn;
    }
    return false;
  }

 private:
  class t_ambInfo {
   public:
    t_ambInfo() {
      _resetCandidate = false;
      _eleSat         = 0.0;
      _numEpo         = 0;
    }
    ~t_ambInfo() {}
    bool     _resetCandidate;
    double   _eleSat;
    unsigned _numEpo;
  };
  e_type       _type;
  t_prn        _prn;
  t_lc::type   _tLC;
  double       _x0;
  bool         _epoSpec;
  int          _indexOld;
  int          _indexNew;
  double       _sigma0;
  double       _noise;
  t_ambInfo*   _ambInfo;
  bncTime      _lastObsTime;
  bncTime      _firstObsTime;
};

class t_pppParlist {
 public:
  t_pppParlist();
  ~t_pppParlist();
  t_irc set(const bncTime& epoTime, const std::vector<t_pppSatObs*>& obsVector);
  unsigned nPar() const {return _params.size();}
  const std::vector<t_pppParam*>& params() const {return _params;}
  std::vector<t_pppParam*>& params() {return _params;}
  const QMap<char, int>& usedSystems() const {return _usedSystems;}
  void printResult(const bncTime& epoTime, const SymmetricMatrix& QQ,
                   const ColumnVector& xx) const;
  void printParams(const bncTime& epoTime);

 private:
  std::vector<t_pppParam*> _params;
  QMap<char, int>          _usedSystems;
};

}

#endif
