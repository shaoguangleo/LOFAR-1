//# PredictStep.cc: 
//#
//# Copyright (C) 2002-2007
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
#include <BBSControl/PredictStep.h>
#include <BBSControl/CommandVisitor.h>
#include <Common/ParameterSet.h>

namespace LOFAR
{
  namespace BBS
  {

    PredictStep::PredictStep(const string& name, 
                             const ParameterSet& parSet,
                             const Step* parent) :
      SingleStep(name, parent)
    {
      read(parSet.makeSubset("Step." + name + "."));
    }


    CommandResult PredictStep::accept(CommandVisitor &visitor) const
    {
        return visitor.visit(*this);
    }


    const string& PredictStep::type() const 
    {
      static const string theType("Predict");
      return theType;
    }

    const string& PredictStep::operation() const
    {
      static string theOperation("Predict");
      return theOperation;
    }

  } // namespace BBS

} // namespace LOFAR
    
    