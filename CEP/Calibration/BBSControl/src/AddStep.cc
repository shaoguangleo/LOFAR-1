//#  AddStep.cc: 
//#
//#  Copyright (C) 2002-2007
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
//#  $Id: AddStep.cc 12371 2008-12-23 13:18:31Z loose $

#include <lofar_config.h>
#include <BBSControl/AddStep.h>
#include <BBSControl/CommandVisitor.h>
#include <Common/ParameterSet.h>

namespace LOFAR
{
  namespace BBS
  {

    AddStep::AddStep(const string& name, 
                               const ParameterSet& parSet,
                               const Step* parent) :
      SingleStep(name, parent)
    {
      read(parSet.makeSubset("Step." + name + "."));
    }


    void AddStep::accept(CommandVisitor &visitor) const
    {
      visitor.visit(*this);
    }


    const string& AddStep::type() const 
    {
      static const string theType("Add");
      return theType;
    }


    const string& AddStep::operation() const
    {
      static string theOperation("Add");
      return theOperation;
    }

  } // namespace BBS

} // namespace LOFAR
    
    
