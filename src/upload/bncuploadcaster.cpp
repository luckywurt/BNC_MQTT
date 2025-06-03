/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncUploadCaster
 *
 * Purpose:    Connection to NTRIP Caster
 *
 * Author:     L. Mervart
 *
 * Created:    29-Mar-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <math.h>
#include "bncuploadcaster.h"
#include "bncversion.h"
#include "bnccore.h"
#include "bnctableitem.h"
#include "bncsettings.h"
#include "bncsslconfig.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncUploadCaster::bncUploadCaster(const QString &mountpoint,
    const QString &outHost, int outPort, const QString &ntripVersion,
    const QString &userName, const QString &password, int iRow, int rate) {
  bncSettings settings;

  _mountpoint = mountpoint;
  _casterOutHost = outHost;
  _casterOutPort = outPort;
  _ntripVersion = ntripVersion;
  _userName = userName;
  _password = password;
  _outSocket = 0;
  _sOpenTrial = 0;
  _iRow = iRow;
  _rate = rate;

  if (_rate < 0) {
    _rate = 0;
  } else if (_rate > 60) {
    _rate = 60;
  }
  _isToBeDeleted = false;

  connect(this, SIGNAL(newMessage(QByteArray,bool)), BNC_CORE, SLOT(slotMessage(const QByteArray,bool)));

  if (BNC_CORE->_uploadTableItems.find(_iRow) != BNC_CORE->_uploadTableItems.end()) {
    connect(this, SIGNAL(newBytes(QByteArray,double)),
            BNC_CORE->_uploadTableItems.value(iRow),
            SLOT(slotNewBytes(const QByteArray,double)));
  }
  if (BNC_CORE->_uploadEphTableItems.find(_iRow) != BNC_CORE->_uploadEphTableItems.end()) {
    connect(this, SIGNAL(newBytes(QByteArray,double)),
            BNC_CORE->_uploadEphTableItems.value(iRow),
            SLOT(slotNewBytes(const QByteArray,double)));
  }

  _sslIgnoreErrors = (Qt::CheckState(settings.value("sslIgnoreErrors").toInt()) == Qt::Checked);

  _proxyOutHost = settings.value("proxyHost").toString();
  _proxyOutPort = settings.value("proxyPort").toInt();
  (_proxyOutHost.isEmpty()) ? _proxy = false : _proxy = true;

  _secure = false;
  if (_ntripVersion == "2s") {
    if (!QSslSocket::supportsSsl()) {
      emit(newMessage(
          "For SSL support please install OpenSSL run-time libraries: Ntrip Version 2 is tried",
          true));
      _ntripVersion == "2";
    } else {
      _secure = true;
      _casterOutPort = 443;
      // Generate filenames to consider a potential client certificate and private key
      _crtFileName = settings.value("sslClientCertPath").toString() + _casterOutHost + QString(".%1.crt").arg(_casterOutPort);
      _keyFileName = settings.value("sslClientCertPath").toString() + _casterOutHost + QString(".%1.key").arg(_casterOutPort);
    }
  }

  if (!_secure && _proxy) {
    _postExtension = QString("http://%1:%2").arg(_casterOutHost).arg(_casterOutPort);
  } else {
    _postExtension = "";
  }
}

// Safe Desctructor
////////////////////////////////////////////////////////////////////////////
void bncUploadCaster::deleteSafely() {
  _isToBeDeleted = true;
  if (!isRunning()) {
    delete this;
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncUploadCaster::~bncUploadCaster() {
  if (isRunning()) {
    wait();
  }
  if (_outSocket) {
    delete _outSocket;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void bncUploadCaster::slotProxyAuthenticationRequired(const QNetworkProxy&,
    QAuthenticator*) {
  emit newMessage("slotProxyAuthenticationRequired", true);
}

// TSL/SSL
 ////////////////////////////////////////////////////////////////////////////
void bncUploadCaster::slotSslErrors(QList<QSslError> errors) {
  QString msg = "SSL Error: ";
  if (_outSocket) {
    QSslCertificate cert = _outSocket->sslConfiguration().peerCertificate();
    if (!cert.isNull() &&
         cert.issuerInfo(QSslCertificate::OrganizationalUnitName).count() &&
         cert.issuerInfo(QSslCertificate::Organization).count()) {
      msg += QString("Server Certificate Issued by:\n" "%1\n%2\nCannot be verified\n")
#if QT_VERSION >= 0x050000
            .arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName).at(0))
            .arg(cert.issuerInfo(QSslCertificate::Organization).at(0));
#else
            .arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName))
            .arg(cert.issuerInfo(QSslCertificate::Organization));
#endif
    }

    QListIterator<QSslError> it(errors);
    while (it.hasNext()) {
      const QSslError& err = it.next();
      msg += err.errorString();
    }
    emit(newMessage(msg.toLatin1(), true));
  }
}


// Endless Loop
////////////////////////////////////////////////////////////////////////////
void bncUploadCaster::run() {
  while (true) {
    if (_isToBeDeleted) {
      QThread::quit();
      deleteLater();
      return;
    }
    open();
    if (_outSocket && _outSocket->state() == QAbstractSocket::ConnectedState) {
      QMutexLocker locker(&_mutex);
      if (_outBuffer.size() > 0) {
        if (_ntripVersion == "1") {
          _outSocket->write(_outBuffer);
          _outSocket->flush();
        } else {
          QString chunkSize = QString("%1").arg(_outBuffer.size(), 0, 16,  QLatin1Char('0'));
          QByteArray chunkedData = chunkSize.toLatin1() + "\r\n" + _outBuffer  + "\r\n";
          _outSocket->write(chunkedData);
          _outSocket->flush();
        }
        emit newBytes(_mountpoint.toLatin1(), _outBuffer.size());
      }
    }
    if (_rate == 0) {
      {
        QMutexLocker locker(&_mutex);
        _outBuffer.clear();
      }
      msleep(100); //sleep 0.1 sec
    } else {
      sleep(_rate);
    }
  }
}

// Start the Communication with NTRIP Caster
////////////////////////////////////////////////////////////////////////////
void bncUploadCaster::open() {
  const int timeOut = 5000;  // 5 seconds
  QByteArray msg;

  if (_mountpoint.isEmpty()) {
    return;
  }

  if (_outSocket != 0 &&
      _outSocket->state() == QAbstractSocket::ConnectedState) {
    return;
  }

  delete _outSocket; _outSocket = 0;

  double minDt = pow(2.0, _sOpenTrial);
  if (++_sOpenTrial > 4) {
    _sOpenTrial = 4;
  }
  if (_outSocketOpenTime.isValid()
      && _outSocketOpenTime.secsTo(QDateTime::currentDateTime()) < minDt) {
    return;
  } else {
    _outSocketOpenTime = QDateTime::currentDateTime();
  }

  _outSocket = new QSslSocket();
  _outSocket->setProxy(QNetworkProxy::NoProxy);

  if (_sslIgnoreErrors) {
    _outSocket->ignoreSslErrors();
  } else {
    bncSslConfig sslConfig = BNC_SSL_CONFIG;
    QFile clientCrtFile(_crtFileName);
    QFile privateKeyFile(_keyFileName);
    if ( clientCrtFile.exists() && privateKeyFile.exists()) {
      // set local certificate
      clientCrtFile.open(QIODevice::ReadOnly);
      QSslCertificate clientCrt(&clientCrtFile);
      sslConfig.setLocalCertificate(clientCrt);
      // set private key if available
      privateKeyFile.open(QIODevice::ReadOnly);
      QSslKey privateKey(&privateKeyFile, QSsl::Rsa);
      sslConfig.setPrivateKey(privateKey);
    }
    _outSocket->setSslConfiguration(sslConfig);
    connect(_outSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(slotSslErrors(QList<QSslError>)));
  }

  if (!_proxy) {
    if (!connectToHost(_casterOutHost, _casterOutPort, _secure)) {
      return;
    }
  } else {
    if (_ntripVersion == "1") {
      emit(newMessage("No proxy support in Ntrip Version 1 upload!", true));
      delete _outSocket; _outSocket = 0;
      return;
    }
    connect(_outSocket, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
            this,SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    if (!connectToHost(_proxyOutHost, _proxyOutPort, false)) {
      return;
    }

    if (_secure) {
      msg = "CONNECT " + _casterOutHost.toLatin1() + ":"
          + QString("%1").arg(_casterOutPort).toLatin1() + " HTTP/1.1\r\n"
          + "Proxy-Connection: Keep-Alive\r\n"
          + "Host: " + _casterOutHost.toLatin1() + "\r\n"
          + "User-Agent: NTRIP BNC/" BNCVERSION " (" + BNC_OS + ")\r\n"
          + "\r\n";
      _outSocket->write(msg);
      _outSocket->waitForBytesWritten();
      _outSocket->waitForReadyRead();

      QByteArray ans = _outSocket->readAll();
      if (ans.indexOf("200") == -1) {
        int l = ans.indexOf("\r\n", 0);
        emit(newMessage("Proxy: Connection broken for " + _mountpoint.toLatin1() + "@" +
            _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1()  + ": "  + ans.left(l), true));
        delete _outSocket; _outSocket = 0;
        return;
      } else {
        emit(newMessage("Proxy: Connection established for " + _mountpoint.toLatin1()+ "@" +
            _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1() , true));
        _sOpenTrial = 0;
        _outSocket->setPeerVerifyName(_casterOutHost);
        _outSocket->startClientEncryption();
        if (!_outSocket->waitForEncrypted(timeOut)) {
          emit(newMessage("Proxy/Caster: Encrypt timeout for " + _mountpoint.toLatin1() + "@"
                          + _casterOutHost.toLatin1() + ":"
                          + QString("%1) ").arg(_casterOutPort).toLatin1()
                          + _outSocket->errorString().toLatin1(), true));
          delete _outSocket; _outSocket = 0;
          return;
        } else {
          emit(newMessage("Proxy: SSL handshake completed for " + _mountpoint.toLatin1() + "@" +
              _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1(), true));
        }
      }
    }
  }

  if (_ntripVersion == "1") {
    msg = "SOURCE " + _password.toLatin1() + " /" + _mountpoint.toLatin1()
        + "\r\n" + "Source-Agent: NTRIP BNC/" BNCVERSION "\r\n\r\n";
    _outSocket->write(msg);
    _outSocket->waitForBytesWritten();
    _outSocket->waitForReadyRead();
  } else {
    msg = "POST " + _postExtension.toLatin1() + "/" + _mountpoint.toLatin1()
        + " HTTP/1.1\r\n" + "Host: " + _casterOutHost.toLatin1() + "\r\n"
        + "Ntrip-Version: Ntrip/2.0\r\n" + "Authorization: Basic "
        + (_userName + ":" + _password).toLatin1().toBase64() + "\r\n"
        + "User-Agent: NTRIP BNC/" BNCVERSION " (" + BNC_OS + ")\r\n"
        + "Connection: close\r\n" + "Transfer-Encoding: chunked\r\n\r\n";
    _outSocket->write(msg);
    _outSocket->waitForBytesWritten();
    _outSocket->waitForReadyRead();
  }

   QByteArray ans = _outSocket->readAll();

  if (ans.indexOf("200") == -1) {
    delete _outSocket; _outSocket = 0;
    int l = ans.indexOf("\r\n", 0);
    emit(newMessage("Broadcaster: Connection broken for " + _mountpoint.toLatin1() + "@" +
         _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1() +
         ": " + ans.left(l), true));
  } else {
    emit(newMessage("Broadcaster: Connection opened for " + _mountpoint.toLatin1() + "@" +
        _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1() , true));
    _sOpenTrial = 0;
  }
}

// Try connection to NTRIP Caster or Proxy
////////////////////////////////////////////////////////////////////////////
bool bncUploadCaster::connectToHost(QString outHost, int outPort, bool encrypted) {
  const int timeOut = 5000;  // 5 seconds
  if (encrypted) {
    _outSocket->connectToHostEncrypted(outHost, outPort);
    if (!_outSocket->waitForEncrypted(timeOut)) {
      emit(newMessage(
           "Broadcaster: Connect timeout for " + _mountpoint.toLatin1() + "@"
              + outHost.toLatin1() + ":"
              + QString("%1) ").arg(outPort).toLatin1()
              + _outSocket->errorString().toLatin1(), true));
      delete _outSocket; _outSocket = 0;
      return false;
    } else {
      emit(newMessage("Broadcaster: SSL handshake completed for " + _mountpoint.toLatin1() + "@" +
          _casterOutHost.toLatin1() + ":" + QString("%1").arg(_casterOutPort).toLatin1(), true));
    }
  } else {
    _outSocket->connectToHost(outHost, outPort);
    if (!_outSocket->waitForConnected(timeOut)) {
      emit(newMessage("Broadcaster: Connect timeout for " + _mountpoint.toLatin1() + "@"
              + outHost.toLatin1() + ":"
              + QString("%1) ").arg(outPort).toLatin1()
              + _outSocket->errorString().toLatin1(), true));
      delete _outSocket; _outSocket = 0;
      return false;
    }
  }
  return true;
}


