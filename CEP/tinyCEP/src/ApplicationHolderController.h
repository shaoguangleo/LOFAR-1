//#  ApplicationHolderController.h: interpretes commands from ACC and executes them on the TinyApplicationHolder object
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

//# Always #include <lofar_config.h> first!
#include <lofar_config.h>

#ifndef TINYAPPLICATIONHOLDERCONTROLLER_H
#define TINYAPPLICATIONHOLDERCONTROLLER_H

#include <Common/LofarLogger.h>

#include <tinyCEP/Profiler.h>
#include <ACC/ProcControlServer.h>
#include <ACC/ParameterSet.h>
#include <ACC/ProcessControl.h>

#include <tinyCEP/TinyApplicationHolder.h>

using namespace LOFAR;
using namespace LOFAR::ACC;

class ApplicationHolderController : public ProcessControl
{
public:
  // Default constructable
  ApplicationHolderController(TinyApplicationHolder& stationCorrelator, int noRuns = 100);

  // Destructor
  virtual ~ApplicationHolderController();

  // Command to control the application processes.
  virtual bool	define   () ;
  virtual bool	init     () ;
  virtual bool	run      () ;
  virtual bool	pause    (const	string&	condition) ;
  virtual bool	quit  	 ();
  virtual bool	snapshot (const string&	destination) ;
  virtual bool	recover  (const string&	source) ;
  virtual bool	reinit	 (const string&	configID) ;
  virtual string askInfo   (const string& 	keylist) ;
  
  virtual int main(int argc, const char* argv[]);

protected:
  TinyApplicationHolder& itsAH;
  int itsNoRuns;
  int itsNoTotalRuns;
  bool itsIsRunning;
  bool itsShouldQuit;
  ProcControlServer* itsPCcomm;

  // Copying is not allowed
  ApplicationHolderController (const ApplicationHolderController& that);
  ApplicationHolderController& 	operator=(const ApplicationHolderController& that);
};

#endif
