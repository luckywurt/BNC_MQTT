
/* -------------------------------------------------------------------------
 * BKG NTRIP Server
 * -------------------------------------------------------------------------
 *
 * Class:      bncSP3
 *
 * Purpose:    writes SP3 files
 *
 * Author:     L. Mervart
 *
 * Created:    25-Apr-2008
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <iomanip>
#include <sstream>
#include <iostream>
#include <math.h>

#include "bncsp3.h"
#include "bncutils.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncSP3::bncSP3(const QString& fileName) : bncoutf(QString(), QString(), 0) {
  _inpOut    = input;
  _currEpoch = 0;
  _prevEpoch = 0;

  _stream.open(fileName.toLatin1().data());
  if (!_stream.good()) {
    throw "t_sp3File: cannot open file " + fileName;
  }

  while (_stream.good()) {
    getline(_stream, _lastLine);
    if (_lastLine[0] == '*') {
      break;
    }
  }
}

// Constructor
////////////////////////////////////////////////////////////////////////////
bncSP3::bncSP3(const QString& sklFileName, const QString& intr, int sampl)
  : bncoutf(sklFileName, intr, sampl) {
  _inpOut    = output;
  _currEpoch = 0;
  _prevEpoch = 0;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncSP3::~bncSP3() {
  delete _currEpoch;
  delete _prevEpoch;
}

// Write One Epoch
////////////////////////////////////////////////////////////////////////////
t_irc bncSP3::write(int GPSweek, double GPSweeks, const QString& prn,
                    const ColumnVector& xCoM, double sp3Clk,
                    const ColumnVector& v, double sp3ClkRate) {

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
        // Check the sampling interval (print empty epochs)
        // ------------------------------------------------
        if (_sampl > 0) {
          for (bncTime ep = _lastEpoTime +_sampl; ep < epoTime; ep = ep +_sampl) {
            _oStr << "\n*  " << ep.datestr(' ') << ' ' << ep.timestr(8, ' ');
          }
        }
      }

      // Print the new epoch
      // -------------------
      _oStr << "\n*  " << epoTime.datestr(' ') << ' ' << epoTime.timestr(8, ' ');

      _lastEpoTime = epoTime;
    }

    _oStr << "\nP" << prn.toLatin1().data()
         << setw(14) << setprecision(6) << xCoM(1) / 1000.0 // [km]
         << setw(14) << setprecision(6) << xCoM(2) / 1000.0 // [km]
         << setw(14) << setprecision(6) << xCoM(3) / 1000.0 // [km]
         << setw(14) << setprecision(6) << sp3Clk * 1e6;     // microseconds

    if (sp3ClkRate) {
      _oStr << "\nV" << prn.toLatin1().data()
           << setw(14) << setprecision(6) << v(1) * 10.0      // [dm/s]
           << setw(14) << setprecision(6) << v(2) * 10.0      // [dm/s]
           << setw(14) << setprecision(6) << v(3) * 10.0      // [dm/s]
           << setw(14) << setprecision(6) << sp3ClkRate * 1e2; // 10^⁻4 microseconds/sec
    }
    return success;
  }
  else {
    return failure;
  }
}

// Close File (write last line)
////////////////////////////////////////////////////////////////////////////
void bncSP3::closeFile() {
  _out << "\nEOF" << endl;
  bncoutf::closeFile();
}

// Write Header
////////////////////////////////////////////////////////////////////////////
void bncSP3::writeHeader(const QDateTime& datTim) {

  int    GPSWeek;
  double GPSWeeks;
  GPSweekFromDateAndTime(datTim, GPSWeek, GPSWeeks);

  double sec = fmod(GPSWeeks, 60.0);

  int    mjd;
  double dayfrac;
  mjdFromDateAndTime(datTim, mjd, dayfrac);

  int numEpo = _numSec;
  if (_sampl > 0) {
    numEpo /= _sampl;
  }

  _out << "#dP" << datTim.toString("yyyy MM dd hh mm").toLatin1().data()
       << setw(12) << setprecision(8) << sec
       << " " << setw(7) << numEpo << " ORBIT IGS20 HLM  IGS" << endl;

  _out << "## "
       << setw(4)  << GPSWeek
       << setw(16) << setprecision(8) << GPSWeeks
       << setw(15) << setprecision(8) << double(_sampl)
       << setw(6)  << mjd
       << setw(16) << setprecision(13) << dayfrac << endl;

  _out << "+  177   G01G02G03G04G05G06G07G08G09G10G11G12G13G14G15G16G17\n"
       << "+        G18G19G20G21G22G23G24G25G26G27G28G29G30G31G32R01R02\n"
       << "+        R03R04R05R06R07R08R09R10R11R12R13R14R15R16R17R18R19\n"
       << "+        R20R21R22R23R24R25R26E01E02E03E04E05E06E07E08E09E10\n"
       << "+        E11E12E13E14E15E16E17E18E19E20E21E22E23E24E25E26E27\n"
       << "+        E28E29E30E31E32E33E34E35E36C01C02C03C04C05C06C07C08\n"
       << "+        C09C10C11C12C13C14C15C16C17C18C19C20C21C22C23C24C25\n"
       << "+        C26C27C28C29C30C31C32C33C34C35C36C37C38C39C40C41C42\n"
       << "+        C43C44C45C46C47C48C49C50C51C52C53C54C55C56C57C58C59\n"
       << "+        C60C61C62C63J01J02J03J04I01I02I03I04I05I06I07S20S24\n"
       << "+        S27S28S29S33S35S37S38  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "++         0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0\n"
       << "%c M  cc GPS ccc cccc cccc cccc cccc ccccc ccccc ccccc ccccc\n"
       << "%c cc cc ccc ccc cccc cccc cccc cccc ccccc ccccc ccccc ccccc\n"
       << "%f  0.0000000  0.000000000  0.00000000000  0.000000000000000\n"
       << "%f  0.0000000  0.000000000  0.00000000000  0.000000000000000\n"
       << "%i    0    0    0    0      0      0      0      0         0\n"
       << "%i    0    0    0    0      0      0      0      0         0\n"
       << "/*                                                          \n"
       << "/*                                                          \n"
       << "/*                                                          \n"
       << "/*                                                          ";
}

// Read Next Epoch
////////////////////////////////////////////////////////////////////////////
const bncSP3::t_sp3Epoch* bncSP3::nextEpoch() {

  delete _prevEpoch; _prevEpoch = _currEpoch; _currEpoch = 0;

  if (_lastLine[0] == '*') {
    _currEpoch = new t_sp3Epoch();
    istringstream in(_lastLine.substr(1).c_str());
    int    YY, MM, DD, hh, mm;
    double ss;
    in >> YY >> MM >> DD >> hh >> mm >> ss;
    _currEpoch->_tt.set(YY, MM, DD, hh, mm, ss);
  }

  while (_stream.good()) {
    getline(_stream, _lastLine);
    if (_stream.eof() || _lastLine.find("EOF") == 0) {
      _stream.close();
      break;
    }
    if (_lastLine[0] == '*') {
      break;
    }
    if (_lastLine[0] == 'V') {
      continue;
    }
    t_sp3Sat* sp3Sat = new t_sp3Sat();
    istringstream in(_lastLine.substr(1).c_str());
    in >> sp3Sat->_prn >> sp3Sat->_xyz(1) >> sp3Sat->_xyz(2) >> sp3Sat->_xyz(3) >> sp3Sat->_clk;
    sp3Sat->_xyz *= 1.e3;

    // Simple Check - check satellite radial distance
    double rr = sp3Sat->_xyz.NormFrobenius();
    const double MINDIST = 2.e7;
    const double MAXDIST = 6.e7;
    if (rr < MINDIST || rr > MAXDIST || std::isnan(rr)) {
      delete sp3Sat;
      continue;
    }

    // Simple Check - clock valid
    if (sp3Sat->_clk == 999999.999999 ||
        sp3Sat->_clk == 0.0) {
      sp3Sat->_clkValid = false;
      sp3Sat->_clk      = 0.0;
    }
    else {
      sp3Sat->_clkValid = true;
      sp3Sat->_clk *= t_CST::c * 1.e-6;
    }

    _currEpoch->_sp3Sat.push_back(sp3Sat);
  }

  return _currEpoch;
}
