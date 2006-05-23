//# Solver.cc: Calculate parameter values using a least squares fitter
//#
//# Copyright (C) 2004
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

#include <lofar_config.h>

#include <BBS/Solver.h>
#include <Common/VectorUtil.h>
#include <Common/LofarLogger.h>
#include <Common/Timer.h>

#include <BBS/BBSTestLogger.h>

#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace casa;

namespace LOFAR
{

Solver::Solver ()
: itsDoSet  (true)
{
  LOG_INFO_STR( "Solver constructor" );
}

Solver::~Solver()
{
  LOG_TRACE_FLOW( "Solver destructor" );
}

void Solver::solve (bool useSVD)
{
  LOG_INFO_STR( "solve using file ");
  ASSERT (!itsDoSet);
  BBSTest::ScopedTimer timer("S:solver");
  vector<ParmData>& globalParms = itsParmInfo.parms();
  for (uint i=0; i<itsFitters.size(); ++i) {
    FitterData& fitObj = itsFitters[i];
    //  timer.start();
    ASSERT (fitObj.solvableValues.size() > 0);
    // Solve the equation. 
    uint rank;
    double fit;
    LOG_INFO_STR( "Solution before: " << fitObj.solvableValues);
    rank = 0;
    ///    BBSTest::Logger::log("Before: ", itsParmInfo);
    bool solFlag = fitObj.fitter.solveLoop (fit, rank,
					    &(fitObj.solvableValues[0]),
					    useSVD);
    ///    BBSTest::Logger::log("After: ", itsParmInfo);
    LOG_INFO_STR( "Solution after:  " << fitObj.solvableValues);

    fitObj.quality.init();
    fitObj.quality.itsSolFlag = solFlag;
    fitObj.quality.itsRank    = rank;
    fitObj.quality.itsFit     = fit;
    fitObj.quality.itsMu      = fitObj.fitter.getWeightedSD();
    fitObj.quality.itsStddev  = fitObj.fitter.getSD();
    fitObj.quality.itsChi     = fitObj.fitter.getChi();

    // Store the new values in the ParmData vector.
    for (uint j=0; j<globalParms.size(); ++j) {
      // Update the values of each parm for this solve domain.
      int nr = globalParms[j].update (i, fitObj.solvableValues);
      DBGASSERT (nr = globalParms[j].info()[i].nscid);
    }
  }
  //  timer.stop();
  //  BBSTest::Logger::log("solver", timer);
  itsDoSet = false;
  return;
}

void Solver::mergeFitters (const vector<LSQFit>& fitters, int prediffer)
{
  ASSERT (uint(prediffer) < itsPredInfo.size());
  // Initialize the solvers (needed after a setSolvable).
  if (itsDoSet) {
    for (uint i=0; i<itsFitters.size(); ++i) {
      itsFitters[i].fitter.set (itsFitters[i].solvableValues.size());
      itsFitters[i].nused = 0;
      itsFitters[i].nflagged = 0;
    }
    itsDoSet = false;
  }
  // Merge all fitters from the given prediffer with the global fitters.
  // Check if things are ok.
  PredifferInfo& predInfo = itsPredInfo[prediffer];
  ASSERT (fitters.size() == predInfo.solveDomainIndices.size());
  for (uint i=0; i<fitters.size(); ++i) {
    // Get index of the corresponding global fitter.
    int fitInx = predInfo.solveDomainIndices[i];
    // Get the mapping to unknowns in global fitter.
    vector<uint>& scidInx = predInfo.scidMap[fitInx];
    // Check if given nr of unknowns matches the index length.
    ASSERT (fitters[i].nUnknowns() == scidInx.size());
    ASSERT (itsFitters[fitInx].fitter.merge (fitters[i],
					     scidInx.size(), &scidInx[0]));
  }
  uint rank;
  double fit;
  vector<double> sol = itsFitters[0].solvableValues;
  cout << "after local " << sol << endl;
  bool solFlag = const_cast<LSQFit&>(fitters[0]).solveLoop (fit, rank,
				       &(sol[0]),
				       true);
  cout << "after local " << sol << endl;
}

void Solver::initSolvableParmData (int nrPrediffers,
				   const vector<MeqDomain>& solveDomains,
				   const MeqDomain& workDomain)
{
  // Initialize the parm name map.
  itsNameMap.clear();
  // The number of fitters is equal to the number of solve domains.
  itsFitters.resize (solveDomains.size());
  // Set the domain info.
  itsParmInfo.set (solveDomains, workDomain);
  // Check if the work domain contains all solve domains.
  ASSERT (itsParmInfo.getDomains().size() == solveDomains.size());
  // Initialize the info for each Prediffer.
  itsPredInfo.resize (nrPrediffers);
  itsDoSet = true;
}

// The solvable parameters coming from the prediffers have to be collected
// and put into a single solver collection.
void Solver::setSolvableParmData (const ParmDataInfo& data,
				  int prediffer)
{
  if (data.empty()) {
    return;
  }
  itsDoSet = true;
  PredifferInfo& predInfo = itsPredInfo[prediffer];
  // Keep the solve domain indices of this Prediffer.
  // It maps the Prediffer solve domains to the global solve domains.
  predInfo.solveDomainIndices = data.getDomainIndices();
  // Initialize the map of scids from prediffer to global fitters.
  predInfo.scidMap.resize (predInfo.solveDomainIndices.size());
  int totnDom = itsParmInfo.getDomainIndices().size();
  // Handle all parms of the given prediffer.
  const vector<ParmData>& predParms = data.parms();
  vector<ParmData>& globalParms = itsParmInfo.parms();
  // Loop through all parm entries and see if already used.
  for (uint i=0; i<predParms.size(); ++i) {
    const ParmData& predParm = predParms[i];
    uint parminx = itsNameMap.size();       // index for new parms
    map<string,int>::const_iterator value =
                                       itsNameMap.find (predParm.getName());
    if (value == itsNameMap.end()) {
      // Not in use yet; so add it to the map and global parminfo.
      DBGASSERT (parminx == globalParms.size());
      itsNameMap[predParm.getName()] = parminx;
      globalParms.push_back (ParmData(predParm.getName(),
				      predParm.getParmDBSeqNr()));
      globalParms[parminx].info().resize (totnDom);
    } else {
      // The parameter has already been used. Get its index.
      parminx = value->second;
      ASSERT (globalParms[parminx] == predParm);
    }
    // Loop through all solve domains of this parm.
    // If already handled, check if equal nscid, otherwise set nscid.
    ParmData& globParm = globalParms[parminx];
    for (uint j=0; j<predInfo.solveDomainIndices.size(); ++j) {
      int domInx = predInfo.solveDomainIndices[j];
      int fScid  = 0;
      if (globParm.info()[domInx].coeff.isNull()) {
	// This domain is new for this parm.
	// Copy the ParmData domain info.
	globParm.info()[domInx] = predParm.info()[j];
	fScid = itsFitters[domInx].solvableValues.size();
	// Append the coeff values to the solvable values of this domain.
	globParm.setValues (domInx, itsFitters[domInx].solvableValues);
      } else {
	ASSERT (globParm.info()[domInx].nscid == predParm.info()[j].nscid);
	fScid = globParm.info()[domInx].firstScid;
      }
      // Map the prediffer scids to the global ones.
      vector<uint>& scidInx = predInfo.scidMap[j];
      int nrs = predParm.info()[j].nscid;
      scidInx.reserve (scidInx.size() + nrs);
      for (int sc=0; sc<nrs; ++sc) {
	scidInx.push_back (fScid+sc);
      }
    }
  }
}

const vector<double>& Solver::getSolvableValues (uint fitterIndex) const
{
  ASSERT (fitterIndex < itsFitters.size());
  return itsFitters[fitterIndex].solvableValues;
}

const Quality& Solver::getQuality (uint fitterIndex) const
{
  ASSERT (fitterIndex < itsFitters.size());
  return itsFitters[fitterIndex].quality;
}

void Solver::show (ostream& os)
{
  os << "Solver Names:" << endl;
  for (map<string,int>::iterator iter = itsNameMap.begin();
       iter != itsNameMap.end();
       iter++) {
    os << ' ' << iter->first << ' ' << iter->second << endl;
  }
  os << "Solver ParmInfo: " << endl;
  itsParmInfo.show (os);
  os << "Solver Fitter Values:" << endl;
  for (uint i=0; i<itsFitters.size(); ++i) {
    os << ' ' << itsFitters[i].solvableValues << endl;
  }
  os << "Solver PredInfo:" << endl;
  for (uint i=0; i<itsPredInfo.size(); ++i) {
    os << " sdindices " << itsPredInfo[i].solveDomainIndices << endl;
    os << " scids    ";
    for (uint j=0; j<itsPredInfo[i].scidMap.size(); ++j) {
      os << " " << itsPredInfo[i].scidMap[j];
    }
    os << endl;
  }
}

} // namespace LOFAR

//# Instantiate the makeNorm template.
#include <scimath/Fitting/LSQFit2.cc>
template void casa::LSQFit::makeNorm<double, double*, int*>(unsigned, int* const&, double* const&, double const&, double const&, bool, bool);
