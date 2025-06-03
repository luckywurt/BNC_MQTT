/*
 * crsEncoder.cpp
 *
 *  Created on: Jun 18, 2024
 *      Author: stuerze
 */


#include "crsEncoder.h"

using namespace std;

// build up RTCM3 for serviceCrs
////////////////////////////////////////////////////////////////////////////
int t_crsEncoder::RTCM3(const t_serviceCrs& serviceCrs, char *buffer, size_t size) {
  STARTDATA
  INITBLOCK
  ADDBITS(12, 1300)
  int N = sizeof(serviceCrs._name) / sizeof(serviceCrs._name[0]);
  if (N > 31) {N = 31;}
  ADDBITS(5, N)
  for(int i = 0; i < N; i++) {
    ADDBITS(8, serviceCrs._name[i])
  }
  SCALEADDBITS(16, 100, serviceCrs._CE)
  ENDBLOCK
  return ressize;
}

// build up RTCM3 for rtcmCrs
////////////////////////////////////////////////////////////////////////////
int t_crsEncoder::RTCM3(const t_rtcmCrs& rtcmCrs, char *buffer, size_t size) {
  STARTDATA
    INITBLOCK

  ADDBITS(12, 1302)
  int N = sizeof(rtcmCrs._name) / sizeof(rtcmCrs._name[0]);
  if (N > 31) {N = 31;}
  ADDBITS(5, N)
  for(int i = 0; i < N; i++) {
    ADDBITS(8, rtcmCrs._name[i])
  }
  ADDBITS(1, rtcmCrs._anchor)
  ADDBITS(5, rtcmCrs._plateNumber)
  int dblinknum = rtcmCrs._databaseLinks.size();
  ADDBITS(3, dblinknum);
  if (dblinknum) {
    for(int d = 0; d < dblinknum; d++) {
      N = sizeof(rtcmCrs._databaseLinks[d]) / sizeof(rtcmCrs._databaseLinks[d][0]);
      for(int i = 0; i < N; i++) {
        ADDBITS(8, rtcmCrs._databaseLinks[d].toLatin1()[i])
      }
    }
  }
  ENDBLOCK
  return ressize;
}

// build up RTCM3 for
////////////////////////////////////////////////////////////////////////////
int t_crsEncoder::RTCM3(const t_helmertPar&  helmertPar, char *buffer, size_t size) {

  STARTDATA
    INITBLOCK

  ADDBITS(12, 1301)
  int N = sizeof(helmertPar._sourceName) / sizeof(helmertPar._sourceName[0]);
  if (N > 31) {N = 31;}
  ADDBITS(5, N)
  for(int i = 0; i < N; i++) {
    ADDBITS(8, helmertPar._sourceName[i])
  }
  N = sizeof(helmertPar._targetName) / sizeof(helmertPar._targetName[0]);
  if (N > 31) {N = 31;}
  ADDBITS(5, N)
  for(int i = 0; i < N; i++) {
    ADDBITS(8, helmertPar._targetName[i])
  }
  ADDBITS(8 , helmertPar._sysIdentNum)
  ADDBITS(10, helmertPar._utilTrafoMessageIndicator)
  ADDBITS(16, (helmertPar._mjd - 44244))

  SCALEADDBITS(23, 1000.0, helmertPar._dx)
  SCALEADDBITS(23, 1000.0, helmertPar._dy)
  SCALEADDBITS(23, 1000.0, helmertPar._dz)

  CRSADDBITSFLOAT(32, helmertPar._ox, 50000.0)
  CRSADDBITSFLOAT(32, helmertPar._oy, 50000.0)
  CRSADDBITSFLOAT(32, helmertPar._oz, 50000.0)

  SCALEADDBITS(25, 100000.0, helmertPar._sc)

  SCALEADDBITS(17, 50000.0, helmertPar._dxr)
  SCALEADDBITS(17, 50000.0, helmertPar._dyr)
  SCALEADDBITS(17, 50000.0, helmertPar._dzr)

  SCALEADDBITS(17, 2500000.0, helmertPar._oxr)
  SCALEADDBITS(17, 2500000.0, helmertPar._oyr)
  SCALEADDBITS(17, 2500000.0, helmertPar._ozr)

  SCALEADDBITS(14, 5000000.0, helmertPar._scr)

  ENDBLOCK
  return ressize;
}

