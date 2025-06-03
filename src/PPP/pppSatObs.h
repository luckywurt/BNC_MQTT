#ifndef PPPSATOBS_H
#define PPPSATOBS_H

#include <string>
#include <map>
#include <newmat.h>
#include "pppInclude.h"
#include "satObs.h"
#include "bnctime.h"

namespace BNC_PPP {

class t_pppStation;

class t_pppSatObs {
 public:
  t_pppSatObs(const t_satObs& satObs);
  ~t_pppSatObs();
  bool                isValid() const {return _valid;};
  bool                isValid(t_lc::type tLC) const;
  bool                isReference() const {return _reference;};
  void                setAsReference() {_reference = true;};
  void                resetReference() {_reference = false;};
  const t_prn&        prn() const {return _prn;}
  const ColumnVector& xc() const {return _xcSat;}
  const bncTime&      time() const {return _time;}
  t_irc               cmpModel(const t_pppStation* station);
  double              obsValue(t_lc::type tLC, bool* valid = 0) const;
  double              cmpValue(t_lc::type tLC) const;
  double              cmpValueForBanc(t_lc::type tLC) const;
  double              rho() const {return _model._rho;}
  double              sagnac() const {return _model._sagnac;}
  double              eleSat() const {return _model._eleSat;}
  bool                modelSet() const {return _model._set;}
  void                printModel() const;
  void                printObsMinusComputed() const;
  void                lcCoeff(t_lc::type tLC,
                              std::map<t_frequency::type, double>& codeCoeff,
                              std::map<t_frequency::type, double>& phaseCoeff,
                              std::map<t_frequency::type, double>& ionoCoeff) const;
  double              lambda(t_lc::type tLC) const;
  double              sigma(t_lc::type tLC) const;
  double              maxRes(t_lc::type tLC) const;
  bool                outlier() const {return _outlier;}
  void                setOutlier() {_outlier = true;}
  void                resetOutlier() {_outlier = false;}
  void                setRes(t_lc::type tLC, double res);
  double              getRes(t_lc::type tLC) const;
  bool                setPseudoObsIono(t_frequency::type freq);
  double              getIonoCodeDelay(t_frequency::type freq) {return _model._ionoCodeDelay[freq];}
  double              getCodeBias(t_frequency::type freq) {return _model._codeBias[freq];}
  t_frequency::type   fType1() const {return _fType1;}
  t_frequency::type   fType2() const {return _fType2;} 

  // RINEX
  bool slip() const {
    for (unsigned ii = 1; ii < t_frequency::max; ii++) {
      if (_obs[ii] && _obs[ii]->_slip) {
        return true;
      }
    }
    return false;
  }

  // RTCM
  int slipCounter() const {
    int cnt = -1;
    for (unsigned ii = 1; ii < t_frequency::max; ii++) {
      if (_obs[ii] && _obs[ii]->_slipCounter > cnt) {
        cnt = _obs[ii]->_slipCounter;
      }
    }
    return cnt;
  }

  int biasJumpCounter() const {
    int jmp = -1;
    for (unsigned ii = 1; ii < t_frequency::max; ii++) {
      if (_obs[ii] && _obs[ii]->_biasJumpCounter > jmp) {
        jmp = _obs[ii]->_biasJumpCounter;
      }
    }
    return jmp;
  }

 private:
  class t_model {
   public:
    t_model() {reset();}
    ~t_model() {}
    void reset() {
      _set       = false;
      _rho       = 0.0;
      _eleSat    = 0.0;
      _azSat     = 0.0;
      _elTx      = 0.0;
      _azTx      = 0.0;
      _recClkM   = 0.0;
      _satClkM   = 0.0;
      _sagnac    = 0.0;
      _antEcc    = 0.0;
      _tropo     = 0.0;
      _tropo0    = 0.0;
      _tideEarth = 0.0;
      _tideOcean = 0.0;
      _windUp    = 0.0;
      _rel       = 0.0;
      for (unsigned ii = 0; ii < t_frequency::max; ii++) {
        _antPCO[ii]        = 0.0;
        _codeBias[ii]      = 0.0;
        _phaseBias[ii]     = 0.0;
        _ionoCodeDelay[ii] = 0.0;
      }
    }
    bool   _set;
    double _rho;
    double _eleSat;
    double _azSat;
    double _elTx;
    double _azTx;
    double _recClkM;
    double _satClkM;
    double _sagnac;
    double _antEcc;
    double _tropo;
    double _tropo0;
    double _tideEarth;
    double _tideOcean;
    double _windUp;
    double _rel;
    double _antPCO[t_frequency::max];
    double _codeBias[t_frequency::max];
    double _phaseBias[t_frequency::max];
    double _ionoCodeDelay[t_frequency::max];
  };

  void prepareObs(const t_satObs& satObs);

  bool                         _valid;
  bool                         _reference;
  t_frequency::type            _fType1;
  t_frequency::type            _fType2;
  t_prn                        _prn;
  bncTime                      _time;
  int                          _channel;
  t_frqObs*                    _obs[t_frequency::max];
  ColumnVector                 _xcSat;
  ColumnVector                 _vvSat;
  t_model                      _model;
  bool                         _outlier;
  std::map<t_lc::type, double> _res;
  double                       _signalPropagationTime;
  double                       _stecSat;
  double                       _tropo0;
  QString                      _signalPriorities;
	};

}

#endif
