//#  StationCorrelator.cc: Main program station correlator
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

#include <Common/KeyValueMap.h>
#include <Common/LofarLogger.h>
#include <Common/lofar_iostream.h>

#include <StationCorrelator.h>

using namespace LOFAR;

StationCorrelator::~StationCorrelator() {

}

void StationCorrelator::define(const KeyValueMap& params) {

}

void StationCorrelator::run(int steps) {

}

void StationCorrelator::dump() const {

}

void StationCorrelator::quit() {

}


int main (int argc, const char** argv) {

  INIT_LOGGER("StationCorrelator");
  
  try {
    
    StationCorrelator correlator;
    correlator.setarg(argc, argv);

    correlator.baseDefine();
    correlator.baseRun();
    correlator.baseDump();
    correlator.baseQuit();

  } catch (...) {
    cout << "Unexpected exception" << endl;
  }
  return 0;
}
