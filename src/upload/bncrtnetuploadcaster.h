#ifndef BNCRTNETUPLOADCASTER_H
#define BNCRTNETUPLOADCASTER_H

#include <newmat.h>
#include <iostream>
#include "bncuploadcaster.h"
#include "bnctime.h"
#include "ephemeris.h"
#include "../RTCM3/clock_and_orbit/clock_orbit_rtcm.h"
#include "../RTCM3/clock_and_orbit/clock_orbit_igs.h"
#include "../RTCM3/crs.h"
#include "../RTCM3/crsEncoder.h"

class bncEphUser;
class bncoutf;
class bncClockRinex;
class bncSP3;
class bncBiasSinex;

class bncRtnetUploadCaster : public bncUploadCaster {
 Q_OBJECT
 public:
  bncRtnetUploadCaster(const QString& mountpoint,
                  const QString& outHost, int outPort,
                  const QString& ntripVersion, const QString& userName,
                  const QString& password,
                  const QString& crdTrafo, const QString& ssrFormat,
                  bool  CoM,
                  const QString& sp3FileName,
                  const QString& rnxFileName,
                  const QString& bsxFileName,
                  int PID, int SID, int IOD, int iRow);
  void decodeRtnetStream(char* buffer, int bufLen);
 protected:
  virtual ~bncRtnetUploadCaster();
 private:
  t_irc processSatellite(const t_eph* eph, int GPSweek,
                        double GPSweeks, const QString& prn,
                        const ColumnVector& rtnAPC,
                        double ura,
                        const ColumnVector& rtnClk,
                        const ColumnVector& rtnVel,
                        const ColumnVector& rtnCoM,
                        const ColumnVector& rtnClkSig,
                        struct SsrCorr::ClockOrbit::SatData* sd,
                        QString& outLine);
  void decodeRtnetEpoch(QStringList epochLines);
  bool corrIsOutOfRange(struct SsrCorr::ClockOrbit::SatData* sd);

  void crdTrafo(int GPSWeek, ColumnVector& xyz, double& dc);
  // TODO: the following line can be deleted if all parameters are updated regarding ITRF2020
  void crdTrafo14(int GPSWeek, ColumnVector& xyz, double& dc);

  int determineUpdateInd(double samplingRate);

  QString        _casterID;
  bncEphUser*    _ephUser;
  QString        _rtnetStreamBuffer;
  QString        _crdTrafoStr;
  SsrCorr*       _ssrCorr;
  QString        _ssrFormat;
  bool           _CoM;
  bool           _phaseBiasInformationDecoded;
  int            _PID;
  int            _SID;
  int            _IOD;
  int            _samplRtcmClkCorr;
  double         _samplRtcmEphCorr;
  double         _samplRtcmVtec;
  double         _samplRtcmCrs;
  double         _dx;
  double         _dy;
  double         _dz;
  double         _dxr;
  double         _dyr;
  double         _dzr;
  double         _ox;
  double         _oy;
  double         _oz;
  double         _oxr;
  double         _oyr;
  double         _ozr;
  double         _sc;
  double         _scr;
  double         _t0;
  /* TODO: the following lines can be deleted if all parameters are updated regarding ITRF2020*/
  double         _dx14;
  double         _dy14;
  double         _dz14;
  double         _dxr14;
  double         _dyr14;
  double         _dzr14;
  double         _ox14;
  double         _oy14;
  double         _oz14;
  double         _oxr14;
  double         _oyr14;
  double         _ozr14;
  double         _sc14;
  double         _scr14;
  double         _t014;

  bncClockRinex* _rnx;
  bncSP3*        _sp3;
  bncBiasSinex*  _bsx;
  QMap<QString, const t_eph*>* _usedEph;
};

struct phaseBiasesSat {
  phaseBiasesSat() {
    yawAngle = 0.0;
    yawRate = 0.0;
  }
  double yawAngle;
  double yawRate;
};

struct phaseBiasSignal {
  phaseBiasSignal() {
    bias      = 0.0;
    integerIndicator     = 0;
    wlIndicator          = 0;
    discontinuityCounter = 0;
  }
  QString type;
  double bias;
  unsigned int integerIndicator;
  unsigned int wlIndicator;
  unsigned int discontinuityCounter;
};

#endif
