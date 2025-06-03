
#ifndef BNCCOMB_H
#define BNCCOMB_H

#include <fstream>
#include <iostream>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <map>
#include <newmat.h>
#include <deque>
#include "bncephuser.h"
#include "satObs.h"
#include "bncconst.h"
#include "bncoutf.h"
#include "../RTCM3/clock_and_orbit/clock_orbit_rtcm.h"
#include "../RTCM3/clock_and_orbit/clock_orbit_igs.h"

class bncRtnetDecoder;
class bncAntex;
class bncBiasSnx;

class bncComb : public QObject {
 Q_OBJECT
 public:
  static bncComb* getInstance() {
    if (instance == 0) {
      instance = new bncComb;
    }
    return instance;
  }
  bncComb(const bncComb&) = delete;
  bncComb& operator=(const bncComb&) = delete;
  static void destruct() {
    delete instance;
    instance = nullptr;
  }
  int  nStreams() const {return _ACs.size();}

 public slots:
  void slotProviderIDChanged(QString mountPoint);
  void slotNewOrbCorrections(QList<t_orbCorr> orbCorrections);
  void slotNewClkCorrections(QList<t_clkCorr> clkCorrections);
  void slotNewCodeBiases(QList<t_satCodeBias> satCodeBiases);

 private slots:
  void slotReadBiasSnxFile();

 signals:
  void newMessage(QByteArray msg, bool showOnScreen);
  void newOrbCorrections(QList<t_orbCorr>);
  void newClkCorrections(QList<t_clkCorr>);
  void newCodeBiases(QList<t_satCodeBias>);

 private:
  bncComb();                // no public constructor
  ~bncComb();               // no public destructor
  static bncComb* instance; // declaration class variable
  enum e_method{singleEpoch, filter};

  class cmbParam {
   public:
    enum parType {offACgnss, offACSat, clkSat};
    cmbParam(parType type_, int index_, const QString& ac_, const QString& prn_);
    ~cmbParam();
    double partial(char sys, const QString& AC_, const QString& prn_);
    QString toString(char sys) const;
    parType type;
    int     index;
    QString AC;
    QString prn;
    double  xx;
    double  sig0;
    double  sigP;
    bool    epoSpec;
    const t_eph* eph;
  };

  class cmbAC {
   public:
    cmbAC() {
      weightFactor = 1.0;
      numObs['G']  = 0;
      numObs['R']  = 0;
      numObs['E']  = 0;
      numObs['C']  = 0;
      numObs['J']  = 0;
      numObs['S']  = 0;
      numObs['I']  = 0;
      isAPC = false;
    }
    ~cmbAC() {
      numObs.clear();
    }
    QString              mountPoint;
    QString              name;
    double               weightFactor;
    QStringList          excludeSats;
    bool                 isAPC;
    QMap<char, unsigned> numObs;
  };

  class cmbCorr {
   public:
    cmbCorr() {
      _eph                        = 0;
      _iod                        = 0;
      _dClkResult                 = 0.0;
      _satCodeBiasIF              = 0.0;
      _weightFactor               = 1.0;
    }
    ~cmbCorr() {
    }
    QString        _prn;
    bncTime        _time;
    unsigned long  _iod;
    t_eph*         _eph;
    t_orbCorr      _orbCorr;
    t_clkCorr      _clkCorr;
    t_satCodeBias  _satCodeBias;
    QString        _acName;
    double         _satCodeBiasIF;
    double         _dClkResult;
    ColumnVector   _diffRao;
    double         _weightFactor;
    QString ID() {return _acName + "_" + _prn;}
  };

  class cmbEpoch {
   public:
    cmbEpoch() {}
    ~cmbEpoch() {
      clear();
    }
    void clear() {
      QVectorIterator<cmbCorr*> it(corrs);
      while (it.hasNext()) {
        delete it.next();
      }
      corrs.clear();
    }
    QVector<cmbCorr*>      corrs;
  };

  class epoClkData {
   public:
    epoClkData() {}
    ~epoClkData() {
      _clkCorr.erase(_clkCorr.begin(), _clkCorr.end());
    }
    bncTime                 _time;
    std::vector<t_clkCorr>  _clkCorr;
  };

  class cmbRefSig {
   public:
    enum type {dummy = 0, c1, c2, cIF};

    static t_frequency::type toFreq(char sys, type tt) {
      switch (tt) {
      case c1:
        if      (sys == 'G') return t_frequency::G1;
        else if (sys == 'R') return t_frequency::R1;
        else if (sys == 'E') return t_frequency::E1;
        else if (sys == 'C') return t_frequency::C2;
        else if (sys == 'J') return t_frequency::J1;
        else if (sys == 'S') return t_frequency::S1;
        else                 return t_frequency::dummy;
      case c2:
        if      (sys == 'G') return t_frequency::G2;
        else if (sys == 'R') return t_frequency::R2;
        else if (sys == 'E') return t_frequency::E5;
        else if (sys == 'C') return t_frequency::C6;
        else if (sys == 'J') return t_frequency::J2;
        else if (sys == 'S') return t_frequency::S5;
        else                 return t_frequency::dummy;
      case dummy:
      case cIF:
        return t_frequency::dummy;
      }
      return t_frequency::dummy;
    }

    static char toAttrib(char sys, type LC) {
      switch (LC) {
        case c1:
          if      (sys == 'G') return 'W';
          else if (sys == 'R') return 'P';
          else if (sys == 'E') return 'C';
          else if (sys == 'C') return 'I';
          else if (sys == 'J') return 'C';
          else if (sys == 'S') return 'C';
          break;
        case c2:
          if      (sys == 'G') return 'W';
          else if (sys == 'R') return 'P';
          else if (sys == 'E') return 'Q';
          else if (sys == 'C') return 'I';
          else if (sys == 'J') return 'L';
          else if (sys == 'S') return 'Q';
          break;
      case dummy:
      case cIF:
        return '_';
        break;
      }
      return '_';
    }

      static void coeff(char sys, type tLC, double channel, std::map<t_frequency::type, double>& codeCoeff)  {
      codeCoeff.clear();
      t_frequency::type fType1 = toFreq(sys, c1);
      t_frequency::type fType2 = toFreq(sys, c2);
      double f1 = t_CST::freq(fType1, channel);
      double f2 = t_CST::freq(fType2, channel);
      switch (tLC) {
        case c1:
          codeCoeff[fType1] = 1.0;
          return;
        case c2:
          codeCoeff[fType2] = 1.0;
          return;
        case cIF:
          codeCoeff[fType1] =  f1 * f1 / (f1 * f1 - f2 * f2);
          codeCoeff[fType2] = -f2 * f2 / (f1 * f1 - f2 * f2);
          return;
        case cmbRefSig::dummy:
          return;
      }
      return;
    }
  };

  void  processEpoch(bncTime epoTime, const std::vector<t_clkCorr>& clkCorrVec);
  void  processSystem(bncTime epoTime, char sys, QTextStream& out);
  t_irc processEpoch_filter(bncTime epoTime, char sys, QTextStream& out, QMap<QString, cmbCorr*>& resCorr, ColumnVector& dx);
  t_irc processEpoch_singleEpoch(bncTime epoTime, char sys, QTextStream& out, QMap<QString, cmbCorr*>& resCorr, ColumnVector& dx);
  t_irc createAmat(char sys, Matrix& AA, ColumnVector& ll, DiagonalMatrix& PP,
                   const ColumnVector& x0, QMap<QString, cmbCorr*>& resCorr);
  void  dumpResults(bncTime epoTime, QMap<QString, cmbCorr*>& resCorr);
  void  printResults(bncTime epoTime, QTextStream& out, const QMap<QString, cmbCorr*>& resCorr);
  void  switchToLastEph(t_eph* lastEph, cmbCorr* corr);
  t_irc checkOrbits(bncTime epoTime, char sys, QTextStream& out);
  bool excludeSat(const t_prn& prn, const QStringList excludeSats) const;
  QVector<cmbCorr*>& corrs(char sys) {return _buffer[sys].corrs;}

  QMutex                                     _mutex;
  QList<cmbAC*>                              _ACs;
  std::deque<epoClkData*>                    _epoClkData;
  bncTime                                    _lastClkCorrTime;
  bncTime                                    _resTime;
  cmbCorr*                                   _newCorr;
  bncRtnetDecoder*                           _rtnetDecoder;
  QByteArray                                 _log;
  bncAntex*                                  _antex;
  bncBiasSnx*                                _bsx;
  bncoutf*                                   _logFile;
  double                                     _MAX_RES;
  double                                     _MAX_DISPLACEMENT;
  e_method                                   _method;
  int                                        _cmbSampl;
  int                                        _ms;
  QMap<char, cmbEpoch>                       _buffer;
  QMap<char, SymmetricMatrix>                _QQ;
  QMap<char, QString>                        _masterOrbitAC;
  QMap<char, unsigned>                       _masterMissingEpochs;
  QMap<char, bool>                           _masterIsAPC;
  QMap<char, QVector<cmbParam*>>              _params;
  QMap<QString, QMap<t_prn, t_orbCorr> >     _orbCorrections;
  QMap<QString, QMap<t_prn, t_satCodeBias> > _satCodeBiases;
  QMap<char, unsigned>                       _cmbSysPrn;
  bncEphUser                                 _ephUser;
  SsrCorr*                                   _ssrCorr;
  QString                                    _cmbRefAttributes;
  bool                                       _useGps;
  bool                                       _useGlo;
  bool                                       _useGal;
  bool                                       _useBds;
  bool                                       _useQzss;
  bool                                       _useSbas;
  bool                                       _useNavic;
  bool                                       _running;
};

#define BNC_CMB (bncComb::getInstance())

#endif
