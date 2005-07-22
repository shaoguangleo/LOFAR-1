//#  AH_FakeStation.h: 
//#
//#  Copyright (C) 2002-2004
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  $Id$
//#
////////////////////////////////////////////////////////////////////

#ifndef AH_INPUTSECTION_H
#define AH_INPUTSECTION_H

#include <CEPFrame/ApplicationHolder.h>
#include <tinyCEP/WorkHolder.h>
#include <CEPFrame/Step.h>

namespace LOFAR {

// This is the ApplicationHolder for the generator that emulates the stations
// 

class AH_FakeStation: public LOFAR::ApplicationHolder
{
 public:
  AH_FakeStation();
  virtual ~AH_FakeStation();
  virtual void undefine();
  virtual void define  (const LOFAR::KeyValueMap&);
  virtual void prerun  ();
  virtual void run     (int nsteps);
  virtual void dump    () const;
  virtual void quit    ();
 private:

  vector<WorkHolder*> itsWHs;
  vector<Step*> itsSteps;

};
}
#endif
