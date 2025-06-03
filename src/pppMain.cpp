
// Part of BNC, a utility for retrieving decoding and
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
 * Class:      t_pppMain
 *
 * Purpose:    Start of the PPP client(s)
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>

#include "pppMain.h"
#include "pppCrdFile.h"
#include "bncsettings.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_pppMain::t_pppMain() {
  _running = false;
}

// Destructor
//////////////////////////////////////////////////////////////////////////////
t_pppMain::~t_pppMain() {
  stop();
  QListIterator<t_pppOptions*> iOpt(_options);
  while (iOpt.hasNext()) {
    delete iOpt.next();
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppMain::start() {
  if (_running) {
    return;
  }

  try {
    readOptions();

    QListIterator<t_pppOptions*> iOpt(_options);
    while (iOpt.hasNext()) {
      const t_pppOptions* opt = iOpt.next();
      t_pppThread* pppThread = new t_pppThread(opt);
      pppThread->start();
      _pppThreads << pppThread;
      _running = true;
    }
  }
  catch (t_except exc) {
    _running = true;
    stop();
  }
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppMain::stop() {

  if (!_running) {
    return;
  }

  if (_realTime) {
    QListIterator<t_pppThread*> it(_pppThreads);
    while (it.hasNext()) {
      t_pppThread* pppThread = it.next();
      pppThread->exit();
      if (BNC_CORE->mode() != t_bncCore::interactive) {
        while(!pppThread->isFinished()) {
          pppThread->wait();
        }
        delete pppThread;
      }
    }
    _pppThreads.clear();
 }

  _running = false;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppMain::readOptions() {

  QListIterator<t_pppOptions*> iOpt(_options);
  while (iOpt.hasNext()) {
    delete iOpt.next();
  }
  _options.clear();

  bncSettings settings;

  _realTime = false;
  if      (settings.value("PPP/dataSource").toString() == "Real-Time Streams") {
    _realTime = true;
  }
  else if (settings.value("PPP/dataSource").toString() == "RINEX Files") {
    _realTime = false;
  }
  else {
    return;
  }

  QListIterator<QString> iSta(settings.value("PPP/staTable").toStringList());
  while (iSta.hasNext()) {
    QStringList hlp = iSta.next().split(",");

    if (hlp.size() < 10) {
      throw t_except("pppMain: wrong option staTable");
    }

    t_pppOptions* opt = new t_pppOptions();

    opt->_realTime         = _realTime;
    opt->_roverName        = hlp[0].toStdString();
    opt->_aprSigCrd[0]     = hlp[1].toDouble()+1e-10;
    opt->_aprSigCrd[1]     = hlp[2].toDouble()+1e-10;
    opt->_aprSigCrd[2]     = hlp[3].toDouble()+1e-10;
    opt->_noiseCrd[0]      = hlp[4].toDouble()+1e-10;
    opt->_noiseCrd[1]      = hlp[5].toDouble()+1e-10;
    opt->_noiseCrd[2]      = hlp[6].toDouble()+1e-10;
    opt->_aprSigTrp        = hlp[7].toDouble();
    opt->_noiseTrp         = hlp[8].toDouble();
    opt->_nmeaPort         = hlp[9].toInt();
#ifdef USE_PPP
    opt->_signalPriorities = hlp[10].toStdString();
    if (!opt->_signalPriorities.size()) {
      opt->_signalPriorities = "G:12&CWPSLX R:12&CP E:1&CBX E:5&QIX C:26&IQX";
    }
#endif
    if (_realTime) {
      opt->_corrMount.assign(settings.value("PPP/corrMount").toString().toStdString());
      opt->_isAPC = (opt->_corrMount.substr(0,4)=="SSRA");
      opt->_ionoMount.assign(settings.value("PPP/ionoMount").toString().toStdString());
    }
    else {
      opt->_rinexObs.assign(settings.value("PPP/rinexObs").toString().toStdString());
      opt->_rinexNav.assign(settings.value("PPP/rinexNav").toString().toStdString());
      opt->_corrFile.assign(settings.value("PPP/corrFile").toString().toStdString());
      QFileInfo tmp = QFileInfo(QString::fromStdString(opt->_corrFile));
      opt->_isAPC = (tmp.baseName().mid(0,4)=="SSRA");
      opt->_ionoFile.assign(settings.value("PPP/ionoFile").toString().toStdString());
    }

    opt->_crdFile.assign(settings.value("PPP/crdFile").toString().toStdString());
    opt->_antexFileName.assign(settings.value("PPP/antexFile").toString().toStdString());
#ifdef USE_PPP
    opt->_blqFileName.assign(settings.value("PPP/blqFile").toString().toStdString());
#endif
    opt->_sigmaC1      = settings.value("PPP/sigmaC1").toDouble(); if (opt->_sigmaC1 <= 0.0)  opt->_sigmaC1  = 1.00;
    opt->_sigmaL1      = settings.value("PPP/sigmaL1").toDouble(); if (opt->_sigmaL1 <= 0.0)  opt->_sigmaL1  = 0.01;
    opt->_sigmaGIM     = settings.value("PPP/sigmaGIM").toDouble();if (opt->_sigmaGIM <= 0.0) opt->_sigmaGIM = 1.00;
    opt->_corrWaitTime = settings.value("PPP/corrWaitTime").toDouble();
    if (!_realTime || opt->_corrMount.empty()) {
      opt->_corrWaitTime = 0;
    }

    opt->_pseudoObsIono  = false;
    opt->_refSatRequired = false;
#ifdef USE_PPP_SSR_I
    if (settings.value("PPP/lcGPS").toString() == "P3&L3") {
      opt->_LCsGPS.push_back(t_lc::cIF);
      opt->_LCsGPS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGPS").toString() == "P3") {
      opt->_LCsGPS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P3&L3") {
      opt->_LCsGLONASS.push_back(t_lc::cIF);
      opt->_LCsGLONASS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P3") {
      opt->_LCsGLONASS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGLONASS").toString() == "L3") {
      opt->_LCsGLONASS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGalileo").toString() == "P3&L3") {
      opt->_LCsGalileo.push_back(t_lc::cIF);
      opt->_LCsGalileo.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGalileo").toString() == "P3") {
      opt->_LCsGalileo.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGalileo").toString() == "L3") {
      opt->_LCsGalileo.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcBDS").toString() == "P3&L3") {
      opt->_LCsBDS.push_back(t_lc::cIF);
      opt->_LCsBDS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcBDS").toString() == "P3") {
      opt->_LCsBDS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcBDS").toString() == "L3") {
      opt->_LCsBDS.push_back(t_lc::lIF);
    }
#else
    // Pseudo Observations
    if      (settings.value("PPP/constraints").toString() == "no") {
      opt->_pseudoObsIono  = false;
      opt->_refSatRequired = false;
      opt->_ionoModelType = opt->est;
    }
    else if (settings.value("PPP/constraints").toString() == "Ionosphere: pseudo-obs") {
      opt->_pseudoObsIono  = true;
      opt->_refSatRequired = true;
      opt->_ionoModelType = opt->est;
    }
    // GPS
    if (settings.value("PPP/lcGPS").toString() == "Pi&Li") {
      opt->_LCsGPS.push_back(t_lc::c1);
      opt->_LCsGPS.push_back(t_lc::c2);
      opt->_LCsGPS.push_back(t_lc::l1);
      opt->_LCsGPS.push_back(t_lc::l2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGPS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGPS").toString() == "Pi") {
      opt->_LCsGPS.push_back(t_lc::c1);
      opt->_LCsGPS.push_back(t_lc::c2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGPS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGPS").toString() == "P1&L1") {
      opt->_LCsGPS.push_back(t_lc::c1);
      opt->_LCsGPS.push_back(t_lc::l1);
      if (opt->_pseudoObsIono) {
        opt->_LCsGPS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGPS").toString() == "P1") {
        opt->_LCsGPS.push_back(t_lc::c1);
        if (opt->_pseudoObsIono) {
          opt->_LCsGPS.push_back(t_lc::GIM);
        }
    }
    if (settings.value("PPP/lcGPS").toString() == "P3&L3") {
      opt->_LCsGPS.push_back(t_lc::cIF);
      opt->_LCsGPS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGPS").toString() == "P3") {
      opt->_LCsGPS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGPS").toString() == "L3") {
      opt->_LCsGPS.push_back(t_lc::lIF);
    }
    // GLONASS
    if (settings.value("PPP/lcGLONASS").toString() == "Pi&Li") {
      opt->_LCsGLONASS.push_back(t_lc::c1);
      opt->_LCsGLONASS.push_back(t_lc::c2);
      opt->_LCsGLONASS.push_back(t_lc::l1);
      opt->_LCsGLONASS.push_back(t_lc::l2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGLONASS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGLONASS").toString() == "Pi") {
      opt->_LCsGLONASS.push_back(t_lc::c1);
      opt->_LCsGLONASS.push_back(t_lc::c2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGLONASS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P1&L1") {
      opt->_LCsGLONASS.push_back(t_lc::c1);
      opt->_LCsGLONASS.push_back(t_lc::l1);
      if (opt->_pseudoObsIono) {
        opt->_LCsGLONASS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P1") {
        opt->_LCsGLONASS.push_back(t_lc::c1);
        if (opt->_pseudoObsIono) {
          opt->_LCsGLONASS.push_back(t_lc::GIM);
        }
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P3&L3") {
      opt->_LCsGLONASS.push_back(t_lc::cIF);
      opt->_LCsGLONASS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGLONASS").toString() == "P3") {
      opt->_LCsGLONASS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGLONASS").toString() == "L3") {
      opt->_LCsGLONASS.push_back(t_lc::lIF);
    }
    // Galileo
    if (settings.value("PPP/lcGalileo").toString() == "Pi&Li") {
      opt->_LCsGalileo.push_back(t_lc::c1);
      opt->_LCsGalileo.push_back(t_lc::c2);
      opt->_LCsGalileo.push_back(t_lc::l1);
      opt->_LCsGalileo.push_back(t_lc::l2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGalileo.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGalileo").toString() == "Pi") {
      opt->_LCsGalileo.push_back(t_lc::c1);
      opt->_LCsGalileo.push_back(t_lc::c2);
      if (opt->_pseudoObsIono) {
        opt->_LCsGalileo.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGalileo").toString() == "P1&L1") {
      opt->_LCsGalileo.push_back(t_lc::c1);
      opt->_LCsGalileo.push_back(t_lc::l1);
      if (opt->_pseudoObsIono) {
        opt->_LCsGalileo.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcGalileo").toString() == "P1") {
        opt->_LCsGalileo.push_back(t_lc::c1);
        if (opt->_pseudoObsIono) {
          opt->_LCsGalileo.push_back(t_lc::GIM);
        }
    }
    if (settings.value("PPP/lcGalileo").toString() == "P3&L3") {
      opt->_LCsGalileo.push_back(t_lc::cIF);
      opt->_LCsGalileo.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcGalileo").toString() == "P3") {
      opt->_LCsGalileo.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcGalileo").toString() == "L3") {
      opt->_LCsGalileo.push_back(t_lc::lIF);
    }
    // BDS
    if (settings.value("PPP/lcBDS").toString() == "Pi&Li") {
      opt->_LCsBDS.push_back(t_lc::c1);
      opt->_LCsBDS.push_back(t_lc::c2);
      opt->_LCsBDS.push_back(t_lc::l1);
      opt->_LCsBDS.push_back(t_lc::l2);
      if (opt->_pseudoObsIono) {
        opt->_LCsBDS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcBDS").toString() == "Pi") {
      opt->_LCsBDS.push_back(t_lc::c1);
      opt->_LCsBDS.push_back(t_lc::c2);
      if (opt->_pseudoObsIono) {
        opt->_LCsBDS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcBDS").toString() == "P1&L1") {
      opt->_LCsBDS.push_back(t_lc::c1);
      opt->_LCsBDS.push_back(t_lc::l1);
      if (opt->_pseudoObsIono) {
        opt->_LCsBDS.push_back(t_lc::GIM);
      }
    }
    if (settings.value("PPP/lcBDS").toString() == "P1") {
        opt->_LCsBDS.push_back(t_lc::c1);
        if (opt->_pseudoObsIono) {
          opt->_LCsBDS.push_back(t_lc::GIM);
        }
    }
    if (settings.value("PPP/lcBDS").toString() == "P3&L3") {
      opt->_LCsBDS.push_back(t_lc::cIF);
      opt->_LCsBDS.push_back(t_lc::lIF);
    }
    if (settings.value("PPP/lcBDS").toString() == "P3") {
      opt->_LCsBDS.push_back(t_lc::cIF);
    }
    if (settings.value("PPP/lcBDS").toString() == "L3") {
      opt->_LCsBDS.push_back(t_lc::lIF);
    }

    QString     priorStr  = QString::fromStdString(opt->_signalPriorities);
    QStringList priorList = priorStr.split(" ", Qt::SkipEmptyParts);
    QStringList hlpList;
    vector<char> systems = opt->systems();
    for (unsigned iSys= 0; iSys < systems.size(); iSys++) {
      char sys = systems[iSys];
      for (int ii = 0; ii < priorList.size(); ii++) {
        if (priorList[ii].indexOf(":") != -1) {
          hlpList = priorList[ii].split(":", Qt::SkipEmptyParts);
          if (hlpList.size() == 2 && hlpList[0].length() == 1 && hlpList[0][0] == sys) {
            hlpList = hlpList[1].split("&", Qt::SkipEmptyParts);
            if (hlpList.size() == 2) {
              for (int jj = 0; jj < hlpList[0].size(); jj++) {
                char bb = hlpList[0][jj].toLatin1();
                if      (sys == 'G' && opt->_frqBandsGPS.size() < 2) {
                  opt->_frqBandsGPS.push_back(bb);
                }
                else if (sys == 'R' && opt->_frqBandsGLONASS.size() < 2) {
                  opt->_frqBandsGLONASS.push_back(bb);
                }
                else if (sys == 'E' && opt->_frqBandsGalileo.size() < 2) {
                  opt->_frqBandsGalileo.push_back(bb);
                }
                else if (sys == 'C' && opt->_frqBandsBDS.size() < 2) {
                  opt->_frqBandsBDS.push_back(bb);
                }
              }
            }
          }
        }
      }
    }
#endif


    // Information from the coordinate file
    // ------------------------------------
    string crdFileName(settings.value("PPP/crdFile").toString().toStdString());
    if (!crdFileName.empty()) {
      vector<t_pppCrdFile::t_staInfo> staInfoVec;
      t_pppCrdFile::readCrdFile(crdFileName, staInfoVec);
      for (unsigned ii = 0; ii < staInfoVec.size(); ii++) {
        const t_pppCrdFile::t_staInfo& staInfo = staInfoVec[ii];
        if (staInfo._name == opt->_roverName) {
          opt->_xyzAprRover[0] = staInfo._xyz[0];
          opt->_xyzAprRover[1] = staInfo._xyz[1];
          opt->_xyzAprRover[2] = staInfo._xyz[2];
          opt->_neuEccRover[0] = staInfo._neuAnt[0];
          opt->_neuEccRover[1] = staInfo._neuAnt[1];
          opt->_neuEccRover[2] = staInfo._neuAnt[2];
          opt->_antNameRover   = staInfo._antenna;
          opt->_recNameRover   = staInfo._receiver;
          break;
        }
      }
    }

    opt->_minObs      = settings.value("PPP/minObs").toInt(); if (opt->_minObs < 4) opt->_minObs = 4;
    opt->_minEle      = settings.value("PPP/minEle").toDouble() * M_PI / 180.0;
    opt->_maxResC1    = settings.value("PPP/maxResC1").toDouble();  if (opt->_maxResC1 <= 0.0)  opt->_maxResC1  = 3.0;
    opt->_maxResL1    = settings.value("PPP/maxResL1").toDouble();  if (opt->_maxResL1 <= 0.0)  opt->_maxResL1  = 0.03;
    opt->_maxResGIM   = settings.value("PPP/maxResGIM").toDouble(); if (opt->_maxResGIM <= 0.0) opt->_maxResGIM = 3.0;
    opt->_eleWgtCode  = (settings.value("PPP/eleWgtCode").toInt() != 0);
    opt->_eleWgtPhase = (settings.value("PPP/eleWgtPhase").toInt() != 0);
    opt->_seedingTime = settings.value("PPP/seedingTime").toDouble();

    // Some default values
    // -------------------
    opt->_aprSigClk       = 3.0e5;
    opt->_aprSigClkOff    = 3.0e5;
    opt->_aprSigAmb       = 1.0e4;
    opt->_aprSigIon       = 1.0e6;
    opt->_noiseIon        = 1.0e6;
    opt->_aprSigCodeBias  = 10000.0;
    opt->_noiseCodeBias   = 10000.0;
    opt->_aprSigPhaseBias = 10000.0;
    opt->_noisePhaseBias  = 10000.0;

    _options << opt;
  }
}

