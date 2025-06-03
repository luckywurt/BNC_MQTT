/*
 * bncBiasSinex.h
 *
 *  Created on: Feb 18, 2022
 *      Author: stuerze
 */

#ifndef SRC_BNCBIASSINEX_H_
#define SRC_BNCBIASSINEX_H_

#include <fstream>
#include <newmat.h>
#include <iostream>

#include "bncoutf.h"
#include "bncversion.h"
#include "pppOptions.h"
#include "bncsettings.h"
#include "bncantex.h"

using namespace BNC_PPP;


class bncBiasSinex : public bncoutf {
 public:
  bncBiasSinex(const QString& sklFileName, const QString& intr,
              int sampl);
  virtual ~bncBiasSinex();
  virtual t_irc write(int GPSweek, double GPSweeks, const QString& prn,
                      const QString& obsCode, double bias);

 private:
  virtual void writeHeader(const QDateTime& datTim);
  int     _sampl;
  QString _agency;
};

#endif /* SRC_BNCBIASSINEX_H_ */

