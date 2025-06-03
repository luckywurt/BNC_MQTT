#ifndef RTCM3_CLOCK_ORBIT_H
#define RTCM3_CLOCK_ORBIT_H

/* Programheader

 Name:           clock_orbit.h
 Project:        RTCM3
 Version:        $Id: clock_orbit_igs.h 8966 2020-07-01 07:48:35Z stuerze $
 Authors:        Dirk Stöcker, Andrea Stürze
 Description:    state space approach
 */

#include <QList>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "t_prn.h"
#include "bncutils.h"
#include "bits.h"

enum IGS_NUMBERS {
  RTCM_MESSAGE_NUMBER_IGS = 4076,
  IGS_SSR_VERSION = 3
};

/* if some systems aren't supported at all, change the following numbers to zero
 for these systems to save space */
enum COR_CONSTANTS {
  CLOCKORBIT_BUFFERSIZE    = 8192,
  CLOCKORBIT_NUMGPS        = t_prn::MAXPRN_GPS,
  CLOCKORBIT_NUMGLONASS    = t_prn::MAXPRN_GLONASS,
  CLOCKORBIT_NUMGALILEO    = t_prn::MAXPRN_GALILEO,
  CLOCKORBIT_NUMQZSS       = t_prn::MAXPRN_QZSS,
  CLOCKORBIT_NUMSBAS       = t_prn::MAXPRN_SBAS,
  CLOCKORBIT_NUMBDS        = t_prn::MAXPRN_BDS,
  CLOCKORBIT_NUMBIAS       = 150,
  CLOCKORBIT_NUMIONOLAYERS =   4,
  CLOCKORBIT_MAXIONOORDER  =  16,
  CLOCKORBIT_MAXIONODEGREE =  16
};

enum COR_SATSYSTEM {
  CLOCKORBIT_SATGPS = 0,
  CLOCKORBIT_SATGLONASS,
  CLOCKORBIT_SATGALILEO,
  CLOCKORBIT_SATQZSS,
  CLOCKORBIT_SATSBAS,
  CLOCKORBIT_SATBDS,
  CLOCKORBIT_SATNUM
};

enum COR_OFFSETS {
  CLOCKORBIT_OFFSETGPS     = 0,
  CLOCKORBIT_OFFSETGLONASS = CLOCKORBIT_NUMGPS,
  CLOCKORBIT_OFFSETGALILEO = CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS,
  CLOCKORBIT_OFFSETQZSS    = CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                           + CLOCKORBIT_NUMGALILEO,
  CLOCKORBIT_OFFSETSBAS    = CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                           + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS,
  CLOCKORBIT_OFFSETBDS     = CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                           + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                           + CLOCKORBIT_NUMSBAS,
  CLOCKORBIT_COUNTSAT      = CLOCKORBIT_NUMGPS + CLOCKORBIT_NUMGLONASS
                           + CLOCKORBIT_NUMGALILEO + CLOCKORBIT_NUMQZSS
                           + CLOCKORBIT_NUMSBAS + CLOCKORBIT_NUMBDS
};

enum GCOB_RETURN {
  /* all well */
  GCOBR_MESSAGEFOLLOWS = 1,
  GCOBR_OK = 0,
  /* unknown data, a warning */
  GCOBR_UNKNOWNTYPE           = -1,
  GCOBR_UNKNOWNDATA           = -2,
  GCOBR_CRCMISMATCH           = -3,
  GCOBR_SHORTMESSAGE          = -4,
  /* failed to do the work */
  GCOBR_NOCLOCKORBITPARAMETER = -10,
  GCOBR_NOCODEBIASPARAMETER   = -11,
  GCOBR_NOPHASEBIASPARAMETER  = -12,
  GCOBR_NOVTECPARAMETER       = -13,
  /* data mismatch - data in storage does not match new data */
  GCOBR_TIMEMISMATCH          = -20,
  GCOBR_DATAMISMATCH          = -21,
  /* not enough data - can decode the block completely */
  GCOBR_SHORTBUFFER           = -30,
  GCOBR_MESSAGEEXCEEDSBUFFER  = -31
/* NOTE: When an error message has been emitted, the output structures may have been modified.
 * Make a copy of the previous variant before calling the
 function to have a clean state. */
};

class SsrCorr {

public:
  SsrCorr() {};
  virtual ~SsrCorr() {};
  virtual void setCorBase() = 0;
  virtual void setCorOffset() = 0;
  virtual void setCoType() = 0;
  virtual void setCbType() = 0;
  virtual void setPbType() = 0;
  virtual void setVtecType() = 0;
  virtual void setCodeType() = 0;

  typedef unsigned int SatelliteReferenceDatum;
  SatelliteReferenceDatum DATUM_ITRF;
  SatelliteReferenceDatum DATUM_LOCAL;

  typedef unsigned int CorBase;
  CorBase COBBASE_GPS;
  CorBase COBBASE_GLONASS;
  CorBase COBBASE_GALILEO;
  CorBase COBBASE_QZSS;
  CorBase COBBASE_BDS;
  CorBase COBBASE_SBAS;
  CorBase COBBASE_NUM;

  typedef unsigned int CorOffset;
  CorOffset COBOFS_ORBIT;
  CorOffset COBOFS_CLOCK;
  CorOffset COBOFS_COMBINED;
  CorOffset COBOFS_HR;
  CorOffset COBOFS_CBIAS;
  CorOffset COBOFS_PBIAS;
  CorOffset COBOFS_URA;
  CorOffset COBOFS_NUM;

  typedef unsigned int ClockOrbitType;
  ClockOrbitType COTYPE_GPSORBIT;
  ClockOrbitType COTYPE_GPSCLOCK;
  ClockOrbitType COTYPE_GPSCOMBINED;
  ClockOrbitType COTYPE_GPSHR;
  ClockOrbitType COTYPE_GPSURA;

  ClockOrbitType COTYPE_GLONASSORBIT;
  ClockOrbitType COTYPE_GLONASSCLOCK;
  ClockOrbitType COTYPE_GLONASSCOMBINED;
  ClockOrbitType COTYPE_GLONASSHR;
  ClockOrbitType COTYPE_GLONASSURA;

  ClockOrbitType COTYPE_GALILEOORBIT;
  ClockOrbitType COTYPE_GALILEOCLOCK;
  ClockOrbitType COTYPE_GALILEOCOMBINED;
  ClockOrbitType COTYPE_GALILEOHR;
  ClockOrbitType COTYPE_GALILEOURA;

  ClockOrbitType COTYPE_QZSSORBIT;
  ClockOrbitType COTYPE_QZSSCLOCK;
  ClockOrbitType COTYPE_QZSSCOMBINED;
  ClockOrbitType COTYPE_QZSSHR;
  ClockOrbitType COTYPE_QZSSURA;

  ClockOrbitType COTYPE_SBASORBIT;
  ClockOrbitType COTYPE_SBASCLOCK;
  ClockOrbitType COTYPE_SBASCOMBINED;
  ClockOrbitType COTYPE_SBASHR;
  ClockOrbitType COTYPE_SBASURA;

  ClockOrbitType COTYPE_BDSORBIT;
  ClockOrbitType COTYPE_BDSCLOCK;
  ClockOrbitType COTYPE_BDSCOMBINED;
  ClockOrbitType COTYPE_BDSHR;
  ClockOrbitType COTYPE_BDSURA;

  ClockOrbitType COTYPE_AUTO;

  typedef unsigned int CodeBiasType;
  CodeBiasType CBTYPE_GPS;
  CodeBiasType CBTYPE_GLONASS;
  CodeBiasType CBTYPE_GALILEO;
  CodeBiasType CBTYPE_QZSS;
  CodeBiasType CBTYPE_SBAS;
  CodeBiasType CBTYPE_BDS;
  CodeBiasType CBTYPE_AUTO;

  typedef unsigned int PhaseBiasType;
  PhaseBiasType PBTYPE_BASE;
  PhaseBiasType PBTYPE_GPS;
  PhaseBiasType PBTYPE_GLONASS;
  PhaseBiasType PBTYPE_GALILEO;
  PhaseBiasType PBTYPE_QZSS;
  PhaseBiasType PBTYPE_SBAS;
  PhaseBiasType PBTYPE_BDS;
  PhaseBiasType PBTYPE_AUTO;

  typedef unsigned int VtecType;
  VtecType VTEC_BASE;

  typedef unsigned int CodeType;
  CodeType RESERVED;
  CodeType CODETYPE_GPS_L1_CA;
  CodeType CODETYPE_GPS_L1_P;
  CodeType CODETYPE_GPS_L1_Z;
  CodeType CODETYPE_GPS_L1C_D;
  CodeType CODETYPE_GPS_L1C_P;
  CodeType CODETYPE_GPS_L1C_DP;
  CodeType CODETYPE_GPS_L2_CA;
  CodeType CODETYPE_GPS_SEMI_CODELESS;
  CodeType CODETYPE_GPS_L2C_M;
  CodeType CODETYPE_GPS_L2C_L;
  CodeType CODETYPE_GPS_L2C_ML;
  CodeType CODETYPE_GPS_L2_P;
  CodeType CODETYPE_GPS_L2_Z;
  CodeType CODETYPE_GPS_L5_I;
  CodeType CODETYPE_GPS_L5_Q;
  CodeType CODETYPE_GPS_L5_IQ;

  CodeType CODETYPE_GLONASS_L1_CA;
  CodeType CODETYPE_GLONASS_L1_P;
  CodeType CODETYPE_GLONASS_L2_CA;
  CodeType CODETYPE_GLONASS_L2_P;
  CodeType CODETYPE_GLONASS_L1a_OCd;
  CodeType CODETYPE_GLONASS_L1a_OCp;
  CodeType CODETYPE_GLONASS_L1a_OCdp;
  CodeType CODETYPE_GLONASS_L2a_CSI;
  CodeType CODETYPE_GLONASS_L2a_OCp;
  CodeType CODETYPE_GLONASS_L2a_CSIOCp;
  CodeType CODETYPE_GLONASS_L3_I;
  CodeType CODETYPE_GLONASS_L3_Q;
  CodeType CODETYPE_GLONASS_L3_IQ;

  CodeType CODETYPE_GALILEO_E1_A;
  CodeType CODETYPE_GALILEO_E1_B;
  CodeType CODETYPE_GALILEO_E1_C;
  CodeType CODETYPE_GALILEO_E1_BC;
  CodeType CODETYPE_GALILEO_E1_ABC;
  CodeType CODETYPE_GALILEO_E5A_I;
  CodeType CODETYPE_GALILEO_E5A_Q;
  CodeType CODETYPE_GALILEO_E5A_IQ;
  CodeType CODETYPE_GALILEO_E5B_I;
  CodeType CODETYPE_GALILEO_E5B_Q;
  CodeType CODETYPE_GALILEO_E5B_IQ;
  CodeType CODETYPE_GALILEO_E5_I;
  CodeType CODETYPE_GALILEO_E5_Q;
  CodeType CODETYPE_GALILEO_E5_IQ;
  CodeType CODETYPE_GALILEO_E6_A;
  CodeType CODETYPE_GALILEO_E6_B;
  CodeType CODETYPE_GALILEO_E6_C;
  CodeType CODETYPE_GALILEO_E6_BC;
  CodeType CODETYPE_GALILEO_E6_ABC;

  CodeType CODETYPE_QZSS_L1_CA;
  CodeType CODETYPE_QZSS_L1C_D;
  CodeType CODETYPE_QZSS_L1C_P;
  CodeType CODETYPE_QZSS_L2C_M;
  CodeType CODETYPE_QZSS_L2C_L;
  CodeType CODETYPE_QZSS_L2C_ML;
  CodeType CODETYPE_QZSS_L5_I;
  CodeType CODETYPE_QZSS_L5_Q;
  CodeType CODETYPE_QZSS_L5_IQ;
  CodeType CODETYPE_QZSS_L6_D;
  CodeType CODETYPE_QZSS_L6_P;
  CodeType CODETYPE_QZSS_L6_DP;
  CodeType CODETYPE_QZSS_L1C_DP;
  CodeType CODETYPE_QZSS_L1_S;
  CodeType CODETYPE_QZSS_L5_D;
  CodeType CODETYPE_QZSS_L5_P;
  CodeType CODETYPE_QZSS_L5_DP;
  CodeType CODETYPE_QZSS_L6_E;
  CodeType CODETYPE_QZSS_L6_DE;

  CodeType CODETYPE_SBAS_L1_CA;
  CodeType CODETYPE_SBAS_L5_I;
  CodeType CODETYPE_SBAS_L5_Q;
  CodeType CODETYPE_SBAS_L5_IQ;

  CodeType CODETYPE_BDS_B1_I;
  CodeType CODETYPE_BDS_B1_Q;
  CodeType CODETYPE_BDS_B1_IQ;
  CodeType CODETYPE_BDS_B3_I;
  CodeType CODETYPE_BDS_B3_Q;
  CodeType CODETYPE_BDS_B3_IQ;
  CodeType CODETYPE_BDS_B2_I;
  CodeType CODETYPE_BDS_B2_Q;
  CodeType CODETYPE_BDS_B2_IQ;
  CodeType CODETYPE_BDS_B1a_D;
  CodeType CODETYPE_BDS_B1a_P;
  CodeType CODETYPE_BDS_B1a_DP;
  CodeType CODETYPE_BDS_B2a_D;
  CodeType CODETYPE_BDS_B2a_P;
  CodeType CODETYPE_BDS_B2a_DP;
  CodeType CODETYPE_BDS_B1_A;
  CodeType CODETYPE_BDS_B3_A;

  QList<CorBase> corbase;
  QList<unsigned int> satoffset;


#define SSR_MAXURA 5.5 /* > 5466.5mm in meter */
#define COBOFS_MAXNUM 7

  /* satellite system data is stored with offset CLOCKORBIT_OFFSET...
   in the data structures. So first GLONASS satellite is at
   xxx->Sat[CLOCKORBIT_OFFSETGLONASS], first GPS satellite is
   xxx->Sat[CLOCKORBIT_OFFSETGPS]. */

  struct ClockOrbit {
    ClockOrbitType messageType;
    unsigned int EpochTime[CLOCKORBIT_SATNUM]; /* 0 .. system specific maximum */
    unsigned int NumberOfSat[CLOCKORBIT_SATNUM]; /* 0 .. CLOCKORBIT_NUM... */
    unsigned int Supplied[COBOFS_MAXNUM]; /* boolean */
    unsigned int SSRIOD;
    unsigned int SSRProviderID;
    unsigned int SSRSolutionID;
    unsigned int UpdateInterval;
    SatelliteReferenceDatum SatRefDatum;
    struct SatData {
      unsigned int ID; /* all */
      unsigned int IOD; /* all */
      unsigned int toe; /* SBAS, BDS */
      double UserRangeAccuracy; /* accuracy values in [m] */
      double hrclock;
      struct OrbitPart {
        double DeltaRadial; /* m */
        double DeltaAlongTrack; /* m */
        double DeltaCrossTrack; /* m */
        double DotDeltaRadial; /* m/s */
        double DotDeltaAlongTrack; /* m/s */
        double DotDeltaCrossTrack; /* m/s */
      } Orbit;
      struct ClockPart {
        double DeltaA0; /* m */
        double DeltaA1; /* m/s */
        double DeltaA2; /* m/ss */
      } Clock;
    } Sat[CLOCKORBIT_COUNTSAT];
  };

  struct CodeBias {
    CodeBiasType messageType;
    unsigned int EpochTime[CLOCKORBIT_SATNUM]; /* 0 .. system specific maximum */
    unsigned int NumberOfSat[CLOCKORBIT_SATNUM]; /* 0 .. CLOCKORBIT_NUM... */
    unsigned int UpdateInterval;
    unsigned int SSRIOD;
    unsigned int SSRProviderID;
    unsigned int SSRSolutionID;
    struct BiasSat {
      unsigned int ID; /* all */
      unsigned int NumberOfCodeBiases;
      struct CodeBiasEntry {
        CodeType Type;
        float Bias; /* m */
      } Biases[CLOCKORBIT_NUMBIAS];
    } Sat[CLOCKORBIT_COUNTSAT];
  };

  struct PhaseBias {
    PhaseBiasType messageType;
    unsigned int EpochTime[CLOCKORBIT_SATNUM]; /* 0 .. system specific maximum */
    unsigned int NumberOfSat[CLOCKORBIT_SATNUM]; /* 0 .. CLOCKORBIT_NUM... */
    unsigned int UpdateInterval;
    unsigned int SSRIOD;
    unsigned int SSRProviderID;
    unsigned int SSRSolutionID;
    unsigned int DispersiveBiasConsistencyIndicator;
    unsigned int MWConsistencyIndicator;
    struct PhaseBiasSat {
      unsigned int ID; /* all */
      unsigned int NumberOfPhaseBiases;
      double YawAngle; /* radiant */
      double YawRate; /* radiant/s */
      struct PhaseBiasEntry {
        CodeType Type;
        unsigned int SignalIntegerIndicator;
        unsigned int SignalsWideLaneIntegerIndicator;
        unsigned int SignalDiscontinuityCounter;
        float Bias; /* m */
      } Biases[CLOCKORBIT_NUMBIAS];
    } Sat[CLOCKORBIT_COUNTSAT];
  };

  struct VTEC {
    unsigned int EpochTime; /* GPS */
    unsigned int UpdateInterval;
    unsigned int SSRIOD;
    unsigned int SSRProviderID;
    unsigned int SSRSolutionID;
    unsigned int NumLayers; /* 1-4 */
    double Quality;
    struct IonoLayers {
      double Height; /* m */
      unsigned int Degree; /* 1-16 */
      unsigned int Order;  /* 1-16 */
      double Sinus[CLOCKORBIT_MAXIONODEGREE][CLOCKORBIT_MAXIONOORDER];
      double Cosinus[CLOCKORBIT_MAXIONODEGREE][CLOCKORBIT_MAXIONOORDER];
    } Layers[CLOCKORBIT_NUMIONOLAYERS];
  };


  /* return size of resulting data or 0 in case of an error */
  virtual size_t MakeClockOrbit(const struct ClockOrbit *co, CodeType type,
      int moremessagesfollow, char *buffer, size_t size) = 0;
  virtual size_t MakeCodeBias(const struct CodeBias *b, CodeBiasType type,
      int moremessagesfollow, char *buffer, size_t size) = 0;
  virtual size_t MakePhaseBias(const struct PhaseBias *b, CodeBiasType type,
      int moremessagesfollow, char *buffer, size_t size) = 0;
  virtual size_t MakeVTEC(const struct VTEC *v, int moremessagesfollow,
      char *buffer, size_t size) = 0;

  /* buffer should point to a RTCM3 block */
  virtual enum GCOB_RETURN GetSSR(struct ClockOrbit *co, struct CodeBias *b,
      struct VTEC *v, struct PhaseBias *pb, const char *buffer, size_t size,
      int *bytesused) = 0;

  virtual std::string codeTypeToRnxType(char system, CodeType type) = 0;
  virtual CodeType    rnxTypeToCodeType(char system, std::string type) = 0;

#define MPI         3.141592653589793

/* GNSS macros - Header part */
#define T_RTCM_MESSAGE_NUMBER(a)         ADDBITS(12, a)      /* DF002         */
#define T_IGS_SSR_VERSION(a)             ADDBITS( 3, a)      /*        IDF001 */
#define T_IGS_MESSAGE_NUMBER(a)          ADDBITS( 8, a)      /*        IDF002 */
#define T_SSR_EPOCH_TIME(a)              ADDBITS(20, a)      /* DF???  IDF003 */ //T_GPS_EPOCH_TIME(a)
#define T_GLONASS_EPOCH_TIME(a)          ADDBITS(17, a)      /* DF            */

#define T_SSR_UPDATE_INTERVAL(a)         ADDBITS( 4, a)      /* DF391, IDF004 */
#define T_MULTIPLE_MESSAGE_INDICATOR(a)  ADDBITS( 1, a)      /* DF388, IDF005 */
#define T_SSR_IOD(a)                     ADDBITS( 4, a)      /* DF413, IDF007 */
#define T_SSR_PROVIDER_ID(a)             ADDBITS(16, a)      /* DF414, IDF008 */
#define T_SSR_SOLUTION_ID(a)             ADDBITS( 4, a)      /* DF415, IDF009 */
#define T_SATELLITE_REFERENCE_DATUM(a)   ADDBITS( 1, a)      /* DF375, IDF006 */
#define T_NO_OF_SATELLITES(a)            ADDBITS( 6, a)      /* DF387, IDF010 */

/* GNSS macros - Satellite specific part */
#define T_GNSS_SATELLITE_ID(a)           ADDBITS( 6, a)      /*        IDF011 */
#define T_GPS_SATELLITE_ID(a)            ADDBITS( 6, a)      /* DF068         */
#define T_QZSS_SATELLITE_ID(a)           ADDBITS( 4, a)      /* DF249         */
#define T_GLONASS_SATELLITE_ID(a)        ADDBITS( 5, a)      /* DF            */
#define T_GNSS_IOD(a)                    ADDBITS( 8, a)      /*        IDF012 */
#define T_GPS_IODE(a)                    ADDBITS( 8, a)      /* DF071         */
#define T_GLONASS_IOD(a)                 ADDBITS( 8, a)      /* DF239         */
#define T_GALILEO_IOD(a)                 ADDBITS(10, a)      /* DF459         */
#define T_SBAS_T0MOD(a)                  ADDBITS( 9, (a/16)) /* DF468         */
#define T_SBAS_IODCRC(a)                 ADDBITS(24, a)      /* DF469         */
#define T_BDS_TOEMOD(a)                  ADDBITS(10, (a/8))  /* DF470         */
#define T_BDS_IOD(a)                     ADDBITS( 8, a)      /* DF471         */

/* Orbit Corrections */
#define T_DELTA_RADIAL(a)                SCALEADDBITS(22,    10000.0, a) /* DF365, IDF013 */
#define T_DELTA_ALONG_TRACK(a)           SCALEADDBITS(20,     2500.0, a) /* DF366, IDF014 */
#define T_DELTA_CROSS_TRACK(a)           SCALEADDBITS(20,     2500.0, a) /* DF367, IDF015 */
#define T_DELTA_DOT_RADIAL(a)            SCALEADDBITS(21,  1000000.0, a) /* DF368, IDF016 */
#define T_DELTA_DOT_ALONG_TRACK(a)       SCALEADDBITS(19,   250000.0, a) /* DF369, IDF017 */
#define T_DELTA_DOT_CROSS_TRACK(a)       SCALEADDBITS(19,   250000.0, a) /* DF370, IDF018 */

/* Clock Corrections */
#define T_DELTA_CLOCK_C0(a)              SCALEADDBITS(22,    10000.0, a) /* DF376, IDF019 */
#define T_DELTA_CLOCK_C1(a)              SCALEADDBITS(21,  1000000.0, a) /* DF377, IDF020 */
#define T_DELTA_CLOCK_C2(a)              SCALEADDBITS(27, 50000000.0, a) /* DF378, IDF021 */
#define T_HR_CLOCK_CORRECTION(a)         SCALEADDBITS(22,    10000.0, a) /* DF390, IDF022 */

/* Biases */
#define T_NO_OF_BIASES(a)                ADDBITS(5, a)                   /* DF, DF       IDF023 */ //_NO_OF_CODE_BIASES(a)T_NO_OF_PHASE_BIASES(a)

#define T_GNSS_SIGNAL_IDENTIFIER(a)      ADDBITS(5, a)                   /* DF       IDF024 */
#define T_CODE_BIAS(a)                   SCALEADDBITS(14,      100.0, a) /* DF383, IDF025 */
#define T_YAW_ANGLE(a)                   SCALEADDBITS( 9,  256.0/MPI, a) /* DF480, IDF026 */
#define T_YAW_RATE(a)                    SCALEADDBITS( 8, 8192.0/MPI, a) /* DF481, IDF027 */
#define T_PHASE_BIAS(a)                  SCALEADDBITS(20,    10000.0, a) /* DF482, IDF028 */

/* Phase specific part of GNSS phase bias message */
#define T_INTEGER_INDICATOR(a)           ADDBITS( 1, a)                  /* DF483, IDF029 */
#define T_WIDE_LANE_INDICATOR(a)         ADDBITS( 2, a)                  /* DF484, IDF030 */
#define T_DISCONTINUITY_COUNTER(a)       ADDBITS( 4, a)                  /* DF485, IDF031 */
#define T_DISPERSIVE_BIAS_INDICATOR(a)   ADDBITS( 1, a)                  /* DF486, IDF032 */
#define T_MW_CONSISTENCY_INDICATOR(a)    ADDBITS( 1, a)                  /* DF487, IDF033 */

/* URA */
#define T_SSR_URA(a)                     ADDBITS( 6, a)                  /* DF389, IDF034 */

/* Ionosphere */
#define T_NO_IONO_LAYERS(a)              ADDBITS( 2, a-1)                /* DF472, IDF035 */
#define T_IONO_HEIGHT(a)                 SCALEADDBITS( 8,  1/10000.0, a) /* DF473, IDF036 */
#define T_IONO_DEGREE(a)                 ADDBITS( 4, a-1)                /* DF474, IDF037 */
#define T_IONO_ORDER(a)                  ADDBITS( 4, a-1)                /* DF475, IDF038 */
#define T_IONO_COEFF_C(a)                SCALEADDBITS(16,      200.0, a) /* DF476, IDF039 */
#define T_IONO_COEFF_S(a)                SCALEADDBITS(16,      200.0, a) /* DF477, IDF040 */
#define T_VTEC_QUALITY_INDICATOR(a)      SCALEADDBITS( 9,       20.0, a) /* DF478, IDF041 */

static double URAToValue(int ura) {
  int urac, urav;
  urac = ura >> 3;
  urav = ura & 7;
  if (!ura)
    return 0;
  else if (ura == 63)
    return SSR_MAXURA;
  return (pow(3, urac) * (1.0 + urav / 4.0) - 1.0) / 1000.0;
}

static int ValueToURA(double val) {
  int ura;
  if (!val)
    return 0;
  else if (val > 5.4665)
    return 63;
  for (ura = 1; ura < 63 && val > URAToValue(ura); ++ura)
    ;
  return ura;
}

#define DECODESTART \
  int numbits=0; \
  uint64_t bitbuffer=0;

#define LOADSSRBITS(a) { \
  while((a) > numbits) { \
    if(!size--) return GCOBR_SHORTMESSAGE; \
    bitbuffer = (bitbuffer<<8)|((unsigned char)*(buffer++)); \
    numbits += 8; \
  } \
}

/* extract bits from data stream
 b = variable to store result, a = number of bits */
#define GETSSRBITS(b, a) { \
  LOADSSRBITS(a) \
  b = (bitbuffer<<(64-numbits))>>(64-(a)); \
  numbits -= (a); \
}

/* extract bits from data stream
 b = variable to store result, a = number of bits */
#define GETSSRBITSFACTOR(b, a, c) { \
  LOADSSRBITS(a) \
  b = ((bitbuffer<<(64-numbits))>>(64-(a)))*(c); \
  numbits -= (a); \
}

/* extract signed floating value from data stream
 b = variable to store result, a = number of bits */
#define GETSSRFLOATSIGN(b, a, c) { \
  LOADSSRBITS(a) \
  b = ((double)(((int64_t)(bitbuffer<<(64-numbits)))>>(64-(a))))*(c); \
  numbits -= (a); \
}

/* extract floating value from data stream
 b = variable to store result, a = number of bits, c = scale factor */
#define GETSSRFLOAT(b, a, c) { \
  LOADSSRBITS(a) \
  b = ((double)((bitbuffer<<(sizeof(bitbuffer)*8-numbits))>>(sizeof(bitbuffer)*8-(a))))*(c); \
  numbits -= (a); \
}

#define SKIPSSRBITS(b) { LOADSSRBITS(b) numbits -= (b); }

/* GPS macros also used for other systems when matching! */
#define G_HEADER(a)                      GETSSRBITS(a,  8)
#define G_RESERVEDH(a)                   GETSSRBITS(a,  6)
#define G_SIZE(a)                        GETSSRBITS(a, 10)

/* GNSS macros - Header part */
#define G_RTCM_MESSAGE_NUMBER(a)         GETSSRBITS(a, 12)      /* DF002         */

#define G_IGS_SSR_VERSION(a)             GETSSRBITS(a,  3)      /*        IDF001 */
#define G_IGS_MESSAGE_NUMBER(a)          GETSSRBITS(a,  8)      /*        IDF002 */
#define G_SSR_EPOCH_TIME(a)              GETSSRBITS(a, 20)      /*DF      IDF003 */
#define G_SSR_EPOCH_TIME_CHECK(a, b)     {unsigned int temp; GETSSRBITS(temp, 20) \
 if(b && a != temp) return GCOBR_TIMEMISMATCH; a = temp;}
#define G_GLONASS_EPOCH_TIME(a, b)       {unsigned int temp; GETSSRBITS(temp, 17) \
 if(b && a != temp) return GCOBR_TIMEMISMATCH; a = temp;}

#define G_SSR_UPDATE_INTERVAL(a)         GETSSRBITS(a,  4)      /* DF391, IDF004 */
#define G_MULTIPLE_MESSAGE_INDICATOR(a)  GETSSRBITS(a,  1)      /* DF388, IDF005 */
#define G_SSR_IOD(a)                     GETSSRBITS(a,  4)      /* DF413, IDF007 */
#define G_SSR_PROVIDER_ID(a)             GETSSRBITS(a, 16)      /* DF414, IDF008 */
#define G_SSR_SOLUTION_ID(a)             GETSSRBITS(a,  4)      /* DF415, IDF009 */
#define G_SATELLITE_REFERENCE_DATUM(a)   GETSSRBITS(a,  1)      /* DF375, IDF006 */
#define G_NO_OF_SATELLITES(a)            GETSSRBITS(a,  6)      /* DF387, IDF010 */

/* GNSS macros - Satellite specific part */
#define G_GNSS_SATELLITE_ID(a)           GETSSRBITS(a,  6)     /* DF068,  IDF011 */
#define G_GLONASS_SATELLITE_ID(a)        GETSSRBITS(a,  5)     /* DF             */
#define G_QZSS_SATELLITE_ID(a)           GETSSRBITS(a,  4)     /* DF249          */

#define G_GNSS_IOD(a)                    GETSSRBITS(a,  8)     /*DF071, DF237,DF471        IDF012 */
#define G_GALILEO_IOD(a)                 GETSSRBITS(a, 10)          /* DF459 */
#define G_SBAS_T0MOD(a)                  GETSSRBITSFACTOR(a, 9, 16) /* DF468 */
#define G_SBAS_IODCRC(a)                 GETSSRBITS(a, 24)          /* DF469 */
#define G_BDS_TOEMOD(a)                  GETSSRBITSFACTOR(a, 10, 8) /* DF470 */

/* Orbit Corrections */
#define G_DELTA_RADIAL(a)                GETSSRFLOATSIGN(a, 22,   1/10000.0)  /* DF365, IDF013 */
#define G_DELTA_ALONG_TRACK(a)           GETSSRFLOATSIGN(a, 20,    1/2500.0)  /* DF366, IDF014 */
#define G_DELTA_CROSS_TRACK(a)           GETSSRFLOATSIGN(a, 20,    1/2500.0)  /* DF367, IDF015 */
#define G_DELTA_DOT_RADIAL(a)            GETSSRFLOATSIGN(a, 21, 1/1000000.0)  /* DF368, IDF016 */
#define G_DELTA_DOT_ALONG_TRACK(a)       GETSSRFLOATSIGN(a, 19,  1/250000.0)  /* DF369, IDF017 */
#define G_DELTA_DOT_CROSS_TRACK(a)       GETSSRFLOATSIGN(a, 19,  1/250000.0)  /* DF370, IDF018 */

/* Clock Corrections */
#define G_DELTA_CLOCK_C0(a)              GETSSRFLOATSIGN(a, 22,    1/10000.0) /* DF376, IDF019 */
#define G_DELTA_CLOCK_C1(a)              GETSSRFLOATSIGN(a, 21,  1/1000000.0) /* DF377, IDF020 */
#define G_DELTA_CLOCK_C2(a)              GETSSRFLOATSIGN(a, 27, 1/50000000.0) /* DF378, IDF021 */
#define G_HR_CLOCK_CORRECTION(a)         GETSSRFLOATSIGN(a, 22,    1/10000.0) /* DF390, IDF022 */

/* Biases */
#define G_NO_OF_BIASES(a)                GETSSRBITS(a,  5)                    /* DF, DF       IDF023 */
#define G_GNSS_SIGNAL_IDENTIFIER(a)      GETSSRBITS(a,  5)                    /* DF     IDF024 */
#define G_CODE_BIAS(a)                   GETSSRFLOATSIGN(a, 14, 1/100.0)      /* DF383, IDF025 */
#define G_YAW_ANGLE(a)                   GETSSRFLOAT    (a,  9, MPI/256.0)    /* DF480, IDF026 */
#define G_YAW_RATE(a)                    GETSSRFLOATSIGN(a,  8, MPI/8192.0)   /* DF481, IDF027 */
#define G_PHASE_BIAS(a)                  GETSSRFLOATSIGN(a, 20, 1/10000.)     /* DF482, IDF028 */

/* Phase specific part of GNSS phase bias message */
#define G_INTEGER_INDICATOR(a)           GETSSRBITS(a,  1)                    /* DF483, IDF029 */
#define G_WIDE_LANE_INDICATOR(a)         GETSSRBITS(a,  2)                    /* DF484, IDF030 */
#define G_DISCONTINUITY_COUNTER(a)       GETSSRBITS(a,  4)                    /* DF485, IDF031 */
#define G_DISPERSIVE_BIAS_INDICATOR(a)   GETSSRBITS(a,  1)                    /* DF486, IDF032 */
#define G_MW_CONSISTENCY_INDICATOR(a)    GETSSRBITS(a,  1)                    /* DF487, IDF033 */

/* URA */
#define G_SSR_URA(a)                     {int temp; GETSSRBITS(temp, 6) \
 (a) = URAToValue(temp);}                                                  /* DF389, IDF034 */

/* Ionosphere */
#define G_NO_IONO_LAYERS(a) {unsigned int temp; GETSSRBITS(temp, 2) a = temp+1;} /* DF472, IDF035 */
#define G_IONO_HEIGHT(a)                 GETSSRFLOAT(a, 8 ,    10000.0)          /* DF473, IDF036 */
#define G_IONO_DEGREE(a)    {unsigned int temp; GETSSRBITS(temp, 4) a = temp+1;} /* DF474, IDF037 */
#define G_IONO_ORDER(a)     {unsigned int temp; GETSSRBITS(temp, 4) a = temp+1;} /* DF475, IDF038 */
#define G_IONO_COEFF_C(a)                GETSSRFLOATSIGN(a, 16,1/200.0)          /* DF476, IDF039 */
#define G_IONO_COEFF_S(a)                GETSSRFLOATSIGN(a, 16,1/200.0)          /* DF477, IDF040 */
#define G_VTEC_QUALITY_INDICATOR(a)      GETSSRFLOAT     (a, 9, 1/20.0)          /* DF478, IDF041 */

};

#endif /* RTCM3_CLOCK_ORBIT_H */
