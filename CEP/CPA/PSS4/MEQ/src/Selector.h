//# Selector.h: Selects result planes from a result set
//#
//# Copyright (C) 2003
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

#ifndef MEQ_SELECTOR_H
#define MEQ_SELECTOR_H
    
#include <MEQ/Node.h>

#pragma aidgroup Meq
#pragma types #Meq::Selector

// The comments below are used to automatically generate a default
// init-record for the class 

//defrec begin MeqSelector
//  A MeqSelector selects one or more results from the result set of its child.
//  Must have exactly one child.
//field: index []
//  Indices (1-based) of results to select. At least one must be specified.
//defrec end

namespace Meq {    


class Selector : public Node
{
  public:
    Selector ();
    virtual ~Selector ();
    
    virtual void init (DataRecord::Ref::Xfer &initrec, Forest* frst);
    
    virtual void setState (const DataRecord &rec);
    
    virtual TypeId objectType() const
    { return TpMeqSelector; }
    
  protected:
    int getResult (Result::Ref &resref, const Request& request, bool newReq);
  
  private:
    vector<int> selection;
};


} // namespace Meq

#endif
