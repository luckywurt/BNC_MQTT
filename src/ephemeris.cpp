#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#include <newmatio.h>

#include "ephemeris.h"
#include "bncutils.h"
#include "bnctime.h"
#include "bnccore.h"
#include "bncutils.h"
#include "satObs.h"
#include "pppInclude.h"
#include "pppModel.h"
#include "RTCM3/bits.h"

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_eph::t_eph() {
  _checkState = unchecked;
  _type = undefined;
  _orbCorr = 0;
  _clkCorr = 0;
}
// Destructor
////////////////////////////////////////////////////////////////////////////
t_eph::~t_eph() {
  if (_orbCorr)
    delete _orbCorr;
  if (_clkCorr)
    delete _clkCorr;
}

//
////////////////////////////////////////////////////////////////////////////
void t_eph::setOrbCorr(const t_orbCorr *orbCorr) {
  if (_orbCorr) {
    delete _orbCorr;
    _orbCorr = 0;
  }
  _orbCorr = new t_orbCorr(*orbCorr);
}

//
////////////////////////////////////////////////////////////////////////////
void t_eph::setClkCorr(const t_clkCorr *clkCorr) {
  if (_clkCorr) {
    delete _clkCorr;
    _clkCorr = 0;
  }
  _clkCorr = new t_clkCorr(*clkCorr);
}

//
////////////////////////////////////////////////////////////////////////////
t_irc t_eph::getCrd(const bncTime &tt, ColumnVector &xc, ColumnVector &vv,
    bool useCorr) const {

  if (_checkState == bad ||
      _checkState == unhealthy ||
      _checkState == outdated) {
    return failure;
  }

  xc.ReSize(6);
  vv.ReSize(3);
  if (position(tt.gpsw(), tt.gpssec(), xc.data(), vv.data()) != success) {
    return failure;
  }
  if (useCorr) {
    if (_orbCorr && _clkCorr) {
      double dtO = tt - _orbCorr->_time;
      if (_orbCorr->_updateInt) {
        dtO -= (0.5 * ssrUpdateInt[_orbCorr->_updateInt]);
      }
      ColumnVector dx(3);
      dx[0] = _orbCorr->_xr[0] + _orbCorr->_dotXr[0] * dtO;
      dx[1] = _orbCorr->_xr[1] + _orbCorr->_dotXr[1] * dtO;
      dx[2] = _orbCorr->_xr[2] + _orbCorr->_dotXr[2] * dtO;

      RSW_to_XYZ(xc.Rows(1, 3), vv.Rows(1, 3), dx, dx);

      xc[0] -= dx[0];
      xc[1] -= dx[1];
      xc[2] -= dx[2];

      ColumnVector dv(3);
      RSW_to_XYZ(xc.Rows(1, 3), vv.Rows(1, 3), _orbCorr->_dotXr, dv);

      vv[0] -= dv[0];
      vv[1] -= dv[1];
      vv[2] -= dv[2];

      double dtC = tt - _clkCorr->_time;
      if (_clkCorr->_updateInt) {
        dtC -= (0.5 * ssrUpdateInt[_clkCorr->_updateInt]);
      }
      xc[3] += _clkCorr->_dClk + _clkCorr->_dotDClk * dtC
          + _clkCorr->_dotDotDClk * dtC * dtC;
    } else {
      return failure;
    }
  }
  return success;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_eph::setType(QString typeStr) {

  if (typeStr == "LNAV") {
    _type = t_eph::LNAV;
  } else if (typeStr == "FDMA") {
    _type = t_eph::FDMA;
  } else if (typeStr == "FNAV") {
    _type = t_eph::FNAV;
  } else if (typeStr == "INAV") {
    _type = t_eph::INAV;
  } else if (typeStr == "D1") {
    _type = t_eph::D1;
  } else if (typeStr == "D2") {
    _type = t_eph::D2;
  } else if (typeStr == "SBAS") {
    _type = t_eph::SBASL1;
  } else if (typeStr == "CNAV") {
    _type = t_eph::CNAV;
  } else if (typeStr == "CNV1") {
    _type = t_eph::CNV1;
  } else if (typeStr == "CNV2") {
    _type = t_eph::CNV2;
  } else if (typeStr == "CNV3") {
    _type = t_eph::CNV3;
  } else if (typeStr == "L1NV") {
    _type = t_eph::L1NV;
  } else if (typeStr == "L1OC") {
    _type = t_eph::L1OC;
  } else if (typeStr == "L3OC") {
    _type = t_eph::L3OC;
  } else {
    _type = t_eph::undefined;
  }

}


//
//////////////////////////////////////////////////////////////////////////////
QString t_eph::typeStr(e_type type, const t_prn &prn, double version) {
  QString typeStr = "";
  QString epochStart;
  QString eolStr;

  if (version < 4.0) {
    return typeStr;
  }

  if (version == 99.0) { // log output for OUTDATED, WRONG or UNHEALTHY satellites
    epochStart = "";
    eolStr = "";
  } else {
    epochStart = "> ";
    eolStr = "\n";
  }

  QString ephStr = QString("EPH %1 ").arg(prn.toString().c_str());
  switch (type) {
    case undefined:
      typeStr = epochStart + ephStr + "unknown" + eolStr;
      break;
    case LNAV:
      typeStr = epochStart + ephStr + "LNAV" + eolStr;
      break;
    case FDMA:
    case FDMA_M:
      typeStr = epochStart + ephStr + "FDMA" + eolStr;
      break;
    case FNAV:
      typeStr = epochStart + ephStr + "FNAV" + eolStr;
      break;
    case INAV:
      typeStr = epochStart + ephStr + "INAV" + eolStr;
      break;
    case D1:
      typeStr = epochStart + ephStr + "D1  " + eolStr;
      break;
    case D2:
      typeStr = epochStart + ephStr + "D2  " + eolStr;
      break;
    case SBASL1:
      typeStr = epochStart + ephStr + "SBAS" + eolStr;
      break;
    case CNAV:
      typeStr = epochStart + ephStr + "CNAV" + eolStr;
      break;
    case CNV1:
      typeStr = epochStart + ephStr + "CNV1" + eolStr;
      break;
    case CNV2:
      typeStr = epochStart + ephStr + "CNV2" + eolStr;
      break;
    case CNV3:
      typeStr = epochStart + ephStr + "CNV3" + eolStr;
      break;
    case L1NV:
      typeStr = epochStart + ephStr + "L1NV" + eolStr;
      break;
    case L1OC:
      typeStr = epochStart + ephStr + "L1OC" + eolStr;
      break;
    case L3OC:
      typeStr = epochStart + ephStr + "L3OC" + eolStr;
      break;
  }
  return typeStr;
}

//
//////////////////////////////////////////////////////////////////////////////
QString t_eph::rinexDateStr(const bncTime &tt, const t_prn &prn,
    double version) {
  QString prnStr(prn.toString().c_str());
  return rinexDateStr(tt, prnStr, version);
}

//
//////////////////////////////////////////////////////////////////////////////
QString t_eph::rinexDateStr(const bncTime &tt, const QString &prnStr,
    double version) {

  QString datStr;

  unsigned year, month, day, hour, min;
  double sec;
  tt.civil_date(year, month, day);
  tt.civil_time(hour, min, sec);

  QTextStream out(&datStr);

  if (version < 3.0) {
    QString prnHlp = prnStr.mid(1, 2);
    if (prnHlp[0] == '0')
      prnHlp[0] = ' ';
    out << prnHlp
        << QString(" %1 %2 %3 %4 %5%6").arg(year % 100, 2, 10, QChar('0')).arg(
            month, 2).arg(day, 2).arg(hour, 2).arg(min, 2).arg(sec, 5, 'f', 1);
  }
  else if (version == 99) {
    out
        << QString(" %1 %2 %3 %4 %5 %6").arg(year, 4).arg(month, 2, 10,
            QChar('0')).arg(day, 2, 10, QChar('0')).arg(hour, 2, 10, QChar('0')).arg(
            min, 2, 10, QChar('0')).arg(int(sec), 2, 10, QChar('0'));
  }
  else {
    out << prnStr
        << QString(" %1 %2 %3 %4 %5 %6").arg(year, 4).arg(month, 2, 10,
            QChar('0')).arg(day, 2, 10, QChar('0')).arg(hour, 2, 10, QChar('0')).arg(
            min, 2, 10, QChar('0')).arg(int(sec), 2, 10, QChar('0'));
  }

  return datStr;
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_ephGPS::t_ephGPS(double rnxVersion, const QStringList &lines, QString typeStr) {

  setType(typeStr);

  int nLines = 8; // LNAV
  // Source RINEX version < 4
  if (type() == t_eph::undefined) {
    _type = t_eph::LNAV;
  }

  if (type() == t_eph::CNAV ||
      type() == t_eph::L1NV) {
    nLines += 1;
  }
  if (type() == t_eph::CNV2) {
    nLines += 2;
  }

  if (lines.size() != nLines) {
    _checkState = bad;
    return;
  }

  // RINEX Format
  // ------------
  int fieldLen = 19;
  double statusflags = 0.0;

  int pos[4];
  pos[0] = (rnxVersion <= 2.12) ? 3 : 4;
  pos[1] = pos[0] + fieldLen;
  pos[2] = pos[1] + fieldLen;
  pos[3] = pos[2] + fieldLen;

  // Read nLines lines
  // ------------------
  for (int iLine = 0; iLine < nLines; iLine++) {
    QString line = lines[iLine];

    if (iLine == 0) {
      QTextStream in(line.left(pos[1]).toLatin1());
      int year, month, day, hour, min;
      double sec;

      QString prnStr, n;
      in >> prnStr;

      if (prnStr.size() == 1 &&
          (prnStr[0] == 'G' ||
           prnStr[0] == 'J' ||
           prnStr[0] == 'I')) {
        in >> n;
        prnStr.append(n);
      }

      if (       prnStr.at(0) == 'G') {
        _prn.set('G', prnStr.mid(1).toInt());
      } else if (prnStr.at(0) == 'J') {
        _prn.set('J', prnStr.mid(1).toInt());
      } else if (prnStr.at(0) == 'I') {
        _prn.set('I', prnStr.mid(1).toInt());
      } else {
        _prn.set('G', prnStr.toInt());
      }
      _prn.setFlag(type());

      in >> year >> month >> day >> hour >> min >> sec;

      if (year < 80) {
        year += 2000;
      } else if (year < 100) {
        year += 1900;
      }

      _TOC.set(year, month, day, hour, min, sec);

      if (   readDbl(line, pos[1], fieldLen, _clock_bias)
          || readDbl(line, pos[2], fieldLen, _clock_drift)
          || readDbl(line, pos[3], fieldLen, _clock_driftrate)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 1
    // =====================
    else if (iLine == 1) {
      if (type() == t_eph::CNAV ||
          type() == t_eph::CNV2 ||
          type() == t_eph::L1NV) {
        if (   readDbl(line, pos[0], fieldLen, _ADOT)
            || readDbl(line, pos[1], fieldLen, _Crs)
            || readDbl(line, pos[2], fieldLen, _Delta_n)
            || readDbl(line, pos[3], fieldLen, _M0)) {
          _checkState = bad;
          return;
        }
      } else { // LNAV
        if (   readDbl(line, pos[0], fieldLen, _IODE)
            || readDbl(line, pos[1], fieldLen, _Crs)
            || readDbl(line, pos[2], fieldLen, _Delta_n)
            || readDbl(line, pos[3], fieldLen, _M0)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 2
    // =====================
    else if (iLine == 2) {
      if (   readDbl(line, pos[0], fieldLen, _Cuc)
          || readDbl(line, pos[1], fieldLen, _e)
          || readDbl(line, pos[2], fieldLen, _Cus)
          || readDbl(line, pos[3], fieldLen, _sqrt_A)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 3
    // =====================
    else if (iLine == 3) {
      if (type() == t_eph::CNAV ||
          type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _top)
            || readDbl(line, pos[1], fieldLen, _Cic)
            || readDbl(line, pos[2], fieldLen, _OMEGA0)
            || readDbl(line, pos[3], fieldLen, _Cis)) {
          _checkState = bad;
          return;
        }
      } else if (type() == t_eph::L1NV) {
        if (   readDbl(line, pos[0], fieldLen, _IODE)
            || readDbl(line, pos[1], fieldLen, _Cic)
            || readDbl(line, pos[2], fieldLen, _OMEGA0)
            || readDbl(line, pos[3], fieldLen, _Cis)) {
          _checkState = bad;
          return;
        }
      } else { // LNAV
        if (   readDbl(line, pos[0], fieldLen, _TOEsec)
            || readDbl(line, pos[1], fieldLen, _Cic)
            || readDbl(line, pos[2], fieldLen, _OMEGA0)
            || readDbl(line, pos[3], fieldLen, _Cis)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 4
    // =====================
    else if (iLine == 4) {
      if (   readDbl(line, pos[0], fieldLen, _i0)
          || readDbl(line, pos[1], fieldLen, _Crc)
          || readDbl(line, pos[2], fieldLen, _omega)
          || readDbl(line, pos[3], fieldLen, _OMEGADOT)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 5
    // =====================
    else if (iLine == 5 && system() != t_eph::NavIC) {
      if (type() == t_eph::CNAV ||
          type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[1], fieldLen, _Delta_n_dot)
            || readDbl(line, pos[2], fieldLen, _URAI_NED0)
            || readDbl(line, pos[3], fieldLen, _URAI_NED1)) {
          _checkState = bad;
          return;
        }
      }
      else { // LNAV
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[1], fieldLen, _L2Codes)
            || readDbl(line, pos[2], fieldLen, _TOEweek)
            || readDbl(line, pos[3], fieldLen, _L2PFlag)) {
          _checkState = bad;
          return;
        }
      }
    } else if (iLine == 5 && system() == t_eph::NavIC) {
      if (type() == t_eph::LNAV) {
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[2], fieldLen, _TOEweek)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L1NV) {
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[1], fieldLen, _Delta_n_dot)
            || readDbl(line, pos[3], fieldLen, _RSF)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 6
    // =====================
    else if (iLine == 6 && system() != t_eph::NavIC) {
      if (type() == t_eph::CNAV ||
          type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _URAI_ED)
            || readDbl(line, pos[1], fieldLen, _health)
            || readDbl(line, pos[2], fieldLen, _TGD)
            || readDbl(line, pos[3], fieldLen, _URAI_NED2)) {
          _checkState = bad;
          return;
        }
      } else { // LNAV
        if (   readDbl(line, pos[0], fieldLen, _ura)
            || readDbl(line, pos[1], fieldLen, _health)
            || readDbl(line, pos[2], fieldLen, _TGD)
            || readDbl(line, pos[3], fieldLen, _IODC)) {
          _checkState = bad;
          return;
        }
      }
    }
    else if (iLine == 6 && system() == t_eph::NavIC) {
      if (type() == t_eph::LNAV) {
        if (   readDbl(line, pos[0], fieldLen, _ura)
            || readDbl(line, pos[1], fieldLen, _health)
            || readDbl(line, pos[2], fieldLen, _TGD)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L1NV) {
        int i = 0;
        (!_RSF) ? i = 2 : i = 3;
        if (   readDbl(line, pos[0], fieldLen, _URAI)
            || readDbl(line, pos[1], fieldLen, _health)
            || readDbl(line, pos[i], fieldLen, _TGD)) {
          _checkState = bad;
          return;
        }
        _ura = accuracyFromIndex(int(_URAI), system());
      }
    }
    // =====================
    // BROADCAST ORBIT - 7
    // =====================
    else if (iLine == 7) {
      if (type() == t_eph::LNAV) {
        if (readDbl(line, pos[0], fieldLen, _TOT)) {
          _checkState = bad;
          return;
        }
        if (system() != t_eph::NavIC) {
          double fitIntervalRnx;
          if (readDbl(line, pos[1], fieldLen, fitIntervalRnx)) {
            //fit interval BLK, do nothing
            _flags_unknown = true;
          } else {
            _flags_unknown = false;
            if      (system() == t_eph::GPS) { // in RINEX specified always as time period for GPS
              _fitInterval = fitIntervalRnx;
            }
            else if (system() == t_eph::QZSS) { // specified as flag for QZSS
              if (rnxVersion == 3.02) {
                _fitInterval = fitIntervalRnx; // specified as time period
              } else {
                _fitInterval = fitIntervalFromFlag(fitIntervalRnx, _IODC, t_eph::QZSS);
              }
            }
          }
        }
      }
      else if (type() == t_eph::CNAV ||
               type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _ISC_L1CA)
            || readDbl(line, pos[1], fieldLen, _ISC_L2C)
            || readDbl(line, pos[2], fieldLen, _ISC_L5I5)
            || readDbl(line, pos[3], fieldLen, _ISC_L5Q5)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L1NV) {
        if (!_RSF) {
          if (   readDbl(line, pos[0], fieldLen, _ISC_S)
              || readDbl(line, pos[1], fieldLen, _ISC_L1D)) {
            _checkState = bad;
            return;
          }
        } else {
          if (   readDbl(line, pos[2], fieldLen, _ISC_L1P)
              || readDbl(line, pos[3], fieldLen, _ISC_L1D)) {
            _checkState = bad;
            return;
          }
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 8
    // =====================
    else if (iLine == 8) {
      if (type() == t_eph::CNAV) {
        if (   readDbl(line, pos[0], fieldLen, _TOT)
            || readDbl(line, pos[1], fieldLen, _wnop)) {
          _checkState = bad;
          return;
        }
        if (readDbl(line, pos[2], fieldLen, statusflags)) {
          _flags_unknown = true;
        }
        else {
          _flags_unknown = false;
          // Bit 0:
          _intSF      = double(bitExtracted(unsigned(statusflags), 1, 0));
          // Bit 1:
          _L2Cphasing = double(bitExtracted(unsigned(statusflags), 1, 1));
          // Bit 2:
          _alert      = double(bitExtracted(unsigned(statusflags), 1, 2));
        }
      }
      else if (type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _ISC_L1Cd)
            || readDbl(line, pos[1], fieldLen, _ISC_L1Cp)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L1NV) {
        if (   readDbl(line, pos[0], fieldLen, _TOT)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 9
    // =====================
    else if (iLine == 9) {
      if (type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _TOT)
            || readDbl(line, pos[1], fieldLen, _wnop)) {
          _checkState = bad;
          return;
        }
        if (readDbl(line, pos[2], fieldLen, statusflags)) {
          _flags_unknown = true;
        }
        else {
          _flags_unknown = false;
          // Bit 0:
          _intSF = double(bitExtracted(int(statusflags), 1, 0));
          if (system() == t_eph::QZSS) {
            // Bit 1:
            _ephSF = double(bitExtracted(int(statusflags), 1, 1));
          }
        }
      }
    }
  }
}

// Compute GPS Satellite Position (virtual)
////////////////////////////////////////////////////////////////////////////
t_irc t_ephGPS::position(int GPSweek, double GPSweeks, double *xc,
    double *vv) const {

  static const double omegaEarth = 7292115.1467e-11;
  static const double gmGRS = 398.6005e12;

  memset(xc, 0, 6 * sizeof(double));
  memset(vv, 0, 3 * sizeof(double));

  double a0 = _sqrt_A * _sqrt_A;
  if (a0 == 0) {
    return failure;
  }

  double n0 = sqrt(gmGRS / (a0 * a0 * a0));

  bncTime tt(GPSweek, GPSweeks);
  double tk = tt - bncTime(int(_TOEweek), _TOEsec);

  double n = n0 + _Delta_n;
  double M = _M0 + n * tk;
  double E = M;
  double E_last;
  int nLoop = 0;
  do {
    E_last = E;
    E = M + _e * sin(E);

    if (++nLoop == 100) {
      return failure;
    }
  } while (fabs(E - E_last) * a0 > 0.001);
  double v = 2.0 * atan(sqrt((1.0 + _e) / (1.0 - _e)) * tan(E / 2));
  double u0 = v + _omega;
  double sin2u0 = sin(2 * u0);
  double cos2u0 = cos(2 * u0);
  double r = a0 * (1 - _e * cos(E)) + _Crc * cos2u0 + _Crs * sin2u0;
  double i = _i0 + _IDOT * tk + _Cic * cos2u0 + _Cis * sin2u0;
  double u = u0 + _Cuc * cos2u0 + _Cus * sin2u0;
  double xp = r * cos(u);
  double yp = r * sin(u);
  double OM = _OMEGA0 + (_OMEGADOT - omegaEarth) * tk - omegaEarth * _TOEsec;

  double sinom = sin(OM);
  double cosom = cos(OM);
  double sini = sin(i);
  double cosi = cos(i);
  xc[0] = xp * cosom - yp * cosi * sinom;
  xc[1] = xp * sinom + yp * cosi * cosom;
  xc[2] = yp * sini;

  double tc = tt - _TOC;
  xc[3] = _clock_bias + _clock_drift * tc + _clock_driftrate * tc * tc;

  // Velocity
  // --------
  double tanv2 = tan(v / 2);
  double dEdM = 1 / (1 - _e * cos(E));
  double dotv = sqrt((1.0 + _e) / (1.0 - _e)) / cos(E / 2) / cos(E / 2)
      / (1 + tanv2 * tanv2) * dEdM * n;
  double dotu = dotv + (-_Cuc * sin2u0 + _Cus * cos2u0) * 2 * dotv;
  double dotom = _OMEGADOT - omegaEarth;
  double doti = _IDOT + (-_Cic * sin2u0 + _Cis * cos2u0) * 2 * dotv;
  double dotr = a0 * _e * sin(E) * dEdM * n
      + (-_Crc * sin2u0 + _Crs * cos2u0) * 2 * dotv;
  double dotx = dotr * cos(u) - r * sin(u) * dotu;
  double doty = dotr * sin(u) + r * cos(u) * dotu;

  vv[0] = cosom * dotx - cosi * sinom * doty      // dX / dr
  - xp * sinom * dotom - yp * cosi * cosom * dotom   // dX / dOMEGA
  + yp * sini * sinom * doti;        // dX / di

  vv[1] = sinom * dotx + cosi * cosom * doty + xp * cosom * dotom
      - yp * cosi * sinom * dotom - yp * sini * cosom * doti;

  vv[2] = sini * doty + yp * cosi * doti;

  // Relativistic Correction
  // -----------------------
  xc[3] -= 4.442807633e-10 * _e * sqrt(a0) * sin(E);

  xc[4] = _clock_drift + _clock_driftrate * tc;
  xc[5] = _clock_driftrate;

  return success;
}

// RINEX Format String
//////////////////////////////////////////////////////////////////////////////
QString t_ephGPS::toString(double version) const {

  if (version < 4.0 &&
      (type() == t_eph::CNAV ||
       type() == t_eph::CNV2 ||
       type() == t_eph::L1NV )) {
    return "";
  }

  QString ephStr = typeStr(_type, _prn, version);
  QString rnxStr = ephStr + rinexDateStr(_TOC, _prn, version);

  QTextStream out(&rnxStr);

  out
      << QString("%1%2%3\n")
      .arg(_clock_bias,      19, 'e', 12)
      .arg(_clock_drift,     19, 'e', 12)
      .arg(_clock_driftrate, 19, 'e', 12);

  QString fmt = version < 3.0 ? "   %1%2%3%4\n" : "    %1%2%3%4\n";

  // =====================
  // BROADCAST ORBIT - 1
  // =====================
  if (type() == t_eph::CNAV ||
      type() == t_eph::CNV2 ||
      type() == t_eph::L1NV) {
    out
        << QString(fmt)
        .arg(_ADOT,    19, 'e', 12)
        .arg(_Crs,     19, 'e', 12)
        .arg(_Delta_n, 19, 'e', 12)
        .arg(_M0,      19, 'e', 12);
  } else { // LNAV, undefined
    out
        << QString(fmt)
        .arg(_IODE,    19, 'e', 12)
        .arg(_Crs,     19, 'e', 12)
        .arg(_Delta_n, 19, 'e', 12)
        .arg(_M0,      19, 'e', 12);
  }
  // =====================
  // BROADCAST ORBIT - 2
  // =====================
  out
      << QString(fmt)
      .arg(_Cuc,    19, 'e', 12)
      .arg(_e,      19, 'e', 12)
      .arg(_Cus,    19, 'e', 12)
      .arg(_sqrt_A, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 3
  // =====================
  if (type() == t_eph::CNAV ||
      type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg(_top,    19, 'e', 12)
        .arg(_Cic,    19, 'e', 12)
        .arg(_OMEGA0, 19, 'e', 12)
        .arg(_Cis,    19, 'e', 12);
  }
  else if (type() == t_eph::L1NV) {
    out
        << QString(fmt)
        .arg(_IODE,   19, 'e', 12)
        .arg(_Cic,    19, 'e', 12)
        .arg(_OMEGA0, 19, 'e', 12)
        .arg(_Cis,    19, 'e', 12);
  }
  else { // LNAV, undefined
    out
        << QString(fmt)
        .arg(_TOEsec, 19, 'e', 12)
        .arg(_Cic,    19, 'e', 12)
        .arg(_OMEGA0, 19, 'e', 12)
        .arg(_Cis,    19, 'e', 12);
  }
  // =====================
  // BROADCAST ORBIT - 4
  // =====================
  out
      << QString(fmt)
      .arg(_i0,       19, 'e', 12)
      .arg(_Crc,      19, 'e', 12)
      .arg(_omega,    19, 'e', 12)
      .arg(_OMEGADOT, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 5
  // =====================
  if (system() != t_eph::NavIC) {
    if (type() == t_eph::CNAV ||
        type() == t_eph::CNV2) {
      out
          << QString(fmt)
          .arg(_IDOT,        19, 'e', 12)
          .arg(_Delta_n_dot, 19, 'e', 12)
          .arg(_URAI_NED0,   19, 'e', 12)
          .arg(_URAI_NED1,   19, 'e', 12);
    }
    else { // LNAV, undefined
      out
          << QString(fmt)
          .arg(_IDOT,    19, 'e', 12)
          .arg(_L2Codes, 19, 'e', 12)
          .arg(_TOEweek, 19, 'e', 12)
          .arg(_L2PFlag, 19, 'e', 12);
    }
  }
  else {
    if (type() == t_eph::LNAV ||
        type() == t_eph::undefined) {
      out
      << QString(fmt)
      .arg(_IDOT,    19, 'e', 12)
      .arg("",       19, QChar(' '))
      .arg(_TOEweek, 19, 'e', 12)
      .arg("",       19, QChar(' '));
    }
    else if (type() == t_eph::L1NV) {
      out
      << QString(fmt)
      .arg(_IDOT,        19, 'e', 12)
      .arg(_Delta_n_dot, 19, 'e', 12)
      .arg("",           19, QChar(' '))
      .arg(_RSF,         19, 'e', 12);
    }
  }
  // =====================
  // BROADCAST ORBIT - 6
  // =====================
  if (system() != t_eph::NavIC) {
    if (type() == t_eph::CNAV ||
        type() == t_eph::CNV2) {
      out
          << QString(fmt)
          .arg(_URAI_ED,   19, 'e', 12)
          .arg(_health,    19, 'e', 12)
          .arg(_TGD,       19, 'e', 12)
          .arg(_URAI_NED2, 19, 'e', 12);
    }
    else { // LNAV, undefined
      out
          << QString(fmt)
          .arg(_ura,    19, 'e', 12)
          .arg(_health, 19, 'e', 12)
          .arg(_TGD,    19, 'e', 12)
          .arg(_IODC,   19, 'e', 12);
    }
  }
  else {
    if (type() == t_eph::LNAV ||
        type() == t_eph::undefined) {
      out
          << QString(fmt)
          .arg(_ura,    19, 'e', 12)
          .arg(_health, 19, 'e', 12)
          .arg(_TGD,    19, 'e', 12);
    }
    else if (type() == t_eph::L1NV) {
      int i = 0; (!_RSF) ? i = 2 : i = 3;
      if (i == 2) {
        out
            << QString(fmt)
            .arg(_URAI,   19, 'e', 12)
            .arg(_health, 19, 'e', 12)
            .arg(_TGD,    19, 'e', 12)
            .arg("",  19, QChar(' '));
      }
      else {
        out
            << QString(fmt)
            .arg(_URAI,   19, 'e', 12)
            .arg(_health, 19, 'e', 12)
            .arg("",      19, QChar(' '))
            .arg(_TGD,    19, 'e', 12);
      }
    }
  }
  // =====================
  // BROADCAST ORBIT - 7
  // =====================
  if (type() == t_eph::LNAV ||
      type() == t_eph::undefined) {

    double tot = _TOT;
    if (tot == 0.9999e9 && version < 3.0) {
      tot = 0.0;
    }
    // fitInterval
    if (system() == t_eph::NavIC) {
      out
          << QString(fmt)
          .arg(tot, 19, 'e', 12)
          .arg("",  19, QChar(' '))
          .arg("",  19, QChar(' '))
          .arg("",  19, QChar(' '));
    }
    else {
      if (_flags_unknown) {
        out
            << QString(fmt)
            .arg(tot,            19, 'e', 12)
            .arg("",             19, QChar(' '))
            .arg("",             19, QChar(' '))
            .arg("",             19, QChar(' '));
      }
      else {
        // for GPS and QZSS in version 3.02 specified in hours
        double fitIntervalRnx = _fitInterval;
        // otherwise specified as flag
        if (system() == t_eph::QZSS && version != 3.02) {
          (_fitInterval == 2.0) ? fitIntervalRnx = 0.0 : fitIntervalRnx = 1.0;
        }
        out
            << QString(fmt)
            .arg(tot,            19, 'e', 12)
            .arg(fitIntervalRnx, 19, 'e', 12)
            .arg("",             19, QChar(' '))
            .arg("",             19, QChar(' '));
      }
    }
  }
  else if (type() == t_eph::CNAV ||
           type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg(_ISC_L1CA, 19, 'e', 12)
        .arg(_ISC_L2C,  19, 'e', 12)
        .arg(_ISC_L5I5, 19, 'e', 12)
        .arg(_ISC_L5Q5, 19, 'e', 12);
  }
  else if (type() == t_eph::L1NV) {
    if (_RSF) {
      out
          << QString(fmt)
          .arg(_ISC_S,   19, 'e', 12)
          .arg(_ISC_L1D, 19, 'e', 12)
          .arg("",       19, QChar(' '))
          .arg("",       19, QChar(' '));
    }
    else {
      out
          << QString(fmt)
          .arg("",       19, QChar(' '))
          .arg("",       19, QChar(' '))
          .arg(_ISC_L1P, 19, 'e', 12)
          .arg(_ISC_L1D, 19, 'e', 12);
    }
  }
  // =====================
  // BROADCAST ORBIT - 8
  // =====================
  if (type() == t_eph::CNAV) {
    int intFlags = 0;
    if (!_flags_unknown) {
      // Bit 0:
      if (_intSF)      {intFlags |= (1 << 0);}
      // Bit 1:
      if (_L2Cphasing) {intFlags |= (1 << 1);}
      // Bit 2:
      if (_alert)      {intFlags |= (1 << 2);}
      out
          << QString(fmt)
          .arg(_TOT,             19, 'e', 12)
          .arg(_wnop,            19, 'e', 12)
          .arg(double(intFlags), 19, 'e', 12)
          .arg("",               19, QChar(' '));
    }
    else {
      out
          << QString(fmt)
          .arg(_TOT,  19, 'e', 12)
          .arg(_wnop, 19, 'e', 12)
          .arg("",    19, QChar(' '))
          .arg("",    19, QChar(' '));
    }
  }
  else if (type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg(_ISC_L1Cd, 19, 'e', 12)
        .arg(_ISC_L1Cp, 19, 'e', 12)
        .arg("", 19, QChar(' '))
        .arg("", 19, QChar(' '));
  }
  else if (type() == t_eph::L1NV) {
    out
        << QString(fmt)
        .arg(_TOT, 19, 'e', 12)
        .arg("",   19, QChar(' '))
        .arg("",   19, QChar(' '))
        .arg("",   19, QChar(' '));
  }
  // =====================
  // BROADCAST ORBIT - 9
  // =====================
  if (type() == t_eph::CNV2) {
    int intFlags = 0;
    if (!_flags_unknown) {
      // Bit 0:
      if (_intSF)   {intFlags |= (1 << 0);}
      if (system() == t_eph::QZSS) {
        // Bit 1:
        if (_ephSF) {intFlags |= (1 << 1);}
      }
      out
          << QString(fmt)
          .arg(_TOT,             19, 'e', 12)
          .arg(_wnop,            19, 'e', 12)
          .arg(double(intFlags), 19, 'e', 12)
          .arg("",               19, QChar(' '));
    }
    else {
      out
          << QString(fmt)
          .arg(_TOT, 19, 'e', 12)
          .arg(_wnop, 19, 'e', 12)
          .arg("",    19, QChar(' '))
          .arg("",    19, QChar(' '));
    }
  }
  return rnxStr;
}

// Health status of GPS Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephGPS::isUnhealthy() const {

  switch (system()) {
    case t_eph::GPS:
    case t_eph::QZSS:
      switch (type()) {
        case  t_eph::LNAV:
        case  t_eph::CNAV:
        case  t_eph::CNV2:
          if (_health == 0.0) {
            return 0;
          }
          else {
            return 1;
          }
          break;
      }
      break;
    case t_eph::NavIC:
      switch (type()) {
        case t_eph::LNAV:
          if (_health == 0.0) { // L1 & S healthy
            return 0;
          }
          if (_health == 1.0 || // L5 healthy and S unhealthy,
              _health == 2.0 || // L5 unhealthy and S healthy
              _health == 3.0) { // both L5 and S unhealthy
            return 1;
          }
          break;
        case t_eph::L1NV:
          if (_health == 0.0) { // All navigation data on L1-SPS signal OK
            return 0;
          }
          if (_health == 1.0) { // Some or all navigation data on L1-SPS signal bad
            return 1;
          }
          break;
      }
      break;
  }
  return 0;
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_ephGlo::t_ephGlo(double rnxVersion, const QStringList &lines, const QString typeStr) {

  setType(typeStr);

  int nLines = 4;

  // Source RINEX version < 4
  if (type() == t_eph::undefined) {
    // cannot be determined from input data
    // but is set to be able to work with old RINEX files in BNC applications
    _type = t_eph::FDMA_M;
  }

  if (rnxVersion >= 3.05) {
    nLines += 1;
  } else {
    _M_delta_tau = 0.9999e9; // unknown
    _M_FT        = 1.5e1;    // unknown
    _statusflags_unknown = true;
    _healthflags_unknown = true;
  }

  if (lines.size() != nLines) {
    _checkState = bad;
    return;
  }

  // RINEX Format
  // ------------
  int fieldLen = 19;
  double statusflags = 0.0;
  double healthflags = 0.0;
  double sourceflags = 0.0;
  _tauC = 0.0;
  _tau1 = 0.0;
  _tau2 = 0.0;
  _additional_data_availability = 0.0;

  int pos[4];
  pos[0] = (rnxVersion <= 2.12) ? 3 : 4;
  pos[1] = pos[0] + fieldLen;
  pos[2] = pos[1] + fieldLen;
  pos[3] = pos[2] + fieldLen;

  // Read four lines
  // ---------------
  for (int iLine = 0; iLine < nLines; iLine++) {
    QString line = lines[iLine];

    if (iLine == 0) {
      QTextStream in(line.left(pos[1]).toLatin1());

      int year, month, day, hour, min;
      double sec;

      QString prnStr, n;
      in >> prnStr;
      if (prnStr.size() == 1 && prnStr[0] == 'R') {
        in >> n;
        prnStr.append(n);
      }

      if (prnStr.at(0) == 'R') {
        _prn.set('R', prnStr.mid(1).toInt());
      } else {
        _prn.set('R', prnStr.toInt());
      }

      in >> year >> month >> day >> hour >> min >> sec;
      if (year < 80) {
        year += 2000;
      } else if (year < 100) {
        year += 1900;
      }

      _gps_utc = gnumleap(year, month, day);

      _TOC.set(year, month, day, hour, min, sec);
      _TOC = _TOC + _gps_utc;
      int nd = int((_TOC.gpssec())) / (24.0 * 60.0 * 60.0);
      if (   readDbl(line, pos[1], fieldLen, _tau)
          || readDbl(line, pos[2], fieldLen, _gamma)
          || readDbl(line, pos[3], fieldLen, _tki)) {
        _checkState = bad;
        return;
      }
      _tki -= nd * 86400.0;
      _tau = -_tau;
    }
    // =====================
    // BROADCAST ORBIT - 1
    // =====================
    else if (iLine == 1) {
      if (   readDbl(line, pos[0], fieldLen, _x_pos)
          || readDbl(line, pos[1], fieldLen, _x_vel)
          || readDbl(line, pos[2], fieldLen, _x_acc)
          || readDbl(line, pos[3], fieldLen, _health)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 2
    // =====================
    else if (iLine == 2) {
      if (type() == t_eph::FDMA ||
          type() == t_eph::FDMA_M) {
        if (   readDbl(line, pos[0], fieldLen, _y_pos)
            || readDbl(line, pos[1], fieldLen, _y_vel)
            || readDbl(line, pos[2], fieldLen, _y_acc)
            || readDbl(line, pos[3], fieldLen, _frq_num)) {
          _checkState = bad;
          return;
        }
      }
      else { //L1OC, L3OC
        if (   readDbl(line, pos[0], fieldLen, _y_pos)
            || readDbl(line, pos[1], fieldLen, _y_vel)
            || readDbl(line, pos[2], fieldLen, _y_acc)
            || readDbl(line, pos[3], fieldLen, statusflags)) {
          _checkState = bad;
          return;
        }
         _data_validity = int(statusflags);
      }
    }
    // =====================
    // BROADCAST ORBIT - 3
    // =====================
    else if (iLine == 3) {
      if (type() == t_eph::FDMA ||
          type() == t_eph::FDMA_M) {
        if (   readDbl(line, pos[0], fieldLen, _z_pos)
            || readDbl(line, pos[1], fieldLen, _z_vel)
            || readDbl(line, pos[2], fieldLen, _z_acc)
            || readDbl(line, pos[3], fieldLen, _E)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L1OC) {
        if (   readDbl(line, pos[0], fieldLen, _z_pos)
            || readDbl(line, pos[1], fieldLen, _z_vel)
            || readDbl(line, pos[2], fieldLen, _z_acc)
            || readDbl(line, pos[3], fieldLen, _TGD_L2OCp)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _z_pos)
            || readDbl(line, pos[1], fieldLen, _z_vel)
            || readDbl(line, pos[2], fieldLen, _z_acc)
            || readDbl(line, pos[3], fieldLen, _TGD_L3OCp)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 4
    // =====================
    else if (iLine == 4) {
      if (type() == t_eph::FDMA ||
          type() == t_eph::FDMA_M) {
        if (readDbl(line, pos[0], fieldLen, statusflags)) {
          //status flags BLK, do nothing
          _statusflags_unknown = true;
        } else {
          _statusflags_unknown = false;
          // status flags
          // ============
          // bit 0-1
          _M_P =  double(bitExtracted(int(statusflags), 2, 0));
          // bit 2-3
          _P1 =   double(bitExtracted(int(statusflags), 2, 2));
          // bit 4
          _P2 =   double(bitExtracted(int(statusflags), 1, 4));
          // bit 5
          _P3 =   double(bitExtracted(int(statusflags), 1, 5));
          // bit 6
          _M_P4 = double(bitExtracted(int(statusflags), 1, 6));
          // bit 7-8
          _M_M =  double(bitExtracted(int(statusflags), 2, 7));
          /// GLO M/K exclusive flags/values only valid if flag M is set to '01'
          if (!_M_M) {
            _M_P  = 0.0;
            _M_l3 = 0.0;
            _M_P4 = 0.0;
            _M_FE = 0.0;
            _M_FT = 0.0;
            _M_NA = 0.0;
            _M_NT = 0.0;
            _M_N4 = 0.0;
            _M_l5 = 0.0;
            _M_tau_GPS   = 0.0;
            _M_delta_tau = 0.0;
            _type = t_eph::FDMA;
          }
          else {
            _type = t_eph::FDMA_M;
          }
        }
        if (   readDbl(line, pos[1], fieldLen, _M_delta_tau)
            || readDbl(line, pos[2], fieldLen, _M_FT)) {
          _checkState = bad;
          return;
        }
        if (readDbl(line, pos[3], fieldLen, healthflags)) {
          // health flags BLK
          _healthflags_unknown = true;
        } else {
          _healthflags_unknown = false;
          // health flags
          // ============
          // bit 0 (is to be ignored, if bit 1 is zero)
          _almanac_health = double(bitExtracted(int(healthflags), 1, 0));
          // bit 1
          _almanac_health_availablility_indicator =
                            double(bitExtracted(int(healthflags), 1, 1));
          //  bit 2; GLO-M/K only, health bit of string 3
          _M_l3 =           double(bitExtracted(int(healthflags), 1, 2));
        }
      }
      else if (type() == t_eph::L1OC ||
               type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _sat_type)
            || readDbl(line, pos[1], fieldLen, sourceflags)
            || readDbl(line, pos[2], fieldLen, _EE)
            || readDbl(line, pos[3], fieldLen, _ET)) {
          _checkState = bad;
          return;
        }
        // sourceflags:
        // ============
        // bit 0-1
        _RT = double(bitExtracted(int(sourceflags), 2, 0));
        // bit 2-3:
        _RE = double(bitExtracted(int(sourceflags), 2, 2));
      }
    }
    // =====================
    // BROADCAST ORBIT - 5
    // =====================
    else if (iLine == 5) {
      if (type() == t_eph::L1OC ||
          type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _attitude_P2)
            || readDbl(line, pos[1], fieldLen, _Tin)
            || readDbl(line, pos[2], fieldLen, _tau1)
            || readDbl(line, pos[3], fieldLen, _tau2)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 6
    // =====================
    else if (iLine == 6) {
      if (type() == t_eph::L1OC ||
          type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _yaw)
            || readDbl(line, pos[1], fieldLen, _sn)
            || readDbl(line, pos[2], fieldLen, _angular_rate)
            || readDbl(line, pos[3], fieldLen, _angular_acc)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 7
    // =====================
    else if (iLine == 7) {
      if (type() == t_eph::L1OC ||
          type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _angular_rate_max)
            || readDbl(line, pos[1], fieldLen, _X_PC)
            || readDbl(line, pos[2], fieldLen, _Y_PC)
            || readDbl(line, pos[3], fieldLen, _Z_PC)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 8
    // =====================
    else if (iLine == 8) {
      if (type() == t_eph::L1OC ||
          type() == t_eph::L3OC) {
        if (   readDbl(line, pos[0], fieldLen, _M_FE)
            || readDbl(line, pos[1], fieldLen, _M_FT)
            || readDbl(line, pos[3], fieldLen, _TOT)) {
          _checkState = bad;
          return;
        }
      }
    }
  }

  _prn.setFlag(type());

  // Initialize status vector
  // ------------------------
  _tt = _TOC;
  _xv.ReSize(6);
  _xv = 0.0;
  _xv(1) = _x_pos * 1.e3;
  _xv(2) = _y_pos * 1.e3;
  _xv(3) = _z_pos * 1.e3;
  _xv(4) = _x_vel * 1.e3;
  _xv(5) = _y_vel * 1.e3;
  _xv(6) = _z_vel * 1.e3;
}

// Compute Glonass Satellite Position (virtual)
////////////////////////////////////////////////////////////////////////////
t_irc t_ephGlo::position(int GPSweek, double GPSweeks, double *xc,
    double *vv) const {

  static const double nominalStep = 10.0;

  memset(xc, 0, 6 * sizeof(double));
  memset(vv, 0, 3 * sizeof(double));

  double dtPos = bncTime(GPSweek, GPSweeks) - _tt;

  if (fabs(dtPos) > 24 * 3600.0) {
    return failure;
  }

  int nSteps = int(fabs(dtPos) / nominalStep) + 1;
  double step = dtPos / nSteps;

  double acc[3];
  acc[0] = _x_acc * 1.e3;
  acc[1] = _y_acc * 1.e3;
  acc[2] = _z_acc * 1.e3;

  for (int ii = 1; ii <= nSteps; ii++) {
    _xv = rungeKutta4(_tt.gpssec(), _xv, step, acc, glo_deriv);
    _tt = _tt + step;
  }

  // Position and Velocity
  // ---------------------
  xc[0] = _xv(1);
  xc[1] = _xv(2);
  xc[2] = _xv(3);

  vv[0] = _xv(4);
  vv[1] = _xv(5);
  vv[2] = _xv(6);

  // Clock Correction
  // ----------------
  double dtClk = bncTime(GPSweek, GPSweeks) - _TOC;
  xc[3] = -_tau + _gamma * dtClk;

  xc[4] = _gamma;
  xc[5] = 0.0;

  return success;
}

// RINEX Format String
//////////////////////////////////////////////////////////////////////////////
QString t_ephGlo::toString(double version) const {

  if (version < 4.0 &&
      (type() == t_eph::L1OC ||
       type() == t_eph::L3OC )) {
    return "";
  }

  QString ephStr = typeStr(_type, _prn, version);
  QString rnxStr = ephStr + rinexDateStr(_TOC - _gps_utc, _prn, version);
  int nd = int((_TOC - _gps_utc).gpssec()) / (24.0 * 60.0 * 60.0);
  QTextStream out(&rnxStr);

  out
      << QString("%1%2%3\n")
      .arg(-_tau,               19, 'e', 12)
      .arg(_gamma,              19, 'e', 12)
      .arg(_tki + nd * 86400.0, 19, 'e', 12);

  QString fmt = version < 3.0 ? "   %1%2%3%4\n" : "    %1%2%3%4\n";
  // =====================
  // BROADCAST ORBIT - 1
  // =====================
  out
      << QString(fmt)
      .arg(_x_pos,  19, 'e', 12)
      .arg(_x_vel,  19, 'e', 12)
      .arg(_x_acc,  19, 'e', 12)
      .arg(_health, 19, 'e', 12);

  // =====================
  // BROADCAST ORBIT - 2
  // =====================
  if (type() == t_eph::FDMA ||
      type() == t_eph::FDMA_M) {
    out
        << QString(fmt)
        .arg(_y_pos,   19, 'e', 12)
        .arg(_y_vel,   19, 'e', 12)
        .arg(_y_acc,   19, 'e', 12)
        .arg(_frq_num, 19, 'e', 12);
  }
  else { //L1OC, L3OC
    out
        << QString(fmt)
        .arg(_y_pos,                 19, 'e', 12)
        .arg(_y_vel,                 19, 'e', 12)
        .arg(_y_acc,                 19, 'e', 12)
        .arg(double(_data_validity), 19, 'e', 12);
  }
  // =====================
  // BROADCAST ORBIT - 3
  // =====================
  if (type() == t_eph::FDMA ||
      type() == t_eph::FDMA_M) {
    out
        << QString(fmt)
        .arg(_z_pos,  19, 'e', 12)
        .arg(_z_vel,  19, 'e', 12)
        .arg(_z_acc,  19, 'e', 12)
        .arg(_E,      19, 'e', 12);
  }
  else if (type() == t_eph::L1OC) {
    out
        << QString(fmt)
        .arg(_z_pos,     19, 'e', 12)
        .arg(_z_vel,     19, 'e', 12)
        .arg(_z_acc,     19, 'e', 12)
        .arg(_TGD_L2OCp, 19, 'e', 12);
  }
  else if (type() == t_eph::L3OC) {
    out
        << QString(fmt)
        .arg(_z_pos,     19, 'e', 12)
        .arg(_z_vel,     19, 'e', 12)
        .arg(_z_acc,     19, 'e', 12)
        .arg(_TGD_L3OCp, 19, 'e', 12);
  }
  if (version >= 3.05) {
    // =====================
    // BROADCAST ORBIT - 4
    // =====================
    if (type() == t_eph::FDMA ||
        type() == t_eph::FDMA_M){
      int statusflags = 0;
      int healthflags = 0;
      if (!_statusflags_unknown ) {
        // bit 0-1
        if      (_M_P == 1.0) {statusflags |= (1 << 0);}
        else if (_M_P == 2.0) {statusflags |= (1 << 1);}
        else if (_M_P == 3.0) {statusflags |= (1 << 0); statusflags |= (1 << 1);}
        // bit 2-3
        if      (_P1 == 1.0)  {statusflags |= (1 << 2);}
        else if (_P1 == 2.0)  {statusflags |= (1 << 3);}
        else if (_P1 == 3.0)  {statusflags |= (1 << 2); statusflags |= (1 << 3);}
        // bit 4
        if (_P2)              {statusflags |= (1 << 4);}
        // bit 5
        if (_P3)              {statusflags |= (1 << 5);}
        // bit 6
        if (_M_P4)            {statusflags |= (1 << 6);}
        // bit 7-8
        if (_M_M == 1.0)      {statusflags |= (1 << 7);}
      }
      if (!_healthflags_unknown) {
        // bit 0 (is to be ignored, if bit 1 is zero)
        if (_almanac_health) {healthflags |= (1 << 0);}
        // bit 1
        if (_almanac_health_availablility_indicator) {healthflags |= (1 << 1);}
        //  bit 2
        if (_M_l3) {healthflags |= (1 << 2);}
      }

      if (_statusflags_unknown && _healthflags_unknown) {
        out
            << QString(fmt)
            .arg("",           19, QChar(' ')) // status-flags BNK (unknown)
            .arg(_M_delta_tau, 19, 'e', 12)
            .arg(_M_FT,        19, 'e', 12)
            .arg("",           19, QChar(' '));// health-flags BNK (unknown)
      }
      else if (!_statusflags_unknown && _healthflags_unknown) {
        out
            << QString(fmt)
            .arg(double(statusflags),  19, 'e', 12)
            .arg(_M_delta_tau,         19, 'e', 12)
            .arg(_M_FT,                19, 'e', 12)
            .arg("",                   19, QChar(' '));// health-flags BNK (unknown)
      }
      else if (_statusflags_unknown && !_healthflags_unknown) {
        out
            << QString(fmt)
            .arg("",                  19, QChar(' ')) // status-flags BNK (unknown)
            .arg(_M_delta_tau,        19, 'e', 12)
            .arg(_M_FT,               19, 'e', 12)
            .arg(double(healthflags), 19, 'e', 12);
      }
      else if (!_statusflags_unknown && !_healthflags_unknown) {
        out
            << QString(fmt)
            .arg(double(statusflags),  19, 'e', 12)
            .arg(_M_delta_tau,         19, 'e', 12)
            .arg(_M_FT,                19, 'e', 12)
            .arg(double(healthflags),  19, 'e', 12);
      }
    }
    else if (type() == t_eph::L1OC ||
             type() == t_eph::L3OC) {
      int sourceflags = 0;
      // bit 0-1
      if      (_RT == 1.0) {sourceflags |= (1 << 0);}
      else if (_RT == 2.0) {sourceflags |= (1 << 1);}
      else if (_RT == 3.0) {sourceflags |= (1 << 0); sourceflags |= (1 << 1);}
      // bit 2-3
      if      (_RE == 1.0)  {sourceflags |= (1 << 2);}
      else if (_RE == 2.0)  {sourceflags |= (1 << 3);}
      else if (_RE == 3.0)  {sourceflags |= (1 << 2); sourceflags |= (1 << 3);}
      out
          << QString(fmt)
          .arg(_sat_type  ,         19, 'e', 12)
          .arg(double(sourceflags), 19, 'e', 12)
          .arg(_ET,                 19, 'e', 12)
          .arg(_EE,                 19, 'e', 12);
    }
    // =====================
    // BROADCAST ORBIT - 5
    // =====================
    if (type() == t_eph::L1OC ||
        type() == t_eph::L3OC) {
      out
          << QString(fmt)
          .arg(_attitude_P2, 19, 'e', 12)
          .arg(_Tin,         19, 'e', 12)
          .arg(_tau1,        19, 'e', 12)
          .arg(_tau2,        19, 'e', 12);
    }
    // =====================
    // BROADCAST ORBIT - 6
    // =====================
    if (type() == t_eph::L1OC ||
        type() == t_eph::L3OC) {
      out
          << QString(fmt)
          .arg(_yaw,          19, 'e', 12)
          .arg(_sn,           19, 'e', 12)
          .arg(_angular_rate, 19, 'e', 12)
          .arg(_angular_acc,  19, 'e', 12);
    }
    // =====================
    // BROADCAST ORBIT - 7
    // =====================
    if (type() == t_eph::L1OC ||
        type() == t_eph::L3OC) {
      out
          << QString(fmt)
          .arg(_angular_rate_max, 19, 'e', 12)
          .arg(_X_PC,             19, 'e', 12)
          .arg(_Y_PC,             19, 'e', 12)
          .arg(_Z_PC,             19, 'e', 12);
    }
    // =====================
    // BROADCAST ORBIT - 8
    // =====================
    if (type() == t_eph::L1OC ||
        type() == t_eph::L3OC) {
      out
          << QString(fmt)
          .arg(_M_FE, 19, 'e', 12)
          .arg(_M_FT, 19, 'e', 12)
          .arg("",    19, QChar(' '))
          .arg(_TOT,  19, 'e', 12);
    }
  }
  return rnxStr;
}

// Derivative of the state vector using a simple force model (static)
////////////////////////////////////////////////////////////////////////////
ColumnVector t_ephGlo::glo_deriv(double /* tt */, const ColumnVector &xv,
    double *acc) {

  // State vector components
  // -----------------------
  ColumnVector rr = xv.rows(1, 3);
  ColumnVector vv = xv.rows(4, 6);

  // Acceleration
  // ------------
  static const double gmWGS = 398.60044e12;
  static const double AE = 6378136.0;
  static const double OMEGA = 7292115.e-11;
  static const double C20 = -1082.6257e-6;

  double rho = rr.NormFrobenius();
  double t1 = -gmWGS / (rho * rho * rho);
  double t2 = 3.0 / 2.0 * C20 * (gmWGS * AE * AE)
      / (rho * rho * rho * rho * rho);
  double t3 = OMEGA * OMEGA;
  double t4 = 2.0 * OMEGA;
  double z2 = rr(3) * rr(3);

  // Vector of derivatives
  // ---------------------
  ColumnVector va(6);
  va(1) = vv(1);
  va(2) = vv(2);
  va(3) = vv(3);
  va(4) = (t1 + t2 * (1.0 - 5.0 * z2 / (rho * rho)) + t3) * rr(1) + t4 * vv(2)
      + acc[0];
  va(5) = (t1 + t2 * (1.0 - 5.0 * z2 / (rho * rho)) + t3) * rr(2) - t4 * vv(1)
      + acc[1];
  va(6) = (t1 + t2 * (3.0 - 5.0 * z2 / (rho * rho))) * rr(3) + acc[2];

  return va;
}

// IOD of Glonass Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephGlo::IOD() const {
  bncTime tMoscow = _TOC - _gps_utc + 3 * 3600.0;
  return (unsigned long) tMoscow.daysec() / 900;
}

// Health status of Glonass Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephGlo::isUnhealthy() const {

  if (_almanac_health_availablility_indicator) {
    if ((_health == 0 && _almanac_health == 0)
        || (_health == 1 && _almanac_health == 0)
        || (_health == 1 && _almanac_health == 1)) {
      return 1;
    }
  } else if (!_almanac_health_availablility_indicator) {
    if (_health) {
      return 1;
    }
  }
  return 0; /* (_health == 0 && _almanac_health == 1) or (_health == 0) */
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_ephGal::t_ephGal(double rnxVersion, const QStringList &lines, const QString typeStr) {

  setType(typeStr);

  const int nLines = 8;

  if (lines.size() != nLines) {
    _checkState = bad;
    return;
  }

  // RINEX Format
  // ------------
  int fieldLen = 19;
  double SVhealth = 0.0;
  double datasource = 0.0;

  int pos[4];
  pos[0] = (rnxVersion <= 2.12) ? 3 : 4;
  pos[1] = pos[0] + fieldLen;
  pos[2] = pos[1] + fieldLen;
  pos[3] = pos[2] + fieldLen;

  // Read eight lines
  // ----------------
  for (int iLine = 0; iLine < nLines; iLine++) {
    QString line = lines[iLine];

    if (iLine == 0) {
      QTextStream in(line.left(pos[1]).toLatin1());

      int year, month, day, hour, min;
      double sec;

      QString prnStr, n;
      in >> prnStr;
      if (prnStr.size() == 1 && prnStr[0] == 'E') {
        in >> n;
        prnStr.append(n);
      }
      if (prnStr.at(0) == 'E') {
        _prn.set('E', prnStr.mid(1).toInt());
      } else {
        _prn.set('E', prnStr.toInt());
      }

      in >> year >> month >> day >> hour >> min >> sec;
      if (year < 80) {
        year += 2000;
      } else if (year < 100) {
        year += 1900;
      }

      _TOC.set(year, month, day, hour, min, sec);

      if (   readDbl(line, pos[1], fieldLen, _clock_bias)
          || readDbl(line, pos[2], fieldLen, _clock_drift)
          || readDbl(line, pos[3], fieldLen, _clock_driftrate)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 1
    // =====================
    else if (iLine == 1) {
      if (   readDbl(line, pos[0], fieldLen, _IODnav)
          || readDbl(line, pos[1], fieldLen, _Crs)
          || readDbl(line, pos[2], fieldLen, _Delta_n)
          || readDbl(line, pos[3], fieldLen, _M0)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 2
    // =====================
    else if (iLine == 2) {
      if (   readDbl(line, pos[0], fieldLen, _Cuc)
          || readDbl(line, pos[1], fieldLen, _e)
          || readDbl(line, pos[2], fieldLen, _Cus)
          || readDbl(line, pos[3], fieldLen, _sqrt_A)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 3
    // =====================
    else if (iLine == 3) {
      if (   readDbl(line, pos[0], fieldLen, _TOEsec)
          || readDbl(line, pos[1], fieldLen, _Cic)
          || readDbl(line, pos[2], fieldLen, _OMEGA0)
          || readDbl(line, pos[3], fieldLen, _Cis)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 4
    // =====================
    else if (iLine == 4) {
      if (   readDbl(line, pos[0], fieldLen, _i0)
          || readDbl(line, pos[1], fieldLen, _Crc)
          || readDbl(line, pos[2], fieldLen, _omega)
          || readDbl(line, pos[3], fieldLen, _OMEGADOT)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 5
    // =====================
    else if (iLine == 5) {
      if (   readDbl(line, pos[0], fieldLen, _IDOT)
          || readDbl(line, pos[1], fieldLen, datasource)
          || readDbl(line, pos[2], fieldLen, _TOEweek)) {
        _checkState = bad;
        return;
      }
      else {
        if (bitExtracted(unsigned(datasource), 1, 8)) {
          _fnav = true;
          _type = t_eph::FNAV;
          _inav = false;
          /* set unused I/NAV values */
          _E5b_HS = 0.0;
          _E1B_HS = 0.0;
          _E1B_DataInvalid = false;
          _E5b_DataInvalid = false;
        }
        if (bitExtracted(unsigned(datasource), 1, 9)) {
          _fnav = false;
          _inav = true;
          _type = t_eph::INAV;
          /* set unused F/NAV values */
          _E5a_HS = 0.0;
          _E5a_DataInvalid = false;
        }
        // GAL week # in RINEX is aligned/identical to continuous GPS week # used in RINEX
        // but GST week # started at the first GPS roll-over (continuous GPS week 1024)
        _TOEweek -= 1024.0;
      }
    }
    // =====================
    // BROADCAST ORBIT - 6
    // =====================
    else if (iLine == 6) {
      if (   readDbl(line, pos[0], fieldLen, _SISA)
          || readDbl(line, pos[1], fieldLen, SVhealth)
          || readDbl(line, pos[2], fieldLen, _BGD_1_5A)
          || readDbl(line, pos[3], fieldLen, _BGD_1_5B)) {
        _checkState = bad;
        return;
      } else {
        // Bit 0
        _E1B_DataInvalid = bitExtracted(int(SVhealth), 1, 0);
        // Bit 1-2
        _E1B_HS =   double(bitExtracted(int(SVhealth), 2, 1));
        // Bit 3
        _E5a_DataInvalid = bitExtracted(int(SVhealth), 1, 3);
        // Bit 4-5
        _E5a_HS =   double(bitExtracted(int(SVhealth), 2, 4));
        // Bit 6
        _E5b_DataInvalid = bitExtracted(int(SVhealth), 1, 6);
        // Bit 7-8
        _E5b_HS =   double(bitExtracted(int(SVhealth), 2, 7));
        if (_fnav) {
          _BGD_1_5B = 0.0;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 7
    // =====================
    else if (iLine == 7) {
      if (readDbl(line, pos[0], fieldLen, _TOT)) {
        _checkState = bad;
        return;
      }
    }
  }
  _prn.setFlag(type());
}

// Compute Galileo Satellite Position (virtual)
////////////////////////////////////////////////////////////////////////////
t_irc t_ephGal::position(int GPSweek, double GPSweeks, double *xc,
    double *vv) const {

  static const double omegaEarth = 7292115.1467e-11;
  static const double gmWGS = 398.6004418e12;

  memset(xc, 0, 6 * sizeof(double));
  memset(vv, 0, 3 * sizeof(double));

  double a0 = _sqrt_A * _sqrt_A;
  if (a0 == 0) {
    return failure;
  }

  double n0 = sqrt(gmWGS / (a0 * a0 * a0));

  bncTime tt(GPSweek, GPSweeks);
  double tk = tt - bncTime(_TOC.gpsw(), _TOEsec);

  double n = n0 + _Delta_n;
  double M = _M0 + n * tk;
  double E = M;
  double E_last;
  int nLoop = 0;
  do {
    E_last = E;
    E = M + _e * sin(E);

    if (++nLoop == 100) {
      return failure;
    }
  } while (fabs(E - E_last) * a0 > 0.001);
  double v = 2.0 * atan(sqrt((1.0 + _e) / (1.0 - _e)) * tan(E / 2));
  double u0 = v + _omega;
  double sin2u0 = sin(2 * u0);
  double cos2u0 = cos(2 * u0);
  double r = a0 * (1 - _e * cos(E)) + _Crc * cos2u0 + _Crs * sin2u0;
  double i = _i0 + _IDOT * tk + _Cic * cos2u0 + _Cis * sin2u0;
  double u = u0 + _Cuc * cos2u0 + _Cus * sin2u0;
  double xp = r * cos(u);
  double yp = r * sin(u);
  double OM = _OMEGA0 + (_OMEGADOT - omegaEarth) * tk - omegaEarth * _TOEsec;

  double sinom = sin(OM);
  double cosom = cos(OM);
  double sini = sin(i);
  double cosi = cos(i);
  xc[0] = xp * cosom - yp * cosi * sinom;
  xc[1] = xp * sinom + yp * cosi * cosom;
  xc[2] = yp * sini;

  double tc = tt - _TOC;
  xc[3] = _clock_bias + _clock_drift * tc + _clock_driftrate * tc * tc;

  // Velocity
  // --------
  double tanv2 = tan(v / 2);
  double dEdM = 1 / (1 - _e * cos(E));
  double dotv = sqrt((1.0 + _e) / (1.0 - _e)) / cos(E / 2) / cos(E / 2)
      / (1 + tanv2 * tanv2) * dEdM * n;
  double dotu = dotv + (-_Cuc * sin2u0 + _Cus * cos2u0) * 2 * dotv;
  double dotom = _OMEGADOT - omegaEarth;
  double doti = _IDOT + (-_Cic * sin2u0 + _Cis * cos2u0) * 2 * dotv;
  double dotr = a0 * _e * sin(E) * dEdM * n
      + (-_Crc * sin2u0 + _Crs * cos2u0) * 2 * dotv;
  double dotx = dotr * cos(u) - r * sin(u) * dotu;
  double doty = dotr * sin(u) + r * cos(u) * dotu;

  vv[0] = cosom * dotx - cosi * sinom * doty      // dX / dr
  - xp * sinom * dotom - yp * cosi * cosom * dotom   // dX / dOMEGA
  + yp * sini * sinom * doti;        // dX / di

  vv[1] = sinom * dotx + cosi * cosom * doty + xp * cosom * dotom
      - yp * cosi * sinom * dotom - yp * sini * cosom * doti;

  vv[2] = sini * doty + yp * cosi * doti;

  // Relativistic Correction
  // -----------------------
  xc[3] -= 4.442807309e-10 * _e * sqrt(a0) * sin(E);

  xc[4] = _clock_drift + _clock_driftrate * tc;
  xc[5] = _clock_driftrate;

  return success;
}

// Health status of Galileo Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephGal::isUnhealthy() const {
  // SHS; 1 = Out of Service, 3 = In Test, 0 = Signal Ok, 2 = Extended Operations Mode
  if (_E5a_HS == 1 || _E5a_HS == 3 ||
      _E5b_HS == 1 || _E5b_HS == 3 ||
      _E1B_HS == 1 || _E1B_HS == 3) {
    return 1;
  }
  if (_E5a_DataInvalid ||
      _E5b_DataInvalid ||
      _E1B_DataInvalid) {
    return 1;
  }
  if (_SISA == 255.0) { // NAPA: No Accuracy Prediction Available
    return 1;
  }
  /*
   * SDD v1.3: SHS=2 leads to a newly-defined "EOM" status.
   * It also means that the satellite signal may be used for PNT.
   if (_E5aHS  == 2 ||
       _E5bHS  == 2 ||
       _E1_bHS == 2 ) {
     return 1;
   }
   */
  return 0;

}

// RINEX Format String
//////////////////////////////////////////////////////////////////////////////
QString t_ephGal::toString(double version) const {

  QString ephStr = typeStr(_type, _prn, version);
  QString rnxStr = ephStr + rinexDateStr(_TOC, _prn, version);

  QTextStream out(&rnxStr);

  out
      << QString("%1%2%3\n").arg(_clock_bias, 19, 'e', 12).arg(_clock_drift, 19,
          'e', 12).arg(_clock_driftrate, 19, 'e', 12);

  QString fmt = version < 3.0 ? "   %1%2%3%4\n" : "    %1%2%3%4\n";
  // =====================
  // BROADCAST ORBIT - 1
  // =====================
  out
      << QString(fmt)
      .arg(_IODnav,  19, 'e', 12)
      .arg(_Crs,     19, 'e', 12)
      .arg(_Delta_n, 19, 'e', 12)
      .arg(_M0, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 2
  // =====================
  out
      << QString(fmt)
      .arg(_Cuc,    19, 'e', 12).
      arg(_e,       19, 'e', 12)
      .arg(_Cus,    19, 'e', 12)
      .arg(_sqrt_A, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 3
  // =====================
  out
      << QString(fmt).
      arg(_TOEsec,  19, 'e', 12)
      .arg(_Cic,    19, 'e', 12)
      .arg(_OMEGA0, 19, 'e', 12)
      .arg(_Cis,    19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 4
  // =====================
  out
      << QString(fmt)
      .arg(_i0,       19, 'e', 12)
      .arg(_Crc,      19, 'e', 12)
      .arg(_omega,    19, 'e', 12)
      .arg(_OMEGADOT, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 5/6
  // =====================
  int dataSource = 0;
  int SVhealth = 0;
  double BGD_1_5A = _BGD_1_5A;
  double BGD_1_5B = _BGD_1_5B;
  if (_fnav) {
    dataSource |= (1 << 1);
    dataSource |= (1 << 8);
    BGD_1_5B = 0.0;
    // SVhealth
    //   Bit 3  : E5a DVS
    if (_E5a_DataInvalid) {
      SVhealth |= (1 << 3);
    }
    //   Bit 4-5: E5a HS
    if      (_E5a_HS == 1.0) {
      SVhealth |= (1 << 4);
    }
    else if (_E5a_HS == 2.0) {
      SVhealth |= (1 << 5);
    }
    else if (_E5a_HS == 3.0) {
      SVhealth |= (1 << 4);
      SVhealth |= (1 << 5);
    }
  } else if (_inav) {
    // Bit 2 and 0 are set because from MT1046 the data source cannot be determined
    // and RNXv3.03 says both can be set if the navigation messages were merged
    dataSource |= (1 << 0);
    dataSource |= (1 << 2);
    dataSource |= (1 << 9);
    // SVhealth
    //   Bit 0  : E1-B DVS
    if (_E1B_DataInvalid) {
      SVhealth |= (1 << 0);
    }
    //   Bit 1-2: E1-B HS
    if      (_E1B_HS == 1.0) {
      SVhealth |= (1 << 1);
    }
    else if (_E1B_HS == 2.0) {
      SVhealth |= (1 << 2);
    }
    else if (_E1B_HS == 3.0) {
      SVhealth |= (1 << 1);
      SVhealth |= (1 << 2);
    }
    //   Bit 6  : E5b DVS
    if (_E5b_DataInvalid) {
      SVhealth |= (1 << 6);
    }
    //   Bit 7-8: E5b HS
    if (_E5b_HS == 1.0) {
      SVhealth |= (1 << 7);
    }
    else if (_E5b_HS == 2.0) {
      SVhealth |= (1 << 8);
    }
    else if (_E5b_HS == 3.0) {
      SVhealth |= (1 << 7);
      SVhealth |= (1 << 8);
    }
  }
  // =====================
  // BROADCAST ORBIT - 5
  // =====================
  out
      << QString(fmt)
      .arg(_IDOT,              19, 'e', 12)
      .arg(double(dataSource), 19, 'e', 12)
      .arg(_TOEweek + 1024.0,  19, 'e', 12)
      .arg(0.0, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 6
  // =====================
  out
      << QString(fmt)
      .arg(_SISA,            19, 'e', 12)
      .arg(double(SVhealth), 19, 'e', 12)
      .arg(BGD_1_5A,         19, 'e', 12)
      .arg(BGD_1_5B,         19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 7
  // =====================
  double tot = _TOT;
  if (tot == 0.9999e9 && version < 3.0) {
    tot = 0.0;
  }
  out
      << QString(fmt)
      .arg(tot, 19, 'e', 12)
      .arg("",  19, QChar(' '))
      .arg("",  19, QChar(' '))
      .arg("",  19, QChar(' '));

  return rnxStr;
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_ephSBAS::t_ephSBAS(double rnxVersion, const QStringList &lines, const QString typeStr) {

  setType(typeStr);

  const int nLines = 4;

  // Source RINEX version < 4
  if (type() == t_eph::undefined) {
    _type = t_eph::SBASL1;
  }

  if (lines.size() != nLines) {
    _checkState = bad;
    return;
  }

  // RINEX Format
  // ------------
  int fieldLen = 19;

  int pos[4];
  pos[0] = (rnxVersion <= 2.12) ? 3 : 4;
  pos[1] = pos[0] + fieldLen;
  pos[2] = pos[1] + fieldLen;
  pos[3] = pos[2] + fieldLen;

  // Read four lines
  // ---------------
  for (int iLine = 0; iLine < nLines; iLine++) {
    QString line = lines[iLine];

    if (iLine == 0) {
      QTextStream in(line.left(pos[1]).toLatin1());

      int year, month, day, hour, min;
      double sec;

      QString prnStr, n;
      in >> prnStr;
      if (prnStr.size() == 1 && prnStr[0] == 'S') {
        in >> n;
        prnStr.append(n);
      }
      if (prnStr.at(0) == 'S') {
        _prn.set('S', prnStr.mid(1).toInt());
      } else {
        _prn.set('S', prnStr.toInt());
      }
      _prn.setFlag(type());

      in >> year >> month >> day >> hour >> min >> sec;

      if (year < 80) {
        year += 2000;
      } else if (year < 100) {
        year += 1900;
      }

      _TOC.set(year, month, day, hour, min, sec);

      if (   readDbl(line, pos[1], fieldLen, _agf0)
          || readDbl(line, pos[2], fieldLen, _agf1)
          || readDbl(line, pos[3], fieldLen, _TOT)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 1
    // =====================
    else if (iLine == 1) {
      if (   readDbl(line, pos[0], fieldLen, _x_pos)
          || readDbl(line, pos[1], fieldLen, _x_vel)
          || readDbl(line, pos[2], fieldLen, _x_acc)
          || readDbl(line, pos[3], fieldLen, _health)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 2
    // =====================
    else if (iLine == 2) {
      if (   readDbl(line, pos[0], fieldLen, _y_pos)
          || readDbl(line, pos[1], fieldLen, _y_vel)
          || readDbl(line, pos[2], fieldLen, _y_acc)
          || readDbl(line, pos[3], fieldLen, _ura)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 3
    // =====================
    else if (iLine == 3) {
      double iodn;
      if (   readDbl(line, pos[0], fieldLen, _z_pos)
          || readDbl(line, pos[1], fieldLen, _z_vel)
          || readDbl(line, pos[2], fieldLen, _z_acc)
          || readDbl(line, pos[3], fieldLen, iodn)) {
        _checkState = bad;
        return;
      } else {
        _IODN = int(iodn);
      }
    }
  }
  _x_pos *= 1.e3;
  _y_pos *= 1.e3;
  _z_pos *= 1.e3;
  _x_vel *= 1.e3;
  _y_vel *= 1.e3;
  _z_vel *= 1.e3;
  _x_acc *= 1.e3;
  _y_acc *= 1.e3;
  _z_acc *= 1.e3;
}

// IOD of SBAS Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////

unsigned int t_ephSBAS::IOD() const {
  unsigned char buffer[80];
  int size = 0;
  int numbits = 0;
  long long bitbuffer = 0;
  unsigned char *startbuffer = buffer;

  SBASADDBITSFLOAT(30, this->_x_pos, 0.08)
  SBASADDBITSFLOAT(30, this->_y_pos, 0.08)
  SBASADDBITSFLOAT(25, this->_z_pos, 0.4)
  SBASADDBITSFLOAT(17, this->_x_vel, 0.000625)
  SBASADDBITSFLOAT(17, this->_y_vel, 0.000625)
  SBASADDBITSFLOAT(18, this->_z_vel, 0.004)
  SBASADDBITSFLOAT(10, this->_x_acc, 0.0000125)
  SBASADDBITSFLOAT(10, this->_y_acc, 0.0000125)
  SBASADDBITSFLOAT(10, this->_z_acc, 0.0000625)
  SBASADDBITSFLOAT(12, this->_agf0,
      1.0 / static_cast<double>(1 << 30) / static_cast<double>(1 << 1))
  SBASADDBITSFLOAT(8, this->_agf1,
      1.0 / static_cast<double>(1 << 30) / static_cast<double>(1 << 10))
  SBASADDBITS(5, 0); // the last byte is filled by 0-bits to obtain a length of an integer multiple of 8

  return CRC24(size, startbuffer);
}

// Compute SBAS Satellite Position (virtual)
////////////////////////////////////////////////////////////////////////////
t_irc t_ephSBAS::position(int GPSweek, double GPSweeks, double *xc,
    double *vv) const {

  bncTime tt(GPSweek, GPSweeks);
  double dt = tt - _TOC;

  xc[0] = _x_pos + _x_vel * dt + _x_acc * dt * dt / 2.0;
  xc[1] = _y_pos + _y_vel * dt + _y_acc * dt * dt / 2.0;
  xc[2] = _z_pos + _z_vel * dt + _z_acc * dt * dt / 2.0;

  vv[0] = _x_vel + _x_acc * dt;
  vv[1] = _y_vel + _y_acc * dt;
  vv[2] = _z_vel + _z_acc * dt;

  xc[3] = _agf0 + _agf1 * dt;

  xc[4] = _agf1;
  xc[5] = 0.0;

  return success;
}

// Health status of SBAS Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephSBAS::isUnhealthy() const {

// Bit 5
  bool URAindexIs15 =  bitExtracted(int(_health), 1, 5);
  if (URAindexIs15) {
    // in this case it is recommended
    // to set the bits 0,1,2,3 to 1 (MT17health = 15)
    return 1;
  }

  // Bit 4
  bool MT17HealthIsUnavailable = bitExtracted(int(_health), 1, 4);
  if (MT17HealthIsUnavailable) {
    return 0;
  }

  // Bit 0-3
  int MT17health = bitExtracted(int(_health), 4, 0);
  if (MT17health) {
    return 1;
  }

  return 0;
}

// RINEX Format String
//////////////////////////////////////////////////////////////////////////////
QString t_ephSBAS::toString(double version) const {

  QString ephStr = typeStr(_type, _prn, version);
  QString rnxStr = ephStr + rinexDateStr(_TOC, _prn, version);

  QTextStream out(&rnxStr);

  out
      << QString("%1%2%3\n")
      .arg(_agf0, 19, 'e', 12)
      .arg(_agf1, 19, 'e', 12)
      .arg(_TOT,  19, 'e', 12);

  QString fmt = version < 3.0 ? "   %1%2%3%4\n" : "    %1%2%3%4\n";
  // =====================
  // BROADCAST ORBIT - 1
  // =====================
  out
      << QString(fmt)
      .arg(1.e-3 * _x_pos, 19, 'e', 12)
      .arg(1.e-3 * _x_vel, 19, 'e', 12)
      .arg(1.e-3 * _x_acc, 19, 'e', 12)
      .arg(_health,        19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 2
  // =====================
  out
      << QString(fmt)
      .arg(1.e-3 * _y_pos, 19, 'e', 12)
      .arg(1.e-3 * _y_vel, 19, 'e', 12)
      .arg(1.e-3 * _y_acc, 19, 'e', 12)
      .arg(_ura,           19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 3
  // =====================
  out
      << QString(fmt)
      .arg(1.e-3 * _z_pos, 19, 'e', 12)
      .arg(1.e-3 * _z_vel, 19, 'e', 12)
      .arg(1.e-3 * _z_acc, 19, 'e', 12)
      .arg(double(_IODN),  19, 'e', 12);

  return rnxStr;
}

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_ephBDS::t_ephBDS(double rnxVersion, const QStringList &lines, const QString typeStr) {

  setType(typeStr);

  int nLines = 8;

  if (type() == t_eph::CNV1 ||
      type() == t_eph::CNV2) {
    nLines += 2;
  }
  if (type() == t_eph::CNV3) {
    nLines += 1;
  }

  if (lines.size() != nLines) {
    _checkState = bad;
    return;
  }

  // RINEX Format
  // ------------
  int fieldLen = 19;

  int pos[4];
  pos[0] = (rnxVersion <= 2.12) ? 3 : 4;
  pos[1] = pos[0] + fieldLen;
  pos[2] = pos[1] + fieldLen;
  pos[3] = pos[2] + fieldLen;

  // Read eight lines
  // ----------------
  for (int iLine = 0; iLine < nLines; iLine++) {
    QString line = lines[iLine];

    if (iLine == 0) {
      QTextStream in(line.left(pos[1]).toLatin1());

      int year, month, day, hour, min;
      double sec;

      QString prnStr, n;
      in >> prnStr;
      if (prnStr.size() == 1 && prnStr[0] == 'C') {
        in >> n;
        prnStr.append(n);
      }
      if (prnStr.at(0) == 'C') {
        _prn.set('C', prnStr.mid(1).toInt());
      } else {
        _prn.set('C', prnStr.toInt());
      }

      in >> year >> month >> day >> hour >> min >> sec;
      if (year < 80) {
        year += 2000;
      } else if (year < 100) {
        year += 1900;
      }

      _TOC.setBDS(year, month, day, hour, min, sec);

      if (   readDbl(line, pos[1], fieldLen, _clock_bias)
          || readDbl(line, pos[2], fieldLen, _clock_drift)
          || readDbl(line, pos[3], fieldLen, _clock_driftrate)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 1
    // =====================
    else if (iLine == 1) {
      if (type() == t_eph::D1 ||
          type() == t_eph::D2 ||
          type() == t_eph::undefined) {
        double aode;
        if (   readDbl(line, pos[0], fieldLen, aode)
            || readDbl(line, pos[1], fieldLen, _Crs)
            || readDbl(line, pos[2], fieldLen, _Delta_n)
            || readDbl(line, pos[3], fieldLen, _M0)) {
          _checkState = bad;
          return;
        }
        _AODE = int(aode);
      }
      else { //CNV1, CNV2, CNV3
        if (   readDbl(line, pos[0], fieldLen, _ADOT)
            || readDbl(line, pos[1], fieldLen, _Crs)
            || readDbl(line, pos[2], fieldLen, _Delta_n)
            || readDbl(line, pos[3], fieldLen, _M0)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 2
    // =====================
    else if (iLine == 2) {
      if (   readDbl(line, pos[0], fieldLen, _Cuc)
          || readDbl(line, pos[1], fieldLen, _e)
          || readDbl(line, pos[2], fieldLen, _Cus)
          || readDbl(line, pos[3], fieldLen, _sqrt_A)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 3
    // =====================
    else if (iLine == 3) {
      if (   readDbl(line, pos[0], fieldLen, _TOEsec)
          || readDbl(line, pos[1], fieldLen, _Cic)
          || readDbl(line, pos[2], fieldLen, _OMEGA0)
          || readDbl(line, pos[3], fieldLen, _Cis)) {
        _checkState = bad;
        return;
      }
    }
    // =====================
    // BROADCAST ORBIT - 4
    // =====================
    else if (iLine == 4) {
      if (   readDbl(line, pos[0], fieldLen, _i0)
          || readDbl(line, pos[1], fieldLen, _Crc)
          || readDbl(line, pos[2], fieldLen, _omega)
          || readDbl(line, pos[3], fieldLen, _OMEGADOT)) {
        _checkState = bad;
        return;
      }
      else {
        // Source RINEX version < 4
        if (type() == t_eph::undefined) {
          const double iMaxGEO = 10.0 / 180.0 * M_PI;
          if (_i0 > iMaxGEO) {
            _type = t_eph::D1;
          }
          else {
            _type = t_eph::D2;
          }
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 5
    // =====================
    else if (iLine == 5) {
      if (type() == t_eph::CNV1 ||
          type() == t_eph::CNV2 ||
          type() == t_eph::CNV3) {
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[1], fieldLen, _Delta_n_dot)
            || readDbl(line, pos[2], fieldLen, _satType)
            || readDbl(line, pos[3], fieldLen, _top)) {
          _checkState = bad;
          return;
        }
      }
      else { // D1, D2
        if (   readDbl(line, pos[0], fieldLen, _IDOT)
            || readDbl(line, pos[2], fieldLen, _BDTweek)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 6
    // =====================
    else if (iLine == 6) {
      if (type() == t_eph::CNV1 ||
          type() == t_eph::CNV2 ||
          type() == t_eph::CNV3) {
        if (   readDbl(line, pos[0], fieldLen, _SISAI_oe)
            || readDbl(line, pos[1], fieldLen, _SISAI_ocb)
            || readDbl(line, pos[2], fieldLen, _SISAI_oc1)
            || readDbl(line, pos[3], fieldLen, _SISAI_oc2)) {
          _checkState = bad;
          return;
        }
      }
      else { // D1, D2
        double SatH1;
        if (   readDbl(line, pos[0], fieldLen, _ura)
            || readDbl(line, pos[1], fieldLen, SatH1)
            || readDbl(line, pos[2], fieldLen, _TGD1)
            || readDbl(line, pos[3], fieldLen, _TGD2)) {
          _checkState = bad;
          return;
        }
        _SatH1 = int(SatH1);
      }
    }
    // =====================
    // BROADCAST ORBIT - 7
    // =====================
    else if (iLine == 7) {
      if (type() == t_eph::CNV1) {
        if (   readDbl(line, pos[0], fieldLen, _ISC_B1Cd)
            || readDbl(line, pos[2], fieldLen, _TGD_B1Cp)
            || readDbl(line, pos[3], fieldLen, _TGD_B2ap)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::CNV2) {
        if (   readDbl(line, pos[1], fieldLen, _ISC_B2ad)
            || readDbl(line, pos[2], fieldLen, _TGD_B1Cp)
            || readDbl(line, pos[3], fieldLen, _TGD_B2ap)) {
          _checkState = bad;
          return;
        }
      }
      else if (type() == t_eph::CNV3) {
        double health;
        if (   readDbl(line, pos[0], fieldLen, _SISMAI)
            || readDbl(line, pos[1], fieldLen, health)
            || readDbl(line, pos[2], fieldLen, _INTEGRITYF_B2b)
            || readDbl(line, pos[3], fieldLen, _TGD_B2bI)) {
          _checkState = bad;
          return;
        }
        _health = int(health);
      }
      else { // D1, D2
        double aodc;
        if (   readDbl(line, pos[0], fieldLen, _TOT)
            || readDbl(line, pos[1], fieldLen, aodc)) {
          _checkState = bad;
          return;
        }
        if (_TOT == 0.9999e9) {  // 0.9999e9 means not known (RINEX standard)
          _TOT = _TOEsec;
        }
        _AODC = int(aodc);
      }
    }
    // =====================
    // BROADCAST ORBIT - 8
    // =====================
    else if (iLine == 8) {
      double health;
      if (type() == t_eph::CNV1) {
        if (   readDbl(line, pos[0], fieldLen, _SISMAI)
            || readDbl(line, pos[1], fieldLen, health)
            || readDbl(line, pos[2], fieldLen, _INTEGRITYF_B1C)
            || readDbl(line, pos[3], fieldLen, _IODC)) {
          _checkState = bad;
          return;
        }
        _health = int(health);
      }
      else if (type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _SISMAI)
            || readDbl(line, pos[1], fieldLen, health)
            || readDbl(line, pos[2], fieldLen, _INTEGRITYF_B2aB1C)
            || readDbl(line, pos[3], fieldLen, _IODC)) {
          _checkState = bad;
          return;
        }
        _health = int(health);
      }
      else if (type() == t_eph::CNV3) {
        if (readDbl(line, pos[0], fieldLen, _TOT)) {
          _checkState = bad;
          return;
        }
      }
    }
    // =====================
    // BROADCAST ORBIT - 9
    // =====================
    else if (iLine == 9) {
      if (type() == t_eph::CNV1 ||
          type() == t_eph::CNV2) {
        if (   readDbl(line, pos[0], fieldLen, _TOT)
            || readDbl(line, pos[3], fieldLen, _IODE)) {
          _checkState = bad;
          return;
        }
      }
    }
  }
  _prn.setFlag(type());

  _TOE.setBDS(int(_BDTweek), _TOEsec);
  // remark: actually should be computed from second_tot
  //         but it seems to be unreliable in RINEX files
  //_TOT = _TOC.bdssec();
}

// IOD of BDS Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephBDS::IOD() const {
  return (int(_TOC.gpssec()) / 720) % 240;   //return (int(_TOEsec)/720) % 240;
}

// Compute BDS Satellite Position (virtual)
//////////////////////////////////////////////////////////////////////////////
t_irc t_ephBDS::position(int GPSweek, double GPSweeks, double *xc,
    double *vv) const {

  static const double gmBDS = 398.6004418e12;
  static const double omegaBDS = 7292115.0000e-11;

  xc[0] = xc[1] = xc[2] = xc[3] = 0.0;
  vv[0] = vv[1] = vv[2] = 0.0;

  bncTime tt(GPSweek, GPSweeks);

  if (_sqrt_A == 0) {
    return failure;
  }
  double a0 = _sqrt_A * _sqrt_A;

  double n0 = sqrt(gmBDS / (a0 * a0 * a0));
  double tk = tt - _TOE;
  double n = n0 + _Delta_n;
  double M = _M0 + n * tk;
  double E = M;
  double E_last;
  int nLoop = 0;
  do {
    E_last = E;
    E = M + _e * sin(E);

    if (++nLoop == 100) {
      return failure;
    }
  } while (fabs(E - E_last) * a0 > 0.001);

  double v = atan2(sqrt(1 - _e * _e) * sin(E), cos(E) - _e);
  double u0 = v + _omega;
  double sin2u0 = sin(2 * u0);
  double cos2u0 = cos(2 * u0);
  double r = a0 * (1 - _e * cos(E)) + _Crc * cos2u0 + _Crs * sin2u0;
  double i = _i0 + _IDOT * tk + _Cic * cos2u0 + _Cis * sin2u0;
  double u = u0 + _Cuc * cos2u0 + _Cus * sin2u0;
  double xp = r * cos(u);
  double yp = r * sin(u);
  double toesec = (_TOE.gpssec() - 14.0);
  double sinom = 0;
  double cosom = 0;
  double sini = 0;
  double cosi = 0;

  // Velocity
  // --------
  double tanv2 = tan(v / 2);
  double dEdM = 1 / (1 - _e * cos(E));
  double dotv = sqrt((1.0 + _e) / (1.0 - _e)) / cos(E / 2) / cos(E / 2)
      / (1 + tanv2 * tanv2) * dEdM * n;
  double dotu = dotv + (-_Cuc * sin2u0 + _Cus * cos2u0) * 2 * dotv;
  double doti = _IDOT + (-_Cic * sin2u0 + _Cis * cos2u0) * 2 * dotv;
  double dotr = a0 * _e * sin(E) * dEdM * n
      + (-_Crc * sin2u0 + _Crs * cos2u0) * 2 * dotv;

  double dotx = dotr * cos(u) - r * sin(u) * dotu;
  double doty = dotr * sin(u) + r * cos(u) * dotu;

  const double iMaxGEO = 10.0 / 180.0 * M_PI;

  // MEO/IGSO satellite
  // ------------------
  if (_i0 > iMaxGEO) {
    double OM = _OMEGA0 + (_OMEGADOT - omegaBDS) * tk - omegaBDS * toesec;

    sinom = sin(OM);
    cosom = cos(OM);
    sini = sin(i);
    cosi = cos(i);

    xc[0] = xp * cosom - yp * cosi * sinom;
    xc[1] = xp * sinom + yp * cosi * cosom;
    xc[2] = yp * sini;

    // Velocity
    // --------

    double dotom = _OMEGADOT - t_CST::omega;

    vv[0] = cosom * dotx - cosi * sinom * doty    // dX / dr
    - xp * sinom * dotom - yp * cosi * cosom * dotom   // dX / dOMEGA
    + yp * sini * sinom * doti;   // dX / di

    vv[1] = sinom * dotx + cosi * cosom * doty + xp * cosom * dotom
        - yp * cosi * sinom * dotom - yp * sini * cosom * doti;

    vv[2] = sini * doty + yp * cosi * doti;

  }

  // GEO satellite
  // -------------
  else {
    double OM = _OMEGA0 + _OMEGADOT * tk - omegaBDS * toesec;
    double ll = omegaBDS * tk;

    sinom = sin(OM);
    cosom = cos(OM);
    sini = sin(i);
    cosi = cos(i);

    double xx = xp * cosom - yp * cosi * sinom;
    double yy = xp * sinom + yp * cosi * cosom;
    double zz = yp * sini;

    Matrix RX = BNC_PPP::t_astro::rotX(-5.0 / 180.0 * M_PI);
    Matrix RZ = BNC_PPP::t_astro::rotZ(ll);

    ColumnVector X1(3);
    X1 << xx << yy << zz;
    ColumnVector X2 = RZ * RX * X1;

    xc[0] = X2(1);
    xc[1] = X2(2);
    xc[2] = X2(3);

    double dotom = _OMEGADOT;

    double vx = cosom * dotx - cosi * sinom * doty - xp * sinom * dotom
        - yp * cosi * cosom * dotom + yp * sini * sinom * doti;

    double vy = sinom * dotx + cosi * cosom * doty + xp * cosom * dotom
        - yp * cosi * sinom * dotom - yp * sini * cosom * doti;

    double vz = sini * doty + yp * cosi * doti;

    ColumnVector V(3);
    V << vx << vy << vz;

    Matrix RdotZ(3, 3);
    double C = cos(ll);
    double S = sin(ll);
    Matrix UU(3, 3);
    UU[0][0] = -S;
    UU[0][1] = +C;
    UU[0][2] = 0.0;
    UU[1][0] = -C;
    UU[1][1] = -S;
    UU[1][2] = 0.0;
    UU[2][0] = 0.0;
    UU[2][1] = 0.0;
    UU[2][2] = 0.0;
    RdotZ = omegaBDS * UU;

    ColumnVector VV(3);
    VV = RZ * RX * V + RdotZ * RX * X1;

    vv[0] = VV(1);
    vv[1] = VV(2);
    vv[2] = VV(3);
  }

  double tc = tt - _TOC;
  xc[3] = _clock_bias + _clock_drift * tc + _clock_driftrate * tc * tc;

//  dotC  = _clock_drift + _clock_driftrate*tc
//        - 4.442807309e-10*_e * sqrt(a0) * cos(E) * dEdM * n;

  // Relativistic Correction
  // -----------------------
  xc[3] -= 4.442807309e-10 * _e * sqrt(a0) * sin(E);

  xc[4] = _clock_drift + _clock_driftrate * tc;
  xc[5] = _clock_driftrate;

  return success;
}

// Health status of SBAS Ephemeris (virtual)
////////////////////////////////////////////////////////////////////////////
unsigned int t_ephBDS::isUnhealthy() const {

  if (type() == t_eph::CNV1 ||
      type() == t_eph::CNV2 ||
      type() == t_eph::CNV3) {
    return static_cast<unsigned int>(_health);
  }

  return static_cast<unsigned int>(_SatH1);

}

// RINEX Format String
//////////////////////////////////////////////////////////////////////////////
QString t_ephBDS::toString(double version) const {

  if (version < 4.0 &&
      (type() == t_eph::CNV1 ||
       type() == t_eph::CNV2 ||
       type() == t_eph::CNV3 )) {
    return "";
  }

  QString ephStr = typeStr(_type, _prn, version);
  QString rnxStr = ephStr + rinexDateStr(_TOC - 14.0, _prn, version);

  QTextStream out(&rnxStr);

  out
      << QString("%1%2%3\n")
      .arg(_clock_bias,      19, 'e', 12)
      .arg(_clock_drift, 19, 'e', 12)
      .arg(_clock_driftrate, 19, 'e', 12);

  QString fmt = version < 3.0 ? "   %1%2%3%4\n" : "    %1%2%3%4\n";
  // =====================
  // BROADCAST ORBIT - 1
  // =====================
  if (type() == t_eph::D1 ||
      type() == t_eph::D2 ||
      type() == t_eph::undefined) {
    out
        << QString(fmt)
        .arg(double(_AODE), 19, 'e', 12)
        .arg(_Crs,          19, 'e', 12)
        .arg(_Delta_n,      19, 'e', 12)
        .arg(_M0,           19, 'e', 12);
  }
  else { //CNV1, CNV2, CNV3
    out
        << QString(fmt)
        .arg(_ADOT,    19, 'e', 12)
        .arg(_Crs,     19, 'e', 12)
        .arg(_Delta_n, 19, 'e', 12)
        .arg(_M0,      19, 'e', 12);
  }

  // =====================
  // BROADCAST ORBIT - 2
  // =====================
  out
      << QString(fmt)
      .arg(_Cuc,    19, 'e', 12)
      .arg(_e,      19, 'e', 12)
      .arg(_Cus,    19, 'e', 12)
      .arg(_sqrt_A, 19, 'e', 12);

  // =====================
  // BROADCAST ORBIT - 3
  // =====================
  out
      << QString(fmt)
      .arg(_TOEsec, 19, 'e', 12)
      .arg(_Cic,    19, 'e', 12)
      .arg(_OMEGA0, 19, 'e', 12)
      .arg(_Cis,    19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 4
  // =====================
  out
      << QString(fmt)
      .arg(_i0,       19, 'e', 12)
      .arg(_Crc,      19, 'e', 12)
      .arg(_omega,    19, 'e', 12)
      .arg(_OMEGADOT, 19, 'e', 12);
  // =====================
  // BROADCAST ORBIT - 5
  // =====================
  if (type() == t_eph::CNV1 ||
      type() == t_eph::CNV2 ||
      type() == t_eph::CNV3) {
    out
        << QString(fmt)
        .arg(_IDOT,        19, 'e', 12)
        .arg(_Delta_n_dot, 19, 'e', 12)
        .arg(_satType,     19, 'e', 12)
        .arg(_top,         19, 'e', 12);
  }
  else { // D1, D2,
    out
        << QString(fmt)
        .arg(_IDOT,    19, 'e', 12)
        .arg("",       19, QChar(' '))
        .arg(_BDTweek, 19, 'e', 12)
        .arg("",       19, QChar(' '));
  }
  // =====================
  // BROADCAST ORBIT - 6
  // =====================
  if (type() == t_eph::CNV1 ||
      type() == t_eph::CNV2 ||
      type() == t_eph::CNV3) {
    out
        << QString(fmt)
        .arg(_SISAI_oe,  19, 'e', 12)
        .arg(_SISAI_ocb, 19, 'e', 12)
        .arg(_SISAI_oc1, 19, 'e', 12)
        .arg(_SISAI_oc2, 19, 'e', 12);
  }
  else { // D1, D2, undefined
    out
        << QString(fmt)
        .arg(_ura,           19, 'e', 12)
        .arg(double(_SatH1), 19, 'e', 12)
        .arg(_TGD1,          19, 'e', 12)
        .arg(_TGD2,          19, 'e', 12);
  }
  // =====================
  // BROADCAST ORBIT - 7
  // =====================
  if (type() == t_eph::CNV1) {
    out
        << QString(fmt)
        .arg(_ISC_B1Cd, 19, 'e', 12)
        .arg("",        19, QChar(' '))
        .arg(_TGD_B1Cp, 19, 'e', 12)
        .arg(_TGD_B2ap, 19, 'e', 12);
  }
  else if (type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg("", 19, QChar(' '))
        .arg(_ISC_B2ad, 19, 'e', 12)
        .arg(_TGD_B1Cp, 19, 'e', 12)
        .arg(_TGD_B2ap, 19, 'e', 12);
  }
  else if (type() == t_eph::CNV3) {
    out
        << QString(fmt)
        .arg(_SISMAI,         19, 'e', 12)
        .arg(double(_health), 19, 'e', 12)
        .arg(_INTEGRITYF_B2b, 19, 'e', 12)
        .arg(_TGD_B2bI,       19, 'e', 12);
  }
  else { // D1, D2, undefined
    double tots = 0.0;
    if (_receptDateTime.isValid()) { // RTCM stream input
      tots = _TOE.bdssec();
    } else { // RINEX input
      tots = _TOT;
    }
    out
        << QString(fmt)
        .arg(tots,          19, 'e', 12)
        .arg(double(_AODC), 19, 'e', 12)
        .arg("",            19, QChar(' '))
        .arg("",            19, QChar(' '));
  }

  // =====================
  // BROADCAST ORBIT - 8
  // =====================
  if (type() == t_eph::CNV1) {
    out
        << QString(fmt)
        .arg(_SISMAI,         19, 'e', 12)
        .arg(double(_health), 19, 'e', 12)
        .arg(_INTEGRITYF_B1C, 19, 'e', 12)
        .arg(_IODC,           19, 'e', 12);
  }
  else if (type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg(_SISMAI,            19, 'e', 12)
        .arg(double(_health),    19, 'e', 12)
        .arg(_INTEGRITYF_B2aB1C, 19, 'e', 12)
        .arg(_IODC,              19, 'e', 12);
  }
  else if (type() == t_eph::CNV3) {
    out
        << QString(fmt)
        .arg(_TOT, 19, 'e', 12)
        .arg("",   19, QChar(' '))
        .arg("",   19, QChar(' '))
        .arg("",   19, QChar(' '));
  }

  // =====================
  // BROADCAST ORBIT - 9
  // =====================
  if (type() == t_eph::CNV1 ||
      type() == t_eph::CNV2) {
    out
        << QString(fmt)
        .arg(_TOT,  19, 'e', 12)
        .arg("",    19, QChar(' '))
        .arg("",    19, QChar(' '))
        .arg(_IODE, 19, 'e', 12);
  }

  return rnxStr;
}
