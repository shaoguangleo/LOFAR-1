//# WorkerFactory.cc: Factory pattern to generate a workerproxy object
//#
//# Copyright (c) 2007
//# ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
//#
//# This file is part of the LOFAR software suite.
//# The LOFAR software suite is free software: you can redistribute it and/or
//# modify it under the terms of the GNU General Public License as published
//# by the Free Software Foundation, either version 3 of the License, or
//# (at your option) any later version.
//#
//# The LOFAR software suite is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
//#
//# $Id$

#include <lofar_config.h>

#include <LMWCommon/WorkerFactory.h>
#include <LMWCommon/MWError.h>
#include <Common/LofarLogger.h>

namespace LOFAR { namespace CEP {

  void WorkerFactory::push_back (const std::string& name, Creator* creator)
  {
    itsMap[name] = creator;
  }
    
  WorkerProxy::ShPtr WorkerFactory::create (const std::string& name) const
  {
    std::map<std::string,Creator*>::const_iterator iter = itsMap.find(name);
    ASSERTSTR (iter != itsMap.end(),
	       "WorkerProxy " << name << " is unknown");
    return (*iter->second)();
  }

}} // end namespaces
