/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppStation
 *
 * Purpose:    Processed station
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include "pppStation.h"
#include "bncutils.h"
#include "pppModel.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
//////////////////////////////////////////////////////////////////////////////
t_pppStation::t_pppStation() {
  _windUp   = new t_windUp();
  _iono     = new t_iono();
  _dClk = 0.0;
}

// Destructor
//////////////////////////////////////////////////////////////////////////////
t_pppStation::~t_pppStation() {
  delete _windUp;
  delete _iono;
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppStation::setXyzApr(const ColumnVector& xyzApr) {
  _xyzApr = xyzApr;
  _ellApr.ReSize(3);
  xyz2ell(_xyzApr.data(), _ellApr.data());
}

//
//////////////////////////////////////////////////////////////////////////////
void t_pppStation::setNeuEcc(const ColumnVector& neuEcc) {
  _neuEcc = neuEcc;
  _xyzEcc.ReSize(3);
  neu2xyz(_ellApr.data(), _neuEcc.data(), _xyzEcc.data());
}

//
//////////////////////////////////////////////////////////////////////////////
double t_pppStation::windUp(const bncTime& time, t_prn prn,
                         const ColumnVector& rSat, bool ssr, double yaw,
                         const ColumnVector& vSat) const {
  return _windUp->value(time, _xyzApr, prn, rSat, ssr, yaw, vSat);
}

//
//////////////////////////////////////////////////////////////////////////////
double t_pppStation::stec(const t_vTec* vTec, const double signalPropagationTime,
    const ColumnVector& rSat) const {
  return _iono->stec(vTec, signalPropagationTime, rSat, _epochTime, _xyzApr);
}
