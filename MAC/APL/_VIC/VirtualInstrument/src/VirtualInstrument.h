//#  VirtualInstrument.h: handles all events for a task.
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

#ifndef VirtualInstrument_H
#define VirtualInstrument_H

//# Includes

//# GCF Includes

//# local includes
#include <APLCommon/LogicalDevice.h>

//# Common Includes

//# ACC Includes

// forward declaration

namespace LOFAR
{
  
namespace VIC
{

  class VirtualInstrument : public APLCommon::LogicalDevice
  {
    public:

      explicit VirtualInstrument(const string& taskName, const string& parameterFile);
      virtual ~VirtualInstrument();

    protected:
      // protected default constructor
      VirtualInstrument();
      // protected copy constructor
      VirtualInstrument(const VirtualInstrument&);
      // protected assignment operator
      VirtualInstrument& operator=(const VirtualInstrument&);

      /**
      * Initial state additional behaviour must be implemented in the derived classes. 
      */
      virtual ::GCFEvent::TResult concrete_initial_state(::GCFEvent& e, ::GCFPortInterface& p, TLogicalDeviceState newState=LOGICALDEVICE_STATE_NOSTATE);
      /**
      * Claiming state additional behaviour must be implemented in the derived classes. 
      */
      virtual ::GCFEvent::TResult concrete_claiming_state(::GCFEvent& e, ::GCFPortInterface& p, TLogicalDeviceState newState=LOGICALDEVICE_STATE_NOSTATE);
      /**
      * Preparing state additional behaviour must be implemented in the derived classes. 
      */
      virtual ::GCFEvent::TResult concrete_preparing_state(::GCFEvent& e, ::GCFPortInterface& p, TLogicalDeviceState newState=LOGICALDEVICE_STATE_NOSTATE);
      /**
      * active state additional behaviour must be implemented in the derived classes. 
      */
      virtual ::GCFEvent::TResult concrete_active_state(::GCFEvent& e, ::GCFPortInterface& p, TLogicalDeviceState newState=LOGICALDEVICE_STATE_NOSTATE);
      /**
      * Releasing state additional behaviour must be implemented in the derived classes. 
      */
      virtual ::GCFEvent::TResult concrete_releasing_state(::GCFEvent& e, ::GCFPortInterface& p, TLogicalDeviceState newState=LOGICALDEVICE_STATE_NOSTATE);

      /**
      * Implementation of the Claim method is done in the derived classes. 
      */
      virtual void concreteClaim(::GCFPortInterface& port);
      /**
      * Implementation of the Prepare method is done in the derived classes. 
      */
      virtual void concretePrepare(::GCFPortInterface& port);
      /**
      * Implementation of the Resume method is done in the derived classes. 
      */
      virtual void concreteResume(::GCFPortInterface& port);
      /**
      * Implementation of the Suspend method is done in the derived classes. 
      */
      virtual void concreteSuspend(::GCFPortInterface& port);
      /**
      * Implementation of the Release method is done in the derived classes. 
      */
      virtual void concreteRelease(::GCFPortInterface& port);
      /**
      * Implementation of the Disconnected handler is done in the derived classes. 
      */
      virtual void concreteParentDisconnected(::GCFPortInterface& port);
      /**
      * Implementation of the Disconnected handler is done in the derived classes. 
      */
      virtual void concreteChildDisconnected(::GCFPortInterface& port);

    protected:    

    private:
  };
};//APLCommon
};//LOFAR
#endif
