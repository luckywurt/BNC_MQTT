/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncEphUploadCaster
 *
 * Purpose:    Connection to NTRIP Caster for Ephemeris
 *
 * Author:     L. Mervart
 *
 * Created:    03-Apr-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>
#include <math.h>
#include "bncephuploadcaster.h"
#include "bncsettings.h"
#include "RTCM3/ephEncoder.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncEphUploadCaster::bncEphUploadCaster() : bncEphUser(true) {
  bncSettings settings;
  int     sampl    = settings.value("uploadSamplRtcmEph").toInt();

  // List of upload casters
  // ----------------------
  int iRow = -1;
  QListIterator<QString> it(settings.value("uploadEphMountpointsOut").toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(",");
    if (hlp.size() > 5) {
      ++iRow;
      int  outPort = hlp[1].toInt();
      bncUploadCaster* newCaster = new bncUploadCaster(hlp[2], hlp[0], outPort,
                                                       hlp[3], hlp[4],
                                                       hlp[5], iRow, sampl);

      connect(newCaster, SIGNAL(newBytes(QByteArray,double)),
              this, SIGNAL(newBytes(QByteArray,double)));

      newCaster->start();
      _casters.push_back(newCaster);
    }
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncEphUploadCaster::~bncEphUploadCaster() {
  for (int ic = 0; ic < _casters.size(); ic++) {
    _casters[ic]->deleteSafely();
  }
}

// List of Stored Ephemeris changed (virtual)
////////////////////////////////////////////////////////////////////////////
void bncEphUploadCaster::ephBufferChanged() {
  bncSettings settings;
  int iRow = -1;
  QListIterator<QString> it(settings.value("uploadEphMountpointsOut").toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(",");
    if (hlp.size() > 3) {
      ++iRow;
      QString system = hlp[6];
      QByteArray outBuffer;

      QDateTime now = currentDateAndTimeGPS();
      bncTime currentTime(now.toString(Qt::ISODate).toStdString());

      QListIterator<QString> it(prnList());
      while (it.hasNext()) {
        const t_eph* eph = ephLast(it.next());

        bncTime toc = eph->TOC();
        double dt = currentTime -toc;

        const t_ephGPS*  ephGPS  = dynamic_cast<const t_ephGPS*>(eph);
        const t_ephGlo*  ephGlo  = dynamic_cast<const t_ephGlo*>(eph);
        const t_ephGal*  ephGal  = dynamic_cast<const t_ephGal*>(eph);
        const t_ephSBAS* ephSBAS = dynamic_cast<const t_ephSBAS*>(eph);
        const t_ephBDS*  ephBDS  = dynamic_cast<const t_ephBDS*>(eph);

        unsigned char Array[80];
        int size = 0;

        if      (ephGPS && ephGPS->system() == t_eph::GPS  &&  (system == "ALL" || system.contains('G'))) {
          if (dt < 14400.0 || dt > -7200.0) {
            size = t_ephEncoder::RTCM3(*ephGPS, Array);
          }
        }
        else if (ephGPS && ephGPS->system() == t_eph::QZSS && (system == "ALL" || system.contains('J'))) {
          if (dt <  7200.0 || dt > -3600.0) {
            size = t_ephEncoder::RTCM3(*ephGPS, Array);
          }
        }
        else if (ephGlo && (system == "ALL" || system.contains('R'))) {
          if (dt <  3900.0 || dt > -2100.0) {
            size = t_ephEncoder::RTCM3(*ephGlo, Array);
          }
        }
        else if (ephGal && (system == "ALL" || system.contains('E'))) {
          if (dt < 14400.0 || dt >     0.0) {
            size = t_ephEncoder::RTCM3(*ephGal, Array);
          }
        }
        else if (ephSBAS && (system == "ALL" || system.contains('S'))) {
          if (dt <   600.0 || dt >  -600.0) {
            size = t_ephEncoder::RTCM3(*ephSBAS, Array);
          }
        }
        else if (ephBDS && (system == "ALL" || system.contains('C'))) {
          if (dt <  3900.0 || dt >     0.0) {
            size = t_ephEncoder::RTCM3(*ephBDS, Array);
          }
        }
        else if (ephGPS && ephGPS->system() == t_eph::NavIC && (system == "ALL" || system.contains('I'))) {
          if (fabs(dt < 86400.0)) {
            size = t_ephEncoder::RTCM3(*ephGPS, Array);
          }
        }
        if (size > 0) {
          outBuffer += QByteArray((char*) Array, size);
        }
      }
      if (outBuffer.size() > 0) {
        _casters.at(iRow)->setOutBuffer(outBuffer);
      }
    }
  }
}
