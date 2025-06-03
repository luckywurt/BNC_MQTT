#ifndef RTCM3_CLOCK_ORBIT_RTCM_H
#define RTCM3_CLOCK_ORBIT_RTCM_H

/* Programheader

 Name:           clock_orbit_rtcm.h
 Project:        RTCM3
 Version:        $Id: clock_orbit_rtcm.h 9050 2020-08-27 14:01:42Z stuerze $
 Authors:        Dirk Stöcker, Andrea Stürze
 Description:    state space approach: RTCM
 */

#include <string.h>
#include "clock_orbit.h"

class SsrCorrRtcm: public SsrCorr {
  //Q_OBJECT

public:
  SsrCorrRtcm() {
    setCorBase();
    setCorOffset();
    setCoType();
    setCbType();
    setPbType();
    setVtecType();
    setCodeType();

    satoffset << CLOCKORBIT_OFFSETGPS
        << CLOCKORBIT_OFFSETGLONASS
        << CLOCKORBIT_OFFSETGALILEO
        << CLOCKORBIT_OFFSETQZSS
        << CLOCKORBIT_OFFSETSBAS
        << CLOCKORBIT_OFFSETBDS
        << CLOCKORBIT_COUNTSAT;
  };

  ~SsrCorrRtcm() {};

  void setCorBase() {
    COBBASE_GPS     = 1057;
    COBBASE_GLONASS = 1063;
    COBBASE_GALILEO = 1240;
    COBBASE_QZSS    = 1246;
    COBBASE_SBAS    = 1252;
    COBBASE_BDS     = 1258;
    COBBASE_NUM     =    6;
    corbase << COBBASE_GPS
        << COBBASE_GLONASS
        << COBBASE_GALILEO
        << COBBASE_QZSS
        << COBBASE_SBAS
        << COBBASE_BDS;
  };

  void setCorOffset() {
    COBOFS_ORBIT     = 0;
    COBOFS_CLOCK     = 1;
    COBOFS_CBIAS     = 2;
    COBOFS_COMBINED  = 3;
    COBOFS_URA       = 4;
    COBOFS_HR        = 5;
    COBOFS_NUM       = 6;
  };

  void setCoType() {
    COTYPE_GPSORBIT        = COBBASE_GPS + COBOFS_ORBIT;
    COTYPE_GPSCLOCK        = COTYPE_GPSORBIT + 1;
    COTYPE_GPSCOMBINED     = COBBASE_GPS + COBOFS_COMBINED;
    COTYPE_GPSURA          = COTYPE_GPSCOMBINED + 1;
    COTYPE_GPSHR           = COTYPE_GPSURA + 1;

    COTYPE_GLONASSORBIT    = COBBASE_GLONASS + COBOFS_ORBIT;
    COTYPE_GLONASSCLOCK    = COTYPE_GLONASSORBIT + 1;
    COTYPE_GLONASSCOMBINED = COBBASE_GLONASS + COBOFS_COMBINED;
    COTYPE_GLONASSURA      = COTYPE_GLONASSCOMBINED + 1;
    COTYPE_GLONASSHR       = COTYPE_GLONASSURA + 1;

    COTYPE_GALILEOORBIT    = COBBASE_GALILEO + COBOFS_ORBIT,
    COTYPE_GALILEOCLOCK    = COTYPE_GALILEOORBIT + 1;
    COTYPE_GALILEOCOMBINED = COBBASE_GALILEO + COBOFS_COMBINED;
    COTYPE_GALILEOURA      = COTYPE_GALILEOCOMBINED + 1;
    COTYPE_GALILEOHR       = COTYPE_GALILEOURA + 1;

    COTYPE_QZSSORBIT       = COBBASE_QZSS + COBOFS_ORBIT;
    COTYPE_QZSSCLOCK       = COTYPE_QZSSORBIT + 1;
    COTYPE_QZSSCOMBINED    = COBBASE_QZSS + COBOFS_COMBINED,
    COTYPE_QZSSURA         = COTYPE_QZSSCOMBINED + 1;
    COTYPE_QZSSHR          = COTYPE_QZSSURA + 1;

    COTYPE_SBASORBIT       = COBBASE_SBAS + COBOFS_ORBIT;
    COTYPE_SBASCLOCK       = COTYPE_SBASORBIT + 1;
    COTYPE_SBASCOMBINED    = COBBASE_SBAS + COBOFS_COMBINED;
    COTYPE_SBASURA         = COTYPE_SBASCOMBINED + 1;
    COTYPE_SBASHR          = COTYPE_SBASURA + 1;

    COTYPE_BDSORBIT        = COBBASE_BDS + COBOFS_ORBIT;
    COTYPE_BDSCLOCK        = COTYPE_BDSORBIT + 1;
    COTYPE_BDSCOMBINED     = COBBASE_BDS + COBOFS_COMBINED,
    COTYPE_BDSURA          = COTYPE_BDSCOMBINED + 1;
    COTYPE_BDSHR           = COTYPE_BDSURA + 1;

    COTYPE_AUTO = 0;
  };

  void setCbType() {
    CBTYPE_GPS     = COBBASE_GPS     + COBOFS_CBIAS;
    CBTYPE_GLONASS = COBBASE_GLONASS + COBOFS_CBIAS;
    CBTYPE_GALILEO = COBBASE_GALILEO + COBOFS_CBIAS;
    CBTYPE_QZSS    = COBBASE_QZSS    + COBOFS_CBIAS;
    CBTYPE_SBAS    = COBBASE_SBAS    + COBOFS_CBIAS;
    CBTYPE_BDS     = COBBASE_BDS     + COBOFS_CBIAS;
    CBTYPE_AUTO = 0;
  };

  void setPbType() {
    PBTYPE_BASE    = 1265;
    PBTYPE_GPS     = PBTYPE_BASE;
    PBTYPE_GLONASS = PBTYPE_GPS++;
    PBTYPE_GALILEO = PBTYPE_GLONASS++;
    PBTYPE_QZSS    = PBTYPE_GALILEO++;
    PBTYPE_SBAS    = PBTYPE_QZSS++;
    PBTYPE_BDS     = PBTYPE_SBAS++;
    PBTYPE_AUTO = 0;
  };

  void setVtecType() {
      VTEC_BASE   = 1264;
  };

  void setCodeType() {
    RESERVED = 99;

    CODETYPE_GPS_L1_CA          =  0;
    CODETYPE_GPS_L1_P           =  1;
    CODETYPE_GPS_L1_Z           =  2;


    CODETYPE_GPS_L2_CA          =  5;
    CODETYPE_GPS_SEMI_CODELESS  =  6;
    CODETYPE_GPS_L2C_M          =  7;
    CODETYPE_GPS_L2C_L          =  8;
    CODETYPE_GPS_L2C_ML         =  9;
    CODETYPE_GPS_L2_P           = 10;
    CODETYPE_GPS_L2_Z           = 11;


    CODETYPE_GPS_L5_I           = 14;
    CODETYPE_GPS_L5_Q           = 15;
    CODETYPE_GPS_L5_IQ          = 16;
    CODETYPE_GPS_L1C_D          = 17;
    CODETYPE_GPS_L1C_P          = 18;
    CODETYPE_GPS_L1C_DP         = 19;

    CODETYPE_GLONASS_L1_CA      =  0;
    CODETYPE_GLONASS_L1_P       =  1;
    CODETYPE_GLONASS_L2_CA      =  2;
    CODETYPE_GLONASS_L2_P       =  3;
    CODETYPE_GLONASS_L1a_OCd    =  4;
    CODETYPE_GLONASS_L1a_OCp    =  5;
    CODETYPE_GLONASS_L1a_OCdp   =  6;
    CODETYPE_GLONASS_L2a_CSI    =  7;
    CODETYPE_GLONASS_L2a_OCp    =  8;
    CODETYPE_GLONASS_L2a_CSIOCp =  9;
    CODETYPE_GLONASS_L3_I       = 10;
    CODETYPE_GLONASS_L3_Q       = 11;
    CODETYPE_GLONASS_L3_IQ      = 12;

    CODETYPE_GALILEO_E1_A       =  0;
    CODETYPE_GALILEO_E1_B       =  1;
    CODETYPE_GALILEO_E1_C       =  2;
    CODETYPE_GALILEO_E1_BC      =  3;
    CODETYPE_GALILEO_E1_ABC     =  4;
    CODETYPE_GALILEO_E5A_I      =  5;
    CODETYPE_GALILEO_E5A_Q      =  6;
    CODETYPE_GALILEO_E5A_IQ     =  7;
    CODETYPE_GALILEO_E5B_I      =  8;
    CODETYPE_GALILEO_E5B_Q      =  9;
    CODETYPE_GALILEO_E5B_IQ     = 10;
    CODETYPE_GALILEO_E5_I       = 11;
    CODETYPE_GALILEO_E5_Q       = 12;
    CODETYPE_GALILEO_E5_IQ      = 13;
    CODETYPE_GALILEO_E6_A       = 14;
    CODETYPE_GALILEO_E6_B       = 15;
    CODETYPE_GALILEO_E6_C       = 16;
    CODETYPE_GALILEO_E6_BC      = 17;
    CODETYPE_GALILEO_E6_ABC     = 18;

    CODETYPE_QZSS_L1_CA         =  0;
    CODETYPE_QZSS_L1C_D         =  1;
    CODETYPE_QZSS_L1C_P         =  2;
    CODETYPE_QZSS_L2C_M         =  3;
    CODETYPE_QZSS_L2C_L         =  4;
    CODETYPE_QZSS_L2C_ML        =  5;
    CODETYPE_QZSS_L5_I          =  6;
    CODETYPE_QZSS_L5_Q          =  7;
    CODETYPE_QZSS_L5_IQ         =  8;
    CODETYPE_QZSS_L6_D          =  9;
    CODETYPE_QZSS_L6_P          = 10;
    CODETYPE_QZSS_L6_DP         = 11;
    CODETYPE_QZSS_L1C_DP        = 12;
    CODETYPE_QZSS_L1_S          = 13;
    CODETYPE_QZSS_L5_D          = 14;
    CODETYPE_QZSS_L5_P          = 15;
    CODETYPE_QZSS_L5_DP         = 16;
    CODETYPE_QZSS_L6_E          = 17;
    CODETYPE_QZSS_L6_DE         = 18;

    CODETYPE_SBAS_L1_CA         =  0;
    CODETYPE_SBAS_L5_I          =  1;
    CODETYPE_SBAS_L5_Q          =  2;
    CODETYPE_SBAS_L5_IQ         =  3;

    CODETYPE_BDS_B1_I           =  0;
    CODETYPE_BDS_B1_Q           =  1;
    CODETYPE_BDS_B1_IQ          =  2;
    CODETYPE_BDS_B3_I           =  3;
    CODETYPE_BDS_B3_Q           =  4;
    CODETYPE_BDS_B3_IQ          =  5;
    CODETYPE_BDS_B2_I           =  6;
    CODETYPE_BDS_B2_Q           =  7;
    CODETYPE_BDS_B2_IQ          =  8;
    CODETYPE_BDS_B1a_D          =  9;
    CODETYPE_BDS_B1a_P          = 10;
    CODETYPE_BDS_B1a_DP         = 11;
    CODETYPE_BDS_B2a_D          = 12;
    CODETYPE_BDS_B2a_P          = 13;
    CODETYPE_BDS_B2a_DP         = 14;




  }

  std::string       codeTypeToRnxType(char system, CodeType type);
  SsrCorr::CodeType rnxTypeToCodeType(char system, std::string type);

  size_t MakeClockOrbit(const struct ClockOrbit *co, ClockOrbitType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakeCodeBias(const struct CodeBias *b, CodeBiasType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakePhaseBias(const struct PhaseBias *b, PhaseBiasType type,
      int moremessagesfollow, char *buffer, size_t size);
  size_t MakeVTEC(const struct VTEC *v, int moremessagesfollow, char *buffer,
      size_t size);
  enum GCOB_RETURN GetSSR(struct ClockOrbit *co, struct CodeBias *b,
      struct VTEC *v, struct PhaseBias *pb, const char *buffer, size_t size,
      int *bytesused);
 };

#endif /* RTCM3_CLOCK_ORBIT_RTCM_H */
