// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2007
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Czech Technical University Prague, Department of Geodesy
// http://www.fsv.cvut.cz
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncEphUser
 *
 * Purpose:    Base for Classes that use Ephemerides
 *
 * Author:     L. Mervart
 *
 * Created:    27-Jan-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <cmath>
#include <iostream>

#include "bncephuser.h"
#include "bnccore.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncEphUser::bncEphUser(bool connectSlots) {
  if (connectSlots) {
    connect(BNC_CORE, SIGNAL(newGPSEph(t_ephGPS)),
            this, SLOT(slotNewGPSEph(t_ephGPS)), Qt::DirectConnection);

    connect(BNC_CORE, SIGNAL(newGlonassEph(t_ephGlo)),
            this, SLOT(slotNewGlonassEph(t_ephGlo)), Qt::DirectConnection);

    connect(BNC_CORE, SIGNAL(newGalileoEph(t_ephGal)),
            this, SLOT(slotNewGalileoEph(t_ephGal)), Qt::DirectConnection);

    connect(BNC_CORE, SIGNAL(newSBASEph(t_ephSBAS)),
            this, SLOT(slotNewSBASEph(t_ephSBAS)), Qt::DirectConnection);

    connect(BNC_CORE, SIGNAL(newBDSEph(t_ephBDS)),
            this, SLOT(slotNewBDSEph(t_ephBDS)), Qt::DirectConnection);
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncEphUser::~bncEphUser() {
  QMapIterator<QString, deque<t_eph*> > it(_eph);
  while (it.hasNext()) {
    it.next();
    const deque<t_eph*>& qq = it.value();
    for (unsigned ii = 0; ii < qq.size(); ii++) {
      delete qq[ii];
    }
  }
}

// New GPS Ephemeris
////////////////////////////////////////////////////////////////////////////
void bncEphUser::slotNewGPSEph(t_ephGPS eph) {
  putNewEph(&eph, true);
}

// New Glonass Ephemeris
////////////////////////////////////////////////////////////////////////////
void bncEphUser::slotNewGlonassEph(t_ephGlo eph) {
  putNewEph(&eph, true);
}

// New Galileo Ephemeris
////////////////////////////////////////////////////////////////////////////
void bncEphUser::slotNewGalileoEph(t_ephGal eph) {
  putNewEph(&eph, true);
}

// New SBAS Ephemeris
////////////////////////////////////////////////////////////////////////////
void bncEphUser::slotNewSBASEph(t_ephSBAS eph) {
  putNewEph(&eph, true);
}

// New BDS Ephemeris
////////////////////////////////////////////////////////////////////////////
void bncEphUser::slotNewBDSEph(t_ephBDS eph) {
  putNewEph(&eph, true);
}

//
////////////////////////////////////////////////////////////////////////////
t_irc bncEphUser::putNewEph(t_eph* eph, bool realTime) {

  QMutexLocker locker(&_mutex);

  if (eph == 0) {
    return failure;
  }

  const t_ephGPS*     ephGPS     = dynamic_cast<const t_ephGPS*>(eph);
  const t_ephGlo*     ephGlo     = dynamic_cast<const t_ephGlo*>(eph);
  const t_ephGal*     ephGal     = dynamic_cast<const t_ephGal*>(eph);
  const t_ephSBAS*    ephSBAS    = dynamic_cast<const t_ephSBAS*>(eph);
  const t_ephBDS*     ephBDS     = dynamic_cast<const t_ephBDS*>(eph);

  t_eph* newEph = 0;

  if      (ephGPS) {
    newEph = new t_ephGPS(*ephGPS);
  }
  else if (ephGlo) {
    newEph = new t_ephGlo(*ephGlo);
  }
  else if (ephGal) {
    newEph = new t_ephGal(*ephGal);
  }
  else if (ephSBAS) {
    newEph = new t_ephSBAS(*ephSBAS);
  }
  else if (ephBDS) {
    newEph = new t_ephBDS(*ephBDS);
  }
  else {
    return failure;
  }

  QString prn(newEph->prn().toInternalString().c_str());

  const t_eph* ephOld = ephLast(prn);

  if (ephOld &&
      (ephOld->checkState() == t_eph::bad ||
       ephOld->checkState() == t_eph::outdated )) {
    ephOld = 0;
  }

  if ( (ephOld == 0) || eph->isNewerThan(ephOld)) {
    checkEphemeris(eph, realTime);
  }
  else {
    delete newEph;
    return failure;
  }

  if (eph->checkState() != t_eph::bad &&
      eph->checkState() != t_eph::outdated) {
    deque<t_eph*>& qq = _eph[prn];
    newEph->setCheckState(eph->checkState());
    qq.push_back(newEph);
    if (qq.size() > _maxQueueSize) {
      delete qq.front();
      qq.pop_front();
    }
    ephBufferChanged();
    return success;
  }
  else {
    delete newEph;
    return failure;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void bncEphUser::checkEphemeris(t_eph* eph, bool realTime) {

  if (!eph) {
    return;
  }

  // Check whether the epoch is too far away the current time
  // --------------------------------------------------------
  if (realTime) {
    QDateTime now = currentDateAndTimeGPS();
    bncTime   currentTime(now.toString(Qt::ISODate).toStdString());
    if (outDatedBcep(eph, currentTime)) {
      eph->setCheckState(t_eph::outdated);
      return;
    }
  }

  // Check health status
  // -------------------
  if (eph->isUnhealthy()) {
    eph->setCheckState(t_eph::unhealthy);
    return;
  }

  // Simple Check - check satellite radial distance
  // ----------------------------------------------
  ColumnVector xc(6);
  ColumnVector vv(3);
  if (eph->getCrd(eph->TOC(), xc, vv, false) != success) {
    eph->setCheckState(t_eph::bad);
    return;
  }
  double rr = xc.Rows(1,3).NormFrobenius();

  const double MINDIST = 2.e7;
  const double MAXDIST = 6.e7;
  if (rr < MINDIST || rr > MAXDIST || std::isnan(rr)) {
    eph->setCheckState(t_eph::bad);
    return;
  }

  // Check consistency with older ephemeris
  // --------------------------------------
  const double MAXDIFF_INIT = 10.0;
  const double MINDIFF      = 0.005;
  QString      prn          = QString(eph->prn().toInternalString().c_str());
  t_eph*       ephL         = ephLast(prn);

  // maxdiff adapted to individual systems
  char   sys     = eph->prn().system();
  double MAXDIFF = MAXDIFF_INIT;
  if (sys == 'C' ||
      sys == 'S'    ) {
    MAXDIFF *= 10.0;
  }

  if (ephL && ephL->checkState() != t_eph::bad) {
    ColumnVector xcL(6);
    ColumnVector vvL(3);
    if (ephL->getCrd(eph->TOC(), xcL, vvL, false) != success) {
      ephL->setCheckState(t_eph::bad);
      return;
    }

    double diff  = (xc.Rows(1,3) - xcL.Rows(1,3)).NormFrobenius();
    double diffC = fabs(xc(4) - xcL(4)) * t_CST::c;

    if (diff  < MAXDIFF &&
        diffC < MAXDIFF) {
      if (diff  < MINDIFF &&
          diffC < MINDIFF &&
          ephL->checkState() == t_eph::ok) {
        // to prevent the same data sets with different TOC values
        eph->setCheckState(t_eph::bad);
      }
      else {
        eph->setCheckState(t_eph::ok);
        ephL->setCheckState(t_eph::ok);
      }
    }
    else {
      if (ephL->checkState() == t_eph::ok) {
        eph->setCheckState(t_eph::bad);
      }
    }

    // some lines to allow update of ephemeris data sets after an outage
    // -----------------------------------------------------------------
    double dt = eph->TOC() - ephL->TOC();

    // update interval: 2h
    if      (eph->system() == t_eph::GPS     && dt >  7200.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // update interval: 3h
    else if (eph->system() == t_eph::Galileo && dt > 10800.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // updated every 30 minutes + 5 min
    else if (eph->system() == t_eph::GLONASS && dt >  2100.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // updated every ?
    else if (eph->system() == t_eph::QZSS    && dt >  3600.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // maximum update interval: 300 sec
    else if  (eph->system() == t_eph::SBAS   && dt >   600.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // updates 1h + 5 min
    else if  (eph->system() == t_eph::BDS    && dt >  3900.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
    // update interval: up to 24 hours
    else if  (eph->system() == t_eph::NavIC  && dt > 86400.0) {
      ephL->setCheckState(t_eph::outdated);
      return;
    }
  }
}

