//#  MaintenanceVI.h: handles all events for a task.
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

#ifndef MaintenanceVI_H
#define MaintenanceVI_H

//# Includes

//# GCF Includes
#include <GCF/PAL/GCF_ExtPropertySet.h>

//# local includes
#include <APLCommon/LogicalDevice.h>

//# Common Includes

//# ACC Includes

// forward declaration

namespace LOFAR
{
  
namespace AVI
{

  class MaintenanceVI : public APLCommon::LogicalDevice
  {
    public:
      typedef enum
      {
        MAINTENANCESTATUS_OPERATIONAL = 0,
        MAINTENANCESTATUS_INMAINTENANCE = 1
      } TMaintenanceStatus;
      
      // Logical Device version
      static const string VI_VERSION;

      // property defines
      static const string VI_PROPNAME_RESOURCES;
      static const string TYPE_LCU_PIC_MAINTENANCE;
      static const string PROPNAME_STATUS;
      static const string PROPNAME_FROMTIME;
      static const string PROPNAME_TOTIME;

      explicit MaintenanceVI(const string& taskName, const string& parameterFile, GCF::TM::GCFTask* pStartDaemon);
      virtual ~MaintenanceVI();

    protected:
      // protected default constructor
      MaintenanceVI();
      // protected copy constructor
      MaintenanceVI(const MaintenanceVI&);
      // protected assignment operator
      MaintenanceVI& operator=(const MaintenanceVI&);

      virtual void concrete_handlePropertySetAnswer(GCF::TM::GCFEvent& answer);
      /**
      * Initial state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_initial_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);
      /**
      * Idle state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_idle_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);
      /**
      * Claiming state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_claiming_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);
      /**
      * Claimed state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_claimed_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);
      /**
      * Preparing state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_preparing_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);
      /**
      * active state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_active_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLDResult& errorCode);
      /**
      * Releasing state additional behaviour must be implemented in the derived classes. 
      */
      virtual GCF::TM::GCFEvent::TResult concrete_releasing_state(GCF::TM::GCFEvent& e, GCF::TM::GCFPortInterface& p, TLogicalDeviceState& newState, TLDResult& errorCode);

      /**
      * Implementation of the Claim method is done in the derived classes. 
      */
      virtual void concreteClaim(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Prepare method is done in the derived classes. 
      */
      virtual void concretePrepare(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Resume method is done in the derived classes. 
      */
      virtual void concreteResume(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Suspend method is done in the derived classes. 
      */
      virtual void concreteSuspend(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Release method is done in the derived classes. 
      */
      virtual void concreteRelease(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Disconnected handler is done in the derived classes. 
      */
      virtual void concreteParentDisconnected(GCF::TM::GCFPortInterface& port);
      /**
      * Implementation of the Disconnected handler is done in the derived classes. 
      */
      virtual void concreteChildDisconnected(GCF::TM::GCFPortInterface& port);
      virtual void concreteHandleTimers(GCF::TM::GCFTimerEvent& timerEvent, GCF::TM::GCFPortInterface& port);
      virtual void concreteAddExtraKeys(ACC::APS::ParameterSet& /*psSubset*/);

    protected:    

    private:
      APL_DECLARE_SHARED_POINTER(GCF::PAL::GCFExtPropertySet);
      std::vector<std::string>                m_resources;
      std::vector<GCFExtPropertySetSharedPtr> m_maintenancePropertySets;
      
      ALLOC_TRACER_CONTEXT  
  };
};//APLCommon
};//LOFAR
#endif
