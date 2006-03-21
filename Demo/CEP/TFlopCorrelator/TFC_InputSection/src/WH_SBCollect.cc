//#  WH_SBCollect.cc: Joins all data (stations, pols) for a subband
//#
//#  Copyright (C) 2002-2005
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

#include <lofar_config.h>

// General includes
#include <Common/LofarLogger.h>
#include <APS/ParameterSet.h>

// Application specific includes
#include <TFC_InputSection/WH_SBCollect.h>
#include <TFC_Interface/DH_RSP.h>
#include <TFC_Interface/DH_Subband.h>

#include <Common/hexdump.h>

using namespace LOFAR;

WH_SBCollect::WH_SBCollect(const string& name, int sbID, 
			   const ACC::APS::ParameterSet pset) 
  : WorkHolder   (pset.getInt32("Data.NStations"), 
		  pset.getInt32("FakeData.NSubbands")/pset.getInt32("Data.NSubbands"),
		  name,
		  "WH_SBCollect"),
    itsPS        (pset),
    itsSubBandID (sbID)
{
  char str[32];
  for (int i=0; i<itsNinputs; i++)
  {
    sprintf(str, "DH_in_%d", i);
    getDataManager().addInDataHolder(i, new DH_RSP(str, itsPS));
  }
  for(int i=0;i<itsNoutputs; i++)
  {
    sprintf(str, "DH_out_%d", i);
    getDataManager().addOutDataHolder(i, new DH_Subband(str, itsSubBandID, itsPS));
  }
}

WH_SBCollect::~WH_SBCollect() {
}

WorkHolder* WH_SBCollect::construct(const string& name, int sbID, 
				    const ACC::APS::ParameterSet pset) 
{
  return new WH_SBCollect(name, sbID, pset);
}

WH_SBCollect* WH_SBCollect::make(const string& name)
{
  return new WH_SBCollect(name, itsSubBandID, itsPS);
}

void WH_SBCollect::process() 
{ 
  RectMatrix<DH_RSP::BufferType>* inMatrix = &((DH_RSP*)getDataManager().getInHolder(0))->getDataMatrix();
  RectMatrix<DH_Subband::BufferType>* outMatrix = &(((DH_Subband*)getDataManager().getOutHolder(0))->getDataMatrix());

  dimType outStationDim = outMatrix->getDim("Station");
  dimType inStationDim = inMatrix->getDim("Stations");

  RectMatrix<DH_Subband::BufferType>::cursorType outCursor;
  RectMatrix<DH_RSP::BufferType>::cursorType inCursor;

  for (int sb=0; sb<itsNoutputs; sb++) 
  {
    outMatrix = &(((DH_Subband*)getDataManager().getOutHolder(sb))->getDataMatrix());
    outCursor = outMatrix->getCursor( 0 * outStationDim);

    // Loop over all inputs (stations)
    for (int nr=0; nr<itsNinputs; nr++)
    {
      inMatrix = &((DH_RSP*)getDataManager().getInHolder(nr))->getDataMatrix();
      inCursor = inMatrix->getCursor(0*inStationDim);
      // copy all freq, time and pol from an input to output
      inMatrix->cpy2Matrix(inCursor, inStationDim, *outMatrix, outCursor, outStationDim, 1);
      outMatrix->moveCursor(&outCursor, outStationDim);
    }
  }


#if 0
  // dump the contents of outDH to stdout
  cout << "WH_SBCollect output : " << endl;
  dimType outTimeDim = outMatrix->getDim("Time");
  dimType outPolDim = outMatrix->getDim("Polarisation");
  int matrixSize = itsNinputs * 
    outMatrix->getNElemInDim(outTimeDim) * 
    outMatrix->getNElemInDim(outPolDim);    

  hexdump(outMatrix->getBlock(outMatrix->getCursor(0), 
			     outStationDim, 
			     itsNinputs,
			     matrixSize),
	  100);//matrixSize * sizeof(DH_Subband::BufferType));
  hexdump(dynamic_cast<DH_Subband*>(getDataManager().getOutHolder(0))->getBuffer(),
	  100);//matrixSize * sizeof(DH_Subband::BufferType));
  cout << "WH_SBCollect output done " << endl;
#endif

}
