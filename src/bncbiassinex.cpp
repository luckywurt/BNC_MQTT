/* -------------------------------------------------------------------------
 * BNC
 * -------------------------------------------------------------------------
 *
 * Class:      bncBiasSinex
 *
 * Purpose:    writes SINEX Bias files
 *
 * Author:     A. Stuerze
 *
 * Created:    01-Mar-2022
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/



#include "bncbiassinex.h"

#include <math.h>
#include <iomanip>


using namespace BNC_PPP;
using namespace std;


// Constructor
////////////////////////////////////////////////////////////////////////////
bncBiasSinex::bncBiasSinex(const QString& sklFileName, const QString& intr,
    int sampl)
  : bncoutf(sklFileName, intr, sampl) {
  _sampl =  sampl;

  if (!_sampl) {
    _sampl = 5;
  }

  _agency = agencyFromFileName();
}


// Destructor
////////////////////////////////////////////////////////////////////////////
bncBiasSinex::~bncBiasSinex() {
  bncoutf::closeFile();
}


t_irc bncBiasSinex::write(int GPSweek, double GPSweeks, const QString& prn, const QString& obsCode, double bias) {

  if (reopen(GPSweek, GPSweeks) == success) {

      QDateTime datTimStart = dateAndTimeFromGPSweek(GPSweek, GPSweeks);

      int daysec    = int(fmod(GPSweeks, 86400.0));
      int dayOfYear = datTimStart.date().dayOfYear();
      QString yyyy  = datTimStart.toString("yyyy");
      QString timeStrStart  = QString("       %1:%2:%3").arg(yyyy)
                                           .arg(dayOfYear, 3, 10, QLatin1Char('0'))
                                           .arg(daysec   , 5, 10, QLatin1Char('0'));
      QString timeStrEnd = QString(" %1:%2:%3").arg(yyyy)
                                               .arg(dayOfYear, 3, 10, QLatin1Char('0'))
                                               .arg(daysec+_sampl , 5, 10, QLatin1Char('0'));
      QString biasStr =QString("%1").arg((bias * 1.e9 / t_CST::c), 21, 'f', 4, QLatin1Char(' '));
      _out << " OSB       " << prn.toLatin1().data()
           << "           " << obsCode.toLatin1().data()
           << timeStrStart.toLatin1().data()
           << timeStrEnd.toLatin1().data()
           << " ns   "
           << biasStr.toLatin1().data()
           << endl;

    return success;
  }
  else {
    return failure;
  }
  return success;
}

void bncBiasSinex::writeHeader(const QDateTime& datTim) {
  int    GPSWeek;
  double GPSWeeks;
  bncSettings settings;
  GPSweekFromDateAndTime(datTim, GPSWeek, GPSWeeks);
  int daysec    = int(fmod(GPSWeeks, 86400.0));
  int dayOfYear = datTim.date().dayOfYear();
  QString yy    = datTim.toString("yy");
  QString creationTime = QString("%1:%2:%3").arg(yy)
                                            .arg(dayOfYear, 3, 10, QLatin1Char('0'))
                                            .arg(daysec   , 5, 10, QLatin1Char('0'));
  QString startTime = creationTime;
  QString intStr = settings.value("uploadIntr").toString();
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
  int nominalEndSec = nominalStartSec  + intr - _sampl;
  QString endTime = QString("%1:%2:%3").arg(yy)
                                       .arg(dayOfYear     , 3, 10, QLatin1Char('0'))
                                       .arg(nominalEndSec , 5, 10, QLatin1Char('0'));
  int numEpochs = ((nominalEndSec - daysec) / _sampl) +1;
  QString epo  = QString("%1").arg(numEpochs, 5, 10, QLatin1Char('0'));

  _out << "%=BIA 1.00 " << _agency.toStdString() << " "
       << creationTime.toStdString() << " " << _agency.toStdString() << " "
       << startTime.toStdString()    << " " << endTime.toStdString() << " A "
       << epo.toStdString() << endl;

  _out << "+FILE/REFERENCE" << endl;
  _out << "*INFO_TYPE_________ INFO________________________________________________________" << endl;
  _out << " DESCRIPTION       " << " GNSS Satellite Code and Phase Biases from SSR stream  " << endl;
  _out << " INPUT             " << " SSR satellite biases provided by " << _agency.toStdString() <<endl;
  _out << " OUTPUT            " << " Absolute (observation-specific) bias parameters" << endl;
  _out << " SOFTWARE          " << " " << BNCPGMNAME << endl;
  _out << "-FILE/REFERENCE" << endl << endl;

  _out << "+BIAS/DESCRIPTION" << endl;
  _out << "*KEYWORD________________________________ VALUE(S)_______________________________"  << endl;
  _out << " PARAMETER_SAMPLING                     " << " " << left << setw(12) << _sampl << endl;
  _out << " DETERMINATION_METHOD                   " << " AC SPECIFIC" << endl;
  _out << " BIAS_MODE                              " << " ABSOLUTE" << endl;
  _out << " TIME_SYSTEM                            " << " G" << endl;
  _out << "-BIAS/DESCRIPTION" << endl << endl;

  _out << "+BIAS/SOLUTION" << endl;
  _out << "*BIAS SVN_ PRN STATION__ OBS1 OBS2 BIAS_START____ BIAS_END______ UNIT __ESTIMATED_VALUE____ _STD_DEV___ __ESTIMATED_SLOPE____ _STD_DEV__" << endl;
  return;
}





















