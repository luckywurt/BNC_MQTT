/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncNetQueryV2
 *
 * Purpose:    Blocking Network Requests (NTRIP Version 2)
 *
 * Author:     L. Mervart
 *
 * Created:    27-Dec-2008
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>

#include "bncnetqueryv2.h"
#include "bncsettings.h"
#include "bncversion.h"
#include "bncsslconfig.h"
#include "bncsettings.h"

// Constructor
////////////////////////////////////////////////////////////////////////////
bncNetQueryV2::bncNetQueryV2(bool secure) {
  _secure    = secure;
  _manager   = new QNetworkAccessManager(this);
  connect(_manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
          this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
  _reply     = 0;
  _eventLoop = new QEventLoop(this);
  _firstData = true;
  _status    = init;

  bncSettings settings;
  _sslIgnoreErrors = (Qt::CheckState(settings.value("sslIgnoreErrors").toInt()) == Qt::Checked);

  if (_secure ) {
    if (!QSslSocket::supportsSsl()) {
      BNC_CORE->slotMessage("No SSL support, install OpenSSL run-time libraries", true);
      stop();
    }
    // Generate filenames to consider a potential client certificate
    _crtFileName = settings.value("sslClientCertPath").toString() + _url.host() + QString(".%1.crt").arg(_url.port());
    _keyFileName = settings.value("sslClientCertPath").toString() + _url.host() + QString(".%1.key").arg(_url.port());
  }

}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncNetQueryV2::~bncNetQueryV2() {
  delete _eventLoop;
  if (_reply) {
    _reply->abort();
    delete _reply;
  }
  delete _manager;
}

// Stop (quit event loop)
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::stop() {
  if (_reply) {
    _reply->abort();
    delete _reply;
    _reply = 0;
  }
  _eventLoop->quit();
  _status = finished;
}

// End of Request
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::slotFinished() {
  _eventLoop->quit();
  if (_reply && _reply->error() != QNetworkReply::NoError) {
    _status = error;
    if (!_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray().isEmpty()) {
    emit newMessage(_url.path().toLatin1().replace(0,1,"")  +
                    ": NetQueryV2: server replied: " +
                    _reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray(),
                    true);
    } else {
      emit newMessage(_url.path().toLatin1().replace(0,1,"")  +
                         ": NetQueryV2: server replied: " +
                         _reply->errorString().toLatin1(),
                         true);
    }
  }
  else {
    _status = finished;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::slotProxyAuthenticationRequired(const QNetworkProxy&,
                                                    QAuthenticator*) {
  emit newMessage("slotProxyAuthenticationRequired", true);
}

// Start request, block till the next read
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::startRequest(const QUrl& url, const QByteArray& gga) {
  startRequestPrivate(url, gga, false);
}

// Start request, block till the next read
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::keepAliveRequest(const QUrl& url, const QByteArray& gga) {
  startRequestPrivate(url, gga, false);
}

// Start Request (Private Method)
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::startRequestPrivate(const QUrl& url, const QByteArray& gga,
                                        bool full) {

  _status = running;

  // Default scheme and path
  // -----------------------
  _url = url;
  if (_url.scheme().isEmpty()) {
    if (_secure) {
      _url.setScheme("https");
    }
    else {
      _url.setScheme("http");
    }
  }
  if (_url.path().isEmpty()) {
    _url.setPath("/");
  }

  // Network Request
  // ---------------
  bncSslConfig sslConfig = BNC_SSL_CONFIG;

  if (_secure) {
    QFile clientCrtFile(_crtFileName);
    QFile privateKeyFile(_keyFileName);
    if ( clientCrtFile.exists() && privateKeyFile.exists()) {
      // set local certificate if available
      clientCrtFile.open(QIODevice::ReadOnly);
      QSslCertificate clientCrt(&clientCrtFile);
      sslConfig.setLocalCertificate(clientCrt);
      // set private key if available
      privateKeyFile.open(QIODevice::ReadOnly);
      QSslKey privateKey(&privateKeyFile, QSsl::Rsa);
      sslConfig.setPrivateKey(privateKey);
    }
  }

  QNetworkRequest request;
  request.setSslConfiguration(sslConfig);
  request.setUrl(_url);
  request.setRawHeader("Host"         , _url.host().toLatin1());
  request.setRawHeader("Ntrip-Version", "Ntrip/2.0");
  request.setRawHeader("User-Agent"   , "NTRIP BNC/" BNCVERSION " (" BNC_OS ")");
  if (!_url.userName().isEmpty()) {
    QString uName = QUrl::fromPercentEncoding(_url.userName().toLatin1());
    QString passW = QUrl::fromPercentEncoding(_url.password().toLatin1());
    request.setRawHeader("Authorization", "Basic " +
                         (uName + ":" + passW).toLatin1().toBase64());
  }
  if (!gga.isEmpty()) {
    request.setRawHeader("Ntrip-GGA", gga);
  }
  request.setRawHeader("Connection"   , "close");

  if (_reply) {
    delete _reply;
    _reply = 0;
  }
  _reply = _manager->get(request);

  // Connect Signals
  // ---------------
  connect(_reply, SIGNAL(finished()), this, SLOT(slotFinished()));
  connect(_reply, SIGNAL(finished()), _eventLoop, SLOT(quit()));
  connect(_reply, SIGNAL(sslErrors(QList<QSslError>)),this, SLOT(slotSslErrors(QList<QSslError>)));
  if (!full) {
    connect(_reply, SIGNAL(readyRead()), _eventLoop, SLOT(quit()));
  }
}

// Start Request, wait for its completion
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::waitForRequestResult(const QUrl& url, QByteArray& outData) {

  // Send Request
  // ------------
  startRequestPrivate(url, "", true);

  // Wait Loop
  // ---------
  _eventLoop->exec();

  // Copy Data and Return
  // --------------------
  if (_reply) {
    outData = _reply->readAll();
  }
}

// Wait for next data
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::waitForReadyRead(QByteArray& outData) {

  // Wait Loop
  // ---------
  if (!_reply->bytesAvailable()) {
    _eventLoop->exec();
  }
  if (!_reply) {
    return;
  }

  // Check NTRIPv2 error code
  // ------------------------
  if (_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
    _reply->abort();
  }

  // Append Data
  // -----------
  else {
    outData.append(_reply->readAll());
  }
}

// TSL/SSL
////////////////////////////////////////////////////////////////////////////
void bncNetQueryV2::slotSslErrors(QList<QSslError> errors) {

  QString msg = "SSL Error: ";
  QSslCertificate cert = _reply->sslConfiguration().peerCertificate();
  if (!cert.isNull() &&
      cert.issuerInfo(QSslCertificate::OrganizationalUnitName).count() &&
      cert.issuerInfo(QSslCertificate::Organization).count()) {

    msg += QString("Server Certificate Issued by:\n"
                   "%1\n%2\nCannot be verified\n")
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

  if (_sslIgnoreErrors) {
    _reply->ignoreSslErrors();
    BNC_CORE->slotMessage("BNC ignores SSL errors as configured", true);
  }
  else {
    BNC_CORE->slotMessage(msg.toLatin1(), true);
    stop();
  }
  return;
}
