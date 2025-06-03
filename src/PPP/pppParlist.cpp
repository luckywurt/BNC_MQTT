/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      t_pppParlist
 *
 * Purpose:    List of estimated parameters
 *
 * Author:     L. Mervart
 *
 * Created:    29-Jul-2014
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <newmatio.h>

#include "pppParlist.h"
#include "pppSatObs.h"
#include "pppStation.h"
#include "bncutils.h"
#include "bncconst.h"
#include "pppClient.h"

using namespace BNC_PPP;
using namespace std;

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppParam::t_pppParam(e_type type, const t_prn& prn, t_lc::type tLC,
                       const vector<t_pppSatObs*>* obsVector) {

  _type     = type;
  _prn      = prn;
  _tLC      = tLC;
  _x0       = 0.0;
  _indexOld = -1;
  _indexNew = -1;
  _noise    = 0.0;
  _ambInfo = new t_ambInfo();

  switch (_type) {
   case crdX:
     _epoSpec = false;
     _sigma0  = OPT->_aprSigCrd[0];
     _noise   = OPT->_noiseCrd[0];
     break;
   case crdY:
     _epoSpec = false;
     _sigma0  = OPT->_aprSigCrd[1];
     _noise   = OPT->_noiseCrd[1];
     break;
   case crdZ:
     _epoSpec = false;
     _sigma0  = OPT->_aprSigCrd[2];
     _noise   = OPT->_noiseCrd[2];
     break;
   case rClkG:
     _epoSpec = true;
     _sigma0  = OPT->_aprSigClk;
     break;
   case rClkR:
     _epoSpec = true;
     _sigma0  = OPT->_aprSigClk;
     break;
   case rClkE:
     _epoSpec = true;
     _sigma0  = OPT->_aprSigClk;
     break;
   case rClkC:
     _epoSpec = true;
     _sigma0  = OPT->_aprSigClk;
     break;
   case amb:
     _epoSpec = false;
     _sigma0  = OPT->_aprSigAmb;
     if (obsVector) {
       for (unsigned ii = 0; ii < obsVector->size(); ii++) {
         const t_pppSatObs* obs = obsVector->at(ii);
         if (obs->prn() == _prn) {
           _x0 = floor((obs->obsValue(tLC) - obs->cmpValue(tLC)) / obs->lambda(tLC) + 0.5);
           break;
         }
       }
     }
     break;
   case trp:
     _epoSpec = false;
     _sigma0  = OPT->_aprSigTrp;
     _noise   = OPT->_noiseTrp;
     break;
    case ion:
      _epoSpec = false;
      _sigma0  = OPT->_aprSigIon;
      _noise   = OPT->_noiseIon;
     break;
    case cBiasG1:   case cBiasR1:   case cBiasE1:   case cBiasC1:
    case cBiasG2:   case cBiasR2:   case cBiasE2:   case cBiasC2:
      _epoSpec = false;
      _sigma0  = OPT->_aprSigCodeBias;
      _noise   = OPT->_noiseCodeBias;
      break;
    case pBiasG1:   case pBiasR1:   case pBiasE1:   case pBiasC1:
    case pBiasG2:   case pBiasR2:   case pBiasE2:   case pBiasC2:
      _epoSpec = false;
      _sigma0  = OPT->_aprSigPhaseBias;
      _noise   = OPT->_noisePhaseBias;
      break;
  }
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppParam::~t_pppParam() {
  delete _ambInfo;
}
//
////////////////////////////////////////////////////////////////////////////
double t_pppParam::partial(const bncTime& /* epoTime */, const t_pppSatObs* obs,
                           const t_lc::type& tLC) const {

  // Special Case - Melbourne-Wuebbena
  // ---------------------------------
  if (tLC == t_lc::MW && _type != amb) {
    return 0.0;
  }

  const t_pppStation* sta  = PPP_CLIENT->staRover();
  ColumnVector        rhoV = sta->xyzApr() - obs->xc().Rows(1,3);

  map<t_frequency::type, double> codeCoeff;
  map<t_frequency::type, double> phaseCoeff;
  map<t_frequency::type, double> ionoCoeff;
  obs->lcCoeff(tLC, codeCoeff, phaseCoeff, ionoCoeff);

  switch (_type) {
  case crdX:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (sta->xyzApr()[0] - obs->xc()[0]) / rhoV.NormFrobenius();
  case crdY:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (sta->xyzApr()[1] - obs->xc()[1]) / rhoV.NormFrobenius();
  case crdZ:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (sta->xyzApr()[2] - obs->xc()[2]) / rhoV.NormFrobenius();
  case rClkG:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (obs->prn().system() == 'G') ? 1.0 : 0.0;
  case rClkR:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (obs->prn().system() == 'R') ? 1.0 : 0.0;
  case rClkE:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (obs->prn().system() == 'E') ? 1.0 : 0.0;
  case rClkC:
    if (tLC == t_lc::GIM) {return 0.0;}
    return (obs->prn().system() == 'C') ? 1.0 : 0.0;
  case amb:
    if (tLC == t_lc::GIM) {
      return 0.0;
    }
    else {
      if (obs->prn() == _prn) {
        if      (tLC == _tLC) {
          return (obs->lambda(tLC));
        }
        else if (tLC == t_lc::lIF && _tLC == t_lc::MW) {
          return obs->lambda(t_lc::lIF) * obs->lambda(t_lc::MW) / obs->lambda(t_lc::l2);
        }
        else {
          if      (_tLC == t_lc::l1) {
            return obs->lambda(t_lc::l1) * phaseCoeff[obs->fType1()];
          }
          else if (_tLC == t_lc::l2) {
            return obs->lambda(t_lc::l2) * phaseCoeff[obs->fType2()];
          }
        }
      }
    }
    break;
  case trp:
    if      (tLC == t_lc::GIM) {
      return 0.0;
    }
    else {
      return 1.0 / sin(obs->eleSat());
    }
  case ion:
    if (obs->prn() == _prn) {
      if      (tLC == t_lc::c1) {
        return ionoCoeff[obs->fType1()];
      }
      else if (tLC == t_lc::c2) {
        return ionoCoeff[obs->fType2()];
      }
      else if (tLC == t_lc::l1) {
        return ionoCoeff[obs->fType1()];
      }
      else if (tLC == t_lc::l2) {
        return ionoCoeff[obs->fType2()];
      }
      else if (tLC == t_lc::GIM) {
        return 1.0;
      }
    }
    break;
  case cBiasG1:
    if ((obs->prn().system() == 'G') && (tLC == t_lc::c1)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasR1:
    if ((obs->prn().system() == 'R') && (tLC == t_lc::c1)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasE1:
    if ((obs->prn().system() == 'E') && (tLC == t_lc::c1)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasC1:
    if ((obs->prn().system() == 'C') && (tLC == t_lc::c1)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasG2:
    if ((obs->prn().system() == 'G') && (tLC == t_lc::c2)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasR2:
    if ((obs->prn().system() == 'R') && (tLC == t_lc::c2)) {return 1.0;} else {return 0.0;}
    break;
  case cBiasE2:
    if ((obs->prn().system() == 'E') && (tLC == t_lc::c2)) {return 1.0;} else {return 0.0;}
        break;
  case cBiasC2:
    if ((obs->prn().system() == 'C') && (tLC == t_lc::c2)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasG1:
    if ((obs->prn().system() == 'G') && (tLC == t_lc::l1)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasR1:
    if ((obs->prn().system() == 'R') && (tLC == t_lc::l1)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasE1:
    if ((obs->prn().system() == 'E') && (tLC == t_lc::l1)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasC1:
    if ((obs->prn().system() == 'C') && (tLC == t_lc::l1)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasG2:
    if ((obs->prn().system() == 'G') && (tLC == t_lc::l2)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasR2:
    if ((obs->prn().system() == 'R') && (tLC == t_lc::l2)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasE2:
    if ((obs->prn().system() == 'E') && (tLC == t_lc::l2)) {return 1.0;} else {return 0.0;}
    break;
  case pBiasC2:
    if ((obs->prn().system() == 'C') && (tLC == t_lc::l2)) {return 1.0;} else {return 0.0;}
    break;

  }
  return 0.0;
}

//
////////////////////////////////////////////////////////////////////////////
string t_pppParam::toString() const {
  stringstream ss;
  switch (_type) {
  case crdX:
    ss << "CRD_X";
    break;
  case crdY:
    ss << "CRD_Y";
    break;
  case crdZ:
    ss << "CRD_Z";
    break;
  case rClkG:
    ss << "REC_CLK  G  ";
    break;
  case rClkR:
    ss << "REC_CLK  R  ";
    break;
  case rClkE:
    ss << "REC_CLK  E  ";
    break;
  case rClkC:
    ss << "REC_CLK  C  ";
    break;
  case trp:
    ss << "TRP         ";
    break;
  case amb:
    ss << "AMB  " << left << setw(3) << t_lc::toString(_tLC) << right << ' ' << _prn.toString();
    break;
  case ion:
    ss << "ION  " << left << setw(3) << t_lc::toString(_tLC) << right << ' ' << _prn.toString();
    break;
  case cBiasG1:  case pBiasG1:
  case cBiasG2:  case pBiasG2:
    ss << "BIA  " << left << setw(3) << t_lc::toString(_tLC) << right << " G  ";
    break;
  case cBiasR1:  case pBiasR1:
  case cBiasR2:  case pBiasR2:
    ss << "BIA  " << left << setw(3) << t_lc::toString(_tLC) << right << " R  ";
    break;
  case cBiasE1:  case pBiasE1:
  case cBiasE2:  case pBiasE2:
    ss << "BIA  " << left << setw(3) << t_lc::toString(_tLC) << right << " E  ";
    break;
  case cBiasC1:  case pBiasC1:
  case cBiasC2:  case pBiasC2:
    ss << "BIA  " << left << setw(3) << t_lc::toString(_tLC) << right << " C  ";
    break;
  }
  return ss.str();
}

// Constructor
////////////////////////////////////////////////////////////////////////////
t_pppParlist::t_pppParlist() {
}

// Destructor
////////////////////////////////////////////////////////////////////////////
t_pppParlist::~t_pppParlist() {

  for (unsigned ii = 0; ii < _params.size(); ii++) {
    delete _params[ii];
  }
}

//
////////////////////////////////////////////////////////////////////////////
t_irc t_pppParlist::set(const bncTime& epoTime, const std::vector<t_pppSatObs*>& obsVector) {

  // Remove some Parameters
  // ----------------------
  vector<t_pppParam*>::iterator it = _params.begin();
  while (it != _params.end()) {
    t_pppParam* par = *it;

    bool remove = false;

    if      (par->epoSpec()) {
      remove = true;
    }

    else if (par->type() == t_pppParam::amb  ||
             par->type() == t_pppParam::crdX ||
             par->type() == t_pppParam::crdY ||
             par->type() == t_pppParam::crdZ ||
             par->type() == t_pppParam::ion  ||
             par->type() == t_pppParam::cBiasC1 ||
             par->type() == t_pppParam::cBiasC2 ||
             par->type() == t_pppParam::cBiasE1 ||
             par->type() == t_pppParam::cBiasE2 ||
             par->type() == t_pppParam::cBiasR1 ||
             par->type() == t_pppParam::cBiasR2 ||
             par->type() == t_pppParam::pBiasC1 ||
             par->type() == t_pppParam::pBiasC2 ||
             par->type() == t_pppParam::pBiasE1 ||
             par->type() == t_pppParam::pBiasE2 ||
             par->type() == t_pppParam::pBiasR1 ||
             par->type() == t_pppParam::pBiasR2) {
      if (par->lastObsTime().valid() && (epoTime - par->lastObsTime() > 60.0)) {
        remove = true;
      }
    }

    if (remove) {
#ifdef BNC_DEBUG_PPP
//      LOG << "remove0 " << par->toString() << std::endl;
#endif
      delete par;
      it = _params.erase(it);
    }
    else {
      ++it;
    }
  }

  // check which systems have observations
  // -------------------------------------
  _usedSystems['G'] = _usedSystems['R'] = _usedSystems['E'] = _usedSystems['C'] = 0;
  for (unsigned jj = 0; jj < obsVector.size(); jj++) {
    const t_pppSatObs* satObs = obsVector[jj];
    char sys = satObs->prn().system();
    if (OPT->LCs(sys).size()) {
      _usedSystems[sys]++;
    }
  };


  // Check whether parameters have observations
  // ------------------------------------------
  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    if (par->prn() == 0) {
      par->setLastObsTime(epoTime);
      if (par->firstObsTime().undef()) {
        par->setFirstObsTime(epoTime);
      }
    }
    else {
      for (unsigned jj = 0; jj < obsVector.size(); jj++) {
        const t_pppSatObs* satObs = obsVector[jj];
        if (satObs->prn() == par->prn()) {
          par->setLastObsTime(epoTime);
          if (par->firstObsTime().undef()) {
            par->setFirstObsTime(epoTime);
          }
          break;
        }
      }
    }
  }

  // Required Set of Parameters
  // --------------------------
  vector<t_pppParam*> required;

  // Coordinates
  // -----------
  required.push_back(new t_pppParam(t_pppParam::crdX, t_prn(), t_lc::dummy));
  required.push_back(new t_pppParam(t_pppParam::crdY, t_prn(), t_lc::dummy));
  required.push_back(new t_pppParam(t_pppParam::crdZ, t_prn(), t_lc::dummy));

  // Receiver Clocks
  // ---------------
   if (_usedSystems['G']) {
   //if (OPT->useSystem('G')) {
     required.push_back(new t_pppParam(t_pppParam::rClkG, t_prn(), t_lc::dummy));
   }
   if (_usedSystems['R']) {
   //if (OPT->useSystem('R')) {
     required.push_back(new t_pppParam(t_pppParam::rClkR, t_prn(), t_lc::dummy));
   }
   if (_usedSystems['E']) {
   //if (OPT->useSystem('E')) {
     required.push_back(new t_pppParam(t_pppParam::rClkE, t_prn(), t_lc::dummy));
   }
   if (_usedSystems['C']) {
   //if (OPT->useSystem('C')) {
     required.push_back(new t_pppParam(t_pppParam::rClkC, t_prn(), t_lc::dummy));
   }

  // Troposphere
  // -----------
  if (OPT->estTrp()) {
    required.push_back(new t_pppParam(t_pppParam::trp, t_prn(), t_lc::dummy));
  }

  // Ionosphere
  // ----------
  if (OPT->_ionoModelType == OPT->est) {
    for (unsigned jj = 0; jj < obsVector.size(); jj++) {
      const t_pppSatObs* satObs = obsVector[jj];
      char sys = satObs->prn().system();
      std::vector<t_lc::type> LCs = OPT->LCs(sys);
      if (std::find(LCs.begin(), LCs.end(), t_lc::cIF) == LCs.end() &&
          std::find(LCs.begin(), LCs.end(), t_lc::lIF) == LCs.end()) {
        required.push_back(new t_pppParam(t_pppParam::ion, satObs->prn(), t_lc::dummy));
      }
    }
  }
  // Ambiguities
  // -----------
  for (unsigned jj = 0; jj < obsVector.size(); jj++) {
    const t_pppSatObs*  satObs = obsVector[jj];
    char sys = satObs->prn().system();
    const vector<t_lc::type>& ambLCs = OPT->ambLCs(sys);
    for (unsigned ii = 0; ii < ambLCs.size(); ii++) {
      required.push_back(new t_pppParam(t_pppParam::amb, satObs->prn(), ambLCs[ii], &obsVector));
    }
  }

  // Receiver Code Biases
  // --------------------
    if (OPT->_ionoModelType == OPT->PPP_RTK) {
    std::vector<t_lc::type> lc;
    if (OPT->useSystem('G')) {
      lc = OPT->LCs('G');
      if (std::find(lc.begin(), lc.end(), t_lc::c1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasG1, t_prn(), t_lc::c1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasG2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('R')) {
      lc = OPT->LCs('R');
      if (std::find(lc.begin(), lc.end(), t_lc::c1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasR1, t_prn(), t_lc::c1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasR2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('E')) {
      lc = OPT->LCs('E');
      if (std::find(lc.begin(), lc.end(), t_lc::c1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasE1, t_prn(), t_lc::c1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasE2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('C')) {
      lc = OPT->LCs('C');
      if (std::find(lc.begin(), lc.end(), t_lc::c1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasC1, t_prn(), t_lc::c1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasC2, t_prn(), t_lc::c2));
      }
    }
  }

  if (OPT->_pseudoObsIono) {
    std::vector<t_lc::type> lc;
    if (OPT->useSystem('G')) {
      lc = OPT->LCs('G');
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasG2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('R')) {
      lc = OPT->LCs('R');
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasR2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('E')) {
      lc = OPT->LCs('E');
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasE2, t_prn(), t_lc::c2));
      }
    }
    if (OPT->useSystem('C')) {
      lc = OPT->LCs('C');
      if (std::find(lc.begin(), lc.end(), t_lc::c2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::cBiasC2, t_prn(), t_lc::c2));
      }
    }
  }

  if (OPT->_ionoModelType == OPT->PPP_RTK) {
    std::vector<t_lc::type> lc;
    if (OPT->useSystem('G')) {
      lc = OPT->LCs('G');
      if (std::find(lc.begin(), lc.end(), t_lc::l1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasG1, t_prn(), t_lc::l1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::l2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasG2, t_prn(), t_lc::l2));
      }
    }
    if (OPT->useSystem('R')) {
      lc = OPT->LCs('R');
      if (std::find(lc.begin(), lc.end(), t_lc::l1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasR1, t_prn(), t_lc::l1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::l2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasR2, t_prn(), t_lc::l2));
      }
    }
    if (OPT->useSystem('E')) {
      lc = OPT->LCs('E');
      if (std::find(lc.begin(), lc.end(), t_lc::l1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasE1, t_prn(), t_lc::l1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::l2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasE2, t_prn(), t_lc::l2));
      }
    }
    if (OPT->useSystem('C')) {
      lc = OPT->LCs('C');
      if (std::find(lc.begin(), lc.end(), t_lc::l1) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasC1, t_prn(), t_lc::l1));
      }
      if (std::find(lc.begin(), lc.end(), t_lc::l2) != lc.end()) {
        required.push_back(new t_pppParam(t_pppParam::pBiasC2, t_prn(), t_lc::l2));
      }
    }
  }

  // Check if all required parameters are present
  // --------------------------------------------
  for (unsigned ii = 0; ii < required.size(); ii++) {
    t_pppParam* parReq = required[ii];

    bool found = false;
    for (unsigned jj = 0; jj < _params.size(); jj++) {
      t_pppParam* parOld = _params[jj];
      if (parOld->isEqual(parReq)) {
        found = true;
        break;
      }
    }
    if (found) {
      delete parReq;
    }
    else {
#ifdef BNC_DEBUG_PPP
      //LOG << "push_back  parReq " << parReq->toString() << std::endl;
#endif
      _params.push_back(parReq);
    }
  }

  // Set Parameter Indices
  // ---------------------
  sort(_params.begin(), _params.end(), t_pppParam::sortFunction);

  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    par->setIndex(ii);
    for (unsigned jj = 0; jj < obsVector.size(); jj++) {
      const t_pppSatObs* satObs = obsVector[jj];
      if (satObs->prn() == par->prn()) {
        par->setAmbEleSat(satObs->eleSat());
        par->stepAmbNumEpo();
      }
    }
  }

  return success;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppParlist::printResult(const bncTime& epoTime, const SymmetricMatrix& QQ,
                               const ColumnVector& xx) const {

  string epoTimeStr = string(epoTime);
  const t_pppStation* sta = PPP_CLIENT->staRover();

  LOG << endl;

  t_pppParam* parX = 0;
  t_pppParam* parY = 0;
  t_pppParam* parZ = 0;
  for (unsigned ii = 0; ii < _params.size(); ii++) {
    t_pppParam* par = _params[ii];
    if      (par->type() == t_pppParam::crdX) {
      parX = par;
    }
    else if (par->type() == t_pppParam::crdY) {
      parY = par;
    }
    else if (par->type() == t_pppParam::crdZ) {
      parZ = par;
    }
    else {
      int ind = par->indexNew();
      double apr = (par->type() == t_pppParam::trp) ?
        t_tropo::delay_saast(sta->xyzApr(), M_PI/2.0) :  par->x0();
      LOG << epoTimeStr << ' ' << par->toString() << ' '
          << setw(10) << setprecision(4) << apr << ' '
          << showpos << setw(10) << setprecision(4) << xx[ind] << noshowpos << " +- "
          << setw(8)  << setprecision(4) << sqrt(QQ[ind][ind]);
      if (par->type() == t_pppParam::amb) {
        LOG << " el = " << setw(6) << setprecision(2) << par->ambEleSat() * 180.0 / M_PI
            << " epo = " << setw(4) << par->ambNumEpo();
      }
      LOG << endl;
    }
  }

  if (parX && parY && parZ) {

	ColumnVector xyz(3);
    xyz[0] = xx[parX->indexNew()];
    xyz[1] = xx[parY->indexNew()];
    xyz[2] = xx[parZ->indexNew()];

    ColumnVector neu(3);
    xyz2neu(sta->ellApr().data(), xyz.data(), neu.data());

    SymmetricMatrix QQxyz = QQ.SymSubMatrix(1,3);

    SymmetricMatrix QQneu(3);
    covariXYZ_NEU(QQxyz, sta->ellApr().data(), QQneu);

    LOG << epoTimeStr << ' ' << sta->name()
        << " X = " << setprecision(4) << sta->xyzApr()[0] + xyz[0] << " +- "
        << setprecision(4) << sqrt(QQxyz[0][0])

        << " Y = " << setprecision(4) << sta->xyzApr()[1] + xyz[1] << " +- "
        << setprecision(4) << sqrt(QQxyz[1][1])

        << " Z = " << setprecision(4) << sta->xyzApr()[2] + xyz[2] << " +- "
        << setprecision(4) << sqrt(QQxyz[2][2])

        << " dN = " << setprecision(4) << neu[0] << " +- "
        << setprecision(4) << sqrt(QQneu[0][0])

        << " dE = " << setprecision(4) << neu[1] << " +- "
        << setprecision(4) << sqrt(QQneu[1][1])

        << " dU = " << setprecision(4) << neu[2] << " +- "
        << setprecision(4) << sqrt(QQneu[2][2])

        << endl;
  }
  return;
}

//
////////////////////////////////////////////////////////////////////////////
void t_pppParlist::printParams(const bncTime& epoTime) {

  for (unsigned iPar = 0; iPar < _params.size(); iPar++) {
    LOG << _params[iPar]->toString()
        << "\t  lastObsTime().valid() \t" << _params[iPar]->lastObsTime().valid()
        << "\t  epoTime - par->lastObsTime() \t" << (epoTime - _params[iPar]->lastObsTime())
        << endl;
  }
}

