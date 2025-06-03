/*
 * crsEncoder.h
 *
 *  Created on: Jun 18, 2024
 *      Author: stuerze
 */

#ifndef SRC_RTCM3_CRSENCODER_H_
#define SRC_RTCM3_CRSENCODER_H_

#include <QString>
#include <QStringList>
#include "bncutils.h"
#include "crs.h"
#include "bits.h"



class t_crsEncoder {
 public:
  static int RTCM3(const t_serviceCrs&  serviceCrs, char *buffer, size_t size);
  static int RTCM3(const t_rtcmCrs& rtcmCrs, char *buffer, size_t size);
  static int RTCM3(const t_helmertPar&  helmertPar, char *buffer, size_t size);
};


#endif /* SRC_RTCM3_CRSENCODER_H_ */
