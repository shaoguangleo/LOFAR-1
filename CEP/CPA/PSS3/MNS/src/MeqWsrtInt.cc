//# MeqWsrtInt.cc: The integrated point source contribution
//#
//# Copyright (C) 2002
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//# $Id$

#include <MNS/MeqWsrtInt.h>
#include <MNS/MeqIntDFT.h>
#include <MNS/MeqRequest.h>
#include <MNS/MeqResult.h>
#include <MNS/MeqParm.h>
#include <MNS/MeqMatrix.h>
#include <MNS/MeqMatrixTmp.h>
#include <Common/Debug.h>
#include <aips/Arrays/Matrix.h>


MeqWsrtInt::MeqWsrtInt (const MeqWsrtPoint* expr, const MeqJonesExpr* station1,
			const MeqJonesExpr* station2)
: itsExpr  (expr),
  itsStat1 (station1),
  itsStat2 (station2)
{}

MeqWsrtInt::~MeqWsrtInt()
{}

void MeqWsrtInt::calcResult (const MeqRequest& request)
{
  const MeqDomain& domain = request.domain();
  MeqResult result(request.nspid());
  // Loop through all the bins in the domain.
  double stf = -1;
  for (int if=0; if<domain.ny(); if++) {
    double stt = -1;
    for (int it=0; it<domain.ny(); it++) {
      MeqDomain dom(stf, stf+domain.

  // We can only calculate for a single bin.
  const MeqDomain& domain = request.domain();
  Assert (domain.nx() == 1  &&  domain.ny() == 1);
  // Find the maximum nr of cells needed.
  int ncellt = 0;
  int ncellf = 0;
  for (vector<MeqIntSource>::iterator iter = itsSources.begin();
       iter != itsSources.end();
       iter++) {
    vector<int> ncell = itsDFT->ncells (*iter, domain);
    if (ncell[0] > ncellt) {
      ncellt = ncell[0];
    }
    if (ncell[1] > ncellf) {
      ncellf = ncell[1];
    }
  }
  itsNcell.resize (2);
  itsNcell[0] = ncellt;
  itsNcell[1] = ncellf;
  
  // The domain is divided into the required number of cells.
  MeqDomain dftDom (domain.startX(), domain.endX(), ncellt,
		    domain.startY(), domain.endY(), ncellf);
  MeqRequest dftReq (dftDom, request.nspid());
  itsXX = MeqResult(request.nspid());
  itsXY = MeqResult(request.nspid());
  itsYX = MeqResult(request.nspid());
  itsYY = MeqResult(request.nspid());
  Matrix<complex<double> > value(ncellt, ncellf, complex<double>(0,0));
  itsXX.setValue (MeqMatrix (value));
  itsYX.setValue (MeqMatrix (value));
  itsXY.setValue (MeqMatrix (value));
  itsYY.setValue (MeqMatrix (value));
  int srcnr = 0;
  for (vector<MeqIntSource>::iterator iter = itsSources.begin();
       iter != itsSources.end();
       iter++) {
    dftReq.setSourceNr (srcnr++);
    MeqResult ik = iter->getI()->getResult (dftReq);
    MeqResult qk = iter->getQ()->getResult (dftReq);
    MeqResult uk = iter->getU()->getResult (dftReq);
    MeqResult vk = iter->getV()->getResult (dftReq);
    MeqResult dft = itsDFT->getResult (dftReq);
    MeqMatrixTmp ivk = tocomplex(0, vk.getValue());
    MeqMatrix xx = (ik.getValue() + qk.getValue()) * dft.getValue();
    MeqMatrix yx = (uk.getValue() - MeqMatrix(ivk)) * dft.getValue();
    MeqMatrix xy = (uk.getValue() + ivk) * dft.getValue();
    MeqMatrix yy = (ik.getValue() - qk.getValue()) * dft.getValue();
    itsXX.getValueRW() += xx;
    itsXY.getValueRW() += xy;
    itsYX.getValueRW() += yx;
    itsYY.getValueRW() += yy;

    // Evaluate (if needed) for the perturbed parameter values.
    MeqMatrix perturbation;
    for (int spinx=0; spinx<request.nspid(); spinx++) {
      // Determine which expression part is perturbed.
      bool evaliq = false;
      if (ik.isDefined(spinx)) {
	evaliq = true;
	perturbation = ik.getPerturbation(spinx);
      }
      if (qk.isDefined(spinx)) {
	evaliq = true;
	perturbation = qk.getPerturbation(spinx);
      }
      bool evaluv = false;
      if (uk.isDefined(spinx)) {
	evaluv = true;
	perturbation = uk.getPerturbation(spinx);
      }
      MeqMatrix ivkp(ivk);
      if (vk.isDefined(spinx)) {
	evaluv = true;
	perturbation = vk.getPerturbation(spinx);
	ivkp = tocomplex(0, vk.getPerturbedValue(spinx));
      }
      if (dft.isDefined(spinx)) {
	evaliq = true;
	evaluv = true;
	perturbation = dft.getPerturbation(spinx);
      }
      // Calculate if one of the results is perturbed for this parameter.
      if (evaliq || evaluv) {
	const MeqMatrix& dkp = dft.getPerturbedValue(spinx);
	if (evaliq) {
	  const MeqMatrix& ikp = ik.getPerturbedValue(spinx);
	  const MeqMatrix& qkp = qk.getPerturbedValue(spinx);
	  xx = (ikp + qkp) * dkp;
	  yy = (ikp - qkp) * dkp;
	  // If not calculated before, initialize to unperturbed sum.
	  if (! itsXX.isDefined(spinx)) {
	    itsXX.setPerturbedValue (spinx, itsXX.getValue().clone());
	    itsXX.setPerturbation (spinx, perturbation);
	  }
	  if (! itsYY.isDefined(spinx)) {
	    itsYY.setPerturbedValue (spinx, itsYY.getValue().clone());
	    itsYY.setPerturbation (spinx, perturbation);
	  }
	  itsXX.getPerturbedValueRW(spinx) += xx;
	  itsYY.getPerturbedValueRW(spinx) += yy;
	}
	if (evaluv) {
	  const MeqMatrix& ukp = uk.getPerturbedValue(spinx);
	  xy = (ukp - ivkp) * dkp;
	  yx = (ukp + ivkp) * dkp;
	  if (! itsXY.isDefined(spinx)) {
	    itsXY.setPerturbedValue (spinx, itsXY.getValue().clone());
	    itsXY.setPerturbation (spinx, perturbation);
	  }
	  if (! itsYX.isDefined(spinx)) {
	    itsYX.setPerturbedValue (spinx, itsYX.getValue().clone());
	    itsYX.setPerturbation (spinx, perturbation);
	  }
	  itsXY.getPerturbedValueRW(spinx) += xy;
	  itsYX.getPerturbedValueRW(spinx) += yx;
	}
      } else {
	// No perturbed values in result for this parameter.
	// Add unperturbed value if previous results were perturbed.
	if (itsXX.isDefined(spinx)) {
	  itsXX.getPerturbedValueRW(spinx) += xx;
	}
	if (itsXY.isDefined(spinx)) {
	  itsXY.getPerturbedValueRW(spinx) += xx;
	}
	if (itsYX.isDefined(spinx)) {
	  itsYX.getPerturbedValueRW(spinx) += xx;
	}
	if (itsYY.isDefined(spinx)) {
	  itsYY.getPerturbedValueRW(spinx) += xx;
	}
      }
    }
  }
}
