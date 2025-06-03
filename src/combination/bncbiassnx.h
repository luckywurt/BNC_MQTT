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

#ifndef BNCBIAS_H
#define BNCBIAS_H

#include <QtCore>
#include <QString>
#include <QList>
#include <QMap>
#include <vector>
#include <string>
#include <newmat.h>
#include "bncconst.h"
#include "bnctime.h"
#include "bncsettings.h"
#include "satObs.h"
#include "ephemeris.h"
#include "t_prn.h"





class snxSatCodeBias {
 public:
  snxSatCodeBias(bncTime startTime) {
    _startTime = startTime;
  }
  ~snxSatCodeBias() {
    _biasMap.clear();
  }
  bncTime               _startTime;
  QMap<QString, double> _biasMap;
};

class bncBiasSnx {
 public:
  bncBiasSnx();
  ~bncBiasSnx();
  t_irc readFile(const QString& fileName);
  void determineSsrSatCodeBiases(QString prn, double aIF1, double aIF2, t_satCodeBias& satCodeBias);
  void setSsrSatCodeBias(std::string rnxType2ch, double value, t_satCodeBias& satCodeBias);

 private:
  enum e_type {ABS, REL};
  void clear();
  t_irc bncTimeFromSinex(QString snxStr, bncTime& time);
  t_irc getDsbFromOsb();
  t_irc cleanDsb();
  void  print() const;
  void  ssrCodeBiases();
  e_type                         _type;
  QMap<QString, snxSatCodeBias*> _snxSatCodeBiasMap;
  QMap<char, bool>               _useGnss;
};


#endif


















