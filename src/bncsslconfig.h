#ifndef BNCSSLCONFIG_H
#define BNCSSLCONFIG_H

#include <QSslConfiguration>

// Singleton Class
// ---------------
class bncSslConfig : public QSslConfiguration {
 public:
  bncSslConfig();
  ~bncSslConfig();
  static bncSslConfig instance();
  static QString defaultPath();
 private:
};

#define BNC_SSL_CONFIG (bncSslConfig::instance())

#endif
