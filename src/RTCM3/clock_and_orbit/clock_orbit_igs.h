#ifndef RTCM3_CLOCK_ORBIT_IGS_H
#define RTCM3_CLOCK_ORBIT_IGS_H

/* Programheader

 Name:           clock_orbit_igs.h
 Project:        RTCM3
 Version:        $Id: clock_orbit_igs.h 8966 2020-07-01 07:48:35Z stuerze $
 Authors:        Dirk Stöcker, Andrea Stürze
 Description:    state space approach: IGS
 */
#include <QDebug>
#include <string.h>
#include "clock_orbit.h"

class SsrCorrIgs: public SsrCorr {
  //Q_OBJECT

public:
  SsrCorrIgs() {
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

  ~SsrCorrIgs() {};

  void setCorBase() {
    COBBASE_GPS     =  21;
    COBBASE_GLONASS =  41;
    COBBASE_GALILEO =  61;
    COBBASE_QZSS    =  81;
    COBBASE_BDS     = 101;
    COBBASE_SBAS    = 121;
    COBBASE_NUM     =   6;

    corbase << COBBASE_GPS
            << COBBASE_GLONASS
            << COBBASE_GALILEO
            << COBBASE_QZSS
            << COBBASE_SBAS
            << COBBASE_BDS;
  }

  void setCorOffset() {
    COBOFS_ORBIT    = 0; // GPS: IM21
    COBOFS_CLOCK    = 1; // GPS: IM22
    COBOFS_COMBINED = 2; // GPS: IM23
    COBOFS_HR       = 3; // GPS: IM24
    COBOFS_CBIAS    = 4; // GPS: IM25
    COBOFS_PBIAS    = 5; // GPS: IM26
    COBOFS_URA      = 6; // GPS: IM27
    COBOFS_NUM      = 7;
  };

  void setCoType() {
    COTYPE_GPSORBIT        = COBBASE_GPS + COBOFS_ORBIT;
    COTYPE_GPSCLOCK        = COTYPE_GPSORBIT + 1;
    COTYPE_GPSCOMBINED     = COBBASE_GPS + COBOFS_COMBINED;
    COTYPE_GPSHR           = COTYPE_GPSCOMBINED + 1;
    COTYPE_GPSURA          = COBBASE_GPS + COBOFS_URA;

    COTYPE_GLONASSORBIT    = COBBASE_GLONASS + COBOFS_ORBIT;
    COTYPE_GLONASSCLOCK    = COTYPE_GLONASSORBIT + 1;
    COTYPE_GLONASSCOMBINED = COBBASE_GLONASS + COBOFS_COMBINED,
    COTYPE_GLONASSHR       = COTYPE_GLONASSCOMBINED + 1;
    COTYPE_GLONASSURA      = COBBASE_GLONASS + COBOFS_URA,

    COTYPE_GALILEOORBIT    = COBBASE_GALILEO + COBOFS_ORBIT;
    COTYPE_GALILEOCLOCK    = COTYPE_GALILEOORBIT + 1;
    COTYPE_GALILEOCOMBINED = COBBASE_GALILEO + COBOFS_COMBINED,
    COTYPE_GALILEOHR       = COTYPE_GALILEOCOMBINED + 1;
    COTYPE_GALILEOURA      = COBBASE_GALILEO + COBOFS_URA;

    COTYPE_QZSSORBIT       = COBBASE_QZSS + COBOFS_ORBIT;
    COTYPE_QZSSCLOCK       = COTYPE_QZSSORBIT + 1;
    COTYPE_QZSSCOMBINED    = COBBASE_QZSS + COBOFS_COMBINED;
    COTYPE_QZSSHR          = COTYPE_QZSSCOMBINED + 1;
    COTYPE_QZSSURA         = COBBASE_QZSS + COBOFS_URA;

    COTYPE_SBASORBIT       = COBBASE_SBAS + COBOFS_ORBIT;
    COTYPE_SBASCLOCK       = COTYPE_SBASORBIT + 1;
    COTYPE_SBASCOMBINED    = COBBASE_SBAS + COBOFS_COMBINED;
    COTYPE_SBASHR          = COTYPE_SBASCOMBINED + 1;
    COTYPE_SBASURA         = COBBASE_SBAS + COBOFS_URA;

    COTYPE_BDSORBIT        = COBBASE_BDS + COBOFS_ORBIT;
    COTYPE_BDSCLOCK        = COTYPE_BDSORBIT + 1;
    COTYPE_BDSCOMBINED     = COBBASE_BDS + COBOFS_COMBINED;
    COTYPE_BDSHR           = COTYPE_BDSCOMBINED + 1;
    COTYPE_BDSURA          = COBBASE_BDS + COBOFS_URA;

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
    PBTYPE_GPS     = COBBASE_GPS     + COBOFS_PBIAS;
    PBTYPE_GLONASS = COBBASE_GLONASS + COBOFS_PBIAS;
    PBTYPE_GALILEO = COBBASE_GALILEO + COBOFS_PBIAS;
    PBTYPE_QZSS    = COBBASE_QZSS    + COBOFS_PBIAS;
    PBTYPE_SBAS    = COBBASE_SBAS    + COBOFS_PBIAS;
    PBTYPE_BDS     = COBBASE_BDS     + COBOFS_PBIAS;
    PBTYPE_AUTO = 0;
  };

  void setVtecType() {
      VTEC_BASE   = 201;
  };

  void setCodeType() {
    RESERVED = 99;

    CODETYPE_GPS_L1_CA          =  0;
    CODETYPE_GPS_L1_P           =  1;
    CODETYPE_GPS_L1_Z           =  2;
    CODETYPE_GPS_L1C_D          =  3;
    CODETYPE_GPS_L1C_P          =  4;
    CODETYPE_GPS_L2_CA          =  5;
    CODETYPE_GPS_SEMI_CODELESS  =  6;
    CODETYPE_GPS_L2C_M          =  7;
    CODETYPE_GPS_L2C_L          =  8;
    //RESERVED                  =  9;
    CODETYPE_GPS_L2_P           = 10;
    CODETYPE_GPS_L2_Z           = 11;
    //RESERVED                  = 12;
    //RESERVED                  = 13;
    CODETYPE_GPS_L5_I           = 14;
    CODETYPE_GPS_L5_Q           = 15;





    CODETYPE_GLONASS_L1_CA      =  0;
    CODETYPE_GLONASS_L1_P       =  1;
    CODETYPE_GLONASS_L2_CA      =  2;
    CODETYPE_GLONASS_L2_P       =  3;
    CODETYPE_GLONASS_L1a_OCd    =  4;
    CODETYPE_GLONASS_L1a_OCp    =  5;
    CODETYPE_GLONASS_L2a_CSI    =  6;
    CODETYPE_GLONASS_L2a_OCp    =  7;
    CODETYPE_GLONASS_L3_I       =  8;
    CODETYPE_GLONASS_L3_Q       =  9;




    CODETYPE_GALILEO_E1_A       =  0;
    CODETYPE_GALILEO_E1_B       =  1;
    CODETYPE_GALILEO_E1_C       =  2;
    //RESERVED_E1_BC            =  3;
    //RESERVED_E1_ABC           =  4;
    CODETYPE_GALILEO_E5A_I      =  5;
    CODETYPE_GALILEO_E5A_Q      =  6;
    //RESERVED_E5A_IQ           =  7;
    CODETYPE_GALILEO_E5B_I      =  8;
    CODETYPE_GALILEO_E5B_Q      =  9;
    //RESERVED_E5B_IQ           = 10;
    //RESERVED_E5_I             = 11;
    //RESERVED_E5_Q             = 12;
    //RESERVED_E5_IQ            = 13;
    CODETYPE_GALILEO_E6_A       = 14;
    CODETYPE_GALILEO_E6_B       = 15;
    CODETYPE_GALILEO_E6_C       = 16;
    //RESERVED_E6_BC            = 17;
    //RESERVED_E6_ABC           = 18;

    CODETYPE_QZSS_L1_CA         = 0;
    CODETYPE_QZSS_L1C_D         = 1;
    CODETYPE_QZSS_L1C_P         = 2;
    CODETYPE_QZSS_L2C_M         = 3;
    CODETYPE_QZSS_L2C_L         = 4;
    //RESEVED_L2C_ML            = 5;
    CODETYPE_QZSS_L5_I          = 6;
    CODETYPE_QZSS_L5_Q          = 7;
    //RESERVED_L5_IQ            = 8;
    CODETYPE_QZSS_L6_D          = 9;
    CODETYPE_QZSS_L6_P          = 10;
    //RESERVED_L6_DP            = 11;
    //RESERVED_L1C_DP           = 12;
    //RESERVED_L1_S             = 13;
    //RESERVED_L5_D             = 14;
    //RESERVED_L5_P             = 15;
    //RESERVED_L5_DP            = 16;
    CODETYPE_QZSS_L6_E          = 17;
    //RESERVED_L6_DE            = 18;

    CODETYPE_SBAS_L1_CA        =  0;
    CODETYPE_SBAS_L5_I         =  1;
    CODETYPE_SBAS_L5_Q         =  2;
    //RESERVED_CODETYPE_SBAS_L5_IQ      = 3;

    CODETYPE_BDS_B1_I          =  0;
    CODETYPE_BDS_B1_Q          =  1;
    //RESERVED_CODETYPE_BDS_B1_IQ       = 2;
    CODETYPE_BDS_B3_I          =  3;
    CODETYPE_BDS_B3_Q          =  4;
    //RESERVED_CODETYPE_BDS_B3_IQ       = 5;
    CODETYPE_BDS_B2_I          =  6;
    CODETYPE_BDS_B2_Q          =  7;
    //RESERVED_CODETYPE_BDS_B2_IQ       = 8;
    CODETYPE_BDS_B1a_D         =  9;
    CODETYPE_BDS_B1a_P         = 10;
    //RESERVED_CODETYPE_BDS_B1a_DP      = 11;
    CODETYPE_BDS_B2a_D         = 12;
    CODETYPE_BDS_B2a_P         = 13;
    //RESEVED_CODETYPE_BDS_B2a_DP       = 14;
    CODETYPE_BDS_B1_A          = 15; //NEW 1A
    //RESERVED                 = 16;
    //RESERVED                 = 17;
    CODETYPE_BDS_B3_A          = 18;  //NEW 6A
   };

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

#endif /* RTCM3_CLOCK_ORBIT_IGS_H */
