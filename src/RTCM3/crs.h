/*
 * crs.h
 *
 *  Created on: Jun 18, 2024
 *      Author: stuerze
 */

#ifndef SRC_RTCM3_CRS_H_
#define SRC_RTCM3_CRS_H_

#include <cstring>
#include <iostream>
#include <iomanip>
using namespace std;

 class t_serviceCrs {
  public:
  t_serviceCrs() {
    _coordinateEpoch = 0;
    _CE              = 0;
  }
  ~t_serviceCrs() {
    clear();
  }
  void clear(void) {
    memset(_name, '\0', 31 * sizeof(char));
    _coordinateEpoch = 0;
    _CE              = 0;
  }
  void print() {
    cout << "=============" << endl;
      cout << "service Crs" << endl;
      cout << "=============" << endl;
      cout << QString("%1").arg(_name).toStdString().c_str() << endl;
      cout << "_coordinateEpoch: " << _coordinateEpoch << endl;
      cout << "_CE: " << _CE << endl;
    }
    void setCoordinateEpochFromCE() {(!_CE) ? _coordinateEpoch = 0.0 : _coordinateEpoch = 1900 + _CE; }
    void setCEFromCoordinateEpoch() {(!_coordinateEpoch) ? _CE = 0 : _CE = _coordinateEpoch -1900; }
    char     _name[31];
    double   _coordinateEpoch;
    double   _CE;
  };


 class t_rtcmCrs {
  public:
    t_rtcmCrs() {
      _anchor = 0;
      _plateNumber = 0;
    }

    ~t_rtcmCrs() {
      clear();
    }
    void clear() {
      memset(_name, '\0', 31 * sizeof(char));
      _anchor = 0;
      _plateNumber = 0;
      _databaseLinks.clear();
    }
    void print() {
      cout << "=============" << endl;
      cout << "rtcm Crs" << endl;
      cout << "=============" << endl;
      cout << QString("%1").arg(_name).toStdString().c_str() << endl;
      cout << "_anchor: " << _anchor << endl;
      cout << "_plateNumber: " << _plateNumber << endl;
      for (int i = 0; i<_databaseLinks.size(); i++){
        cout << _databaseLinks[i].toStdString() << endl;
      }
    }
    char             _name[31];
    int              _anchor;           // 0.. global CRS (=> Plate number 0), 1.. plate-fixed CRS
    int              _plateNumber;      // 0.. unknown, 7.. Eurasia
    QStringList      _databaseLinks;
  };

 class t_helmertPar {
   public:
     t_helmertPar() {
       _sysIdentNum = 0;
       _utilTrafoMessageIndicator = 0;
       _mjd = 0;
       _dx  = 0.0;
       _dy  = 0.0;
       _dz  = 0.0;
       _dxr = 0.0;
       _dyr = 0.0;
       _dzr = 0.0;
       _ox  = 0.0;
       _oy  = 0.0;
       _oz  = 0.0;
       _oxr = 0.0;
       _oyr = 0.0;
       _ozr = 0.0;
       _sc  = 0.0;
       _scr = 0.0;
     }

     QString IndtoString() {
       int ind = _utilTrafoMessageIndicator;
       QString str = "";
       if (!ind) {
          str += " not utillized";
          return str;
       }
       // Bit 0
       if (ind & (1<<0)) {
         str += "MT1023 ";
       }
       // Bit 1
       if (ind & (1<<1)) {
         str += "MT1024 ";
       }
       // Bit 2
       if (ind & (1<<2)) {
         str += "MT1025 ";
       }
       // Bit 3
       if (ind & (1<<3)) {
         str += "MT1026 ";
       }
       // Bit 4
       if (ind & (1<<4)) {
         str += "MT1027 ";
       }
       // 5 -9
       if (!(ind & (1<<5)) &&
           !(ind & (1<<6)) &&
           !(ind & (1<<7)) &&
           !(ind & (1<<8)) &&
           !(ind & (1<<9))) {
         str += "(and Bit 5-9 is reserved => zero)";
       }
       return str;
     }
     bool operator==(const t_helmertPar& helmertPar1) const {
        if (strncmp(_sourceName,  helmertPar1._sourceName, strlen(_sourceName)) == 0 &&
            strncmp(_targetName,  helmertPar1._targetName, strlen(_targetName)) == 0 &&
            _sysIdentNum == helmertPar1._sysIdentNum &&
            _mjd == helmertPar1._mjd) {
          return true;
        }
        else {
          return false;
        }
     }
     void print() {
       cout << "MT 1301: " << endl;
       cout << "_sourceName:                " << _sourceName << endl;
       cout << "_targetName:                " << _targetName << endl;
       cout << "_sysIdentNum:               " << _sysIdentNum << endl;
       cout << "_utilTrafoMessageIndicator: " << IndtoString().toStdString().c_str() << endl;
       bncTime t; t.setmjd(0, _mjd);
       cout << "_t0   :                     " << "MJD " << _mjd  << " (" << t.datestr() << ")" << endl;
       cout <<setw(10) << setprecision(5) << "_dx   :                     " <<  _dx  << endl;
       cout <<setw(10) << setprecision(5) << "_dy   :                     " <<  _dy  << endl;
       cout <<setw(10) << setprecision(5) << "_dz   :                     " <<  _dz  << endl;
       cout <<setw(10) << setprecision(7) << "_dxr  :                     " <<  _dxr << endl;
       cout <<setw(10) << setprecision(7) << "_dyr  :                     " <<  _dyr << endl;
       cout <<setw(10) << setprecision(7) << "_dzr  :                     " <<  _dzr << endl;
       cout <<setw(10) << setprecision(5) << "_ox   :                     " <<  _ox  << endl;
       cout <<setw(10) << setprecision(5) << "_oy   :                     " <<  _oy  << endl;
       cout <<setw(10) << setprecision(5) << "_oz   :                     " <<  _oz  << endl;
       cout <<setw(15) << setprecision(7) << "_oxr  :                     " <<  _oxr << endl;
       cout <<setw(15) << setprecision(7) << "_oyr  :                     " <<  _oyr << endl;
       cout <<setw(15) << setprecision(7) << "_ozr  :                     " <<  _ozr << endl;
       cout <<setw(10) << setprecision(5) << "_sc   :                     " <<  _sc  << endl;
       cout <<setw(10) << setprecision(7) << "_scr  :                     " <<  _scr << endl;
       cout << endl;
     }
     char           _sourceName[31];
     char           _targetName[31];
     int            _sysIdentNum;
     int            _utilTrafoMessageIndicator;
     int            _mjd; // reference epoch
     double         _dx;  // translation in x
     double         _dy;  // translation in y
     double         _dz;  // translation in z
     double         _dxr; // translation rate in x
     double         _dyr; // translation rate in y
     double         _dzr; // translation rate in z
     double         _ox;  // rotation in x
     double         _oy;  // rotation in y
     double         _oz;  // rotation in z
     double         _oxr; // rotation rate in x
     double         _oyr; // rotation rate in y
     double         _ozr; // rotation rate in z
     double         _sc;  // scale
     double         _scr; // scale rate
   };


#endif /* SRC_RTCM3_CRS_H_ */
