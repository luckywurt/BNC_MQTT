#ifndef EPHEMERIS_H
#define EPHEMERIS_H

#include <newmat.h>
#include <QtCore>
#include <stdio.h>
#include <string>
#include "bnctime.h"
#include "bncconst.h"
#include "t_prn.h"
#include "gnss.h"

class t_orbCorr;
class t_clkCorr;

class t_eph {
 public:
  enum e_system {unknown, GPS, QZSS, GLONASS, Galileo, SBAS, BDS, NavIC};
  enum e_checkState {unchecked, ok, bad, outdated, unhealthy};
  enum e_type {undefined, LNAV, FDMA, FDMA_M, FNAV, INAV, D1, D2, SBASL1, CNAV, CNV1, CNV2, CNV3, L1NV, L1OC, L3OC};

  t_eph();
  virtual ~t_eph();

  virtual e_system  system() const = 0;
  virtual QString toString(double version) const = 0;
  virtual unsigned int IOD() const = 0;
  virtual unsigned int isUnhealthy() const = 0;
  virtual int slotNum() const {return 0;}
  bncTime TOC() const {return _TOC;}
  bool    isNewerThan(const t_eph* eph) const {return earlierTime(eph, this);}
  void    setCheckState(e_checkState checkState) {_checkState = checkState;}
  e_checkState checkState() const {return _checkState;}
  QString checkStateToString() {
    switch (_checkState) {
      case unchecked:  return "unchecked";
      case ok:         return "ok";
      case bad:        return "bad";
      case outdated:   return "outdated";
      case unhealthy:  return "unhealthy";
      default:         return "unknown";
    }
  }
  e_type type() const {return _type;}
  void setType(QString typeStr);
  t_prn  prn()  const {return _prn;}
  t_irc getCrd(const bncTime& tt, ColumnVector& xc, ColumnVector& vv, bool useCorr) const;
  void setOrbCorr(const t_orbCorr* orbCorr);
  void setClkCorr(const t_clkCorr* clkCorr);
  const QDateTime& receptDateTime() const {return _receptDateTime;}
  const QString receptStaID() const {return _receptStaID;}
  static QString rinexDateStr(const bncTime& tt, const t_prn& prn, double version);
  static QString rinexDateStr(const bncTime& tt, const QString& prnStr, double version);
  static QString typeStr(e_type type, const t_prn& prn, double version);
  static bool earlierTime(const t_eph* eph1, const t_eph* eph2) {return eph1->_TOC < eph2->_TOC;}
  static bool prnSort(const t_eph* eph1, const t_eph* eph2) {return eph1->prn() < eph2->prn();}

 protected:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const = 0;
  t_prn            _prn;
  bncTime          _TOC;
  QDateTime        _receptDateTime;
  QString          _receptStaID;
  e_checkState     _checkState;
  e_type           _type; // defined in RINEX 4
  t_orbCorr*       _orbCorr;
  t_clkCorr*       _clkCorr;
};

class t_ephGPS : public t_eph {
 friend class t_ephEncoder;
 friend class RTCM3Decoder;
 public:
  t_ephGPS() {
    _clock_bias      = 0.0;
    _clock_drift     = 0.0;
    _clock_driftrate = 0.0;
    _IODE            = 0.0;
    _Crs             = 0.0;
    _Delta_n         = 0.0;
    _M0              = 0.0;
    _Cuc             = 0.0;
    _e               = 0.0;
    _Cus             = 0.0;
    _sqrt_A          = 0.0;
    _TOEsec          = 0.0;
    _Cic             = 0.0;
    _OMEGA0          = 0.0;
    _Cis             = 0.0;
    _i0              = 0.0;
    _Crc             = 0.0;
    _omega           = 0.0;
    _OMEGADOT        = 0.0;
    _IDOT            = 0.0;
    _L2Codes         = 0.0;
    _TOEweek         = 0.0;
    _L2PFlag         = 0.0;
    _ura             = 0.0;
    _health          = 0.0;
    _TGD             = 0.0;
    _IODC            = 0.0;
    _TOT             = 0.0;
    _fitInterval     = 0.0;
    _ADOT            = 0.0;
    _top             = 0.0;
    _Delta_n_dot     = 0.0;
    _URAI            = 0.0;
    _URAI_NED0       = 0.0;
    _URAI_NED1       = 0.0;
    _URAI_NED2       = 0.0;
    _URAI_ED         = 0.0;
    _ISC_L1CA        = 0.0;
    _ISC_L2C         = 0.0;
    _ISC_L5I5        = 0.0;
    _ISC_L5Q5        = 0.0;
    _ISC_L1Cd        = 0.0;
    _ISC_L1Cp        = 0.0;
    _RSF             = 0.0;
    _ISC_S           = 0.0;
    _ISC_L1D         = 0.0;
    _ISC_L1P         = 0.0;
    _wnop            = 0.0;
    _flags_unknown   = true;
    _intSF           = 0.0;
    _ephSF           = 0.0;
    _L2Cphasing      = 0.0;
    _alert           = 0.0;
    _receptStaID     = "";
  }
  t_ephGPS(double rnxVersion, const QStringList& lines, const QString typeStr);
  virtual ~t_ephGPS() {}

  virtual e_system system() const {
    switch (_prn.system()) {
      case 'J':
        return t_eph::QZSS;
      case 'I':
        return t_eph::NavIC;
    };
    return t_eph::GPS;
  }
  virtual QString toString(double version) const;
  virtual unsigned int  IOD() const { return static_cast<unsigned int>(_IODE); }
  //virtual unsigned int  isUnhealthy() const { return static_cast<unsigned int>(_health);}
  virtual unsigned int isUnhealthy() const;
  double TGD() const {return _TGD;} // Timing Group Delay (P1-P2 DCB)

 private:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const;

  double  _clock_bias;      // [s]
  double  _clock_drift;     // [s/s]
  double  _clock_driftrate; // [s/s^2]

  double  _IODE;            // IODEC in case of NavIC
  double  _Crs;             // [m]
  double  _Delta_n;         // [rad/s]
  double  _M0;              // [rad]

  double  _Cuc;             // [rad]
  double  _e;               // []
  double  _Cus;             // [rad]
  double  _sqrt_A;          // [m^0.5]

  double  _TOEsec;          // [s of GPS week]
  double  _Cic;             // [rad]
  double  _OMEGA0;          // [rad]
  double  _Cis;             // [rad]

  double  _i0;              // [rad]
  double  _Crc;             // [m]
  double  _omega;           // [rad]
  double  _OMEGADOT;        // [rad/s]

  double  _IDOT;            // [rad/s]
  double  _L2Codes;         // Codes on L2 channel  (not valid for NavIC)
  double  _TOEweek;         // GPS week # to go with TOE, cont. number, not mode 1024
  double  _L2PFlag;         // L2 P data flag (not valid for NavIC and QZSS)

  mutable double  _ura;     // SV accuracy [m]
  double  _health;          // SV health
  double  _TGD;             // [s]
  double  _IODC;            // (not valid for NavIC)

  double  _TOT;             // Transmission time
  double  _fitInterval;     // Fit interval in hours (not valid for NavIC)

  double  _ADOT;            // [m/s]
  double  _top;             // [s]
  double  _Delta_n_dot;     // [rad/s^2]

  double _URAI;             // [] user range accuracy index
  double _URAI_NED0;        // []
  double _URAI_NED1;        // []
  double _URAI_NED2;        // []
  double _URAI_ED;          // []

  double _ISC_L1CA;         // [s] inter signal correction
  double _ISC_L2C;          // [s]
  double _ISC_L5I5;         // [s]
  double _ISC_L5Q5;         // [s]
  double _ISC_L1Cd;         // [s]
  double _ISC_L1Cp;         // [s]

  double _RSF;              // [-] Reference Signal Flag for NavIC
  double _ISC_S;            // [s]
  double _ISC_L1D;          // [s]
  double _ISC_L1P;          // [s]

  bool   _flags_unknown;    // [-] status flags are unknown => BNK; fitInterval LNAV from QZSS or GPS
  double _intSF;            // [-] integrity status flag
  double _ephSF;            // [-] ephemeris status flag (QZSS)
  double _L2Cphasing;       // [-] L2C phasing flag
  double _alert;            // [-] alert flag

  double _wnop;             // GPS continuous week number with the ambiguity resolved (same as _TOEweek?)
};

class t_ephGlo : public t_eph {
 friend class t_ephEncoder;
 friend class RTCM3Decoder;
 public:
  t_ephGlo() {
    _xv.ReSize(6);
    _xv      = 0.0;
    _gps_utc = 0.0;
    _tau     = 0.0;
    _tau1    = 0.0;
    _tau2    = 0.0;
    _tauC    = 0.0;
    _Tin     = 0.0;
    _gamma   = 0.0;
    _tki     = 0.0;
    _x_pos   = 0.0;
    _x_vel   = 0.0;
    _x_acc   = 0.0;
    _health  = 0.0;
    _y_pos   = 0.0;
    _y_vel   = 0.0;
    _y_acc   = 0.0;
    _frq_num = 0.0;
    _z_pos   = 0.0;
    _z_vel   = 0.0;
    _z_acc   = 0.0;
    _E       = 0.0;
    _EE      = 0.0;
    _ET      = 0.0;
    _RE      = 0.0;
    _RT      = 0.0;
    _P1      = 0.0;
    _P2      = 0.0;
    _P3      = 0.0;
    _M_M     = 0.0;
    _M_FE    = 0.0;
    _M_FT    = 0.0;
    _M_l3    = 0.0;
    _M_l5    = 0.0;
    _M_NA    = 0.0;
    _M_NT    = 0.0;
    _M_N4    = 0.0;
    _M_P     = 0.0;
    _M_P4    = 0.0;
    _X_PC    = 0.0;
    _Y_PC    = 0.0;
    _Z_PC    = 0.0;
    _TOT     = 0.0;
    _yaw     = 0.0;
    _sn      = 0.0;
    _sat_type  = 0.0;
    _TGD_L2OCp = 0.0;
    _TGD_L3OCp = 0.0;
    _M_tau_GPS = 0.0;
    _M_delta_tau  = 0.0;
    _attitude_P2  = 0.0;
    _angular_rate = 0.0;
    _angular_acc  = 0.0;
    _angular_rate_max = 0.0;
    _data_validity = 0;
    _healthflags_unknown = true;
    _statusflags_unknown = true;
    _almanac_health = 0.0;
    _almanac_health_availablility_indicator = 0.0;
    _additional_data_availability = 0.0;

  }
  t_ephGlo(double rnxVersion, const QStringList& lines, const QString typeStr);
  virtual ~t_ephGlo() {}

  virtual e_system system() const {return t_eph::GLONASS;}
  virtual QString toString(double version) const;
  virtual unsigned int  IOD() const;
  virtual unsigned int isUnhealthy() const;
  virtual int slotNum() const {return int(_frq_num);}
  virtual bool validMdata() const {return (_M_M ? true : false);}
 private:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const;
  static ColumnVector glo_deriv(double /* tt */, const ColumnVector& xv, double* acc);

  mutable bncTime      _tt;    // time
  mutable ColumnVector _xv;    // status vector (position, velocity) at time _tt

  double  _gps_utc;            // [s]
  double  _tau;                // [s]
  double  _tau1;               // [s]
  double  _tau2;               // [s]
  double  _tauC;               // GLONASS time scale correction to UTC(SU) [sec]
  double  _Tin;                // sec of day UTC(SU) [s]

  double  _gamma;              // [-]
  mutable double  _tki;        // message frame time

  double  _x_pos;              // [km]
  double  _x_vel;              // [km/s]
  double  _x_acc;              // [km/s^2]
  double  _health;             // 0 = OK. MSB of Bn word

  double  _y_pos;              // [km]
  double  _y_vel;              // [km/s]
  double  _y_acc;              // [km/s^2]
  double  _frq_num;            // ICD-GLONASS data position

  double  _z_pos;              // [km]
  double  _z_vel;              // [km/s]
  double  _z_acc;              // [km/s^2]

  double  _E;                  // Age of current Information [days]
  double  _EE;                 // Age Of Data eph [days]
  double  _ET;                 // Age Of Data clk [days]

  double  _TGD_L2OCp;          // [sec]
  double  _TGD_L3OCp;          // [sec]

  double  _almanac_health ;    // almanac health bit; 1 = healthy, 1 = not healthy
  double  _almanac_health_availablility_indicator; // 1 = reported in eph record, 0 = not reported
  double  _additional_data_availability;

  double  _sat_type;           // 0 = GLO_SAT, 1 = GLO_SAT_M (M type), 2 = GLO_SAT_K (K type)
  double  _RE;                 // source flags; 01 = relay; 10 = prediction (propagation), 11 = use of inter-satellite measurements
  double  _RT;                 // source flags; 01 = relay, 10 = prediction (propagation), 11 = use of inter-satellite measurements

  double  _P1;                 // update and validity interval [-]; 00 = 0 min, 01 = 30 min, 10 ) 45 min, 11 = 60 min
  double  _P2;                 // flag of oddness or evenness of the value of tb for intervals 30 or 60 minutes [-]
  double  _P3;                 // flag indicating a number of satellites for which almanac is transmitted within given frame [-]

  double  _M_M;                // type of satellite transmitting navigation signal: 0 = GLONASS, 1 = GLONASS-M/K satellite [-]
  double  _M_FE;               // Indicator for predicted satellite User Range Accuracy (URAI_orb) [-]
  double  _M_FT;               // Indicator for predicted satellite User Range Accuracy (URAI_clk) [-]
  double  _M_l3;               // health bit on string 3 GLO-M/K only
  double  _M_l5;               // health flag
  double  _M_NA;               // calendar day number within the 4-year period [days]
  double  _M_NT;               // current date, calendar number of day within 4-year interval [days]
  double  _M_N4;               // 4-year interval number starting from 1996
  double  _M_P;                // control segment parameter that indicates the satellite operation mode with respect of time parameters
  double  _M_P4;               // flag to show that ephemeris parameters are present [-] GLO-M/K only
  double  _M_tau_GPS;          // correction to GPS time relative to GLONASS time [days]
  double  _M_delta_tau;        // [s]

  bool    _statusflags_unknown;// status flags are unknown => BNK in RNX NAV file if message type is FDMA
  bool    _healthflags_unknown;// health flags are unknown => BNK in RNX NAV file if message type is FDMA
  int     _data_validity;      // data validity; 0 = valid, 1 = invalid

  double  _attitude_P2;        // 0 = nominal yaw steering, 1 = rate-limited yaw maneuver
  double  _yaw;                // [rad]
  double  _sn;                 // sign flag
  double  _angular_rate;       // [rad/sec]
  double  _angular_rate_max;   // [rad/sec]
  double  _angular_acc;        // [rad/sec^2]

  double  _X_PC;               // X PC coord [m] GLO manufacturer coordinate system
  double  _Y_PC;               // Y PC coord [m] GLO manufacturer coordinate system
  double  _Z_PC;               // Y PC coord [m] GLO manufacturer coordinate system
  double  _TOT;                // Time of transmission
};

class t_ephGal : public t_eph {
 friend class t_ephEncoder;
 friend class RTCM3Decoder;
 public:
  t_ephGal() {
    _clock_bias      = 0.0;
    _clock_drift     = 0.0;
    _clock_driftrate = 0.0;
    _IODnav          = 0.0;
    _Crs             = 0.0;
    _Delta_n         = 0.0;
    _M0              = 0.0;
    _Cuc             = 0.0;
    _e               = 0.0;
    _Cus             = 0.0;
    _sqrt_A          = 0.0;
    _TOEsec          = 0.0;
    _Cic             = 0.0;
    _OMEGA0          = 0.0;
    _Cis             = 0.0;
    _i0              = 0.0;
    _Crc             = 0.0;
    _omega           = 0.0;
    _OMEGADOT        = 0.0;
    _IDOT            = 0.0;
    _TOEweek         = 0.0;
    _SISA            = 0.0;
    _E5a_HS          = 0.0;
    _E5b_HS          = 0.0;
    _E1B_HS          = 0.0;
    _BGD_1_5A        = 0.0;
    _BGD_1_5B        = 0.0;
    _TOT             = 0.0;
    _inav            = false;
    _fnav            = false;
    _E1B_DataInvalid = false;
    _E5a_DataInvalid = false;
    _E5b_DataInvalid = false;
    _receptStaID     = "";
  };
  t_ephGal(double rnxVersion, const QStringList& lines, const QString typeStr);
  virtual ~t_ephGal() {}

  virtual QString toString(double version) const;
  virtual e_system system() const {return t_eph::Galileo;}
  virtual unsigned int  IOD() const { return static_cast<unsigned long>(_IODnav); }
  virtual unsigned int  isUnhealthy() const;

 private:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const;

  double  _clock_bias;       //  [s]
  double  _clock_drift;      //  [s/s]
  double  _clock_driftrate;  //  [s/s^2]

  double  _IODnav;
  double  _Crs;              //  [m]
  double  _Delta_n;          //  [rad/s]
  double  _M0;               //  [rad]

  double  _Cuc;              //  [rad]
  double  _e;                //
  double  _Cus;              //  [rad]
  double  _sqrt_A;           //  [m^0.5]

  double  _TOEsec;           //  [s]
  double  _Cic;              //  [rad]
  double  _OMEGA0;           //  [rad]
  double  _Cis;              //  [rad]

  double  _i0;               //  [rad]
  double  _Crc;              //  [m]
  double  _omega;            //  [rad]
  double  _OMEGADOT;         //  [rad/s]

  double  _IDOT;             //  [rad/s]
  double  _TOEweek;          //  [-]
  // spare

  mutable double  _SISA;     // Signal In Space Accuracy
  double  _E5a_HS;           //  [0..3] E5a Health Status
  double  _E5b_HS;           //  [0..3] E5b Health Status
  double  _E1B_HS;           //  [0..3] E1B Health Status
  double  _BGD_1_5A;         //  group delay [s]
  double  _BGD_1_5B;         //  group delay [s]

  double  _TOT;              // [s]
  bool    _inav;             // Data comes from I/NAV when <code>true</code>
  bool    _fnav;             // Data comes from F/NAV when <code>true</code>
  bool    _E1B_DataInvalid;  // E1B Data is not valid
  bool    _E5a_DataInvalid;  // E5a Data is not valid
  bool    _E5b_DataInvalid;  // E5b Data is not valid
};

class t_ephSBAS : public t_eph {
 friend class t_ephEncoder;
 friend class RTCM3Decoder;
 public:
  t_ephSBAS() {
    _IODN   = 0;
    _TOT    = 0.0;
    _agf0   = 0.0;
    _agf1   = 0.0;
    _x_pos  = 0.0;
    _x_vel  = 0.0;
    _x_acc  = 0.0;
    _y_pos  = 0.0;
    _y_vel  = 0.0;
    _y_acc  = 0.0;
    _z_pos  = 0.0;
    _z_vel  = 0.0;
    _z_acc  = 0.0;
    _ura    = 0.0;
    _health = 0.0;
    _receptStaID = "";
  }
  t_ephSBAS(double rnxVersion, const QStringList& lines, const QString typeStr);
  virtual ~t_ephSBAS() {}

  virtual e_system  system() const {return t_eph::SBAS;}
  virtual unsigned int IOD() const;
  virtual unsigned int  isUnhealthy() const;
  virtual QString toString(double version) const;

 private:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const;

  int    _IODN;
  double _TOT;   // not used (set to  0.9999e9)
  double _agf0;  // [s]    clock correction
  double _agf1;  // [s/s]  clock correction drift

  double _x_pos; // [m]
  double _x_vel; // [m/s]
  double _x_acc; // [m/s^2]

  double _y_pos; // [m]
  double _y_vel; // [m/s]
  double _y_acc; // [m/s^2]

  double _z_pos; // [m]
  double _z_vel; // [m/s]
  double _z_acc; // [m/s^2]

  mutable double _ura;
  double _health;
};

class t_ephBDS : public t_eph {
 friend class t_ephEncoder;
 friend class RTCM3Decoder;
 public:
 t_ephBDS() {
   _TOT               = 0.0;
   _AODE              = 0;
   _AODC              = 0;
   _ura               = 0.0;
   _clock_bias        = 0.0;
   _clock_drift       = 0.0;
   _clock_driftrate   = 0.0;
   _ADOT              = 0.0;
   _Crs               = 0.0;
   _Delta_n           = 0.0;
   _M0                = 0.0;
   _Cuc               = 0.0;
   _e                 = 0.0;
   _Cus               = 0.0;
   _sqrt_A            = 0.0;
   _Cic               = 0.0;
   _OMEGA0            = 0.0;
   _Cis               = 0.0;
   _i0                = 0.0;
   _Crc               = 0.0;
   _omega             = 0.0;
   _OMEGADOT          = 0.0;
   _IDOT              = 0.0;
   _TOEsec            = 0.0;
   _BDTweek           = 0.0;
   _Delta_n_dot       = 0.0;
   _satType           = 0.0;
   _top               = 0.0;
   _SISAI_oe          = 0.0;
   _SISAI_ocb         = 0.0;
   _SISAI_oc1         = 0.0;
   _SISAI_oc2         = 0.0;
   _ISC_B1Cd          = 0.0;
   _ISC_B2ad          = 0.0;
   _TGD1              = 0.0;
   _TGD2              = 0.0;
   _TGD_B1Cp          = 0.0;
   _TGD_B2ap          = 0.0;
   _TGD_B2bI          = 0.0;
   _SISMAI            = 0.0;
   _SatH1             = 0;
   _health            = 0;
   _INTEGRITYF_B1C    = 0.0;
   _INTEGRITYF_B2aB1C = 0.0;
   _INTEGRITYF_B2b    = 0.0;
   _IODC              = 0.0;
   _IODE              = 0.0;
   _receptStaID       = "";
 }
 t_ephBDS(double rnxVersion, const QStringList& lines, const QString typeStr);
  virtual ~t_ephBDS() {}

  virtual e_system  system() const {return t_eph::BDS;}
  virtual unsigned int IOD() const;
  virtual unsigned int isUnhealthy() const;
  virtual QString toString(double version) const;

 private:
  virtual t_irc position(int GPSweek, double GPSweeks, double* xc, double* vv) const;

  double  _TOT;              // [s] of BDT week
  bncTime _TOE;
  int     _AODE;
  int     _AODC;
  mutable double  _ura;      // user range accuracy [m]
  double  _clock_bias;       // [s]
  double  _clock_drift;      // [s/s]
  double  _clock_driftrate;  // [s/s^2]
  double  _ADOT;            // [m/s]
  double  _Crs;              // [m]
  double  _Delta_n;          // [rad/s]
  double  _M0;               // [rad]
  double  _Cuc;              // [rad]
  double  _e;                // [-]
  double  _Cus;              // [rad]
  double  _sqrt_A;           // [m^0.5]
  double  _Cic;              // [rad]
  double  _OMEGA0;           // [rad]
  double  _Cis;              // [rad]
  double  _i0;               // [rad]
  double  _Crc;              // [m]
  double  _omega;            // [rad]
  double  _OMEGADOT;         // [rad/s]
  double  _IDOT;             // [rad/s]
  double  _TOEsec;           // [s] of BDT week
  double  _BDTweek;          // BDT week

  double  _Delta_n_dot;      // [rad/s^2]
  double  _satType;          // 0..reserved, 1..GEO, 2..IGSO, 3..MEO
  double  _top;              // [s]

  double  _SISAI_oe;         // [-]
  double  _SISAI_ocb;        // [-]
  double  _SISAI_oc1;        // [-]
  double  _SISAI_oc2;        // [-]

  double  _ISC_B1Cd;         // [s]
  double  _ISC_B2ad;         // [s]

  double  _TGD1;             // [s]
  double  _TGD2;             // [s]
  double  _TGD_B1Cp;         // [s]
  double  _TGD_B2ap;         // [s]
  double  _TGD_B2bI;         // [s]

  double  _SISMAI;           // [-]

  int     _SatH1;            // [-]
  int     _health;           // [-]

  double  _INTEGRITYF_B1C;   // 3 bits word from sf 3
  double  _INTEGRITYF_B2aB1C;// 6 bits word with integrity bits in msg 10-11, 30.34 or 40
  double  _INTEGRITYF_B2b;   // 3 bits word from msg 10

  double  _IODC;             // [-]
  double  _IODE;             // [-] IODE are the same as the 8 LSBs of IODC

};

#endif
