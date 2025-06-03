/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncSettings
 *
 * Purpose:    Subclasses the QSettings
 *
 * Author:     L. Mervart
 *
 * Created:    25-Jan-2009
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <QSettings>

#include "bncsettings.h"
#include "bnccore.h"

QMutex bncSettings::_mutex;  // static mutex

// Constructor
////////////////////////////////////////////////////////////////////////////
bncSettings::bncSettings() {
  QMutexLocker locker(&_mutex);

  // First fill the options
  // ----------------------
  if (BNC_CORE->_settings.size() == 0) {
    reRead();
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncSettings::~bncSettings() {
}

// (Re-)read the Options from File or Set the Defaults
////////////////////////////////////////////////////////////////////////////
void bncSettings::reRead() {

  QSettings settings(BNC_CORE->confFileName(), QSettings::IniFormat);

  // Read from File
  // --------------
  if (settings.allKeys().size() > 0) {
    QStringListIterator it(settings.allKeys());
    while (it.hasNext()) {
      QString key = it.next();
      BNC_CORE->_settings[key] = settings.value(key);
    }
  }

  // Set Defaults
  // ------------
  else {
    setValue_p("startTab",            "0");
    setValue_p("statusTab",           "0");
    setValue_p("font",                "");
    setValue_p("casterUrlList", (QStringList()
                               << "http://user:pass@euref-ip.net:2101"
                               << "http://user:pass@igs-ip.net:2101"
                               << "http://user:pass@products.igs-ip.net:2101"
                               << "http://user:pass@mgex.igs-ip.net:2101"));
    setValue_p("mountPoints",         "");
    setValue_p("ntripVersion",       "2");
    // Network
    setValue_p("proxyHost",           "");
    setValue_p("proxyPort",           "");
    setValue_p("sslCaCertPath",       "");
    setValue_p("sslClientCertPath",   "");
    setValue_p("sslIgnoreErrors",    "0");
    // General
    setValue_p("logFile",             "");
    setValue_p("rnxAppend",          "0");
    setValue_p("onTheFlyInterval",  "no");
    setValue_p("autoStart",          "0");
    setValue_p("rawOutFile",          "");
    // RINEX Observations
    setValue_p("rnxPath",             "");
    setValue_p("rnxIntr",        "1 day");
    setValue_p("rnxOnlyWithSKL",      "");
    setValue_p("rnxSampl",       "1 sec");
    setValue_p("rnxSkel",          "skl");
    setValue_p("rnxSkelPath",         "");
    setValue_p("rnxV2Priority",       "");
    setValue_p("rnxScript",           "");
    setValue_p("rnxVersion",         "3");
    // RINEX Ephemeris
    setValue_p("ephPath",             "");
    setValue_p("ephIntr",        "1 day");
    setValue_p("ephOutPort",          "");
    setValue_p("ephVersion",         "3");
    // Reqc
    setValue_p("reqcAction",          "");
    setValue_p("reqcObsFile",         "");
    setValue_p("reqcNavFile",         "");
    setValue_p("reqcOutObsFile",      "");
    setValue_p("reqcOutNavFile",      "");
    setValue_p("reqcOutLogFile",      "");
    setValue_p("reqcSkyPlotSignals",  "G:1&2&5 R:1&2&3 E:1&7 C:2&6 J:1&2 I:5&9 S:1&5");
    setValue_p("reqcPlotDir",         "");
    setValue_p("reqcRnxVersion",      "");
    setValue_p("reqcSampling",        "1 sec");
    setValue_p("reqcStartDateTime",   "1967-11-02T00:00:00");
    setValue_p("reqcEndDateTime",     "2099-01-01T00:00:00");
    setValue_p("reqcLogSummaryOnly",  "");
    setValue_p("reqcRunBy",           "");
    setValue_p("reqcComment",         "");
    setValue_p("reqcOldMarkerName",   "");
    setValue_p("reqcNewMarkerName",   "");
    setValue_p("reqcOldAntennaName",  "");
    setValue_p("reqcNewAntennaName",  "");
    setValue_p("reqcOldReceiverName", "");
    setValue_p("reqcNewReceiverName", "");
    setValue_p("reqcOldAntennaNumber", "");
    setValue_p("reqcNewAntennaNumber","");
    setValue_p("reqcOldAntennadE",    "");
    setValue_p("reqcNewAntennadE",    "");
    setValue_p("reqcOldAntennadN",    "");
    setValue_p("reqcNewAntennadN",    "");
    setValue_p("reqcOldAntennadU",    "");
    setValue_p("reqcNewAntennadU",    "");
    setValue_p("reqcOldReceiverNumber", "");
    setValue_p("reqcNewReceiverNumber", "");
    setValue_p("reqcUseObsTypes",     "");
    setValue_p("reqcV2Priority",      "G:12&PWCSLX G:5&IQX R:12&PC R:3&IQX R:46&ABX E:16&BCXZ E:578&IQX J:1&SLXCZ J:26&SLX J:5&IQX C:267&IQX C:18&DPX I:ABCX S:1&C S:5&IQX");
    // SP3
    setValue_p("sp3CompFile",         "");
    setValue_p("sp3CompExclude",      "");
    setValue_p("sp3CompOutLogFile",   "");
    setValue_p("sp3CompSummaryOnly",  "");
    // Braodcast Corrections
    setValue_p("corrPath",            "");
    setValue_p("corrIntr",       "1 day");
    setValue_p("corrPort",            "");
    // Feed Engine
    setValue_p("outPort",             "");
    setValue_p("outWait",            "5");
    setValue_p("outSampl",       "1 sec");
    setValue_p("outFile",             "");
    setValue_p("outUPort",            "");
    setValue_p("outLockTime",        "0");
    // Serial Output
    setValue_p("serialMountPoint",    "");
    setValue_p("serialPortName",      "");
    setValue_p("serialBaudRate",  "9600");
    setValue_p("serialFlowControl",   "OFF");
    setValue_p("serialDataBits",     "8");
    setValue_p("serialParity",    "NONE");
    setValue_p("serialStopBits",     "1");
    setValue_p("serialAutoNMEA",  "Auto");
    setValue_p("serialFileNMEA",      "");
    setValue_p("serialHeightNMEA",    "");
    setValue_p("serialNMEASampling", "10");
    // Outages
    setValue_p("adviseObsRate",       "");
    setValue_p("adviseFail",        "15");
    setValue_p("adviseReco",         "5");
    setValue_p("adviseScript",        "");
    // Miscellaneous
    setValue_p("miscMount",           "");
    setValue_p("miscIntr",            "");
    setValue_p("miscScanRTCM",       "0");
    setValue_p("miscPort",            "");
    // Combination
    setValue_p("cmbStreams",          "");
    setValue_p("cmbMethod",           "");
    setValue_p("cmbMaxres",           "");
    setValue_p("cmbMaxdisplacement",  "");
    setValue_p("cmbSampl",          "10");
    setValue_p("cmbLogpath",          "");
    setValue_p("cmbGps",             "2");
    setValue_p("cmbGlo",             "2");
    setValue_p("cmbGal",             "2");
    setValue_p("cmbBds",             "2");
    setValue_p("cmbQzss",            "0");
    setValue_p("cmbSbas",            "0");
    setValue_p("cmbNavic",           "0");

    // Upload (clk)
    setValue_p("uploadMountpointsOut","");
    setValue_p("uploadIntr",     "1 day");
    setValue_p("uploadSamplRtcmEphCorr", "0");
    setValue_p("uploadSamplSp3",    "0 sec");
    setValue_p("uploadSamplClkRnx", "0");
    setValue_p("uploadSamplBiaSnx", "0");
    setValue_p("trafo_dx",            "");
    setValue_p("trafo_dy",            "");
    setValue_p("trafo_dz",            "");
    setValue_p("trafo_dxr",           "");
    setValue_p("trafo_dyr",           "");
    setValue_p("trafo_dzr",           "");
    setValue_p("trafo_ox",            "");
    setValue_p("trafo_oy",            "");
    setValue_p("trafo_oz",            "");
    setValue_p("trafo_oxr",           "");
    setValue_p("trafo_oyr",           "");
    setValue_p("trafo_ozr",           "");
    setValue_p("trafo_sc",            "");
    setValue_p("trafo_scr",           "");
    setValue_p("trafo_t0",            "");
    // Upload (eph)
    setValue_p("uploadEphMountpointsOut","");
    setValue_p("uploadSamplRtcmEph",     "5");
  }
}

//
////////////////////////////////////////////////////////////////////////////
QVariant bncSettings::value(const QString& key,
                            const QVariant& defaultValue) const {
  QMutexLocker locker(&_mutex);

  if (BNC_CORE->_settings.contains(key)) {
    return BNC_CORE->_settings[key];
  }
  else {
    return defaultValue;
  }
}

//
////////////////////////////////////////////////////////////////////////////
void bncSettings::setValue(const QString &key, const QVariant& value) {
  QMutexLocker locker(&_mutex);
  setValue_p(key, value);
}

//
////////////////////////////////////////////////////////////////////////////
void bncSettings::setValue_p(const QString &key, const QVariant& value) {
  BNC_CORE->_settings[key] = value;
}

//
////////////////////////////////////////////////////////////////////////////
void bncSettings::remove(const QString& key ) {
  QMutexLocker locker(&_mutex);
  BNC_CORE->_settings.remove(key);
}

//
////////////////////////////////////////////////////////////////////////////
bool bncSettings::contains(const QString& key) const {
  QMutexLocker locker(&_mutex);
  return BNC_CORE->_settings.contains(key);
}

//
////////////////////////////////////////////////////////////////////////////
void bncSettings::sync() {
  QMutexLocker locker(&_mutex);
  QSettings settings(BNC_CORE->confFileName(), QSettings::IniFormat);
  settings.clear();
  QMapIterator<QString, QVariant> it(BNC_CORE->_settings);

  while (it.hasNext()) {
    it.next();
    settings.setValue(it.key(), it.value());
  }

  settings.sync();
}
