// Part of BNC, a utility for retrieving decoding and
// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2007
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Czech Technical University Prague, Department of Geodesy
// http://www.fsv.cvut.cz
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncBiasSnx
 *
 * Purpose:    Biases from Bias SINEX File
 *
 * Author:     A. Stuerze
 *
 * Created:    15-Mar-2022
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include "bncbiassnx.h"

#include <iostream>
#include <newmatio.h>

using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
bncBiasSnx::bncBiasSnx() {
  bncSettings settings;
  _useGnss['G'] = (Qt::CheckState(settings.value("cmbGps").toInt()) == Qt::Checked) ? true : false;
  _useGnss['R'] = (Qt::CheckState(settings.value("cmbGlo").toInt()) == Qt::Checked) ? true : false;
  _useGnss['E'] = (Qt::CheckState(settings.value("cmbGal").toInt()) == Qt::Checked) ? true : false;
  _useGnss['C'] = (Qt::CheckState(settings.value("cmbBds").toInt()) == Qt::Checked) ? true : false;
  _useGnss['J'] = (Qt::CheckState(settings.value("cmbQzss").toInt()) == Qt::Checked) ? true : false;
  _useGnss['S'] = (Qt::CheckState(settings.value("cmbSbas").toInt()) == Qt::Checked) ? true : false;
  _useGnss['I'] = (Qt::CheckState(settings.value("cmbNavic").toInt()) == Qt::Checked) ? true : false;
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncBiasSnx::~bncBiasSnx() {
  clear();
}

// Clear Satellite Map
////////////////////////////////////////////////////////////////////////////
void bncBiasSnx::clear() {
  QMapIterator<QString,snxSatCodeBias*>it(_snxSatCodeBiasMap);
  while (it.hasNext()) {
    it.next();
    delete it.value();
  }
  _snxSatCodeBiasMap.clear();
}

// Print
////////////////////////////////////////////////////////////////////////////
void bncBiasSnx::print() const {
  QMapIterator<QString,snxSatCodeBias*> it(_snxSatCodeBiasMap);
  cout << "============= bncBiasSnx::print() ==========" << endl;
  while (it.hasNext()) {
    it.next();
    cout << it.key().toStdString() << " ";
    snxSatCodeBias* satCbMap = it.value();
    cout << satCbMap->_startTime.datestr() << "_" << satCbMap->_startTime.timestr() << " ";
    QMapIterator <QString, double> itB(satCbMap->_biasMap);
    cout << satCbMap->_biasMap.size() << " ";
    while (itB.hasNext()) {
      itB.next();
      QString osb  = itB.key();
      double  value = itB.value();
      cout << osb.toStdString() << " " << value << " ";
    }
    cout << endl;
  }
}


// Read Bias SINEX File
////////////////////////////////////////////////////////////////////////////
t_irc bncBiasSnx::readFile(const QString& fileName) {
  clear();
  QFile inFile(fileName);
  inFile.open(QIODevice::ReadOnly | QIODevice::Text);

  QTextStream in(&inFile);

  while ( !in.atEnd() ) {
    QString biasType, biasAtrib, svn, prn, station, obs1, obs2, startTimeStr, endTimeStr, unit;
    double value;
    bncTime startTime;

    QString line = in.readLine();//cout << line.toStdString() << endl;
    if      (line.indexOf("+BIAS/SOLUTION") == 14) {
      clear();
    }
    else if (line.indexOf("BIAS_MODE") == 1) {
       if      (line.contains("ABSOLUTE")) {
         _type = ABS;
       }
       else if (line.contains("RELATIVE")) {
         _type = REL;
       }
    }
    else if (line.indexOf("OSB") == 1 || line.indexOf("DSB") == 1) {
      QTextStream inLine(&line, QIODevice::ReadOnly);
      inLine >> biasType;
      if      (biasType == "OSB") {
        inLine >> svn >> prn >> obs1 >> startTimeStr >> endTimeStr >> unit >> value;
        biasAtrib = obs1;
      }
      else if (biasType == "DSB") {
        inLine >> svn;
        if (svn.size() == 1) { // station data
          continue;
        } else {
          inLine >> prn >> obs1 >> obs2 >> startTimeStr >> endTimeStr >> unit >> value;
          biasAtrib = QString("%1-%2").arg(obs1).arg(obs2);
        }
      }
      // bncTime from snx time
      // ---------------------
      if (bncTimeFromSinex(startTimeStr, startTime) != success) {
        continue;
      }
      // conversion of Code Biases
      // -------------------------
      if (unit.contains("ns")) {
        // from nano seconds into meters
        value /= 1.e9;
        value *= t_CST::c;

      }

      // delete existing entries if there are newer once available
      // ---------------------------------------------------------
      if (_snxSatCodeBiasMap.contains(prn)) {
        if (_snxSatCodeBiasMap[prn]->_startTime < startTime) {
          delete _snxSatCodeBiasMap[prn];
          _snxSatCodeBiasMap[prn] = new snxSatCodeBias(startTime);
        }
      }
      else {
        _snxSatCodeBiasMap[prn] = new snxSatCodeBias(startTime);
      }
      // save biases
      // -----------
      QMap<QString, double>& biasMap = _snxSatCodeBiasMap[prn]->_biasMap;
      if (!biasMap.contains(biasAtrib)) {
        biasMap.insert(biasAtrib, value);
      }
    }
  }
  inFile.close();

  if (_type == ABS) {
    getDsbFromOsb();
  }
  cleanDsb();
  //print();
  return success;
}

// bncTime from SINEX Time String
////////////////////////////////////////////////////////////////////////////
t_irc bncBiasSnx::bncTimeFromSinex(QString snxStr, bncTime& time) {
  bool ok = false;
  QStringList hlp = snxStr.split(":");// yyyy:doy:dssec

  int year = hlp[0].toInt(&ok, 10);
  if (!ok) {
    return failure;
  }
  time.set(year, 1, 1, 0, 0, 0.0);

  int doy  = hlp[1].toInt(&ok, 10);
  if (!ok) {
    return failure;
  }
  time += (doy * 24.0 * 3600.0);

  int daysec = hlp[2].toInt(&ok, 10);
  if (!ok) {
    return failure;
  }

  time += daysec;

  return success;
}


// get DSBs from OSBs
////////////////////////////////////////////////////////////////////////////
t_irc bncBiasSnx::getDsbFromOsb(void) {
  QMapIterator<QString,snxSatCodeBias*>itSat(_snxSatCodeBiasMap);
  while (itSat.hasNext()) {
    itSat.next();
    char sys = itSat.key().at(0).toLatin1();
    QMap<QString, double>& cb = itSat.value()->_biasMap;
    switch (sys) {
      case 'G':
        if  (cb.contains("C1W") && cb.contains("C2W")) {cb.insert("C1W-C2W", cb["C1W"] - cb["C2W"]);}
        if  (cb.contains("C1C") && cb.contains("C1W")) {cb.insert("C1C-C1W", cb["C1C"] - cb["C1W"]);}
        if  (cb.contains("C2C") && cb.contains("C2W")) {cb.insert("C2C-C2W", cb["C2C"] - cb["C2W"]);}
        if  (cb.contains("C2W") && cb.contains("C2S")) {cb.insert("C2W-C2S", cb["C2W"] - cb["C2S"]);}
        if  (cb.contains("C2W") && cb.contains("C2L")) {cb.insert("C2W-C2L", cb["C2W"] - cb["C2L"]);}
        if  (cb.contains("C2W") && cb.contains("C2X")) {cb.insert("C2W-C2X", cb["C2W"] - cb["C2X"]);}
        if  (cb.contains("C1C") && cb.contains("C2W")) {cb.insert("C1C-C2W", cb["C1C"] - cb["C2W"]);}
        if  (cb.contains("C1C") && cb.contains("C5Q")) {cb.insert("C1C-C5Q", cb["C1C"] - cb["C5Q"]);}
        if  (cb.contains("C1C") && cb.contains("C5X")) {cb.insert("C1C-C5X", cb["C1C"] - cb["C5X"]);}
        break;
      case 'R':
        if  (cb.contains("C1P") && cb.contains("C2P")) {cb.insert("C1P-C2P", cb["C1P"] - cb["C2P"]);}
        if  (cb.contains("C1C") && cb.contains("C1P")) {cb.insert("C1C-C1P", cb["C1C"] - cb["C1P"]);}
        if  (cb.contains("C2C") && cb.contains("C2P")) {cb.insert("C2C-C2P", cb["C2C"] - cb["C2P"]);}
        if  (cb.contains("C1C") && cb.contains("C2C")) {cb.insert("C1C-C2C", cb["C1C"] - cb["C2C"]);}
        if  (cb.contains("C1C") && cb.contains("C2P")) {cb.insert("C1C-C2P", cb["C1C"] - cb["C2P"]);}
        break;
      case 'E':
        if  (cb.contains("C1C") && cb.contains("C5Q")) {cb.insert("C1C-C5Q", cb["C1C"] - cb["C5Q"]);}
        if  (cb.contains("C1C") && cb.contains("C6C")) {cb.insert("C1C-C6C", cb["C1C"] - cb["C6C"]);}
        if  (cb.contains("C1C") && cb.contains("C7Q")) {cb.insert("C1C-C7Q", cb["C1C"] - cb["C7Q"]);}
        if  (cb.contains("C1C") && cb.contains("C8Q")) {cb.insert("C1C-C8Q", cb["C1C"] - cb["C8Q"]);}
        break;
      case 'C':
        if  (cb.contains("C2I") && cb.contains("C6I")) {cb.insert("C2I-C6I", cb["C2I"] - cb["C6I"]);}
        if  (cb.contains("C2I") && cb.contains("C7I")) {cb.insert("C2I-C7I", cb["C2I"] - cb["C7I"]);}
        if  (cb.contains("C1X") && cb.contains("C6I")) {cb.insert("C1X-C6I", cb["C1X"] - cb["C6I"]);}
        if  (cb.contains("C1P") && cb.contains("C6I")) {cb.insert("C1P-C6I", cb["C1P"] - cb["C6I"]);}
        if  (cb.contains("C1D") && cb.contains("C6I")) {cb.insert("C1D-C6I", cb["C1D"] - cb["C6I"]);}
        if  (cb.contains("C1X") && cb.contains("C7Z")) {cb.insert("C1X-C7Z", cb["C1X"] - cb["C7Z"]);}
        if  (cb.contains("C1X") && cb.contains("C8X")) {cb.insert("C1X-C8X", cb["C1X"] - cb["C8X"]);}
        if  (cb.contains("C1X") && cb.contains("C5X")) {cb.insert("C1X-C5X", cb["C1X"] - cb["C5X"]);}
        if  (cb.contains("C1P") && cb.contains("C5P")) {cb.insert("C1P-C5P", cb["C1P"] - cb["C5P"]);}
        if  (cb.contains("C1D") && cb.contains("C5D")) {cb.insert("C1D-C5D", cb["C1D"] - cb["C5D"]);}
        break;
      case 'J':
        if  (cb.contains("C1C") && cb.contains("C1X")) {cb.insert("C1C-C1X", cb["C1C"] - cb["C1X"]);}
        if  (cb.contains("C1C") && cb.contains("C2L")) {cb.insert("C1C-C2L", cb["C1C"] - cb["C2L"]);}
        if  (cb.contains("C1C") && cb.contains("C5X")) {cb.insert("C1C-C5X", cb["C1C"] - cb["C5X"]);}
        if  (cb.contains("C1C") && cb.contains("C5Q")) {cb.insert("C1C-C5Q", cb["C1C"] - cb["C5Q"]);}
        if  (cb.contains("C1X") && cb.contains("C2X")) {cb.insert("C1X-C2X", cb["C1X"] - cb["C2X"]);}
        if  (cb.contains("C1X") && cb.contains("C5X")) {cb.insert("C1X-C5X", cb["C1X"] - cb["C5X"]);}
        break;
      default:
        break;
    }
  }
  return success;
}

//
////////////////////////////////////////////////////////////////////////////
t_irc bncBiasSnx::cleanDsb() {

  for (auto itS = _snxSatCodeBiasMap.begin(); itS != _snxSatCodeBiasMap.end();) {
    char sys = itS.key().at(0).toLatin1();
    if (!_useGnss[sys]) {
      delete itS.value();
      itS = _snxSatCodeBiasMap.erase(itS);
    }
    else {
      QMap<QString, double>& cb = itS.value()->_biasMap;
      for (auto itB = cb.begin(); itB != cb.end();){
        if  (itB.key().size() == 3) {
          itB = cb.erase(itB);
        }
        else {
          ++itB;
        }
      }
      ++itS;
    }
  }
  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void bncBiasSnx::determineSsrSatCodeBiases(QString prn, double aIF1, double aIF2,
                                           t_satCodeBias& satCodeBias) {
  std::string rnxType2ch;
  double value;
  if (_snxSatCodeBiasMap.contains(prn)) {
    char sys = prn.at(0).toLatin1();
    QMap<QString, double>& cb = _snxSatCodeBiasMap[prn]->_biasMap;
    switch (sys) {
      case 'G':
        if  (cb.contains("C1W-C2W")) {
          double dcb_1W2W = cb.find("C1W-C2W").value();
          rnxType2ch = "1W";
          value =  aIF2 * dcb_1W2W;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          rnxType2ch = "2W";
          value = -aIF1 * dcb_1W2W;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          if   (cb.contains("C1C-C1W")) {
            double dcb_1C1W = cb.find("C1C-C1W").value();
            rnxType2ch = "1C";
            value = dcb_1C1W + aIF2 * dcb_1W2W;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            if  (cb.contains("C1C-C5Q")) {
              double dcb_1C5Q = cb.find("C1C-C5Q").value();
              rnxType2ch = "5Q";
              value = dcb_1C1W + aIF2 * dcb_1W2W - dcb_1C5Q;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
            if  (cb.contains("C1C-C5X")) {
              double dcb_1C5X = cb.find("C1C-C5X").value();
              rnxType2ch = "5X";
              value = dcb_1C1W + aIF2 * dcb_1W2W - dcb_1C5X;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
          else {
            if  (cb.contains("C1C-C2W")) {
              double dcb_1C2W = cb.find("C1C-C2W").value();
              rnxType2ch = "1C";
              value = dcb_1C2W - aIF1 * dcb_1W2W;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
          if   (cb.contains("C2C-C2W")) {
            double dcb_2C2W = cb.find("C2C-C2W").value();
            rnxType2ch = "2C";
            value = dcb_2C2W - aIF1 * dcb_1W2W;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if   (cb.contains("C2W-C2S")) {
            double dcb_2W2S = cb.find("C2W-C2S").value();
            rnxType2ch = "2S";
            value = -aIF1 * dcb_1W2W - dcb_2W2S;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if   (cb.contains("C2W-C2L")) {
            double dcb_2W2L = cb.find("C2W-C2L").value();
            rnxType2ch = "2L";
            value = -aIF1 * dcb_1W2W - dcb_2W2L;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if   (cb.contains("C2W-C2X")) {
            double dcb_2W2X = cb.find("C2W-C2X").value();
            rnxType2ch = "2X";
            value = -aIF1 * dcb_1W2W - dcb_2W2X;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
        }
        break;
      case 'R':
        if  (cb.contains("C1P-C2P")) {
          double dcb_1P2P = cb.find("C1P-C2P").value();
          rnxType2ch = "1P";
          value =  aIF2 * dcb_1P2P;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          rnxType2ch = "2P";
          value = -aIF1 * dcb_1P2P;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          if   (cb.contains("C1C-C1P")) {
            double dcb_1C1P = cb.find("C1C-C1P").value();
            rnxType2ch = "1C";
            value = dcb_1C1P + aIF2 * dcb_1P2P;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if   (cb.contains("C2C-C2P")) {
            double dcb_2C2P = cb.find("C2C-C2P").value();
            rnxType2ch = "2C";
            value = dcb_2C2P - aIF1 * dcb_1P2P;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
        }
        break;
      case 'E':
        if  (cb.contains("C1C-C5Q")) {
          double dcb_1C5Q = cb.find("C1C-C5Q").value();
          rnxType2ch = "5Q";
          value = -aIF1 * dcb_1C5Q;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          rnxType2ch = "1C";
          value =  aIF2 * dcb_1C5Q;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          if  (cb.contains("C1C-C6C")) {
            double dcb_1C6C = cb.find("C1C-C6C").value();
            rnxType2ch = "6C";
            value = aIF2 * dcb_1C5Q - dcb_1C6C;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if  (cb.contains("C1C-C7Q")) {
            double dcb_1C7Q = cb.find("C1C-C7Q").value();
            rnxType2ch = "7Q";
            value = aIF2 * dcb_1C5Q - dcb_1C7Q;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if  (cb.contains("C1C-C8Q")) {
            double dcb_1C8Q = cb.find("C1C-C8Q").value();
            rnxType2ch = "8Q";
            value = aIF2 * dcb_1C5Q - dcb_1C8Q;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
        }
        break;
      case 'C':
        if  (cb.contains("C2I-C6I")) {
          double dcb_2I6I = cb.find("C2I-C6I").value();
          rnxType2ch = "2I";
          value =  aIF2 * dcb_2I6I;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          rnxType2ch = "6I";
          value = -aIF1 * dcb_2I6I;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          if (cb.contains("C2I-C7I")) {
            double dcb_2I7I = cb.find("C2I-C7I").value();
            rnxType2ch = "7I";
            value = aIF2 * dcb_2I6I - dcb_2I7I;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
          if (cb.contains("C1X-C6I")) {
            double dcb_1X6I = cb.find("C1X-C6I").value();
            rnxType2ch = "1X";
            value = dcb_1X6I - aIF1 * dcb_2I6I;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            if (cb.contains("C1X-C7Z")) {
              double dcb_1X7Z = cb.find("C1X-C7Z").value();
              rnxType2ch = "7Z";
              value = dcb_1X6I - aIF1 * dcb_2I6I - dcb_1X7Z;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
            if (cb.contains("C1X-C8X")) {
              double dcb_1X8X = cb.find("C1X-C8X").value();
              rnxType2ch = "8X";
              value = dcb_1X6I - aIF1 * dcb_2I6I - dcb_1X8X;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
            if (cb.contains("C1X-C5X")) {
              double dcb_1X5X = cb.find("C1X-C5X").value();
              rnxType2ch = "5X";
              value = dcb_1X6I - aIF1 * dcb_2I6I - dcb_1X5X;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
          if (cb.contains("C1P-C6I")) {
            double dcb_1P6I = cb.find("C1P-C6I").value();
            rnxType2ch = "1P";
            value = dcb_1P6I - aIF1 * dcb_2I6I;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            if (cb.contains("C1P-C5P")) {
              double dcb_1P5P = cb.find("C1P-C5P").value();
              rnxType2ch = "5P";
              value = dcb_1P6I - aIF1 * dcb_2I6I - dcb_1P5P;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
          if (cb.contains("C1D-C6I")) {
            double dcb_1D6I = cb.find("C1D-C6I").value();
            rnxType2ch = "1D";
            value = dcb_1D6I - aIF1 * dcb_2I6I;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            if (cb.contains("C1D-C5D")) {
              double dcb_1D5D = cb.find("C1D-C5D").value();
              rnxType2ch = "5D";
              value = dcb_1D6I - aIF1 * dcb_2I6I - dcb_1D5D;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
        }
        break;
      case 'J':
        if (cb.contains("C1C-C2L")) {
          double dcb_1C2L = cb.find("C1C-C2L").value();
          rnxType2ch = "1C";
          value =  aIF2 * dcb_1C2L;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          rnxType2ch = "2L";
          value = -aIF1 * dcb_1C2L;
          setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          if (cb.contains("C1C-C1X")) {
            double dcb_1C1X = cb.find("C1C-C1X").value();
            rnxType2ch = "1X";
            value =  aIF2 * dcb_1C2L - dcb_1C1X;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            if (cb.contains("C1X-C2X")) {
              double dcb_1X2X = cb.find("C1X-C2X").value();
              rnxType2ch = "2X";
              value =  aIF2 * dcb_1C2L - dcb_1C1X - dcb_1X2X;
              setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
            }
          }
          if (cb.contains("C1C-C5X")) {
            double dcb_1C5X = cb.find("C1C-C5X").value();
            rnxType2ch = "5X";
            value =  aIF2 * dcb_1C2L - dcb_1C5X;
            setSsrSatCodeBias(rnxType2ch, value, satCodeBias);
          }
        }
        break;
      default:
        break;
    }
  }
  return;
}


void bncBiasSnx::setSsrSatCodeBias(std::string rnxType2ch, double value,
                                    t_satCodeBias& satCodeBias) {
  t_frqCodeBias frqCodeBias;

  frqCodeBias._rnxType2ch = rnxType2ch;
  frqCodeBias._value = value;

  satCodeBias._bias.push_back(frqCodeBias);
  return;
}
