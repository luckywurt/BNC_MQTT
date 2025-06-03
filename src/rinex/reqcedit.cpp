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
 * Class:      t_reqcEdit
 *
 * Purpose:    Edit/Concatenate RINEX Files
 *
 * Author:     L. Mervart
 *
 * Created:    11-Apr-2012
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>
#include "reqcedit.h"
#include "bnccore.h"
#include "bncsettings.h"
#include "bncutils.h"
#include "rnxobsfile.h"
#include "rnxnavfile.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_reqcEdit::t_reqcEdit(QObject* parent) : QThread(parent) {

  bncSettings settings;

  _logFileName    = settings.value("reqcOutLogFile").toString(); expandEnvVar(_logFileName);
  _logFile        = 0;
  _logStream      = 0;
  _obsFileNames   = settings.value("reqcObsFile").toString().split(",", Qt::SkipEmptyParts);
  _outObsFileName = settings.value("reqcOutObsFile").toString();
  _navFileNames   = settings.value("reqcNavFile").toString().split(",", Qt::SkipEmptyParts);
  _outNavFileName = settings.value("reqcOutNavFile").toString();
  int version     = settings.value("reqcRnxVersion").toInt();
  if (version == 2) {
    _rnxVersion = defaultRnxObsVersion2;
  }
  else if (version == 3) {
    _rnxVersion = defaultRnxObsVersion3;
  }
  else if (version == 4) {
    _rnxVersion = defaultRnxObsVersion4;
  }
  _samplingRate   = settings.value("reqcSampling").toString().split("sec").first().toDouble();
  _begTime        = bncTime(settings.value("reqcStartDateTime").toString().toLatin1().data());
  _endTime        = bncTime(settings.value("reqcEndDateTime").toString().toLatin1().data());

  _checkEph = false;

}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_reqcEdit::~t_reqcEdit() {
  for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
    delete _rnxObsFiles[ii];
  }
  for (int ii = 0; ii < _ephs.size(); ii++) {
    delete _ephs[ii];
  }
  delete _logStream; _logStream     = 0;
  delete _logFile; _logFile = 0;
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::run() {

  // Open Log File
  // -------------
  if (!_logFileName.isEmpty()) {
    _logFile = new QFile(_logFileName);
    if (_logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
      _logStream = new QTextStream();
      _logStream->setDevice(_logFile);
    }
  }

  // Log File Header
  // ---------------
  if (_logStream) {
    *_logStream << QByteArray(78, '-') << Qt::endl;
    *_logStream << "RINEX File Editing\n";
    *_logStream << QByteArray(78, '-') << Qt::endl;

    *_logStream << QByteArray("Program").leftJustified(15) << ": "
          << BNC_CORE->pgmName() << Qt::endl;
    *_logStream << QByteArray("Run by").leftJustified(15) << ": "
          << BNC_CORE->userName() << Qt::endl;
    *_logStream << QByteArray("Date").leftJustified(15) << ": "
          << QDateTime::currentDateTime().toUTC().toString("yyyy-MM-dd hh:mm:ss") << Qt::endl;
    *_logStream << QByteArray("RINEX Version").leftJustified(15) << ": "
          << _rnxVersion << Qt::endl;
    *_logStream << QByteArray("Sampling").leftJustified(15) << ": "
          << _samplingRate << " sec" << Qt::endl;
    *_logStream << QByteArray("Start time").leftJustified(15) << ": "
          << _begTime.datestr().c_str() << ' '
          << _begTime.timestr(0).c_str() << Qt::endl;
    *_logStream << QByteArray("End time").leftJustified(15) << ": "
          << _endTime.datestr().c_str() << ' '
          << _endTime.timestr(0).c_str() << Qt::endl;
    *_logStream << QByteArray("Input Obs Files").leftJustified(15) << ": "
          << _obsFileNames.join(",") << Qt::endl;
    *_logStream << QByteArray("Input Nav Files").leftJustified(15) << ": "
          << _navFileNames.join(",") << Qt::endl;
    *_logStream << QByteArray("Output Obs File").leftJustified(15) << ": "
          << _outObsFileName << Qt::endl;
    *_logStream << QByteArray("Output Nav File").leftJustified(15) << ": "
          << _outNavFileName << Qt::endl;

    *_logStream << QByteArray(78, '-') << Qt::endl;
    _logStream->flush();
  }

  // Handle Observation Files
  // ------------------------
  editObservations();

  // Handle Navigations Files
  // ------------------------
  editEphemerides();

  // Exit (thread)
  // -------------
  if (BNC_CORE->mode() != t_bncCore::interactive) {
    qApp->exit(9);
    msleep(100); //sleep 0.1 sec
  }
  else {
    emit finished();
    deleteLater();
  }

}

// Initialize input observation files, sort them according to start time
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::initRnxObsFiles(const QStringList& obsFileNames,
                                 QVector<t_rnxObsFile*>& rnxObsFiles,
                                 QTextStream* log) {

  QStringListIterator it(obsFileNames);
  while (it.hasNext()) {
    QString fileName = it.next();
    if (fileName.indexOf('*') != -1 ||
        fileName.indexOf('?') != -1) {
      QFileInfo fileInfo(fileName);
      QDir dir = fileInfo.dir();
      QStringList filters; filters << fileInfo.fileName();
      QListIterator<QFileInfo> it(dir.entryInfoList(filters));
      while (it.hasNext()) {
        QString filePath = it.next().filePath();
        t_rnxObsFile* rnxObsFile = 0;
        try {
          rnxObsFile = new t_rnxObsFile(filePath, t_rnxObsFile::input);
          rnxObsFiles.append(rnxObsFile);
        }
        catch (...) {
          delete rnxObsFile;
          if (log) {
            *log << "Error in rnxObsFile " << filePath.toLatin1().data() << Qt::endl;
          }
        }
      }
    }
    else {
      t_rnxObsFile* rnxObsFile = 0;
      try {
        rnxObsFile = new t_rnxObsFile(fileName, t_rnxObsFile::input);
        rnxObsFiles.append(rnxObsFile);
      }
      catch (...) {
        if (log) {
          *log << "Error in rnxObsFile " << fileName.toLatin1().data() << Qt::endl;
        }
      }
    }
  }
  std::stable_sort(rnxObsFiles.begin(), rnxObsFiles.end(),
              t_rnxObsFile::earlierStartTime);
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::editObservations() {

  // Easy Exit
  // ---------
  if (_obsFileNames.isEmpty() || _outObsFileName.isEmpty()) {
    return;
  }

  t_reqcEdit::initRnxObsFiles(_obsFileNames, _rnxObsFiles, _logStream);

  // Initialize output observation file
  // ----------------------------------
  t_rnxObsFile outObsFile(_outObsFileName, t_rnxObsFile::output);


  // Put together all run by date entries
  // ------------------------------------
  QStringList runByDate;
  if (_rnxVersion >= 4.0 && _rnxObsFiles.size() > 1) {
    for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
      t_rnxObsFile* rnxObsFile = _rnxObsFiles[ii];
      QStringListIterator itRunByDt(rnxObsFile->runByDate());
      while (itRunByDt.hasNext()) {
        runByDate.append(itRunByDt.next());
      }
    }
    runByDate.removeDuplicates();
  }

  // Select observation types
  // ------------------------
  bncSettings settings;
  QStringList useObsTypes = settings.value("reqcUseObsTypes").toString().split(" ", Qt::SkipEmptyParts);

  // Put together all observation types
  // ----------------------------------
  if (_rnxObsFiles.size() > 1 && useObsTypes.size() == 0) {
    for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
      t_rnxObsFile* obsFile = _rnxObsFiles[ii];
      for (int iSys = 0; iSys < obsFile->numSys(); iSys++) {
        char sys = obsFile->system(iSys);
        if (sys != ' ') {
          for (int iType = 0; iType < obsFile->nTypes(sys); iType++) {
            QString type = obsFile->obsType(sys, iType);
            if (_rnxVersion < 3.0) {
              useObsTypes << type;
            }
            else {
              useObsTypes << QString(sys) + ":" + type;
            }
          }
        }
      }
    }
    useObsTypes.removeDuplicates();
  }

  // Put together all phase shifts
  // -----------------------------
  QStringList phaseShifts;
  if (_rnxVersion >= 3.0 && _rnxObsFiles.size() > 1) {
    for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
      t_rnxObsFile* obsFile = _rnxObsFiles[ii];
      phaseShifts << obsFile->phaseShifts();
    }
    phaseShifts.removeDuplicates();
  }

  // Put together all GLONASS biases
  // -------------------------------
  QStringList gloBiases;
  if (_rnxVersion >= 3.0 && _rnxObsFiles.size() > 1) {
    for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
      t_rnxObsFile* obsFile = _rnxObsFiles[ii];
      if (ii == 0 &&  obsFile->numGloBiases() == 4) {
        break;
      }
      else {
        gloBiases << obsFile->gloBiases();
      }
    }
    gloBiases.removeDuplicates();
  }

  // Put together all GLONASS slots
  // -----------------------------
  QStringList gloSlots;
  if (_rnxVersion >= 3.0 && _rnxObsFiles.size() > 1) {
    for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
      t_rnxObsFile* obsFile = _rnxObsFiles[ii];
      if (ii == 0 &&
          obsFile->numGloSlots() == signed(t_prn::MAXPRN_GLONASS)) {
        break;
      }
      else {
        gloSlots << obsFile->gloSlots();
      }
    }
    gloSlots.removeDuplicates();
  }

  // Loop over all input observation files
  // -------------------------------------
  for (int ii = 0; ii < _rnxObsFiles.size(); ii++) {
    t_rnxObsFile* obsFile = _rnxObsFiles[ii];
    if (_logStream) {
      *_logStream << "Input Obs File: " << obsFile->fileName() << "  start: "
            << obsFile->startTime().datestr().c_str() << ' '
            << obsFile->startTime().timestr(0).c_str() << Qt::endl;
    }
    if (ii == 0) {
      outObsFile.setHeader(obsFile->header(), int(_rnxVersion), &useObsTypes,
          &phaseShifts, &gloBiases, &gloSlots, &runByDate);
      if (_begTime.valid() && _begTime > outObsFile.startTime()) {
        outObsFile.setStartTime(_begTime);
      }
      if (_samplingRate > outObsFile.interval()) {
        outObsFile.setInterval(_samplingRate);
      }
      editRnxObsHeader(outObsFile);
      bncSettings settings;
      QMap<QString, QString> txtMap;
      QString runBy = settings.value("reqcRunBy").toString();
      if (!runBy.isEmpty()) {
        txtMap["RUN BY"]  = runBy;
      }
      QString comment = settings.value("reqcComment").toString();
      if (!comment.isEmpty()) {
        txtMap["COMMENT"]  = comment;
      }
      if (int(_rnxVersion) < int(obsFile->header().version())) {
        addRnxConversionDetails(obsFile, txtMap);
      }
      outObsFile.header().write(outObsFile.stream(), &txtMap);
    }
    t_rnxObsFile::t_rnxEpo* epo = 0;
    try {
      while ( (epo = obsFile->nextEpoch()) != 0) {
        if (_begTime.valid() && epo->tt < _begTime) {
          continue;
        }
        if (_endTime.valid() && epo->tt > _endTime) {
          break;
        }

        int sec = int(nint(epo->tt.gpssec()*10));
        if (sec % (int(_samplingRate)*10) == 0) {
          applyLLI(obsFile, epo);
          outObsFile.writeEpoch(epo);
        }
        else {
          rememberLLI(obsFile, epo);
        }
      }
    }
    catch (QString str) {
      if (_logStream) {
        *_logStream << "Exception " << str << Qt::endl;
      }
      else {
        qDebug() << str;
      }
      return;
    }
    catch (...) {
      if (_logStream) {
        *_logStream << "Exception unknown" << Qt::endl;
      }
      else {
        qDebug() << "Exception unknown";
      }
      return;
    }
  }
}

// Change RINEX Header Content
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::editRnxObsHeader(t_rnxObsFile& obsFile) {

  bncSettings settings;

  QString oldMarkerName   = settings.value("reqcOldMarkerName").toString();
  QString newMarkerName   = settings.value("reqcNewMarkerName").toString();
  if (!newMarkerName.isEmpty()) {
    if (oldMarkerName.isEmpty() ||
        QRegExp(oldMarkerName).exactMatch(obsFile.markerName())) {
      obsFile.setMarkerName(newMarkerName);
    }
  }

  QString oldAntennaName  = settings.value("reqcOldAntennaName").toString();
  QString newAntennaName  = settings.value("reqcNewAntennaName").toString();
  if (!newAntennaName.isEmpty()) {
    if (oldAntennaName.isEmpty() ||
        QRegExp(oldAntennaName).exactMatch(obsFile.antennaName())) {
      obsFile.setAntennaName(newAntennaName);
    }
  }

  QString oldAntennaNumber  = settings.value("reqcOldAntennaNumber").toString();
  QString newAntennaNumber  = settings.value("reqcNewAntennaNumber").toString();
  if (!newAntennaNumber.isEmpty()) {
    if (oldAntennaNumber.isEmpty() ||
        QRegExp(oldAntennaNumber).exactMatch(obsFile.antennaNumber())) {
      obsFile.setAntennaNumber(newAntennaNumber);
    }
  }

  const ColumnVector& obsFileAntNEU = obsFile.antNEU();
  QString oldAntennadN = settings.value("reqcOldAntennadN").toString();
  QString newAntennadN = settings.value("reqcNewAntennadN").toString();
  if(!newAntennadN.isEmpty()) {
    if (oldAntennadN.isEmpty() ||
        oldAntennadN.toDouble() == obsFileAntNEU(1)) {
      obsFile.setAntennaN(newAntennadN.toDouble());
    }
  }
  QString oldAntennadE = settings.value("reqcOldAntennadE").toString();
  QString newAntennadE = settings.value("reqcNewAntennadE").toString();
  if(!newAntennadE.isEmpty()) {
    if (oldAntennadE.isEmpty() ||
        oldAntennadE.toDouble() == obsFileAntNEU(2)) {
      obsFile.setAntennaE(newAntennadE.toDouble());
    }
  }
  QString oldAntennadU = settings.value("reqcOldAntennadU").toString();
  QString newAntennadU = settings.value("reqcNewAntennadU").toString();
  if(!newAntennadU.isEmpty()) {
    if (oldAntennadU.isEmpty() ||
        oldAntennadU.toDouble() == obsFileAntNEU(3)) {
      obsFile.setAntennaU(newAntennadU.toDouble());
    }
  }

  QString oldReceiverType = settings.value("reqcOldReceiverName").toString();
  QString newReceiverType = settings.value("reqcNewReceiverName").toString();
  if (!newReceiverType.isEmpty()) {
    if (oldReceiverType.isEmpty() ||
        QRegExp(oldReceiverType).exactMatch(obsFile.receiverType())) {
      obsFile.setReceiverType(newReceiverType);
    }
  }

  QString oldReceiverNumber = settings.value("reqcOldReceiverNumber").toString();
  QString newReceiverNumber = settings.value("reqcNewReceiverNumber").toString();
  if (!newReceiverNumber.isEmpty()) {
    if (oldReceiverNumber.isEmpty() ||
        QRegExp(oldReceiverNumber).exactMatch(obsFile.receiverNumber())) {
      obsFile.setReceiverNumber(newReceiverNumber);
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::rememberLLI(const t_rnxObsFile* obsFile,
                             const t_rnxObsFile::t_rnxEpo* epo) {

  if (_samplingRate == 0) {
    return;
  }

  for (unsigned iSat = 0; iSat < epo->rnxSat.size(); iSat++) {
    const t_rnxObsFile::t_rnxSat& rnxSat = epo->rnxSat[iSat];
    char    sys = rnxSat.prn.system();
    QString prn(rnxSat.prn.toString().c_str());

    for (int iType = 0; iType < obsFile->nTypes(sys); iType++) {
      QString type = obsFile->obsType(sys, iType);
      if (!_lli[prn].contains(iType)) {
        _lli[prn][iType] = 0;
      }
      if (rnxSat.obs.contains(type) && rnxSat.obs[type].lli & 1) {
        _lli[prn][iType] |= 1;
      }
    }
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::applyLLI(const t_rnxObsFile* obsFile,
                          t_rnxObsFile::t_rnxEpo* epo) {

 if (_samplingRate == 0) {
    return;
  }

  for (unsigned iSat = 0; iSat < epo->rnxSat.size(); iSat++) {
    t_rnxObsFile::t_rnxSat& rnxSat = epo->rnxSat[iSat];
    char    sys = rnxSat.prn.system();
    QString prn(rnxSat.prn.toString().c_str());

    for (int iType = 0; iType < obsFile->nTypes(sys); iType++) {
      QString type = obsFile->obsType(sys, iType);
      if (_lli[prn].contains(iType) && _lli[prn][iType] & 1) {
        if (rnxSat.obs.contains(type)) {
          rnxSat.obs[type].lli |= 1;
        }
      }
    }
  }

  _lli.clear();
}

/// Read All Ephemerides
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::readEphemerides(const QStringList& navFileNames,
                                 QVector<t_eph*>& ephs, QTextStream* log,
                                 bool checkEph) {

  QStringListIterator it(navFileNames);
  while (it.hasNext()) {
    QString fileName = it.next();
    if (fileName.indexOf('*') != -1 ||
        fileName.indexOf('?') != -1) {
      QFileInfo fileInfo(fileName);
      QDir dir = fileInfo.dir();
      QStringList filters; filters << fileInfo.fileName();
      QListIterator<QFileInfo> it(dir.entryInfoList(filters));
      while (it.hasNext()) {
        QString filePath = it.next().filePath();
        appendEphemerides(filePath, ephs, log, checkEph);
      }
    }
    else {
      appendEphemerides(fileName, ephs, log, checkEph);
    }
  }
  // TODO: enable user decision
  std::stable_sort(ephs.begin(), ephs.end(), t_eph::earlierTime);
  //std::stable_sort(ephs.begin(), ephs.end(), t_eph::prnSort);
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::editEphemerides() {

  // Easy Exit
  // ---------
  if (_navFileNames.isEmpty() || _outNavFileName.isEmpty()) {
    return;
  }
  // Concatenate all comments and all run by date lines
  // --------------------------------------------------
  QStringList comments;
  QStringList runByDate;
  bncSettings settings;
  QString comment = settings.value("reqcComment").toString();
  if (!comment.isEmpty()) {
    comments.append(comment);
  }
  QStringListIterator it(_navFileNames);
  while (it.hasNext()) {
    QString fileName = it.next();

    t_rnxNavFile rnxNavFile(fileName, t_rnxNavFile::input);
    QStringListIterator itCmnt(rnxNavFile.comments());
    while (itCmnt.hasNext()) {
      comments.append(itCmnt.next());
    }
    QStringListIterator itRunByDt(rnxNavFile.runByDate());
    while (itRunByDt.hasNext()) {
      runByDate.append(itRunByDt.next());
    }
  }
  comments.removeDuplicates();
  runByDate.removeDuplicates();

  // Read Ephemerides
  // ----------------
  t_reqcEdit::readEphemerides(_navFileNames, _ephs, _logStream, _checkEph);

  // Check Satellite Systems
  // -----------------------
  bool haveGPS     = false;
  bool haveGlonass = false;
  QMap<t_eph::e_system, bool> haveGnss;
  for (int ii = 0; ii < _ephs.size(); ii++) {
    const t_eph* eph = _ephs[ii];
    switch (eph->system()) {
      case t_eph::GPS:
        haveGPS = true;
        haveGnss[t_eph::GPS] = true;
        break;
      case t_eph::GLONASS:
        haveGlonass = true;
        haveGnss[t_eph::GLONASS] = true;
        break;
      case t_eph::Galileo:
        haveGnss[t_eph::Galileo] = true;
        break;
      case t_eph::BDS:
        haveGnss[t_eph::BDS] = true;
        break;
      case t_eph::QZSS:
        haveGnss[t_eph::QZSS] = true;
        break;
      case t_eph::NavIC:
        haveGnss[t_eph::NavIC] = true;
        break;
      case t_eph::SBAS:
        haveGnss[t_eph::SBAS] = true;
        break;
      default:
        haveGnss[t_eph::unknown] = true;
    }
  }

  // Initialize output navigation file
  // ---------------------------------
  t_rnxNavFile outNavFile(_outNavFileName, t_rnxNavFile::output);

  outNavFile.setGlonass(haveGlonass);

  if (_rnxVersion < 3.0) {
    if (haveGPS && haveGlonass) {
      outNavFile.setVersion(defaultRnxNavVersion3);
    }
    if (haveGPS && !haveGlonass) {
      outNavFile.setVersion(defaultRnxNavVersion2);
    }
    if (!haveGPS && haveGlonass) {
      outNavFile.setVersion(defaultRnxNavVersion2);
    }
  }

  if (_rnxVersion >= 3.0 &&  _rnxVersion < 4.0) {
    outNavFile.setVersion(defaultRnxNavVersion3);
  }

  if (_rnxVersion >= 4.0) {
    outNavFile.setVersion(defaultRnxNavVersion4);
  }

  if (outNavFile.version() > 3.0) {
    if (haveGnss.size() > 1) {
      outNavFile.setGnssTypeV3(t_eph::unknown);
    }
    else if (haveGnss.size() == 1){
      outNavFile.setGnssTypeV3(haveGnss.keys().first());
    }
  }

  QMap<QString, QString> txtMap;
  QString runBy = settings.value("reqcRunBy").toString();
  if (!runBy.isEmpty()) {
    txtMap["RUN BY"]  = runBy;
  }
  if (!runByDate.empty()) {
	  txtMap["RUN BY DATE"] = runByDate.join("\\n");
  }
  if (!comments.isEmpty()) {
    txtMap["COMMENT"]  = comments.join("\\n");
  }

  int mergedNavFiles = _navFileNames.size();
  unsigned year, month, day;
  int gps_utc = 0;
  if (_ephs.size()) {
    _ephs.at(0)->TOC().civil_date(year, month, day);
    gps_utc = gnumleap(year, month, day);
  }
  outNavFile.writeHeader(&txtMap, mergedNavFiles, gps_utc);

  // Loop over all ephemerides
  // -------------------------
  for (int ii = 0; ii < _ephs.size(); ii++) {
    const t_eph* eph = _ephs[ii];
    bncTime begTime = _begTime;
    bncTime endTime = _endTime;
    if (eph->system() == t_eph::BDS) {
      begTime += 14;
      endTime += 14;
    }
    if (begTime.valid() && eph->TOC() < begTime) {
      continue;
    }
    if (endTime.valid() && eph->TOC() > endTime) {
      break;
    }
    if (eph->checkState() == t_eph::bad) {
      continue;
    }

    if (outNavFile.version() < 3.0) {
  	  if (outNavFile.glonass() && eph->system() != t_eph::GLONASS) {
  	    continue;
  	  }
  	  if (!outNavFile.glonass() && eph->system() != t_eph::GPS) {
  	    continue;
      }
    }
    outNavFile.writeEph(eph);
  }
}

//
////////////////////////////////////////////////////////////////////////////
void t_reqcEdit::appendEphemerides(const QString& fileName,
                                   QVector<t_eph*>& ephs, QTextStream* log,
                                   bool checkEph) {
  t_rnxNavFile rnxNavFile(fileName, t_rnxNavFile::input);
  unsigned numOK  = 0;
  unsigned numBad = 0;
  unsigned numUnhealthy = 0;
  bncEphUser ephUser(false);

  if (log) {
    QFileInfo navFi(rnxNavFile.fileName());
      *log << "Input Nav File     : " << navFi.fileName()      << Qt::endl;
    if (checkEph) {
      *log << "RINEX Version      : " << rnxNavFile.version()  << Qt::endl;
    }
  }

  for (unsigned ii = 0; ii < rnxNavFile.ephs().size(); ii++) {
    t_eph* eph   = rnxNavFile.ephs()[ii];
    bool   isNew = true;
    for (int iOld = 0; iOld < ephs.size(); iOld++) {
      const t_eph* ephOld = ephs[iOld];
      if (ephOld->prn() == eph->prn() &&
          ephOld->TOC() == eph->TOC()) {
        isNew = false;
        break;
      }
    }

    if (isNew) {

      if (checkEph) {
        ephUser.putNewEph(eph, false);
        if      (eph->checkState() == t_eph::bad) {
          ++numBad;
        }
        else if (eph->checkState() == t_eph::unhealthy) {
          ++numUnhealthy;
        }
        else {
          ++numOK;
        }
        if (eph->checkState() == t_eph::bad) {
          continue;
        }
      }

      if      (eph->system() == t_eph::GPS) {
        ephs.append(new t_ephGPS(*dynamic_cast<t_ephGPS*>(eph)));
      }
      else if (eph->system() == t_eph::GLONASS) {
        ephs.append(new t_ephGlo(*dynamic_cast<t_ephGlo*>(eph)));
      }
      else if (eph->system() == t_eph::Galileo) {
        ephs.append(new t_ephGal(*dynamic_cast<t_ephGal*>(eph)));
      }
      else if (eph->system() == t_eph::QZSS) {
        ephs.append(new t_ephGPS(*dynamic_cast<t_ephGPS*>(eph)));
      }
      else if (eph->system() == t_eph::SBAS) {
        ephs.append(new t_ephSBAS(*dynamic_cast<t_ephSBAS*>(eph)));
      }
      else if (eph->system() == t_eph::BDS) {
        ephs.append(new t_ephBDS(*dynamic_cast<t_ephBDS*>(eph)));
      }
      else if (eph->system() == t_eph::NavIC) {
        ephs.append(new t_ephGPS(*dynamic_cast<t_ephGPS*>(eph)));
      }
    }
  }

  if (log && checkEph) {
    *log << "Ephemeris Check    : " << numOK        << " OK   "
                                    << numUnhealthy << " UNHEALTHY   "
                                    << numBad       << " WRONG\n";
    if (numBad > 0) {
      for (unsigned ii = 0; ii < rnxNavFile.ephs().size(); ii++) {
        t_eph* eph = rnxNavFile.ephs()[ii];
        QFileInfo navFi(fileName);
        if (eph->checkState() == t_eph::bad) {
          if (log) {
            *log << "                   : "
                 << QString("WRONG     %2:%3\n")
                         .arg(eph->typeStr(eph->type(), eph->prn(), 99.0))
                         .arg(eph->rinexDateStr(eph->TOC(), eph->prn(), 99.0)).toLatin1();
          }
        }/* switched off because of to many SBAS satellites are set to unhealthy
        if (eph->checkState() == t_eph::unhealthy) {
          if (log) {
            *log << "                   : "
                 << QString("UNHEALTHY %2:%3\n")
                         .arg(eph->typeStr(eph->type(), eph->prn(), 99.0))
                         .arg(eph->rinexDateStr(eph->TOC(), eph->prn(), 99.0)).toLatin1();
          }
        }*/
      }
    }
    *log << Qt::endl;
  }
}

void t_reqcEdit::addRnxConversionDetails(const t_rnxObsFile* obsFile,
                                          QMap<QString, QString>& txtMap) {

  int key = 0;
  QString systems = obsFile->header().usedSystems();
  QString comment = QString("Signal priorities for RINEX 3 => 2 conversion:");
  QString commentKey = QString("COMMENT %1").arg(key, 3, 10, QChar('0'));
  txtMap.insert(commentKey, comment);

  for(int ii = 0; ii < obsFile->numSys(); ii++) {
    char sys = systems[ii].toLatin1();
    txtMap.insert(commentKey, comment);
    QMap <char, QString>  signalPriorityMap;
    QStringList preferredAttribListSys = obsFile->signalPriorities(sys);
    QStringList types = obsFile->header().obsTypes(sys);
    for (int jj = 0; jj < types.size(); jj++) {
      QString inType = types[jj];
      char band = inType[1].toLatin1();
      for (int ii = 0; ii < preferredAttribListSys.size(); ii++) {
        QString preferredAttrib;
        if (preferredAttribListSys[ii].indexOf("&") != -1) {
          QStringList hlp = preferredAttribListSys[ii].split("&", Qt::SkipEmptyParts);
          if (hlp.size() == 2 && hlp[0].contains(band)) {
            preferredAttrib = hlp[1];
          }
        }
        else {
          preferredAttrib = preferredAttribListSys[ii];
        }
        if (!signalPriorityMap.contains(band) && !preferredAttrib.isEmpty()){
          signalPriorityMap[band] = preferredAttrib;
        }
      }
    }
    QMapIterator<char, QString> it(signalPriorityMap);
    while (it.hasNext()) {
        it.next();
        key++;
        comment = QString("%1 band %2: %3").arg(sys).arg(it.key()).arg(it.value());
        commentKey = QString("COMMENT %1").arg(key, 3, 10, QChar('0'));
        txtMap.insert(commentKey, comment);
    }
  }
}
