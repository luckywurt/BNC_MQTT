
/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncSinexTro
 *
 * Purpose:    writes SINEX TRO files
 *
 * Author:     A. Stürze
 *
 * Created:    19-Feb-2015
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <math.h>
#include <iomanip>

#include "bncsinextro.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncSinexTro::bncSinexTro(const t_pppOptions* opt,
                         const QString& sklFileName, const QString& intr,
                         int sampl)
  : bncoutf(sklFileName, intr, sampl) {

  _opt   = opt;
  _sampl = sampl;

  _antex = 0;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncSinexTro::~bncSinexTro() {
  closeFile();
  if (_antex)
    delete _antex;
}

// Write Header
////////////////////////////////////////////////////////////////////////////
void bncSinexTro::writeHeader(const QDateTime& datTim) {
  int    GPSWeek;
  double GPSWeeks;
  bncSettings settings;
  GPSweekFromDateAndTime(datTim, GPSWeek, GPSWeeks);
  int daysec    = int(fmod(GPSWeeks, 86400.0));
  int dayOfYear = datTim.date().dayOfYear();
  QString yyyy  = datTim.toString("yyyy");
  QString creationTime = QString("%1:%2:%3").arg(yyyy)
                                            .arg(dayOfYear, 3, 10, QLatin1Char('0'))
                                            .arg(daysec   , 5, 10, QLatin1Char('0'));
  QString startTime = creationTime;
  QString intStr = settings.value("PPP/snxtroIntr").toString();
  int intr, indHlp = 0;
  if      ((indHlp = intStr.indexOf("min")) != -1) {
    intr = intStr.left(indHlp-1).toInt();
    intr *= 60;
  }
  else if ((indHlp = intStr.indexOf("hour")) != -1) {
    intr = intStr.left(indHlp-1).toInt();
    intr *= 3600;
  }
  else if ((indHlp = intStr.indexOf("day")) != -1) {
    intr = intStr.left(indHlp-1).toInt();
    intr *= 86400;
  }
  int nominalStartSec = daysec - (int(fmod(double(daysec), double(intr))));
  int nominalEndSec = nominalStartSec + intr - _sampl;
  QString endTime = QString("%1:%2:%3").arg(yyyy)
                                       .arg(dayOfYear     , 3, 10, QLatin1Char('0'))
                                       .arg(nominalEndSec , 5, 10, QLatin1Char('0'));
  int numEpochs = ((nominalEndSec - daysec) / _sampl) +1;
  QString epo    = QString("%1").arg(numEpochs, 5, 10, QLatin1Char('0'));
  QString ac     = QString("%1").arg(settings.value("PPP/snxtroAc").toString(),3,QLatin1Char(' '));
  QString solId  = settings.value("PPP/snxtroSolId").toString();
  QString corr = "";
  if      (settings.value("PPP/dataSource").toString() == "Real-Time Streams") {
    corr = settings.value("PPP/corrMount").toString();
  }
  else if (settings.value("PPP/dataSource").toString() == "RINEX Files") {
    corr = settings.value("PPP/corrFile").toString();
  }
  QString signalPriorities = QString::fromStdString(_opt->_signalPriorities);
  if (!signalPriorities.size()) {
    signalPriorities = "G:12&CWPSLX R:12&CP E:1&CBX E:5&QIX C:26&IQX";
  }
  QStringList priorList = signalPriorities.split(" ", Qt::SkipEmptyParts);
  QStringList frqStrList;
  for (unsigned iFreq = 1; iFreq < t_frequency::max; iFreq++) {
    t_frequency::type frqType = static_cast<t_frequency::type>(iFreq);
    char frqSys = t_frequency::toString(frqType)[0];
    char frqNum = t_frequency::toString(frqType)[1];
    QStringList hlp;
    for (int ii = 0; ii < priorList.size(); ii++) {
      if (priorList[ii].indexOf(":") != -1) {
        hlp = priorList[ii].split(":", Qt::SkipEmptyParts);
        if (hlp.size() == 2 && hlp[0].length() == 1 && hlp[0][0] == frqSys) {
          hlp = hlp[1].split("&", Qt::SkipEmptyParts);
        }
        if (hlp.size() == 2 && hlp[0].indexOf(frqNum) != -1) {
          frqStrList.append(QString("%1%2").arg(frqSys).arg(frqNum));
        }
      }
    }
  }

  _out << "%=TRO 2.00 " << ac.toStdString() << " "
       << creationTime.toStdString() << " " << ac.toStdString() << " "
       << startTime.toStdString()    << " " << endTime.toStdString() << " P "
       << _opt->_roverName.substr(0,9) << endl;
  _out << "*-------------------------------------------------------------------------------" << endl;
  _out << "+FILE/REFERENCE" << endl;
  _out << "*INFO_TYPE_________ INFO________________________________________________________" << endl;
  _out << " DESCRIPTION        " << "BNC generated SINEX TRO file" << endl;
  _out << " OUTPUT             " << "Total Troposphere Zenith Path Delay Product" << endl;
  _out << " SOFTWARE           " <<  BNCPGMNAME <<  endl;
  _out << " INPUT              " << "OBS: "    << _opt->_roverName.substr(0,9) << ", SSR: " << corr.toStdString() << endl;
  _out << " VERSION NUMBER     " <<  QString("%1").arg(solId, 3, QLatin1Char('0')).toStdString() << endl;
  _out << "-FILE/REFERENCE" << endl;
  _out << "*-------------------------------------------------------------------------------" << endl;

  QString systems;
  if (settings.value("PPP/lcGPS"    ).toString() != "no") {systems += "G, ";}
  if (settings.value("PPP/lcGLONASS").toString() != "no") {systems += "R, ";}
  if (settings.value("PPP/lcGalileo").toString() != "no") {systems += "E, ";}
  if (settings.value("PPP/lcBDS"    ).toString() != "no") {systems += "C";}
  QString blqFileName = QString::fromStdString(_opt->_blqFileName);
  QString blqFileBaseName;
  QString blqFileExtension;
  if (! blqFileName.isEmpty()) {
    QFileInfo fileInfo(blqFileName);
    blqFileBaseName  = fileInfo.baseName();
    blqFileExtension = fileInfo.completeSuffix();
    if (!blqFileExtension.isEmpty()) {
      blqFileExtension = "." + blqFileExtension;
    }
  }
  bool eleWeighting = false;
  if (settings.value("PPP/eleWgtCode").toInt() > 0 ||
      settings.value("PPP/eleWgtCode").toInt() > 0 ) {
    eleWeighting = true;
  }

  _out << "+TROP/DESCRIPTION" << endl;
  _out << "*KEYWORD______________________ VALUE(S)______________" << endl;
  _out << " TROPO SAMPLING INTERVAL       " << left << setw(22) << _sampl << endl;
  _out << " DATA SAMPLING INTERVAL        " << left << setw(22) << 1 << endl;
  _out << " ELEVATION CUTOFF ANGLE        " << left << setw(22) <<  int(_opt->_minEle * 180.0/M_PI) << endl;
  _out << " OBSERVATION WEIGHTING         " << ((eleWeighting) ? "SINEL" : "") << endl;
  _out << " GNSS SYSTEMS                  " << systems.toStdString()  << endl;
  _out << " TIME SYSTEM                   " << "G"      << endl;
  _out << " TROPO MODELING METHOD         " << "KALMAN FILTER"  << endl;
  if (! blqFileName.isEmpty()) {
    _out << " OCEAN TIDE LOADING MODEL      " << blqFileBaseName.toStdString() + blqFileExtension.toStdString() << endl;
  }
  _out << " TROP MAPPING FUNCTION         " << "Saastamoinen" << endl;
  _out << " TROPO PARAMETER NAMES         " << "TROTOT STDEV" << endl;
  _out << " TROPO PARAMETER UNITS         " << "1e+03  1e+03" << endl;
  _out << "-TROP/DESCRIPTION"<< endl;
  _out << "*-------------------------------------------------------------------------------" << endl;

  double recEll[3];
  int lonD, lonM,  latD, latM;
  double lonS, latS;
  xyz2ell(_opt->_xyzAprRover.data(), recEll);
  deg2DMS(recEll[0] * 180.0 / M_PI, latD, latM, latS);
  deg2DMS(recEll[1] * 180.0 / M_PI, lonD, lonM, lonS);
  QString country;
  QListIterator<QString> it(settings.value("mountPoints").toStringList());
  while (it.hasNext()) {
    QStringList hlp = it.next().split(" ");
    if (hlp.size() < 7)
      continue;
    if (hlp.join(" ").indexOf(QString::fromStdString(_opt->_roverName), 0) != -1) {
      country = hlp[2];
    }
  }
  _out << "+SITE/ID" << endl;
  _out << "*STATION__ PT __DOMES__ T _STATION_DESCRIPTION__ _LONGITUDE _LATITUDE_ _HGT_ELI_" << endl;
  _out << " " << _opt->_roverName.substr(0,9) << "  A           P "
       << country.toStdString() << "                   "
       << QString(" %1").arg(recEll[0]* 180.0 / M_PI,10, 'f', 6, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg(recEll[1]* 180.0 / M_PI,10, 'f', 6, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg(recEll[2], 9, 'f', 3, QLatin1Char(' ')).toStdString()
       << endl;
  _out << "-SITE/ID" << endl;
  _out << "*-------------------------------------------------------------------------------" << endl;

  _out << "+TROP/COORDINATES" << endl;
  _out << "*STATION__ PT SOLN T __DATA_START__ __DATA_END____ __STA_X_____ __STA_Y_____ __STA_Z_____ SYSTEM REMRK" << endl;
  _out << " " << _opt->_roverName.substr(0,9) << "  A "
       << QString("%1").arg(solId, 4, QLatin1Char(' ')).toStdString() << " P "
       << startTime.toStdString() << " " << endTime.toStdString()
       << QString(" %1").arg(_opt->_xyzAprRover(1), 12, 'f', 3, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg(_opt->_xyzAprRover(2), 12, 'f', 3, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg(_opt->_xyzAprRover(3), 12, 'f', 3, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg("IGS20", 6, QLatin1Char(' ')).toStdString()
       << QString(" %1").arg(ac,      5, QLatin1Char(' ')).toStdString() << endl;
  _out << "-TROP/COORDINATES"<< endl;
  _out << "*-------------------------------------------------------------------------------" << endl;


  _out << "+SITE/ECCENTRICITY" << endl;
  _out << "*                                                     UP______ NORTH___ EAST____" << endl;
  _out << "*STATION__ PT SOLN T __DATA_START__ __DATA_END____ AXE MARKER->ARP(m)__________" << endl;
  _out << " " << _opt->_roverName.substr(0,9) << "  A "
       << QString("%1").arg(solId, 4, QLatin1Char(' ')).toStdString() << " P "
       << startTime.toStdString() << " " << endTime.toStdString()
       << QString(" %1").arg("UNE", 3, QLatin1Char(' ')).toStdString()
       << QString("%1").arg(_opt->_neuEccRover(3), 8, 'f', 4, QLatin1Char(' ')).toStdString()
       << QString("%1").arg(_opt->_neuEccRover(1), 8, 'f', 4, QLatin1Char(' ')).toStdString()
       << QString("%1").arg(_opt->_neuEccRover(2), 8, 'f', 4, QLatin1Char(' ')).toStdString() << endl;
  _out << "-SITE/ECCENTRICITY" << endl;
  _out << "*-------------------------------------------------------------------------------" << endl;

  if (!_opt->_recNameRover.empty()) {
    _out << "+SITE/RECEIVER" << endl;
    _out << "*STATION__ PT SOLN T __DATA_START__ __DATA_END____ DESCRIPTION_________ S/N_________________ FIRMW______" << endl;
    _out << " " << _opt->_roverName.substr(0,9) << "  A "
         << QString("%1").arg(solId, 4, QLatin1Char(' ')).toStdString() << " P "
         << startTime.toStdString() << " " << endTime.toStdString() << " "
         << left << std::setw(20) << _opt->_recNameRover
         << " --------------------" << " -----------" << endl;
    _out << "-SITE/RECEIVER" << endl;
    _out << "*-------------------------------------------------------------------------------" << endl;
  }
  if (!_opt->_antexFileName.empty()) {
    _antex = new bncAntex(_opt->_antexFileName.c_str());
    _out << "+SITE/ANTENNA" << endl;
    _out << "*STATION__ PT SOLN T __DATA_START__ __DATA_END____ DESCRIPTION_________ S/N_________________ PCV_MODEL_" << endl;
    _out << " " << _opt->_roverName.substr(0,9) << "  A "
         << QString("%1").arg(solId, 4, QLatin1Char(' ')).toStdString() << " P "
         << startTime.toStdString() << " " << endTime.toStdString()
         << QString(" %1").arg(_opt->_antNameRover.c_str(), 20,QLatin1Char(' ')).toStdString()
         << " --------------------" << _antex->snxCodeSinexString(_opt->_antNameRover).toStdString() << endl;
    _out << "-SITE/ANTENNA" << endl;
    _out << "*-------------------------------------------------------------------------------" << endl;
    delete _antex;
    _antex = 0;
}
  _out << "+TROP/SOLUTION" << endl;
  _out << "*STATION__ ____EPOCH_____ TROTOT STDDEV " << endl;
}

// Write One Epoch
////////////////////////////////////////////////////////////////////////////
t_irc bncSinexTro::write(QByteArray staID, int GPSWeek, double GPSWeeks,
    double trotot, double stdev) {

  QDateTime datTim = dateAndTimeFromGPSweek(GPSWeek, GPSWeeks);
  int daysec    = int(fmod(GPSWeeks, 86400.0));
  int dayOfYear = datTim.date().dayOfYear();
  QString yyyy    = datTim.toString("yyyy");
  QString time  = QString("%1:%2:%3").arg(yyyy)
                                     .arg(dayOfYear, 3, 10, QLatin1Char('0'))
                                     .arg(daysec   , 5, 10, QLatin1Char('0'));

  if ((reopen(GPSWeek, GPSWeeks) == success) &&
      (fmod(daysec, double(_sampl)) == 0.0)) {
    _out << ' '  << staID.left(9).data() << ' ' << time.toStdString()
         << QString(" %1").arg(trotot * 1000.0, 6, 'f', 1, QLatin1Char(' ')).toStdString()
         << QString(" %1").arg(stdev  * 1000.0, 6, 'f', 1, QLatin1Char(' ')).toStdString()  << endl;
    _out.flush();
    return success;
  }  else {
    return failure;
  }
}

// Close File (write last lines)
////////////////////////////////////////////////////////////////////////////
void bncSinexTro::closeFile() {
  _out << "-TROP/SOLUTION" << endl;
  _out << "%=ENDTROP" << endl;
  bncoutf::closeFile();
}




