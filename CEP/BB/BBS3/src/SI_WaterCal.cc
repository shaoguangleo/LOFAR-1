//#  SI_WaterCal.cc:  The watercal calibration strategy
//#
//#  Copyright (C) 2002-2003
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

#include <BBS3/SI_WaterCal.h>
#include <Common/Debug.h>
#include <Common/KeyValueMap.h>
#include <BBS3/MeqCalibraterImpl.h>
#include <unistd.h>

namespace LOFAR
{

SI_WaterCal::SI_WaterCal(MeqCalibrater* cal, const KeyValueMap& args)
  : StrategyImpl(),
    itsCal(cal),
    itsCurIter(-1),
    itsInitialized(false),
    itsFirstCall(true)
{
  itsNIter = args.getInt("nrIterations", 1);
  itsSourceNo = args.getInt("source", 1);
  itsTimeInterval = args.getFloat("timeInterval", 10.0);
  itsStartChan = args.getInt("startChan", 0);
  itsEndChan = args.getInt("endChan", 0);
  itsAnt = (const_cast<KeyValueMap&>(args))["antennas"].getVecInt();

  TRACER1("Creating WaterCal strategy implementation with " 
	  << "number of iterations = " << itsNIter << ", "
	  << "source number = " << itsSourceNo << ", "
	  << " time interval = " << itsTimeInterval);

}

SI_WaterCal::~SI_WaterCal()
{}

bool SI_WaterCal::execute(vector<string>& parmNames, 
			 vector<string>& resultParmNames,
			 vector<double>& resultParmValues,
			 Quality& resultQuality,
			 int& resultIterNo)
{
  AssertStr(itsCal != 0, 
	    "Calibrator pointer not set for this watercal strategy");

  if (itsFirstCall)
  {
    if (!itsInitialized)
    {
      itsCal->select(itsAnt, itsAnt, itsStartChan, itsEndChan);
      itsCal->setTimeInterval(itsTimeInterval);
      itsCal->clearSolvableParms();
      itsInitialized = true;
    }

    vector<string> emptyP(0);
    itsCal->setSolvableParms(parmNames, emptyP, true);

    vector<int> emptyS;
    vector<int> sources;
    sources.push_back(itsSourceNo);
    vector<int>::const_iterator iter;
    for (iter = itsExtraPeelSrcs.begin(); iter!= itsExtraPeelSrcs.end(); iter++)
    {
      if (*iter != itsSourceNo)
      { 
	sources.push_back(*iter);
	TRACER1("SI_Randomized::execute : Adding an extra peel source " << *iter);
      }
    }
    itsCal->peel(sources, emptyS);  
    
    itsCal->resetIterator();
    itsCal->nextInterval();
    TRACER1("Next interval");
   
    itsFirstCall = false;
  }

  TRACER1("Next iteration: " << itsCurIter+1);
  if (++itsCurIter >= itsNIter)          // Next iteration
  {                                      // Finished with all iterations
    TRACER1("Next interval");
    if (itsCal->nextInterval() == false) // Next time interval
    {                                    // Finished with all time intervals
      itsCurIter = -1;
      itsInitialized = false;
      itsFirstCall = true;
      return false;                      // Finished with all sources
    }
    itsCurIter = 0;                      // Reset iterator
  }

  // The actual solve
  TRACER1("Solve for source = " << itsSourceNo  
	  << " iteration = " << itsCurIter << " of " << itsNIter);

  itsCal->solve(false, resultParmNames, resultParmValues, resultQuality);
  //  itsCal->SubtractOptimizedSources();
  // itsCal->showCurrentParms ();

  itsCal->saveParms();
  resultIterNo = itsCurIter;
  return true;
}

bool SI_WaterCal::useParms (const vector<string>& parmNames, 
			    const vector<double>& parmValues, 
			    const vector<int>& srcNumbers)
{
  AssertStr(itsCal != 0, 
	    "Calibrator pointer not set for this watercal strategy");

  itsCal->select(itsAnt, itsAnt, itsStartChan, itsEndChan);
  itsCal->setTimeInterval(itsTimeInterval);
  itsCal->clearSolvableParms();
  itsInitialized = true;
  //itsCal->ShowSettings();
  itsCal->resetIterator();
  itsCal->nextInterval();

  itsCal->setParmValues(parmNames, parmValues);

  vector<string> emptyP(0);
  itsCal->setSolvableParms(const_cast<vector<string>&>(parmNames), 
                           emptyP, true); // Add all parms

  itsCal->saveParms();                               // save values in parm table

  itsCal->clearSolvableParms();

  for (unsigned int i=0; i < srcNumbers.size(); i++)      
  { 
    itsExtraPeelSrcs.push_back(srcNumbers[i]);
    TRACER1("SI_WaterCal::useParms : Using a start solution for source " 
	    << srcNumbers[i]);
  }

  return true;
}

} // namespace LOFAR
