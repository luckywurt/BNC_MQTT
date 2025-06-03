
/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncClockRinex
 *
 * Purpose:    writes RINEX Clock files
 *
 * Author:     L. Mervart
 *
 * Created:    29-Mar-2011
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <math.h>
#include <iomanip>

#include "bncclockrinex.h"
#include "bncsettings.h"
#include "bncversion.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncClockRinex::bncClockRinex(const QString& sklFileName, const QString& intr,
                             int sampl)
  : bncoutf(sklFileName, intr, sampl) {
  bncSettings settings;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncClockRinex::~bncClockRinex() {
  bncoutf::closeFile();
}

// Write One Epoch
////////////////////////////////////////////////////////////////////////////
t_irc bncClockRinex::write(int GPSweek, double GPSweeks, const QString& prn,
                           double clkRnx, double clkRnxRate, double clkRnxAcc,
                           double clkRnxSig, double clkRnxRateSig, double clkRnxAccSig) {

  _oStr.setf(ios::fixed);

  if (reopen(GPSweek, GPSweeks) == success) {

    bncTime epoTime(GPSweek, GPSweeks);

    if (epoTime != _lastEpoTime) {

      // print out epoch before
      if (_lastEpoTime.valid()) {
        _out << _oStr.str();
        if (_lastEpoTime.daysec() != 0.0) {
          _oStr.str(std::string());
        }
      }

      _lastEpoTime = epoTime;
    }

    int numValues = 1;
    if (clkRnxSig && clkRnxRate && clkRnxRateSig) {
      numValues += 3;
    }
    if (clkRnxAcc && clkRnxAccSig) {
      numValues += 2;
    }

    _oStr << "AS " << prn.toLatin1().data()
         << "  " <<  epoTime.datestr(' ') << ' ' << epoTime.timestr(6, ' ')
         << "  " << numValues << "   "
         << fortranFormat(clkRnx, 19, 12).toLatin1().data();

    if (numValues >=2) {
      _oStr << " " << fortranFormat(clkRnxSig, 19, 12).toLatin1().data() << endl;
    }
    if (numValues == 4) {
      _oStr << fortranFormat(clkRnxRate, 19, 12).toLatin1().data() << " ";
      _oStr << fortranFormat(clkRnxRateSig, 19, 12).toLatin1().data() << " ";
    }
    if (numValues == 6) {
      _oStr << fortranFormat(clkRnxAcc, 19, 12).toLatin1().data() << " ";
      _oStr << " " << fortranFormat(clkRnxAccSig, 19, 12).toLatin1().data();
    }
    _oStr << endl;

    return success;
  }
  else {
    return failure;
  }
}

// Write Header
////////////////////////////////////////////////////////////////////////////
void bncClockRinex::writeHeader(const QDateTime& datTim) {

  _out << "     3.00           C                                       "  << "RINEX VERSION / TYPE" << endl;
 ;
  _out << QString("BNC v%1").arg(BNCVERSION).leftJustified(40, ' ', true).toLatin1().data()
       << datTim.toString("yyyyMMdd hhmmss").leftJustified(20, ' ', true).toLatin1().data()
       << "PGM / RUN BY / DATE" << endl;

  _out << "     1    AS                                                "
       << "# / TYPES OF DATA" << endl;

  _out << "unknown                                                     "
       << "ANALYSIS CENTER" << endl;

  _out << "   177                                                      "
       << "# OF SOLN SATS" << endl;

  _out << "G01 G02 G03 G04 G05 G06 G07 G08 G09 G10 G11 G12 G13 G14 G15 PRN LIST" << endl;
  _out << "G16 G17 G18 G19 G20 G21 G22 G23 G25 G26 G27 G28 G29 G30 G31 PRN LIST" << endl;
  _out << "G32 R01 R02 R03 R05 R06 R07 R08 R09 R10 R11 R12 R13 R14 R15 PRN LIST" << endl;
  _out << "R16 R17 R18 R19 R20 R21 R22 R23 R24 R25 R26 E01 E02 E03 E04 PRN LIST" << endl;
  _out << "R16 R17 R18 R19 R20 R21 R22 R23 R24 R25 R26 E01 E02 E03 E04 PRN LIST" << endl;
  _out << "E05 E06 E07 E08 E09 E10 E11 E12 E13 E14 E15 E16 E17 E18 E19 PRN LIST" << endl;
  _out << "E20 E21 E22 E23 E24 E25 E26 E27 E28 E29 E30 E31 E32 E33 E34 PRN LIST" << endl;
  _out << "E35 E36 C01 C02 C03 C04 C05 C06 C07 C08 C09 C10 C11 C12 C13 PRN LIST" << endl;
  _out << "C14 C15 C16 C17 C18 C19 C20 C21 C22 C23 C24 C25 C26 C27 C28 PRN LIST" << endl;
  _out << "C29 C30 C31 C32 C33 C34 C35 C36 C37 C38 C39 C40 C41 C42 C43 PRN LIST" << endl;
  _out << "C44 C45 C46 C47 C48 C49 C50 C51 C52 C53 C54 C55 C56 C57 C58 PRN LIST" << endl;
  _out << "C59 C60 C61 C62 C63 J01 J02 J03 J04 I01 I02 I03 I04 I05 I06 PRN LIST" << endl;
  _out << "I07 S20 S24 S27 S28 S29 S33 S35 S37 S38                     PRN LIST" << endl;

  _out << "     0    IGS20                                             "
       << "# OF SOLN STA / TRF" << endl;

  _out << "                                                            "
       << "END OF HEADER" << endl;
}

