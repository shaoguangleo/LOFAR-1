//#  GSA_WaitForAnswer.h: 
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

#ifndef  GSA_WAITFORANSWER_H
#define  GSA_WAITFORANSWER_H

#include <HotLinkWaitForAnswer.hxx>   
#include <Common/lofar_string.h>

class GSAService;
class DpMsgAnswer;
class DpHLGroup;

// ---------------------------------------------------------------
// Our callback class

class GSAWaitForAnswer : public HotLinkWaitForAnswer
{
  public:
    GSAWaitForAnswer(GSAService& service);
    virtual ~GSAWaitForAnswer() {};
    
    void hotLinkCallBack(DpMsgAnswer& answer);
    inline const string& getPropName() const {return _propName;}
    inline void setPropName(const string& propName) {_propName = propName;}

  protected:
    // Answer on conenct
    void hotLinkCallBack(DpHLGroup& group);

  private:
    GSAService& _service;
    string  _propName;
};                                 

#endif
