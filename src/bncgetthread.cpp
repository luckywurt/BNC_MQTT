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
 * Class:      bncGetThread
 *
 * Purpose:    Thread that retrieves data from NTRIP caster
 *
 * Author:     L. Mervart
 *
 * Created:    24-Dec-2005
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>

#include "bncgetthread.h"
#include "bnctabledlg.h"
#include "bnccore.h"
#include "bncutils.h"
#include "bnctime.h"
#include "bnczerodecoder.h"
#include "bncnetqueryv0.h"
#include "bncnetqueryv1.h"
#include "bncnetqueryv2.h"
#include "bncnetqueryrtp.h"
#include "bncnetqueryudp.h"
#include "bncnetqueryudp0.h"
#include "bncnetquerys.h"
#include "bncsettings.h"
#include "bncwindow.h"
#include "latencychecker.h"
#include "upload/bncrtnetdecoder.h"
#include "RTCM/RTCM2Decoder.h"
#include "RTCM3/RTCM3Decoder.h"
#include "serial/qextserialport.h"

using namespace std;

// Constructor 1
////////////////////////////////////////////////////////////////////////////
bncGetThread::bncGetThread(bncRawFile* rawFile) {

  _rawFile      = rawFile;
  _format       = rawFile->format();
  _staID        = rawFile->staID();
  _rawOutput    = false;
  _ntripVersion = "N";

  initialize();
}

// Constructor 2
////////////////////////////////////////////////////////////////////////////
bncGetThread::bncGetThread(const QUrl& mountPoint, const QByteArray& format,
    const QByteArray& latitude, const QByteArray& longitude,
    const QByteArray& nmea, const QByteArray& ntripVersion) {
  _rawFile = 0;
  _mountPoint   = mountPoint;
  _staID        = mountPoint.path().mid(1).toLatin1();
  _format       = format;
  _latitude     = latitude;
  _longitude    = longitude;
  _nmea         = nmea;
  _ntripVersion = ntripVersion;

  bncSettings settings;
  if (!settings.value("rawOutFile").toString().isEmpty()) {
    _rawOutput = true;
  }
  else {
    _rawOutput = false;
  }

  _latencycheck = true; // in order to allow at least to check for reconnect

  _NMEASampl = settings.value("serialNMEASampling").toInt();

  initialize();
  initDecoder();
}

// Initialization (common part of the constructor)
////////////////////////////////////////////////////////////////////////////
void bncGetThread::initialize() {

  bncSettings settings;

  setTerminationEnabled(true);

  connect(this, SIGNAL(newMessage(QByteArray,bool)),
  BNC_CORE, SLOT(slotMessage(const QByteArray,bool)));

  _isToBeDeleted = false;
  _query = 0;
  _nextSleep = 0;
  _miscMount = settings.value("miscMount").toString();
  _decoder = 0;

  // NMEA Port
  // -----------
  QListIterator<QString> iSta(settings.value("PPP/staTable").toStringList());
  int nmeaPort = 0;
  while (iSta.hasNext()) {
    QStringList hlp = iSta.next().split(",");
    if (hlp.size() < 10) {
      continue;
    }
    QByteArray mp = hlp[0].toLatin1();
    if (_staID == mp) {
      nmeaPort = hlp[9].toInt();
    }
  }
  if (nmeaPort != 0) {
    _nmeaServer = new QTcpServer;
    _nmeaServer->setProxy(QNetworkProxy::NoProxy);
    if (!_nmeaServer->listen(QHostAddress::Any, nmeaPort)) {
      QString message = "bncCaster: Cannot listen on port "
                      + QByteArray::number(nmeaPort) + ": "
                      + _nmeaServer->errorString();
      emit newMessage(message.toLatin1(), true);
    } else {
      connect(_nmeaServer, SIGNAL(newConnection()), this,
          SLOT(slotNewNMEAConnection()));
      connect(BNC_CORE, SIGNAL(newNMEAstr(QByteArray, QByteArray)), this,
          SLOT(slotNewNMEAstr(QByteArray, QByteArray)));
      _nmeaSockets = new QList<QTcpSocket*>;
      _nmeaPortsMap[_staID] = nmeaPort;
    }
  } else {
    _nmeaServer = 0;
    _nmeaSockets = 0;
  }

  // Serial Port
  // -----------
  _serialNMEA = NO_NMEA;
  _serialOutFile = 0;
  _serialPort = 0;

  if (!_staID.isEmpty()
      && settings.value("serialMountPoint").toString() == _staID) {
    _serialPort = new QextSerialPort(
        settings.value("serialPortName").toString());
    _serialPort->setTimeout(0, 100);

    // Baud Rate
    // ---------
    QString hlp = settings.value("serialBaudRate").toString();
    if (hlp == "110") {
      _serialPort->setBaudRate(BAUD110);
    } else if (hlp == "300") {
      _serialPort->setBaudRate(BAUD300);
    } else if (hlp == "600") {
      _serialPort->setBaudRate(BAUD600);
    } else if (hlp == "1200") {
      _serialPort->setBaudRate(BAUD1200);
    } else if (hlp == "2400") {
      _serialPort->setBaudRate(BAUD2400);
    } else if (hlp == "4800") {
      _serialPort->setBaudRate(BAUD4800);
    } else if (hlp == "9600") {
      _serialPort->setBaudRate(BAUD9600);
    } else if (hlp == "19200") {
      _serialPort->setBaudRate(BAUD19200);
    } else if (hlp == "38400") {
      _serialPort->setBaudRate(BAUD38400);
    } else if (hlp == "57600") {
      _serialPort->setBaudRate(BAUD57600);
    } else if (hlp == "115200") {
      _serialPort->setBaudRate(BAUD115200);
    }

    // Parity
    // ------
    hlp = settings.value("serialParity").toString();
    if (hlp == "NONE") {
      _serialPort->setParity(PAR_NONE);
    } else if (hlp == "ODD") {
      _serialPort->setParity(PAR_ODD);
    } else if (hlp == "EVEN") {
      _serialPort->setParity(PAR_EVEN);
    } else if (hlp == "SPACE") {
      _serialPort->setParity(PAR_SPACE);
    }

    // Data Bits
    // ---------
    hlp = settings.value("serialDataBits").toString();
    if (hlp == "5") {
      _serialPort->setDataBits(DATA_5);
    } else if (hlp == "6") {
      _serialPort->setDataBits(DATA_6);
    } else if (hlp == "7") {
      _serialPort->setDataBits(DATA_7);
    } else if (hlp == "8") {
      _serialPort->setDataBits(DATA_8);
    }
    hlp = settings.value("serialStopBits").toString();
    if (hlp == "1") {
      _serialPort->setStopBits(STOP_1);
    } else if (hlp == "2") {
      _serialPort->setStopBits(STOP_2);
    }

    // Flow Control
    // ------------
    hlp = settings.value("serialFlowControl").toString();
    if (hlp == "XONXOFF") {
      _serialPort->setFlowControl(FLOW_XONXOFF);
    } else if (hlp == "HARDWARE") {
      _serialPort->setFlowControl(FLOW_HARDWARE);
    } else {
      _serialPort->setFlowControl(FLOW_OFF);
    }

    // Open Serial Port
    // ----------------
    _serialPort->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
    if (!_serialPort->isOpen()) {
      delete _serialPort;
      _serialPort = 0;
      emit(newMessage((_staID + ": Cannot open serial port\n"), true));
    }
    connect(_serialPort, SIGNAL(readyRead()), this,
        SLOT(slotSerialReadyRead()));

    // Automatic NMEA
    // --------------
    QString nmeaMode = settings.value("serialAutoNMEA").toString();
    if (nmeaMode == "Auto") {
      _serialNMEA = AUTO_NMEA;
      QString fName = settings.value("serialFileNMEA").toString();
      if (!fName.isEmpty()) {
        _serialOutFile = new QFile(fName);
        if (Qt::CheckState(settings.value("rnxAppend").toInt())
            == Qt::Checked) {
          _serialOutFile->open(QIODevice::WriteOnly | QIODevice::Append);
        } else {
          _serialOutFile->open(QIODevice::WriteOnly);
        }
      }
    }
    // Manual NMEA
    // -----------
    if ((nmeaMode == "Manual GPGGA") ||
        (nmeaMode == "Manual GNGGA")) {
      _serialNMEA = MANUAL_NMEA;
      bncSettings settings;
      QString hlp = settings.value("serialHeightNMEA").toString();
      if (hlp.isEmpty()) {
        hlp = "0.0";
      }
      QByteArray _serialHeightNMEA = hlp.toLatin1();
      _manualNMEAString = ggaString(_latitude, _longitude, _serialHeightNMEA, nmeaMode);
    }
  }

  if (!_staID.isEmpty() && _latencycheck) {
    _latencyChecker = new latencyChecker(_staID);
    _rtcmObs       = false;
    _rtcmSsrOrb    = false;
    _rtcmSsrClk    = false;
    _rtcmSsrOrbClk = false;
    _rtcmSsrCbi    = false;
    _rtcmSsrPbi    = false;
    _rtcmSsrVtec   = false;
    _rtcmSsrUra    = false;
    _rtcmSsrHr     = false;
    _rtcmSsrIgs    = false;
    _ssrEpoch      = 0;
  } else {
    _latencyChecker = 0;
  }
}

// Instantiate the decoder
//////////////////////////////////////////////////////////////////////////////
t_irc bncGetThread::initDecoder() {

  _decoder = 0;

  if (_format.indexOf("RTCM_2") != -1 ||
      _format.indexOf("RTCM2")  != -1 ||
      _format.indexOf("RTCM 2") != -1) {
    emit(newMessage(_staID + ": Get data in RTCM 2.x format", true));
    _decoder = new RTCM2Decoder(_staID.data());
  } else if (_format.indexOf("RTCM_3") != -1 ||
      _format.indexOf("RTCM3")  != -1 ||
      _format.indexOf("RTCM 3") != -1) {
    emit(newMessage(_staID + ": Get data in RTCM 3.x format", true));
    RTCM3Decoder* newDecoder = new RTCM3Decoder(_staID, _rawFile);
    _decoder = newDecoder;
    connect((RTCM3Decoder*) newDecoder, SIGNAL(newMessage(QByteArray,bool)),
        this, SIGNAL(newMessage(QByteArray,bool)));
  } else if (_format == "ZERO") {
    emit(newMessage(_staID + ": Forward data in original format", true));
    _decoder = new bncZeroDecoder(_staID, false);
  }
  else if (_format == "ZERO2FILE") {
    emit(newMessage(_staID + ": Get data in original format and store it", true));
    _decoder = new bncZeroDecoder(_staID, true);
  }
  else if (_format.indexOf("RTNET") != -1) {
    emit(newMessage(_staID + ": Get data in RTNet format", true));
    _decoder = new bncRtnetDecoder();
  } else {
    emit(newMessage(_staID + ": Unknown data format " + _format, true));
    _isToBeDeleted = true;
    return failure;
  }

  msleep(100); //sleep 0.1 sec

  _decoder->initRinex(_staID, _mountPoint, _latitude, _longitude, _nmea,
      _ntripVersion);

  if (_rawFile) {
    _decodersRaw[_staID] = _decoder;
  }

  return success;
}

// Current decoder in use
////////////////////////////////////////////////////////////////////////////
GPSDecoder* bncGetThread::decoder() {
  if (!_rawFile) {
    return _decoder;
  } else {
    if (_decodersRaw.contains(_staID) || initDecoder() == success) {
      return _decodersRaw[_staID];
    }
  }
  return 0;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncGetThread::~bncGetThread() {
  if (isRunning()) {
    wait();
  }
  if (_query) {
    _query->stop();
    _query->deleteLater();
  }
  if (_rawFile) {
    QMapIterator<QString, GPSDecoder*> it(_decodersRaw);
    while (it.hasNext()) {
      it.next();
      delete it.value();
    }
    _decodersRaw.clear();
  } else {
    delete _decoder;
  }
  delete _rawFile;
  delete _serialOutFile;
  delete _serialPort;
  delete _latencyChecker;
  emit getThreadFinished(_staID);
}

//
////////////////////////////////////////////////////////////////////////////
void bncGetThread::terminate() {
  _isToBeDeleted = true;

  if (_nmeaPortsMap.contains(_staID)) {
    _nmeaPortsMap.remove(_staID);
  }
  if (_nmeaServer) {
    delete _nmeaServer;
  }
  if (_nmeaSockets) {
    delete _nmeaSockets;
  }

#ifdef BNC_DEBUG
  if (BNC_CORE->mode() != t_bncCore::interactive) {
    while (!isFinished()) {
      wait();
    }
    delete this;
  } else {
    if (!isRunning()) {
      delete this;
    }
  }
#else
  if (!isRunning()) {delete this;}
#endif

}

// Run
////////////////////////////////////////////////////////////////////////////
void bncGetThread::run() {

  while (true) {
    try {
      if (_isToBeDeleted) {
        emit(newMessage(_staID + ": is to be deleted", true));
        QThread::exit(4);
        this->deleteLater();
        return;
      }

      if (tryReconnect() != success) {
        if (_latencyChecker) {
          _latencyChecker->checkReconnect();
        }
        continue;
      }

      // Delete old observations
      // -----------------------
      if (_rawFile) {
        QMapIterator<QString, GPSDecoder*> itDec(_decodersRaw);
        while (itDec.hasNext()) {
          itDec.next();
          GPSDecoder* decoder = itDec.value();
          decoder->_obsList.clear();
        }
      } else {
        _decoder->_obsList.clear();
      }

      // Read Data
      // ---------
      QByteArray data;
      if (_query) {
        _query->waitForReadyRead(data);
      } else if (_rawFile) {
        data = _rawFile->readChunk();
        _format = _rawFile->format();
        _staID = _rawFile->staID();

        QCoreApplication::processEvents();

        if (data.isEmpty() || BNC_CORE->sigintReceived) {
          emit(newMessage("No more data or SIGINT/SIGTERM received", true));
          BNC_CORE->stopPPP();
          BNC_CORE->stopCombination();
          sleep(2);
          ::exit(5);
        }
      }

      qint64 nBytes = data.size();

      // Timeout, reconnect
      // ------------------
      if (nBytes == 0) {
        if (_latencyChecker) {
          _latencyChecker->checkReconnect();
        }
        emit(newMessage(_staID + ": Data timeout, reconnecting", true));
        msleep(10000); //sleep 10 sec, G. Weber
        // MQTT消息新增
        emit sigStaTimeout(_staID);
        continue;
      } else {
        emit newBytes(_staID, nBytes);
        emit newRawData(_staID, data);
        // MQTT消息新增
        emit sigUpdateThroughput(_staID, int(nBytes));
      }

      // Output Data
      // -----------
      if (_rawOutput) {
        BNC_CORE->writeRawData(data, _staID, _format);
      }

      if (_serialPort) {
        slotSerialReadyRead();
        _serialPort->write(data);
      }

      // Decode Data
      // -----------
      vector<string> errmsg;
      if (!decoder()) {
        _isToBeDeleted = true;
        continue;
      }

      t_irc irc = decoder()->Decode(data.data(), data.size(), errmsg);

      if (irc != success) {
        continue;
      }
      // Perform various scans and checks
      // --------------------------------
      if (_latencyChecker) {
        _latencyChecker->checkOutage(irc);
        QListIterator<int> it(decoder()->_typeList);
        _ssrEpoch = static_cast<int>(decoder()->corrGPSEpochTime());
        if (_ssrEpoch != -1) {
          if (_rtcmSsrOrb) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1057);
            _rtcmSsrOrb = false;
          }
          if (_rtcmSsrClk) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1058);
            _rtcmSsrClk = false;
          }
          if (_rtcmSsrOrbClk) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1060);
            _rtcmSsrOrbClk = false;
          }
          if (_rtcmSsrCbi) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1059);
            _rtcmSsrCbi = false;
          }
          if (_rtcmSsrPbi) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1265);
            _rtcmSsrPbi = false;
          }
          if (_rtcmSsrVtec) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1264);
            _rtcmSsrVtec = false;
          }
          if (_rtcmSsrUra) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1061);
            _rtcmSsrUra = false;
          }
          if (_rtcmSsrHr) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 1062);
            _rtcmSsrHr = false;
          }
          if (_rtcmSsrIgs) {
            _latencyChecker->checkCorrLatency(_ssrEpoch, 4076);
            _rtcmSsrIgs = false;
          }
        }
        while (it.hasNext()) {
          int rtcmType = it.next();
          if ((rtcmType >= 1001 && rtcmType <= 1004) || // legacy RTCM OBS
              (rtcmType >= 1009 && rtcmType <= 1012) || // legacy RTCM OBS
              (rtcmType >= 1070 && rtcmType <= 1137)) { // MSM RTCM OBS
            _rtcmObs = true;
          } else if ((rtcmType >= 1057 && rtcmType <= 1068) ||
                     (rtcmType >= 1240 && rtcmType <= 1270) ||
					           (rtcmType == 4076)) {
            switch (rtcmType) {
              case 1057: case 1063: case 1240: case 1246: case 1252: case 1258:
                _rtcmSsrOrb = true;
                break;
              case 1058: case 1064: case 1241: case 1247: case 1253: case 1259:
                _rtcmSsrClk = true;
                break;
              case 1060: case 1066: case 1243: case 1249: case 1255: case 1261:
                _rtcmSsrOrbClk = true;
                break;
              case 1059: case 1065: case 1242: case 1248: case 1254: case 1260:
                _rtcmSsrCbi = true;
                break;
              case 1265: case 1266: case 1267: case 1268: case 1269: case 1270:
                _rtcmSsrPbi = true;
                break;
              case 1264:
                _rtcmSsrVtec = true;
                break;
              case 1061: case 1067: case 1244: case 1250: case 1256: case 1262:
                _rtcmSsrUra = true;
                break;
              case 1062: case 1068: case 1245: case 1251: case 1257: case 1263:
                _rtcmSsrHr = true;
                break;
              case 4076:
            	_rtcmSsrIgs = true;
            	break;
            }
          }
        }
        if (_rtcmObs) {
          _latencyChecker->checkObsLatency(decoder()->_obsList);
        }
        emit newLatency(_staID, _latencyChecker->currentLatency());
        // MQTT消息新增
        emit sigUpdateLatency(_staID, _latencyChecker->currentLatency());
      }
      miscScanRTCM();

      // Loop over all observations (observations output)
      // ------------------------------------------------
      QListIterator<t_satObs> it(decoder()->_obsList);

      QList<t_satObs> obsListHlp;

      while (it.hasNext()) {
        const t_satObs& obs = it.next();

        // Check observation epoch
        // -----------------------
        if (!_rawFile) {
          bool wrongObservationEpoch = checkForWrongObsEpoch(obs._time);
          if (wrongObservationEpoch) {
            QString prn(obs._prn.toString().c_str());
            QString type = QString("%1").arg(obs._type);
            emit(newMessage(_staID + " (" + prn.toLatin1() + ")" + ": Wrong observation epoch(s)" + "( MT: " + type.toLatin1() + ")", false));
            continue;
          }
        }

        // Check observations coming twice (e.g. KOUR0 Problem)
        // ----------------------------------------------------
        if (!_rawFile) {
          QString prn(obs._prn.toString().c_str());
          bncTime obsTime = obs._time;
          QMap<QString, bncTime>::const_iterator it = _prnLastEpo.find(prn);
          if (it != _prnLastEpo.end()) {
            bncTime oldTime = it.value();
            if (obsTime < oldTime) {
              emit(newMessage(_staID + ": old observation " + prn.toLatin1(), false));
              continue;
            } else if (obsTime == oldTime) {
              emit(newMessage(_staID + ": observation coming more than once " + prn.toLatin1(), false));
              continue;
            }
          }
          _prnLastEpo[prn] = obsTime;
        }

        decoder()->dumpRinexEpoch(obs, _format);

        // Save observations
        // -----------------
        obsListHlp.append(obs);
      }

      // Emit signal
      // -----------
      if (!_isToBeDeleted && obsListHlp.size() > 0) {
        emit newObs(_staID, obsListHlp);
      }

    }
    catch (Exception& exc) {
      emit(newMessage(_staID + " " + exc.what(), true));
      // MQTT消息新增
      emit sigStaError(_staID, QString(exc.what()));
      _isToBeDeleted = true;
    }
    catch (std::exception& exc) {
      emit(newMessage(_staID + " " + exc.what(), true));
      // MQTT消息新增
      emit sigStaError(_staID, QString(exc.what()));
      _isToBeDeleted = true;
    }
    catch (const string& error) {
      emit(newMessage(_staID + " ERROR: " + error.c_str(), true));
      // MQTT消息新增
      emit sigStaError(_staID, QString(error.c_str()));
      _isToBeDeleted = true;
    }
    catch (const char* error) {
      emit(newMessage(_staID + " ERROR: " + error, true));
      // MQTT消息新增
      emit sigStaError(_staID, QString(error));
      _isToBeDeleted = true;
    }
    catch (QString error) {
      emit(newMessage(_staID + " ERROR: " + error.toStdString().c_str(), true));
      // MQTT消息新增
      emit sigStaError(_staID, error);
      _isToBeDeleted = true;
    }
    catch (...) {
      emit(newMessage(_staID + " bncGetThread: unknown exception", true));
      // MQTT消息新增
      emit sigStaError(_staID, QString("bncGetThread: unknown exception"));
      _isToBeDeleted = true;
    }
  }
}

// Try Re-Connect
////////////////////////////////////////////////////////////////////////////
t_irc bncGetThread::tryReconnect() {

  // Easy Return
  // -----------
  if (_query && _query->status() == bncNetQuery::running) {
    _nextSleep = 0;
    if (_rawFile) {
      QMapIterator<QString, GPSDecoder*> itDec(_decodersRaw);
      while (itDec.hasNext()) {
        itDec.next();
        GPSDecoder* decoder = itDec.value();
        decoder->setRinexReconnectFlag(false);
      }
    } else {
      _decoder->setRinexReconnectFlag(false);
    }
    return success;
  }

  // Start a new query
  // -----------------
  if (!_rawFile) {

    sleep(_nextSleep);
    if (_nextSleep == 0) {
      _nextSleep = 1;
    } else {
      _nextSleep = 2 * _nextSleep;
      if (_nextSleep > 256) {
        _nextSleep = 256;
      }
#ifdef MLS_SOFTWARE
      if (_nextSleep > 4) {
        _nextSleep = 4;
      }
#endif
    }
    delete _query;
    if (_ntripVersion == "U") {
      _query = new bncNetQueryUdp();
    } else if (_ntripVersion == "R") {
      _query = new bncNetQueryRtp();
    } else if (_ntripVersion == "S") {
      _query = new bncNetQueryS();
    } else if (_ntripVersion == "N") {
      _query = new bncNetQueryV0();
    } else if (_ntripVersion == "UN") {
      _query = new bncNetQueryUdp0();
    } else if (_ntripVersion == "2") {
      _query = new bncNetQueryV2(false);
    } else if (_ntripVersion == "2s") {
      _query = new bncNetQueryV2(true);
    } else {
      _query = new bncNetQueryV1();
    }
    if (_nmea == "yes") {
      if (_serialNMEA == MANUAL_NMEA) {
        _query->startRequest(_mountPoint, _manualNMEAString);
        _lastNMEA = QDateTime::currentDateTime();
      } else if (_serialNMEA == AUTO_NMEA) {
        if (_serialPort) {
          int nb = _serialPort->bytesAvailable();
          if (nb > 0) {
            QByteArray data = _serialPort->read(nb);
            int i1 = data.indexOf("$GPGGA");
            if (i1 == -1) {
              i1 = data.indexOf("$GNGGA");
            }
            if (i1 != -1) {
              int i2 = data.indexOf("*", i1);
              if (i2 != -1 && data.size() > i2 + 1) {
                QByteArray gga = data.mid(i1, i2 - i1 + 3);
                _query->startRequest(_mountPoint, gga);
                _lastNMEA = QDateTime::currentDateTime();
              }
            }
          }
        }
      }
    } else {
      _query->startRequest(_mountPoint, "");
    }

    if (_query->status() != bncNetQuery::running) {
      // MQTT消息新增
      emit sigStaDisconnected(_staID);
      return failure;
    }
  }

  if (_rawFile) {
    QMapIterator<QString, GPSDecoder*> itDec(_decodersRaw);
    while (itDec.hasNext()) {
      itDec.next();
      GPSDecoder* decoder = itDec.value();
      decoder->setRinexReconnectFlag(false);
    }
  } else {
    _decoder->setRinexReconnectFlag(false);
  }

  return success;
}

// RTCM scan output
//////////////////////////////////////////////////////////////////////////////
void bncGetThread::miscScanRTCM() {

  if (!decoder()) {
    return;
  }

  bncSettings settings;
  if (Qt::CheckState(settings.value("miscScanRTCM").toInt()) == Qt::Checked) {

    if (_miscMount == _staID || _miscMount == "ALL") {
      // RTCM message types
      // ------------------
      for (int ii = 0; ii < decoder()->_typeList.size(); ii++) {
        QString type = QString("%1 ").arg(decoder()->_typeList[ii]);
        emit(newMessage(_staID + ": Received message type " + type.toLatin1(), true));
      }

      // Check Observation Types
      // -----------------------
      for (int ii = 0; ii < decoder()->_obsList.size(); ii++) {
        t_satObs& obs = decoder()->_obsList[ii];
        QVector<QString>& rnxTypes = _rnxTypes[obs._prn.system()];
        bool allFound = true;
        for (unsigned iFrq = 0; iFrq < obs._obs.size(); iFrq++) {
          if (obs._obs[iFrq]->_codeValid) {
            QString rnxStr('C');
            rnxStr.append(obs._obs[iFrq]->_rnxType2ch.c_str());
            if (_format.indexOf("RTCM_2") != -1
                || _format.indexOf("RTCM2") != -1
                || _format.indexOf("RTCM 2") != -1) {
              rnxStr = t_rnxObsFile::type3to2(obs._prn.system(), rnxStr);
            }
            if (rnxTypes.indexOf(rnxStr) == -1) {
              rnxTypes.push_back(rnxStr);
              allFound = false;
            }
          }
          if (obs._obs[iFrq]->_phaseValid) {
            QString rnxStr('L');
            rnxStr.append(obs._obs[iFrq]->_rnxType2ch.c_str());
            if (_format.indexOf("RTCM_2") != -1
                || _format.indexOf("RTCM2") != -1
                || _format.indexOf("RTCM 2") != -1) {
              rnxStr = t_rnxObsFile::type3to2(obs._prn.system(), rnxStr);
            }
            if (rnxTypes.indexOf(rnxStr) == -1) {
              rnxTypes.push_back(rnxStr);
              allFound = false;
            }
          }
          if (obs._obs[iFrq]->_dopplerValid) {
            QString rnxStr('D');
            rnxStr.append(obs._obs[iFrq]->_rnxType2ch.c_str());
            if (_format.indexOf("RTCM_2") != -1
                || _format.indexOf("RTCM2") != -1
                || _format.indexOf("RTCM 2") != -1) {
              rnxStr = t_rnxObsFile::type3to2(obs._prn.system(), rnxStr);
            }
            if (rnxTypes.indexOf(rnxStr) == -1) {
              rnxTypes.push_back(rnxStr);
              allFound = false;
            }
          }
          if (obs._obs[iFrq]->_snrValid) {
            QString rnxStr('S');
            rnxStr.append(obs._obs[iFrq]->_rnxType2ch.c_str());
            if (_format.indexOf("RTCM_2") != -1
                || _format.indexOf("RTCM2") != -1
                || _format.indexOf("RTCM 2") != -1) {
              rnxStr = t_rnxObsFile::type3to2(obs._prn.system(), rnxStr);
            }
            if (rnxTypes.indexOf(rnxStr) == -1) {
              rnxTypes.push_back(rnxStr);
              allFound = false;
            }
          }
        }
        if (!allFound) {
          QString msg;
          QTextStream str(&msg);
          str << obs._prn.system() << QString("    %1  ").arg(rnxTypes.size());
          for (int iType = 0; iType < rnxTypes.size(); iType++) {
            str << " " << rnxTypes[iType];
          }
          emit(newMessage(_staID + ": Observation Types: " + msg.toLatin1(),  true));
        }
      }

      // RTCMv3 antenna descriptor
      // -------------------------
      for (int ii = 0; ii < decoder()->_antType.size(); ii++) {
        QString ant1 = QString(": Antenna Descriptor: %1 ").arg(decoder()->_antType[ii].descriptor);
        emit(newMessage(_staID + ant1.toLatin1(), true));
        if (strlen(decoder()->_antType[ii].serialnumber)) {
          QString ant2 = QString(": Antenna Serial Number: %1 ").arg(decoder()->_antType[ii].serialnumber);
          emit(newMessage(_staID + ant2.toLatin1(), true));
        }
      }

      // RTCM Antenna Coordinates
      // ------------------------
      for (int ii = 0; ii < decoder()->_antList.size(); ii++) {
        QByteArray antT;
        if (decoder()->_antList[ii].type == GPSDecoder::t_antRefPoint::ARP) {
          antT = "ARP";
        } else if (decoder()->_antList[ii].type == GPSDecoder::t_antRefPoint::APC) {
          antT = "APC";
        }
        QByteArray ant1, ant2, ant3;
        ant1 = QString("%1 ").arg(decoder()->_antList[ii].xx, 0, 'f', 4).toLatin1();
        ant2 = QString("%1 ").arg(decoder()->_antList[ii].yy, 0, 'f', 4).toLatin1();
        ant3 = QString("%1 ").arg(decoder()->_antList[ii].zz, 0, 'f', 4).toLatin1();
        emit(newMessage(_staID + ": " + antT + " (ITRF) X " + ant1 + "m", true));
        emit(newMessage(_staID + ": " + antT + " (ITRF) Y " + ant2 + "m", true));
        emit(newMessage(_staID + ": " + antT + " (ITRF) Z " + ant3 + "m", true));
        double hh = 0.0;
        if (decoder()->_antList[ii].height_f) {
          hh = decoder()->_antList[ii].height;
          QByteArray ant4 = QString("%1 ").arg(hh, 0, 'f', 4).toLatin1();
          emit(newMessage(
              _staID + ": Antenna height above marker " + ant4 + "m", true));
        }
        emit(newAntCrd(_staID, decoder()->_antList[ii].xx,
            decoder()->_antList[ii].yy, decoder()->_antList[ii].zz, hh, antT));
      }

      // RTCMv3 receiver descriptor
      // --------------------------
      for (int ii = 0; ii < decoder()->_recType.size(); ii++) {
        QString rec1 = QString(": Receiver Descriptor: %1 ").arg(decoder()->_recType[ii].descriptor);
        QString rec2 = QString(": Receiver Firmware Version: %1 ").arg(decoder()->_recType[ii].firmware);
        QString rec3 = QString(": Receiver Serial Number: %1 ").arg(decoder()->_recType[ii].serialnumber);
        emit(newMessage(_staID + rec1.toLatin1(), true));
        emit(newMessage(_staID + rec2.toLatin1(), true));
        emit(newMessage(_staID + rec3.toLatin1(), true));
      }

      // RTCM GLONASS slots
      // ------------------
      if (decoder()->_gloFrq.size()) {
        bool allFound = true;
        QString slot = decoder()->_gloFrq;
        slot.replace("  ", " ").replace(" ", ":");
        if (_gloSlots.indexOf(slot) == -1) {
          _gloSlots.append(slot);
          allFound = false;
        }
        if (!allFound) {
          _gloSlots.sort();
          emit(newMessage(
              _staID + ": GLONASS Slot:Freq " + _gloSlots.join(" ").toLatin1(),
              true));
        }
      }
      if (fmod(decoder()->corrGPSEpochTime(), 60.0) == 0.0) {
        // Service CRS
        // -----------
        for (int ii = 0; ii < decoder()->_serviceCrs.size(); ii++) {
          QString servicecrsname  = QString(": Service CRS Name: %1 ").arg(decoder()->_serviceCrs[ii]._name);
          QString coordinateEpoch = QString(": Service CRS Coordinate Epoch: %1 ").arg(decoder()->_serviceCrs[ii]._coordinateEpoch);
          //QString ce = QString(": CE: %1 ").arg(decoder()->_serviceCrs[ii]._CE);
          emit(newMessage(_staID + servicecrsname.toLatin1(), true));
          emit(newMessage(_staID + coordinateEpoch.toLatin1(), true));
          //emit(newMessage(_staID + ce.toLatin1(), true));
        }

        // RTCM CRS
        // -----------
        for (int ii = 0; ii < decoder()->_rtcmCrs.size(); ii++) {
          QString rtcmcrsname = QString(": RTCM CRS Name: %1 ").arg(decoder()->_rtcmCrs[ii]._name);
          QString anchor      = QString(": RTCM CRS Anchor: %1 ").arg(decoder()->_rtcmCrs[ii]._anchor);
          QString platenumber = QString(": RTCM CRS Plate Number: %1 ").arg(decoder()->_rtcmCrs[ii]._plateNumber);
          emit(newMessage(_staID + rtcmcrsname.toLatin1(), true));
          emit(newMessage(_staID + anchor.toLatin1(), true));
          emit(newMessage(_staID + platenumber.toLatin1(), true));
          for (int i = 0; i<decoder()->_rtcmCrs[ii]._databaseLinks.size(); i++) {
            QString dblink = QString(": Database Link: %1 ").arg(decoder()->_rtcmCrs[ii]._databaseLinks[i]);
            emit(newMessage(_staID + dblink.toLatin1(), true));
          }
        }

        // Helmert Parameters
        //-------------------
        for (int ii = 0; ii < decoder()->_helmertPar.size(); ii++) {
          t_helmertPar& helmertPar = decoder()->_helmertPar[ii];
          bncTime t; t.setmjd(0, helmertPar._mjd); QString dateStr = QString::fromStdString(t.datestr());
          QString sourcename  = QString(": MT1301 Source Name: %1 ").arg(helmertPar._sourceName);
          QString targetname  = QString(": MT1301 Target Name: %1 ").arg(helmertPar._targetName);
          QString sysidentnum = QString(": MT1301 Sys Ident Num: %1 ").arg(helmertPar._sysIdentNum);
          QString trafomessageind = QString(": MT1301 Trafo Ident Num: %1 ").arg(helmertPar.IndtoString());
          QString epoch = QString(": MT1301 t0: MJD %1 (%2) ").arg(helmertPar._mjd).arg(dateStr);
          QString partrans = QString(": MT1301 Helmert Par Trans: dx = %1, dy = %2, dz = %3, dxr = %4, dyr = %5, dzr = %6")
              .arg(helmertPar._dx).arg(helmertPar._dy).arg(helmertPar._dz)
              .arg(helmertPar._dxr).arg(helmertPar._dyr).arg(helmertPar._dzr);
          QString parrot = QString(": MT1301 Helmert Par Rot: ox = %1, oy = %2, oz = %3, oxr = %4, oyr = %5, ozr = %6")
              .arg(helmertPar._ox).arg(helmertPar._oy).arg(helmertPar._oz)
              .arg(helmertPar._oxr).arg(helmertPar._oyr).arg(helmertPar._ozr);
          QString parscale = QString(": MT1301 Helmert Par Scale: sc = %1, scr = %2").arg(helmertPar._sc).arg(helmertPar._scr);
          emit(newMessage(_staID + sourcename.toLatin1(), true));
          emit(newMessage(_staID + targetname.toLatin1(), true));
          emit(newMessage(_staID + sysidentnum.toLatin1(), true));
          emit(newMessage(_staID + trafomessageind.toLatin1(), true));
          emit(newMessage(_staID + epoch.toLatin1(), true));
          emit(newMessage(_staID + partrans.toLatin1(), true));
          emit(newMessage(_staID + parrot.toLatin1(), true));
          emit(newMessage(_staID + parscale.toLatin1(), true));
        }
      }
    }
  }

#ifdef MLS_SOFTWARE
  for (int ii=0; ii <decoder()->_antList.size(); ii++) {
    QByteArray antT;
    if (decoder()->_antList[ii].type == GPSDecoder::t_antInfo::ARP) {
      antT = "ARP";
    }
    else if (decoder()->_antList[ii].type == GPSDecoder::t_antInfo::APC) {
      antT = "APC";
    }
    double hh = 0.0;
    if (decoder()->_antList[ii].height_f) {
      hh = decoder()->_antList[ii].height;
    }
    emit(newAntCrd(_staID, decoder()->_antList[ii].xx,
            decoder()->_antList[ii].yy, decoder()->_antList[ii].zz,
            hh, antT));
  }

  for (int ii = 0; ii <decoder()->_typeList.size(); ii++) {
    emit(newRTCMMessage(_staID, decoder()->_typeList[ii]));
  }
#endif

  decoder()->_gloFrq.clear();
  decoder()->_typeList.clear();
  decoder()->_antType.clear();
  decoder()->_recType.clear();
  decoder()->_antList.clear();
}

// Handle Data from Serial Port
////////////////////////////////////////////////////////////////////////////
void bncGetThread::slotSerialReadyRead() {

  if (_serialPort) {

    if (_nmea == "yes" && _serialNMEA == MANUAL_NMEA) {
      if (_NMEASampl) {
        int dt = _lastNMEA.secsTo(QDateTime::currentDateTime());
        if (dt && (fmod(double(dt), double(_NMEASampl)) == 0.0)) {
          _query->sendNMEA(_manualNMEAString);
          _lastNMEA = QDateTime::currentDateTime();
        }
      }
    }

    int nb = _serialPort->bytesAvailable();
    if (nb > 0) {
      QByteArray data = _serialPort->read(nb);

      if (_nmea == "yes" && _serialNMEA == AUTO_NMEA) {
        int i1 = data.indexOf("$GPGGA");
        if (i1 == -1) {
          i1 = data.indexOf("$GNGGA");
        }
        if (i1 != -1) {
          int i2 = data.indexOf("*", i1);
          if (i2 != -1 && data.size() > i2 + 1) {
            QByteArray gga = data.mid(i1, i2 - i1 + 3);
            if (_NMEASampl) {
              int dt = _lastNMEA.secsTo(QDateTime::currentDateTime());
              if (dt && (fmod(double(dt), double(_NMEASampl)) == 0.0)) {
                _query->sendNMEA(gga);
                _lastNMEA = QDateTime::currentDateTime();
              }
            }
          }
        }
      }

      if (_serialOutFile) {
        _serialOutFile->write(data);
        _serialOutFile->flush();
      }
    }
  }
}

void bncGetThread::slotNewNMEAConnection() {
  _nmeaSockets->push_back(_nmeaServer->nextPendingConnection());
  emit(newMessage(
      QString("New PPP client on port: # %1").arg(_nmeaSockets->size()).toLatin1(),
      true));
}

//
////////////////////////////////////////////////////////////////////////////
void bncGetThread::slotNewNMEAstr(QByteArray staID, QByteArray str) {
  if (_nmeaPortsMap.contains(staID)) {
    int nmeaPort = _nmeaPortsMap.value(staID);
    QMutableListIterator<QTcpSocket*> is(*_nmeaSockets);
    while (is.hasNext()) {
      QTcpSocket* sock = is.next();
      if (sock->localPort() == nmeaPort) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
          sock->write(str);
        } else if (sock->state() != QAbstractSocket::ConnectingState) {
          delete sock;
          is.remove();
        }
      }
    }
  }
}
