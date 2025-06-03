#ifndef PPPCLIENT_H
#define PPPCLIENT_H

#include <sstream>
#include <vector>
#include "pppInclude.h"
#include "ephemeris.h"
#include "pppOptions.h"
#include "pppModel.h"

class bncAntex;

namespace BNC_PPP {

class t_pppEphPool;
class t_pppObsPool;
class t_pppSatObs;
class t_pppStation;
class t_pppFilter;

class t_pppClient : public interface_pppClient {
 public:
  t_pppClient(const t_pppOptions* opt);
  ~t_pppClient();

  void putEphemeris(const t_eph* eph);
  void putTec(const t_vTec* vTec);
  void putOrbCorrections(const std::vector<t_orbCorr*>& corr);
  void putClkCorrections(const std::vector<t_clkCorr*>& corr);
  void putCodeBiases(const std::vector<t_satCodeBias*>& biases);
  void putPhaseBiases(const std::vector<t_satPhaseBias*>& biases);
  void processEpoch(const std::vector<t_satObs*>& satObs, t_output* output);

  const t_pppEphPool* ephPool() const {return _ephPool;}
  const t_pppObsPool* obsPool() const {return _obsPool;}
  const bncAntex*     antex() const {return _antex;}
  const t_pppStation* staRover() const {return _staRover;}

  std::ostringstream& log() {return *_log;}
  const t_pppOptions* opt() const {return _opt;}
  static t_irc bancroft(const Matrix& BBpass, ColumnVector& pos);

  static t_pppClient* instance();

  void reset();

 private:
  void initOutput(t_output* output);
  void finish(t_irc irc, int ind);
  void clearObs();
  t_irc prepareObs(const std::vector<t_satObs*>& satObs,
                   std::vector<t_pppSatObs*>& obsVector, bncTime& epoTime);
  bool  preparePseudoObs(std::vector<t_pppSatObs*>& obsVector);
  void  useObsWithCodeBiasesOnly(std::vector<t_pppSatObs*>& obsVector);
  t_irc cmpModel(t_pppStation* station, const ColumnVector& xyzc,
                 std::vector<t_pppSatObs*>& obsVector);
  t_irc cmpBancroft(const bncTime& epoTime, std::vector<t_pppSatObs*>& obsVector,
                    ColumnVector& xyzc, bool print);
  double cmpOffGps(std::vector<t_pppSatObs*>& obsVector);
  double cmpOffGlo(std::vector<t_pppSatObs*>& obsVector);
  double cmpOffGal(std::vector<t_pppSatObs*>& obsVector);
  double cmpOffBds(std::vector<t_pppSatObs*>& obsVector);


  t_output*                 _output;
  t_pppEphPool*             _ephPool;
  t_pppObsPool*             _obsPool;
  bncTime                   _epoTimeRover;
  t_pppStation*             _staRover;
  bncAntex*                 _antex;
  t_pppFilter*              _filter;
  std::vector<t_pppSatObs*> _obsRover;
  std::ostringstream*       _log;
  t_pppOptions*             _opt;
  t_tides*                  _tides;
  bool                      _pseudoObsIono;
  QMap<char, int>           _usedSystems;
  bool                      _running;
};

}; // namespace BNC_PPP

#define PPP_CLIENT (BNC_PPP::t_pppClient::instance())
#define LOG        (BNC_PPP::t_pppClient::instance()->log())
#define OPT        (BNC_PPP::t_pppClient::instance()->opt())

#endif
