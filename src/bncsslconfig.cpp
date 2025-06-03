/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncSslConfig
 *
 * Purpose:    Singleton Class that inherits QSslConfiguration class
 *
 * Author:     L. Mervart
 *
 * Created:    22-Aug-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iostream>

#include <QApplication>
#include <QDir>
#include <QStringList>

#include "bncsslconfig.h"
#include "bncsettings.h"


// Singleton
////////////////////////////////////////////////////////////////////////////
bncSslConfig bncSslConfig::instance() {
  static bncSslConfig _sslConfig;
  return _sslConfig;
}

// Constructor
////////////////////////////////////////////////////////////////////////////
bncSslConfig::bncSslConfig() :
  QSslConfiguration(QSslConfiguration::defaultConfiguration()) {

  bncSettings settings;
  QString dirName = settings.value("sslCaCertPath").toString();
  if (dirName.isEmpty()) {
    dirName =  defaultPath();
  }

  QList<QSslCertificate> caCerts = this->caCertificates();

  QDir dir(dirName);
  QStringList nameFilters;
  nameFilters << "*.crt";
  nameFilters << "*.pem";
  QStringList fileNames = dir.entryList(nameFilters, QDir::Files);
  QStringListIterator it(fileNames);
  while (it.hasNext()) {
    QString fileName = it.next();
    caCerts += QSslCertificate::fromPath(dirName+QDir::separator()+fileName);
  }

  this->setCaCertificates(caCerts);


}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncSslConfig::~bncSslConfig() {
}

// Destructor
////////////////////////////////////////////////////////////////////////////
QString bncSslConfig::defaultPath() {
  return "/etc/ssl/certs/";
}

