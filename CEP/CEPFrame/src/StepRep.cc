//  StepRep.cc:
//
//  Copyright (C) 2000, 2001
//  ASTRON (Netherlands Foundation for Research in Astronomy)
//  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  $Id$
//
//
//////////////////////////////////////////////////////////////////////

#include <Common/lofar_iostream.h>
#include <Common/lofar_strstream.h>
#include <Common/lofar_algorithm.h>    // for min,max

#include "CEPFrame/StepRep.h"
#include "CEPFrame/SimulRep.h"
#include "CEPFrame/Step.h"
#include TRANSPORTERINCLUDE
#include "CEPFrame/TH_Mem.h"
#include "CEPFrame/Profiler.h"
#include "Common/Debug.h"
#ifdef HAVE_CORBA
#include "CEPFrame/Corba/BS_Corba.h"
#include "CEPFrame/Corba/CorbaMonitor.h"
#endif 

// this will give all instances of Step the same event in the Profiling output
int          StepRep::theirProcessProfilerState=0;
unsigned int StepRep::theirNextID=0;
unsigned int StepRep::theirEventCnt=0;
int          StepRep::theirCurAppl=0;


StepRep::StepRep (const WorkHolder& worker, 
		  const string& name,
		  bool addNameSuffix,
		  bool monitor)
: itsRefCount (1),
  itsWorker   (0),
  itsParent   (0),
  itsID       (-1),
  itsNode     (0),
  itsAppl     (0),
  itsRate     (1),
  itsAddSuffix(addNameSuffix),
  itsSeqNr    (-1),
  itsName     (name),
  itsMonitor  (0)
{
  itsWorker = worker.baseMake();
  itsCurRank = TRANSPORTER::getCurrentRank();
  setID();

  if (monitor) {
    TRACER2("Create controllable Simul " << name);
#ifdef HAVE_CORBA
    // Create a CorbaMonitor object    
    itsMonitor = new CorbaMonitor(BS_Corba::getPOA(),
				  BS_Corba::getPOAManager(),
				  name,
				  getWorker());
#else
    TRACER1("CORBA is not configured, so CorbaMonitor cannot be used in Simul ");
#endif 
  }

  if (theirProcessProfilerState == 0) {
    theirProcessProfilerState = Profiler::defineState("WH_Process","yellow");
  }
  TRACER2("Create Step " << name << " ID = " << getID());

}

StepRep::~StepRep() 
{
  delete itsWorker;
#ifdef HAVE_CORBA
  delete itsMonitor;
#endif
}


bool StepRep::isSimul() const
{
  return false;
}

void StepRep::setSeqNr (int seqNr)
{
  itsSeqNr = seqNr;
  if (itsAddSuffix) {
    ostringstream os;
    os << '_' << itsSeqNr << ends;
    itsName += os.str();
  }
}


void StepRep::preprocess()
{
  if (shouldProcess()) {
    TRACER3("Preprocess Step " << getName() << " on node/appl (" 
	   << getNode() << '/' << getAppl() << ')');
    itsWorker->basePreprocess();
  }
}

void StepRep::process()
{
  if (! doHandle()) {
    TRACER4( "Skip Process " << theirEventCnt << " " << itsRate);
    return;
  }
  // extra check if process is on the right node
  if (getNode() < 0) {
    cout << "StepRep::Process Node < 0 for Step " << getName() << endl;
    return;
  }
  if (! shouldProcess()) {
    TRACER4("Step " << getName() << " Not on right node/appl(" 
	   << getNode() << '/' << getAppl() << "); will skip Process");
    return;
  }  
  // Call the baseProcess() method in the WorkHolder
  Profiler::enterState (theirProcessProfilerState);
  itsWorker->baseProcess();
  Profiler::leaveState (theirProcessProfilerState);
}

void StepRep::postprocess()
{
  if (shouldProcess()) {
    TRACER3("Postprocess Step " << getName() << " on node/appl (" 
	   << getNode() << '/' << getAppl() << ')');
    itsWorker->basePostprocess();
  }
}

void StepRep::runOnNode (int aNode, int applNr)
{ 
  itsNode = aNode;
  itsAppl = applNr;
}


void StepRep::setID()
{
  itsID = theirNextID++;
  // Step pointer will be stored in the Transport objects of the WorkHolder.
  for (int ch=0; ch < itsWorker->getInputs(); ch++) {
    getInTransport(ch).setStep (*this);
    // Set id of intransport
    getInTransport(ch).setItsID (theirNextID++);
  }
  for (int ch=0; ch < itsWorker->getOutputs(); ch++) { 
    getOutTransport(ch).setStep (*this);
    // Set id of outtransport
    getOutTransport(ch).setItsID (theirNextID++);
  }
}

		       
bool StepRep::connectData (const TransportHolder& prototype,
			   DataHolder& sourceData, 
			   DataHolder& targetData)
{
  Transport& sourceTP = sourceData.getTransport();
  Transport& targetTP = targetData.getTransport();
  AssertStr (sourceTP.getRate() == targetTP.getRate(),
	     "StepRep::connectData; inRate " <<
	     sourceTP.getRate() << " and outRate " <<
	     targetTP.getRate() << " not equal!");
  AssertStr (sourceData.getType() == targetData.getType(),
	     "StepRep::connectData; inType " <<
	     sourceData.getType() << " and outType " <<
	     targetData.getType() << " not equal!");
  sourceTP.makeTransportHolder (prototype);
  targetTP.makeTransportHolder (prototype);
  DbgAssert (sourceTP.getItsID() >= 0);
  // Use the source ID as the tag for MPI send/receive.
  sourceTP.setWriteTag (sourceTP.getItsID());
  targetTP.setReadTag (sourceTP.getItsID());
  // Set the source DataHolder of a target.
  targetTP.setSourceAddr (&sourceData);
  // Set the target DataHolder of a source.
  sourceTP.setTargetAddr (&targetData);
  return true;
}

bool StepRep::connectRep (StepRep* aStep,
			  int   thisDHIndex,
			  int   thatDHIndex,
			  int   nrDH,
			  const TransportHolder& prototype)
{
  // determine how much DataHolders to loop
  if (nrDH < 0) {
    nrDH = min(aStep->getWorker()->getOutputs(),
	       this->getWorker()->getInputs());
  }
  bool result=true;
  for (int i=0; i<nrDH; i++) {
    int thisInx = i + thisDHIndex;  // DataHolder nr in this Step
    int thatInx = i + thatDHIndex;  // DataHolder nr in aStep
    
    result &= connectData (prototype,
			   aStep->getOutData(thatInx),
			   this->getInData(thisInx));
    TRACER2( "StepRep::connect " << getName().c_str() << " (ID = "
	   << getID() << ") DataHolder " << thisInx << " to "
	   << aStep->getName().c_str() 
	   << " (ID = " << aStep->getID() << ") DataHolder " << thatInx);
  }
  return result;
}


bool StepRep::connectInput (Step* aStep,
			    const TransportHolder& prototype)
{
  return connectRep (aStep->getRep(), 0, 0, -1, prototype);
}

bool StepRep::connectInputArray (Step* aStep[],
				 int   nrSteps,
				 const TransportHolder& prototype)
{
  if (aStep==NULL) return false;
  if (nrSteps < 0) {  // set nrSteps automatically
    nrSteps = getWorker()->getInputs();
  }
  int dhIndex=0;
  for (int item=0; item<nrSteps; item++) {
    AssertStr (getWorker()->getInputs() >= 
	       dhIndex+aStep[item]->getWorker()->getOutputs(),
	       "connect " << getName() << " - " << aStep[item]->getName() <<
	       "; not enough inputs");
    connectRep (aStep[item]->getRep(), dhIndex, 0, -1, prototype);
    dhIndex += aStep[item]->getWorker()->getOutputs();
  }
  if (dhIndex != getWorker()->getInputs()) {
    cerr << "StepRep::connectInputArray() - Warning:" << endl
	 << "  " << getName() << " - " << aStep[0]->getName() 
	 << ", unequal number of inputs and outputs" << endl;
  }
  return true;
}


bool StepRep::connectOutputArray (Step* aStep[],
				  int   nrSteps,
				  const TransportHolder& prototype)
{
  if (aStep == NULL) {
    return false;
  }
  if (nrSteps < 0) {  // set nrSteps automatically
    nrSteps = getWorker()->getOutputs();
  }
  int dhIndex=0;
  for (int item=0; item<nrSteps; item++) {

    AssertStr (getWorker()->getOutputs() >= 
	       dhIndex+aStep[item]->getWorker()->getInputs(),
	       "connect " << getName() << " - " << aStep[item]->getName() <<
	       "; not enough inputs");
    aStep[item]->getRep()->connectRep (this, 0, dhIndex, -1, prototype);
    dhIndex += aStep[item]->getWorker()->getInputs();

  }
  if (dhIndex != getWorker()->getOutputs()) {
    cerr << "StepRep::connectOutputArray() - Warning:" << endl
	 << "  " << getName() << " - " << aStep[0]->getName()
	 << ", unequal number of inputs and outputs" << endl;
  }
  return true;
}


bool StepRep::checkConnections (ostream& os, const StepRep* parent)
{
  bool result = true;
  // The parent must match.
  DbgAssert (getParent() == parent);
  // Check if the input DataHolders are correctly connected.
  for (int ch=0; ch<getWorker()->getInputs(); ch++) {
    // Make sure the Transport belongs to this Step.
    Transport& tp = getInTransport(ch);
    DbgAssert (&(tp.getStep()) == this);
    // An input DataHolder in a Step cannot have a target DataHolder
    // (but in a Simul it must have one).
    if (isSimul()  &&  tp.getTargetAddr() == 0) {
      os << "Input DataHolder " << getInData(ch).getName() << " of Simul "
	 << getName() << " not connected to its first Step(s)" << endl;
      result = false;
    }
    if (!isSimul()  &&  tp.getTargetAddr() != 0) {
      os << "Input DataHolder " << getInData(ch).getName() << " of Step "
	 << getName() << " also connected as output to Step "
	 << tp.getTargetAddr()->getTransport().getStep().getName() << endl;
      result = false;
    }
    // The input DataHolder must have a source DataHolder.
    DataHolder* src = tp.getSourceAddr();
    if (src == 0) {
      os << "Input DataHolder " << getInData(ch).getName() << " of Step "
	 << getName() << " not connected to output of previous Step" << endl;
      result = false;
    } else {
      // That source DataHolder must have this DataHolder as the target.
      // Otherwise a new connect might be done to another DataHolder.
      DataHolder* dst = src->getTransport().getTargetAddr();
      DbgAssert (dst == &(getInData(ch)));
      // The source must belong to the given parent or must be the parent
      // itself (in case Simul input is connected to Step input).
      StepRep& srcstep = src->getTransport().getStep();
      bool first = (&srcstep == parent);
      if (!first) {
	if (srcstep.getParent() != parent) {
	  os << "Input DataHolder " << dst->getName() << " of Step "
	     << getName()
	     << " connected to output of a Step in another Simul" << endl;
	  result = false;
	} else {
	  if (srcstep.getSeqNr() >= dst->getTransport().getStep().getSeqNr()) {
	    os << "Input DataHolder " << dst->getName() << " of Step "
	       << getName() << " connected to output of a later Step" << endl;
	    result = false;
	  }
	}
      }
    }
  }
  for (int ch=0; ch<getWorker()->getOutputs(); ch++) {
    Transport& tp = getOutTransport(ch);
    DbgAssert (&(tp.getStep()) == this);
    // An output DataHolder in a Step cannot have a source DataHolder
    // (but in a Simul it must have one).
    if (isSimul()  &&  tp.getSourceAddr() == 0) {
      os << "Output DataHolder " << getOutData(ch).getName() << " of Simul "
	 << getName() << " not connected to its last Step(s)" << endl;
      result = false;
    }
    // The output DataHolder must have a target DataHolder.
    // Only the outer Simul does not need to have a destination.
    DataHolder* dst = tp.getTargetAddr();
    if (dst == 0) {
      os << "Output DataHolder " << getOutData(ch).getName() << " of Step "
	 << getName() << " not connected to input of next Step" << endl;
      result = false;
    } else {
      // That target DataHolder must have this DataHolder as the source.
      // Otherwise a new connect might be done to another DataHolder.
      DataHolder* src = dst->getTransport().getSourceAddr();
      DbgAssert (src == &(getOutData(ch)));
      // The target must belong to the given parent or must be the parent
      // itself (in case Step output is connected to Simul output).
      StepRep& dststep = dst->getTransport().getStep();
      bool last = (&dststep == parent);
      if (!last) {
	if (dststep.getParent() != parent) {
	  os << "Output DataHolder " << src->getName() << " of Step "
	     << getName()
	     << " connected to input of a Step in another Simul" << endl;
	  result = false;
	}
      }
    }
  }
  return result;
}


void StepRep::shortcutConnections()
{}


void StepRep::simplifyConnections()
{
  // Test for each input if connected to an output on the same node.
  for (int ch=0; ch<getWorker()->getInputs(); ch++) {
    Transport& dsttp = getInTransport(ch);
    DataHolder* src = dsttp.getSourceAddr();
    if (src) {
      Transport& srctp = src->getTransport();
      if (srctp.getNode() == dsttp.getNode()) {
	srctp.makeTransportHolder (TH_Mem());
	dsttp.makeTransportHolder (TH_Mem());
      }
    }
  }
}

void StepRep::optimizeConnectionsWith(const TransportHolder& newTH)
{
  cdebug(3) << "optimizeConnectionsWith  " << newTH.getType() << endl;
  for (int ch=0; ch<getWorker()->getInputs(); ch++)
  {
    Transport& dsttp = getInTransport(ch);
    DataHolder* src = dsttp.getSourceAddr();
    if (src)
    {
      Transport& srctp = src->getTransport();

      // attemptReplace returns pointer to allocated replacement
      // TransportHolder or 0 if replacement is not possible.
      if (newTH.connectionPossible(srctp.getNode(), dsttp.getNode()))
      {
	cdebug(3) << "replace source " << srctp.getTransportHolder()->getType()
	     << " with " << newTH.getType() << endl;
	cdebug(3) << "replace target " << dsttp.getTransportHolder()->getType()
	     << " with " << newTH.getType() << endl;
	srctp.makeTransportHolder(newTH);
	dsttp.makeTransportHolder(newTH);
      }
    }
  }
}

void StepRep::dump() const
{
  // cout << "StepRep::dump " << itsName << endl;
  if (shouldProcess()) itsWorker->dump();
}

bool StepRep::setRate (int rate, int dhIndex)
{
  bool result = true;
  result &= setInRate (rate, dhIndex);
  result &= setOutRate (rate, dhIndex);
  itsRate = rate;
  return result;
}

bool StepRep::setInRate(int rate, int dhIndex)
{
  AssertStr (rate>0,
	     "Step Rate not within limits");
  AssertStr (dhIndex >= -1,
	     "Step dhIndex not within limits");
  if (dhIndex >= 0) {
    getInTransport(dhIndex).setRate (rate); 
    return true;
  } else {
    if (dhIndex == -1) {
      for (int ch=0; ch<getWorker()->getInputs(); ch++) {
	getInTransport(ch).setRate (rate); 
      }
      return true;
    }
  }
  return false;
}

bool StepRep::setOutRate (int rate, int dhIndex)
{
  AssertStr (rate>0,
	     "Step Rate not within limits");
  AssertStr (dhIndex >= -1,
	     "Step dhIndex not within limits");
  if (dhIndex >= 0) {
    getOutTransport(dhIndex).setRate (rate); 
    return true;
  } else {
    if (dhIndex == -1) {
      for (int ch=0; ch<getWorker()->getOutputs(); ch++) {
	getOutTransport(ch).setRate (rate); 
      }
      return true;
    }
  }
  return false;
}
