//#  WH_Correlator.cc: Round robin BG/L correlator. Using hard real-time
//#  property of BG/L so we don't have to synchronize.
//#
//#  Copyright (C) 2002-2004
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  This program is free software; you can redistribute it and/or modify
//#  it under the terms of the GNU General Public License as published by
//#  the Free Software Foundation; either version 2 of the License, or
//#  (at your option) any later version.
//#
//#  This program is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#  GNU General Public License for more details.
//#
//#  You should have received a copy of the GNU General Public License
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//#  $Id$

#include <stdio.h> 

// Application Specific includes
#include <WH_Correlator.h>

using namespace LOFAR;


WH_Correlator::WH_Correlator(const string& name, 
			     unsigned int nin,
			     unsigned int nout,
			     unsigned int nelements,
			     unsigned int nsamples,
			     unsigned int nchannels,
			     unsigned int nruns) 
  : WorkHolder(nin, nout, name, "WH_Correlator"),
    itsNelements(nelements),
    itsNsamples(nsamples),
    itsNchannels(nchannels),
    itsNruns(nruns)
{
  
  char str[8];
  for (unsigned int i = 0; i < nin; i++) {
    sprintf(str, "%d", i);

    if (i == 0) {
      getDataManager().addInDataHolder(i, new DH_CorrCube(string("in_") + str,
							  itsNelements, 
							  itsNsamples, 
							  itsNchannels));
    } else {
      getDataManager().addInDataHolder(i, new DH_Vis(string("in_") + str,
							    itsNelements, 
							    itsNchannels));
    }
  }
    
  for (unsigned int i = 0; i < nout; i++) {
    sprintf(str, "%d", i);
    if (i == 0) {
      getDataManager().addOutDataHolder(i, new DH_Vis(string("out_")+str,
						      itsNelements, itsNchannels));
    } else {
      getDataManager().addOutDataHolder(i, new DH_CorrCube(string("out_") + str, 
							   itsNelements, 
							   itsNsamples, 
							   itsNchannels));
    }
  }
}

WH_Correlator::~WH_Correlator() {
}

WorkHolder* WH_Correlator::construct(const string& name, 
				     unsigned int nin, 
				     unsigned int nout, 
				     unsigned int nelements, 
				     unsigned int nsamples,
				     unsigned int nchannels, 
				     unsigned int nruns) {
  return new WH_Correlator(name, nin, nout, nelements, nsamples, nchannels, nruns);
}

WH_Correlator* WH_Correlator::make(const string& name) {
  return new WH_Correlator(name, 
			   getDataManager().getInputs(), 
			   getDataManager().getOutputs(),
			   itsNelements, 
			   itsNsamples, 
			   itsNchannels, 
			   itsNruns);
}

void WH_Correlator::preprocess() {
}

void WH_Correlator::process() {

  if (TH_MPI::getCurrentRank() == 0) {
    
    WH_Correlator::master();
    
  } else {

    WH_Correlator::slave();

  }
}

void WH_Correlator::postprocess() {
}

void WH_Correlator::dump() {
}

/* un-optimized correlator for now, but using pointers to save memcpy's */
void WH_Correlator::correlator_core(DH_CorrCube::BufferType& sig, 
				    DH_Vis::BufferType& cor) {

  /* This does the cross and autocorrelation on the lower half of the matrix */
  /* The upper half is not calculated, but is simply the complex conjugate   */
  /* the lower half. This should be implemented as a post correlation        */
  /* procedure later on, but is for now omitted. -- CB                       */

  cor = sig;

//   cout << "Correlating" << endl;
//   for (int time = 0; time < itsNsamples; time++) {
//     for (int x = 0; x < itsNelements; x++) {
//       for (int y = 0; y <= x; y++) {
// 	*(cor+x*itsNelements+y) += DH_Vis::BufferType
// 				     (// real
// 				      (sig+x*itsNelements+time)->real() * 
// 				      (sig+y*itsNelements+time)->real() -
// 				      (sig+x*itsNelements+time)->imag() *
// 				      (sig+y*itsNelements+time)->imag(),
// 				      // imag
// 				      (sig+x*itsNelements+time)->real() * 
// 				      (sig+y*itsNelements+time)->imag() +
// 				      (sig+x*itsNelements+time)->imag() * 
// 				      (sig+y*itsNelements+time)->real()
// 				      );
//       }
//     }
//   }
}

void WH_Correlator::master() {
  
  for (int i = 1; i < TH_MPI::getNumberOfNodes(); i++) {
    
    /* [CHECKME] */
//     *(((DH_CorrCube*)getDataManager().getOutHolder(i))->getBuffer()) = 
//       *(((DH_CorrCube*)getDataManager().getInHolder(0))->getBuffer()) + 
//       DH_CorrCube::BufferType (itsNelements * itsNsamples * i);


    memcpy((((DH_CorrCube*)getDataManager().getOutHolder(i))->getBuffer()), 
	   (((DH_CorrCube*)getDataManager().getInHolder(0))->getBuffer()) +
	   itsNelements * itsNsamples * i,
	   itsNelements *itsNsamples);
  }

  /* Gather and assign results */ 
  for (int i = 1; i < TH_MPI::getNumberOfNodes(); i++) {
    
    /* [CHECKME] */
//     *(((DH_Vis*)getDataManager().getOutHolder(0))->getBuffer()) +
//       DH_Vis::BufferType (itsNelements * itsNsamples * i) = 
//       *(((DH_Vis*)getDataManager().getInHolder(i))->getBuffer());

    memcpy((((DH_Vis*)getDataManager().getOutHolder(0))->getBuffer()) +
	    itsNelements * itsNsamples * i, 
	    (((DH_Vis*)getDataManager().getInHolder(i))->getBuffer()),
	    itsNelements * itsNsamples);
    
  }
  cout << "END"  << endl;
}

void WH_Correlator::slave() {

  correlator_core(*((DH_CorrCube*)getDataManager().getInHolder(0))->getBuffer(), 
		  *((DH_Vis*)getDataManager().getOutHolder(0))->getBuffer());
}
