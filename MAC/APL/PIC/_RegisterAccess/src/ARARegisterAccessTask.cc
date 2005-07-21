//#
//#  ARARegisterAccessTask.cc: implementation of ARARegisterAccessTask class
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
#undef PACKAGE
#undef VERSION
#include <lofar_config.h>
#include <Common/LofarLogger.h>

// this include needs to be first!
#include "RSP_Protocol.ph"

#include "ARARegisterAccessTask.h"
#include "ARAConstants.h"

#include <Common/lofar_iostream.h>
#include <Common/lofar_strstream.h>
#include <time.h>
#include <Common/lofar_string.h>
#include <Common/lofar_vector.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <GCF/ParameterSet.h>
#include <GCF/GCF_Defines.h>
#include <GCF/GCF_PValue.h>
#include <GCF/GCF_PVInteger.h>
#include <GCF/GCF_PVUnsigned.h>
#include <GCF/GCF_PVBool.h>
#include <GCF/GCF_PVDouble.h>
#include <GCF/GCF_PVString.h>
#include <GCF/GCF_PVDynArr.h>
#include "ARAPropertyDefines.h"
#include "ARAPhysicalModel.h"

using namespace LOFAR::GCF::Common;
using namespace LOFAR::GCF::TM;
using namespace LOFAR::GCF::PAL;

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;


namespace LOFAR
{

namespace ARA
{

string RegisterAccessTask::m_RSPserverName("ARAtestRSPserver");

RegisterAccessTask::RegisterAccessTask(string name)
    : GCFTask((State)&RegisterAccessTask::initial, name),
      m_answer(),
      m_myPropertySetMap(),
      m_myPropsLoaded(false),
      m_myPropsLoadCounter(0),
      m_RSPclient(*this, m_RSPserverName, GCFPortInterface::SAP, RSP_PROTOCOL),
      m_physicalModel(),
      m_propertySet2RCUMap(),
      m_subStatusHandle(0),
      m_subStatsHandleSubbandPower(0),
      m_subStatsHandleBeamletPower(0),
      m_n_racks(1),
      m_n_subracks_per_rack(1),
      m_n_boards_per_subrack(1),
      m_n_aps_per_board(1),
      m_n_rcus_per_ap(2),
      m_n_rcus(2),
      m_status_update_interval(1),
      m_stats_update_interval(1),
      m_centralized_stats(false),
      m_integrationTime(0),
      m_integrationMethod(0),
      m_integratingStatisticsSubband(),
      m_numStatisticsSubband(0),
      m_integratingStatisticsBeamlet(),
      m_numStatisticsBeamlet(0),
      m_integrationTimerID(0)
{
  registerProtocol(RSP_PROTOCOL, RSP_PROTOCOL_signalnames);
  m_answer.setTask(this);
  
  ParameterSet::instance()->adoptFile("RegisterAccess.conf");

  char scopeString[300];
  int rack;
  int subrack;
  int board;
  int ap;
  int rcu;
  int globalRcuNr(0);
  
  m_n_racks               = ParameterSet::instance()->getInt(PARAM_N_RACKS);
  m_n_subracks_per_rack   = ParameterSet::instance()->getInt(PARAM_N_SUBRACKS_PER_RACK);
  m_n_boards_per_subrack  = ParameterSet::instance()->getInt(PARAM_N_BOARDS_PER_SUBRACK);
  m_n_aps_per_board       = ParameterSet::instance()->getInt(PARAM_N_APS_PER_BOARD);
  m_n_rcus_per_ap         = ParameterSet::instance()->getInt(PARAM_N_RCUS_PER_AP);
  m_n_rcus                = m_n_rcus_per_ap*
                              m_n_aps_per_board*
                              m_n_boards_per_subrack*
                              m_n_subracks_per_rack*
                              m_n_racks;
  m_status_update_interval = ParameterSet::instance()->getInt(PARAM_STATUS_UPDATE_INTERVAL);
  m_stats_update_interval  = ParameterSet::instance()->getInt(PARAM_STATISTICS_UPDATE_INTERVAL);
  m_centralized_stats      = (0!=ParameterSet::instance()->getInt(PARAM_STATISTICS_CENTRALIZED));
  
  // fill MyPropertySets map
  addMyPropertySet(SCOPE_PIC, TYPE_LCU_PIC, PSCAT_LCU_PIC, PROPS_Station, GCFMyPropertySet::USE_DB_DEFAULTS);
  addMyPropertySet(SCOPE_PIC_Maintenance, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
  for(rack=0;rack<m_n_racks;rack++)
  {
    sprintf(scopeString,SCOPE_PIC_RackN,rack);
    addMyPropertySet(scopeString,TYPE_LCU_PIC_Rack, PSCAT_LCU_PIC_Rack, PROPS_Rack);
    sprintf(scopeString,SCOPE_PIC_RackN_Maintenance,rack);
    addMyPropertySet(scopeString,TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
    sprintf(scopeString,SCOPE_PIC_RackN_Alert,rack);
    addMyPropertySet(scopeString,TYPE_LCU_PIC_Alert, PSCAT_LCU_PIC_Alert, PROPS_Alert);

    for(subrack=0;subrack<m_n_subracks_per_rack;subrack++)
    {
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN,rack,subrack);
      addMyPropertySet(scopeString, TYPE_LCU_PIC_SubRack, PSCAT_LCU_PIC_SubRack, PROPS_SubRack);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_Maintenance,rack,subrack);
      addMyPropertySet(scopeString, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_Alert,rack,subrack);
      addMyPropertySet(scopeString, TYPE_LCU_PIC_Alert, PSCAT_LCU_PIC_Alert, PROPS_Alert);
      
      for(board=0;board<m_n_boards_per_subrack;board++)
      {
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_Board, PSCAT_LCU_PIC_Board, PROPS_Board);
        
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_MEPStatus,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_MEPStatus, PSCAT_LCU_PIC_MEPStatus, PROPS_MEPStatus);
        
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_Maintenance,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
        
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_Alert,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_Alert, PSCAT_LCU_PIC_Alert, PROPS_Alert);
        
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_ETH,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_Ethernet, PSCAT_LCU_PIC_Ethernet, PROPS_Ethernet);
        
        sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_BP,rack,subrack,board);
        addMyPropertySet(scopeString, TYPE_LCU_PIC_FPGA, PSCAT_LCU_PIC_FPGA, PROPS_FPGA);
    
        for(ap=0;ap<m_n_aps_per_board;ap++)
        {
          sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rack,subrack,board,ap);
          addMyPropertySet(scopeString, TYPE_LCU_PIC_FPGA, PSCAT_LCU_PIC_FPGA, PROPS_FPGA);
          sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_SYNCStatus,rack,subrack,board,ap);
          addMyPropertySet(scopeString, TYPE_LCU_PIC_SYNCStatus, PSCAT_LCU_PIC_SYNCStatus, PROPS_SYNCStatus);
          sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_BoardRCUStatus,rack,subrack,board,ap);
          addMyPropertySet(scopeString, TYPE_LCU_PIC_BoardRCUStatus, PSCAT_LCU_PIC_BoardRCUStatus, PROPS_BoardRCUStatus);
          for(rcu=0;rcu<m_n_rcus_per_ap;rcu++)
          {
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_RCU, PSCAT_LCU_PIC_RCU, PROPS_RCU);

            m_propertySet2RCUMap[string(scopeString)] = globalRcuNr++;

            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_ADCStatistics,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_ADCStatistics, PSCAT_LCU_PIC_ADCStatistics, PROPS_ADCStatistics);
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_Maintenance,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_LFA,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_LFA, PSCAT_LCU_PIC_LFA, PROPS_LFA);
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_HFA,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_HFA, PSCAT_LCU_PIC_HFA, PROPS_HFA);
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_LFA_Maintenance,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN_HFA_Maintenance,rack,subrack,board,ap,rcu);
            addMyPropertySet(scopeString, TYPE_LCU_PIC_Maintenance, PSCAT_LCU_PIC_Maintenance, PROPS_Maintenance);
          }
        }
      }
    }  
  }
  
  // subscribe to maintenanace properties and alert properties
  
}

RegisterAccessTask::~RegisterAccessTask()
{
}

void RegisterAccessTask::addMyPropertySet(const char* scope,
    const char* type, 
    TPSCategory category, 
    const TPropertyConfig propconfig[],
    GCFMyPropertySet::TDefaultUse defaultUse)
{
  boost::shared_ptr<GCFMyPropertySet> propsPtr(new GCFMyPropertySet(scope,type,category,&m_answer, defaultUse));
  m_myPropertySetMap[scope]=propsPtr;
  
  propsPtr->initProperties(propconfig);
}

bool RegisterAccessTask::isConnected()
{
  return m_RSPclient.isConnected();
}

GCFEvent::TResult RegisterAccessTask::initial(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch(e.signal)
  {
    case F_INIT:
    {
      break;
    }

    case F_ENTRY:
    {
      LOG_INFO("Enabling MyPropertySets...");
      m_myPropsLoadCounter=0;
      TMyPropertySetMap::iterator it;
      for(it=m_myPropertySetMap.begin();it!=m_myPropertySetMap.end();++it)
      {
        it->second->enable();
      }
      break;
    }

    case F_MYPS_ENABLED:
    {
      GCFPropSetAnswerEvent* pPropAnswer = static_cast<GCFPropSetAnswerEvent*>(&e);
      assert(pPropAnswer);
      if(pPropAnswer->result != 0)
      {
        LOG_WARN(formatString("MyPropset %s could not be enabled: %d",pPropAnswer->pScope,pPropAnswer->result));
      }
      m_myPropsLoadCounter++;
      LOG_INFO(formatString("MyPropset %d enabled", m_myPropsLoadCounter));
      if(m_myPropsLoadCounter == m_myPropertySetMap.size())
      {
        m_myPropsLoaded=true;
        TRAN(RegisterAccessTask::myPropSetsLoaded);
      }
      break;
    }
    
    case F_EXIT:
    {
      // cancel timers
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::myPropSetsLoaded(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch(e.signal)
  {
    case F_INIT:
    {
      break;
    }

    case F_ENTRY:
    {
      LOG_INFO("NOT configuring propsets using APCs...");
      TRAN(RegisterAccessTask::APCsLoaded);
      break;
    }

    case F_PS_CONFIGURED:
    {
/*
      m_APCsLoadCounter++;
      LOG_INFO(formatString("Propset %d configured", m_APCsLoadCounter));
      if(m_APCsLoadCounter == m_APCMap.size())
      {
        m_APCsLoaded=true;
        TRAN(RegisterAccessTask::APCsLoaded);
      }
*/
      break;
    }
    
    case F_EXIT:
    {
    	// cancel timers
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::APCsLoaded(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch(e.signal)
  {
    case F_INIT:
    {
      break;
    }

    case F_ENTRY:
    {
      if (!m_RSPclient.isConnected()) 
      {
        bool res=m_RSPclient.open(); // need this otherwise GTM_Sockethandler is not called
        LOG_DEBUG(formatString("m_RSPclient.open() returned %s",(res?"true":"false")));
        if(!res)
        {
          m_RSPclient.setTimer((long)3);
        }  
      } 
      break;
    }

    case F_CONNECTED:
    {
      LOG_DEBUG(formatString("port '%s' connected", port.getName().c_str()));
      if (isConnected())
      {
        TRAN(RegisterAccessTask::connected);
      }
      break;
    }

    case F_DISCONNECTED:
    {
      port.setTimer((long)3); // try again in 3 seconds
      LOG_WARN(formatString("port '%s' disconnected, retry in 3 seconds...", port.getName().c_str()));
      port.close();
      break;
    }

    case F_TIMER:
    {
      LOG_INFO(formatString("port '%s' retry of open...", port.getName().c_str()));
      port.open();
      break;
    }

    case F_VCHANGEMSG:
    {
      // check which property changed
      GCFPropValueEvent* pPropAnswer = static_cast<GCFPropValueEvent*>(&e);
      assert(pPropAnswer);

      if(strstr(pPropAnswer->pPropName,"Maintenance") != 0)
      {
        handleMaintenance(string(pPropAnswer->pPropName),*pPropAnswer->pValue);
      }
      break;
    }
    
    case F_EXIT:
    {
      // cancel timers
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::connected(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch (e.signal)
  {

    case F_INIT:
      break;
      
    case F_ENTRY:
    {
      // get versions
      RSPGetversionEvent getversion;
      getversion.timestamp.setNow();
      getversion.cache = true;
      m_RSPclient.send(getversion);
      
      break;
    }

    case F_TIMER:
    {
      break;
    }

    case RSP_GETVERSIONACK:
    {
      LOG_INFO("RSP_GETVERSIONACK received");
      RSPGetversionackEvent ack(e);

      if(ack.status != SUCCESS)
      {
        LOG_ERROR("RSP_GETVERSION failure");
      }
      else
      {
        char scopeString[300];
        char version[20];
        for (int board = 0; board < ack.versions.rsp().extent(blitz::firstDim); board++)
        {
          int rackNr;
          int subRackNr;
          int relativeBoardNr;
          getBoardRelativeNumbers(board,rackNr,subRackNr,relativeBoardNr);
          sprintf(version,"%d.%d",ack.versions.rsp()(board) >> 4,ack.versions.rsp()(board) & 0xF);
          LOG_INFO(formatString("board[%d].version = 0x%x",board,ack.versions.rsp()(board)));
          sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN,rackNr,subRackNr,relativeBoardNr);
          updateVersion(scopeString,string(version),double(ack.timestamp));
          
          sprintf(version,"%d.%d",ack.versions.bp()(board)  >> 4,ack.versions.bp()(board)  & 0xF);
          LOG_INFO(formatString("bp[%d].version = 0x%x",board,ack.versions.bp()(board)));
          sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_BP,rackNr,subRackNr,relativeBoardNr);
          updateVersion(scopeString,string(version),double(ack.timestamp));
  
          for (int ap = 0; ap < EPA_Protocol::N_AP; ap++)
          {
            sprintf(version,"%d.%d",ack.versions.ap()(board * EPA_Protocol::N_AP + ap) >> 4,
                                    ack.versions.ap()(board * EPA_Protocol::N_AP + ap) &  0xF);
            LOG_INFO(formatString("ap[%d][%d].version = 0x%x",board,ap,ack.versions.ap()(board * EPA_Protocol::N_AP + ap)));
            sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rackNr,subRackNr,relativeBoardNr,ap);
            updateVersion(scopeString,string(version),double(ack.timestamp));
          }
        }
      }
      
      // subscribe to status updates
      RSPSubstatusEvent substatus;
      substatus.timestamp.setNow();
      substatus.rcumask = std::bitset<MAX_N_RCUS>((1<<m_n_rcus)-1);
      substatus.period = m_status_update_interval;
      m_RSPclient.send(substatus);
      
      break;
    }
    
    case RSP_SUBSTATUSACK:
    {
      LOG_INFO("RSP_SUBSTATUSACK received");
      RSPSubstatusackEvent ack(e);

      if(ack.status != SUCCESS)
      {
        LOG_ERROR("RSP_SUBSTATUS failure");
      }
      else
      {
        m_subStatusHandle = ack.handle;
      }
      
      TRAN(RegisterAccessTask::subscribingStatsSubbandPower);
      
      break;
    }
    
    case RSP_UPDSTATUS:
    {
    	// handle updstatus events even though we are not subscribed to them yet
    	// this is done to relax the requirements for the ARAtest application
    	// (or you might call it lazyness)
      LOG_INFO("RSP_UPDSTATUS received");
      status = handleUpdStatus(e,port);
      break;
    }
    
    case F_DISCONNECTED:
    {
    	LOG_DEBUG(formatString("port %s disconnected", port.getName().c_str()));
    	port.close();

    	TRAN(RegisterAccessTask::APCsLoaded);
      break;
    }

    case F_EXIT:
    {
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::subscribingStatsSubbandPower(GCFEvent& e, GCFPortInterface &port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch (e.signal)
  {

    case F_INIT:
      break;
      
    case F_ENTRY:
    {
      // subscribe to status updates
      RSPSubstatsEvent substats;
      substats.timestamp.setNow();
      substats.rcumask = std::bitset<MAX_N_RCUS>((1<<m_n_rcus)-1);
      substats.period = m_stats_update_interval;
      substats.type = RSP_Protocol::Statistics::SUBBAND_POWER;
      substats.reduction = RSP_Protocol::REPLACE;
      m_RSPclient.send(substats);
      
      break;
    }

    case RSP_SUBSTATSACK:
    {
      LOG_INFO("RSP_SUBSTATSACK received");
      RSPSubstatsackEvent ack(e);

      if(ack.status != SUCCESS)
      {
        LOG_ERROR("RSP_SUBSTATS failure");
      }
      else
      {
        m_subStatsHandleSubbandPower = ack.handle;
      }
      
      TRAN(RegisterAccessTask::subscribingStatsBeamletPower);
      break;
    }
    
    case F_DISCONNECTED:
    {
      LOG_DEBUG(formatString("port %s disconnected", port.getName().c_str()));
      port.close();

      TRAN(RegisterAccessTask::APCsLoaded);
      break;
    }

    case F_EXIT:
    {
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::subscribingStatsBeamletPower(GCFEvent& e, GCFPortInterface &port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch (e.signal)
  {

    case F_INIT:
      break;
      
    case F_ENTRY:
    {
      // subscribe to status updates
      RSPSubstatsEvent substats;
      substats.timestamp.setNow();
      substats.rcumask = std::bitset<MAX_N_RCUS>((1<<m_n_rcus)-1);
      substats.period = m_stats_update_interval;
      substats.type = RSP_Protocol::Statistics::BEAMLET_POWER;
      substats.reduction = RSP_Protocol::REPLACE;
      m_RSPclient.send(substats);
      
      break;
    }

    case RSP_SUBSTATSACK:
    {
      LOG_INFO("RSP_SUBSTATSACK received");
      RSPSubstatsackEvent ack(e);

      if(ack.status != SUCCESS)
      {
        LOG_ERROR("RSP_SUBSTATS failure");
      }
      else
      {
        m_subStatsHandleBeamletPower = ack.handle;
      }
      
      TRAN(RegisterAccessTask::operational);
      break;
    }
    
    case F_DISCONNECTED:
    {
      LOG_DEBUG(formatString("port %s disconnected", port.getName().c_str()));
      port.close();

      TRAN(RegisterAccessTask::APCsLoaded);
      break;
    }

    case F_EXIT:
    {
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::operational(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch (e.signal)
  {

    case F_INIT:
      break;
      
    case F_ENTRY:
    {
      TMyPropertySetMap::iterator propsetIt = m_myPropertySetMap.find(SCOPE_PIC);
      if(propsetIt != m_myPropertySetMap.end())
      {
        boost::shared_ptr<GCFPVInteger> pvIntegrationTime(static_cast<GCFPVInteger*>(propsetIt->second->getValue(PROPNAME_INTEGRATIONTIME)));
        if(pvIntegrationTime != 0)
        {
          m_integrationTime = pvIntegrationTime->getValue();
          m_integrationTimerID = m_RSPclient.setTimer(static_cast<double>(m_integrationTime));
        }
        boost::shared_ptr<GCFPVInteger> pvIntegrationMethod(static_cast<GCFPVInteger*>(propsetIt->second->getValue(PROPNAME_INTEGRATIONMETHOD)));
        if(pvIntegrationMethod != 0)
        {
          m_integrationMethod = pvIntegrationMethod->getValue();
        }
      }
      
      break;
    }

    case RSP_UPDSTATUS:
    {
      LOG_INFO("RSP_UPDSTATUS received");
      status = handleUpdStatus(e,port);
      break;
    }
    
    case RSP_UPDSTATS:
    {
      LOG_INFO("RSP_UPDSTATS received");
      status = handleUpdStats(e,port);
      break;
    }
    
    case RSP_SETRCUACK:
    {
      LOG_INFO("RSP_SETRCUACK received");
      break;
    }
    
    case F_DISCONNECTED:
    {
      LOG_DEBUG(formatString("port %s disconnected", port.getName().c_str()));
      port.close();

      TRAN(RegisterAccessTask::APCsLoaded);
      break;
    }

    case F_VCHANGEMSG:
    {
      // check which property changed
      GCFPropValueEvent* pPropAnswer = static_cast<GCFPropValueEvent*>(&e);
      assert(pPropAnswer);

      if(strstr(pPropAnswer->pPropName,"Maintenance") != 0)
      {
        handleMaintenance(string(pPropAnswer->pPropName),*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_LBAENABLE) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_LBAENABLE,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_HBAENABLE) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_HBAENABLE,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_BANDSEL) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_BANDSEL,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_FILSEL0) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_FILSEL0,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_FILSEL1) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_FILSEL1,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_VLENABLE) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_VLENABLE,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_VHENABLE) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_VHENABLE,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_VDDVCCEN) != 0)
      {
        handleRCUSettings(string(pPropAnswer->pPropName),BIT_VDDVCCEN,*pPropAnswer->pValue);
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_INTEGRATIONTIME) != 0)
      {
        GCFPVInteger pvInt;
        pvInt.copy(*pPropAnswer->pValue);
        m_integrationTime = pvInt.getValue();
        LOG_INFO(formatString("integration time changed to %d",m_integrationTime));

        m_RSPclient.cancelTimer(m_integrationTimerID);
        
        if(m_integrationTime == 0)
        {
          m_integratingStatisticsSubband.free();
          m_numStatisticsSubband=0;
          m_integratingStatisticsBeamlet.free();
          m_numStatisticsBeamlet=0;
        }
        else
        {
          m_integratingStatisticsSubband.free();
          m_numStatisticsSubband=0;
          m_integratingStatisticsBeamlet.free();
          m_numStatisticsBeamlet=0;
          m_integrationTimerID = m_RSPclient.setTimer(static_cast<double>(m_integrationTime));
        }
      }
      else if(strstr(pPropAnswer->pPropName,PROPNAME_INTEGRATIONMETHOD) != 0)
      {
        GCFPVInteger pvInt;
        pvInt.copy(*pPropAnswer->pValue);
        m_integrationMethod = pvInt.getValue();
        LOG_INFO(formatString("integration method changed to %d",m_integrationMethod));
      }
      break;
    }
    
    case F_TIMER:
    {
      GCFTimerEvent& timerEvent=static_cast<GCFTimerEvent&>(e);
      if(timerEvent.id == m_integrationTimerID)
      {
        _integrateStatistics();
        m_integrationTimerID = m_RSPclient.setTimer(static_cast<double>(m_integrationTime));
      }
      break;
    }
    
    case F_EXIT:
    {
      m_RSPclient.cancelTimer(m_integrationTimerID);
      
      // unsubscribe from status updates
      RSPUnsubstatusEvent unsubStatus;
      unsubStatus.handle = m_subStatusHandle; // remove subscription with this handle
      m_RSPclient.send(unsubStatus);

      // unsubscribe from status updates
      RSPUnsubstatsEvent unsubStats;
      unsubStats.handle = m_subStatsHandleSubbandPower; // remove subscription with this handle
      m_RSPclient.send(unsubStats);
      unsubStats.handle = m_subStatsHandleBeamletPower; // remove subscription with this handle
      m_RSPclient.send(unsubStats);
      break;
    }

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult RegisterAccessTask::handleUpdStatus(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  {
    RSPUpdstatusEvent updStatusEvent(e);

    time_t curTime=(time_t)updStatusEvent.timestamp.sec();
    LOG_INFO(formatString("UpdStatus:time:%s:status:%d:handle:%d", 
        ctime(&curTime),
        updStatusEvent.status,
        updStatusEvent.handle));

    blitz::Array<EPA_Protocol::BoardStatus,  1>& boardStatus = updStatusEvent.sysstatus.board();
    blitz::Array<EPA_Protocol::RCUStatus,  1>& rcuStatus = updStatusEvent.sysstatus.rcu();
    
    int rackNr;
    int subRackNr;
    int relativeBoardNr;
    char scopeString[300];
    double timestamp = double(updStatusEvent.timestamp);

    int boardNr;
    for(boardNr=boardStatus.lbound(blitz::firstDim); boardNr <= boardStatus.ubound(blitz::firstDim); ++boardNr)
    {
      getBoardRelativeNumbers(boardNr,rackNr,subRackNr,relativeBoardNr);
      LOG_INFO(formatString("UpdStatus:Rack:%d:SubRack:%d:Board::%d\n",rackNr,subRackNr,relativeBoardNr));
      
      uint8   rspVoltage_15 = boardStatus(boardNr).rsp.voltage_15;
      uint8   rspVoltage_33 = boardStatus(boardNr).rsp.voltage_33;
      uint8   rspFfi0       = boardStatus(boardNr).rsp.ffi0;
      uint8   rspFfi1       = boardStatus(boardNr).rsp.ffi1;
      LOG_INFO(formatString("UpdStatus:RSP voltage_15:%d:voltage_33:%d:ffi0:%d:ffi1:%d",rspVoltage_15,rspVoltage_33,rspFfi0,rspFfi1));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN,rackNr,subRackNr,relativeBoardNr);
      updateBoardProperties(scopeString,rspVoltage_15,rspVoltage_33,rspFfi0,rspFfi1,timestamp);
      
      uint8   bpStatus  = boardStatus(boardNr).fpga.bp_status;
      uint8   bpTemp    = boardStatus(boardNr).fpga.bp_temp;
      uint8   ap1Status = boardStatus(boardNr).fpga.ap1_status;
      uint8   ap1Temp   = boardStatus(boardNr).fpga.ap1_temp;
      uint8   ap2Status = boardStatus(boardNr).fpga.ap2_status;
      uint8   ap2Temp   = boardStatus(boardNr).fpga.ap2_temp;
      uint8   ap3Status = boardStatus(boardNr).fpga.ap3_status;
      uint8   ap3Temp   = boardStatus(boardNr).fpga.ap3_temp;
      uint8   ap4Status = boardStatus(boardNr).fpga.ap4_status;
      uint8   ap4Temp   = boardStatus(boardNr).fpga.ap4_temp;
      LOG_INFO(formatString("UpdStatus:BP status:%d:temp:%d",bpStatus,bpTemp));
      LOG_INFO(formatString("UpdStatus:AP1 status:%d:temp:%d",ap1Status,ap1Temp));
      LOG_INFO(formatString("UpdStatus:AP2 status:%d:temp:%d",ap2Status,ap2Temp));
      LOG_INFO(formatString("UpdStatus:AP3 status:%d:temp:%d",ap3Status,ap3Temp));
      LOG_INFO(formatString("UpdStatus:AP4 status:%d:temp:%d",ap4Status,ap4Temp));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN,rackNr,subRackNr,relativeBoardNr);
      updateFPGAboardProperties(scopeString,timestamp);
      
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_BP,rackNr,subRackNr,relativeBoardNr);
      updateFPGAproperties(scopeString,bpStatus,bpTemp,timestamp);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rackNr,subRackNr,relativeBoardNr,0);
      updateFPGAproperties(scopeString,ap1Status,ap1Temp,timestamp);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rackNr,subRackNr,relativeBoardNr,1);
      updateFPGAproperties(scopeString,ap2Status,ap2Temp,timestamp);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rackNr,subRackNr,relativeBoardNr,2);
      updateFPGAproperties(scopeString,ap3Status,ap3Temp,timestamp);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN,rackNr,subRackNr,relativeBoardNr,3);
      updateFPGAproperties(scopeString,ap4Status,ap4Temp,timestamp);

      uint32    ethFrames     = boardStatus(boardNr).eth.nof_frames;
      uint32    ethErrors     = boardStatus(boardNr).eth.nof_errors;
      uint8     ethLastError  = boardStatus(boardNr).eth.last_error;
      uint8     ethFfi0       = boardStatus(boardNr).eth.ffi0;
      uint8     ethFfi1       = boardStatus(boardNr).eth.ffi1;
      uint8     ethFfi2       = boardStatus(boardNr).eth.ffi2;
      LOG_INFO(formatString("UpdStatus:ETH frames:%d:errors:%d:last_error:%d:ffi0:%d:ffi1:%d:ffi2:%d",ethFrames,ethErrors,ethLastError,ethFfi0,ethFfi1,ethFfi2));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_ETH,rackNr,subRackNr,relativeBoardNr);
      updateETHproperties(scopeString,ethFrames,ethErrors,ethLastError,ethFfi0,ethFfi1,ethFfi2,timestamp);  
  
      uint32    mepSeqnr = boardStatus(boardNr).mep.seqnr;
      uint8     mepError = boardStatus(boardNr).mep.error;
      uint8     mepFfi0  = boardStatus(boardNr).mep.ffi0;
      LOG_INFO(formatString("UpdStatus:MEP seqnr:%d:error:%d:ffi:%d",mepSeqnr,mepError,mepFfi0));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_MEPStatus,rackNr,subRackNr,relativeBoardNr);
      updateMEPStatusProperties(scopeString,mepSeqnr,mepError,mepFfi0,timestamp);  
      
      uint32    syncSample_count = boardStatus(boardNr).ap1_sync.sample_count;
      uint32    syncSync_count   = boardStatus(boardNr).ap1_sync.sync_count;
      uint32    syncError_count  = boardStatus(boardNr).ap1_sync.error_count;
      LOG_INFO(formatString("SyncStatus ap1:clock_count:%d:count:%d:errors:%d",syncSample_count,syncSync_count,syncError_count));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_SYNCStatus,rackNr,subRackNr,relativeBoardNr,0);
      updateSYNCStatusProperties(scopeString,syncSample_count,syncSync_count,syncError_count,timestamp);

      syncSample_count = boardStatus(boardNr).ap2_sync.sample_count;
      syncSync_count   = boardStatus(boardNr).ap2_sync.sync_count;
      syncError_count  = boardStatus(boardNr).ap2_sync.error_count;
      LOG_INFO(formatString("SyncStatus ap2:clock_count:%d:count:%d:errors:%d",syncSample_count,syncSync_count,syncError_count));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_SYNCStatus,rackNr,subRackNr,relativeBoardNr,1);
      updateSYNCStatusProperties(scopeString,syncSample_count,syncSync_count,syncError_count,timestamp);

      syncSample_count = boardStatus(boardNr).ap3_sync.sample_count;
      syncSync_count   = boardStatus(boardNr).ap3_sync.sync_count;
      syncError_count  = boardStatus(boardNr).ap3_sync.error_count;
      LOG_INFO(formatString("SyncStatus ap3:clock_count:%d:count:%d:errors:%d",syncSample_count,syncSync_count,syncError_count));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_SYNCStatus,rackNr,subRackNr,relativeBoardNr,2);
      updateSYNCStatusProperties(scopeString,syncSample_count,syncSync_count,syncError_count,timestamp);

      syncSample_count = boardStatus(boardNr).ap4_sync.sample_count;
      syncSync_count   = boardStatus(boardNr).ap4_sync.sync_count;
      syncError_count  = boardStatus(boardNr).ap4_sync.error_count;
      LOG_INFO(formatString("SyncStatus ap4:clock_count:%d:count:%d:errors:%d",syncSample_count,syncSync_count,syncError_count));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_SYNCStatus,rackNr,subRackNr,relativeBoardNr,3);
      updateSYNCStatusProperties(scopeString,syncSample_count,syncSync_count,syncError_count,timestamp);

      int apNr=0;
      uint8     boardRCUstatusStatusX       = boardStatus(boardNr).ap1_rcu.statusx;
      uint8     boardRCUstatusStatusY       = boardStatus(boardNr).ap1_rcu.statusy;
      uint8     boardRCUstatusFFI0          = boardStatus(boardNr).ap1_rcu.ffi0;
      uint8     boardRCUstatusFFI1          = boardStatus(boardNr).ap1_rcu.ffi1;
      uint32    boardRCUstatusNofOverflowX  = boardStatus(boardNr).ap1_rcu.nof_overflowx;
      uint32    boardRCUstatusNofOverflowY  = boardStatus(boardNr).ap1_rcu.nof_overflowy;
      LOG_INFO(formatString("BoardRCUStatus ap1:statusX:%d:statusY:%d:ffi0:%d:ffi1:%d:nofOverflowX:%d:nofOverflowY:%d",boardRCUstatusStatusX,boardRCUstatusStatusY,boardRCUstatusFFI0,boardRCUstatusFFI1,boardRCUstatusNofOverflowX,boardRCUstatusNofOverflowY));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_BoardRCUStatus,rackNr,subRackNr,relativeBoardNr,apNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusFFI0,boardRCUstatusFFI1,timestamp);
      int rcuNr=0;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusX,boardRCUstatusNofOverflowX,timestamp);
      rcuNr++;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusY,boardRCUstatusNofOverflowY,timestamp);

      apNr++;
      boardRCUstatusStatusX       = boardStatus(boardNr).ap2_rcu.statusx;
      boardRCUstatusStatusY       = boardStatus(boardNr).ap2_rcu.statusy;
      boardRCUstatusFFI0          = boardStatus(boardNr).ap2_rcu.ffi0;
      boardRCUstatusFFI1          = boardStatus(boardNr).ap2_rcu.ffi1;
      boardRCUstatusNofOverflowX  = boardStatus(boardNr).ap2_rcu.nof_overflowx;
      boardRCUstatusNofOverflowY  = boardStatus(boardNr).ap2_rcu.nof_overflowy;
      LOG_INFO(formatString("BoardRCUStatus ap2:statusX:%d:statusY:%d:ffi0:%d:ffi1:%d:nofOverflowX:%d:nofOverflowY:%d",boardRCUstatusStatusX,boardRCUstatusStatusY,boardRCUstatusFFI0,boardRCUstatusFFI1,boardRCUstatusNofOverflowX,boardRCUstatusNofOverflowY));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_BoardRCUStatus,rackNr,subRackNr,relativeBoardNr,apNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusFFI0,boardRCUstatusFFI1,timestamp);
      rcuNr=0;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusX,boardRCUstatusNofOverflowX,timestamp);
      rcuNr++;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusY,boardRCUstatusNofOverflowY,timestamp);

      apNr++;
      boardRCUstatusStatusX       = boardStatus(boardNr).ap3_rcu.statusx;
      boardRCUstatusStatusY       = boardStatus(boardNr).ap3_rcu.statusy;
      boardRCUstatusFFI0          = boardStatus(boardNr).ap3_rcu.ffi0;
      boardRCUstatusFFI1          = boardStatus(boardNr).ap3_rcu.ffi1;
      boardRCUstatusNofOverflowX  = boardStatus(boardNr).ap3_rcu.nof_overflowx;
      boardRCUstatusNofOverflowY  = boardStatus(boardNr).ap3_rcu.nof_overflowy;
      LOG_INFO(formatString("BoardRCUStatus ap3:statusX:%d:statusY:%d:ffi0:%d:ffi1:%d:nofOverflowX:%d:nofOverflowY:%d",boardRCUstatusStatusX,boardRCUstatusStatusY,boardRCUstatusFFI0,boardRCUstatusFFI1,boardRCUstatusNofOverflowX,boardRCUstatusNofOverflowY));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_BoardRCUStatus,rackNr,subRackNr,relativeBoardNr,apNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusFFI0,boardRCUstatusFFI1,timestamp);
      rcuNr=0;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusX,boardRCUstatusNofOverflowX,timestamp);
      rcuNr++;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusY,boardRCUstatusNofOverflowY,timestamp);

      apNr++;
      boardRCUstatusStatusX       = boardStatus(boardNr).ap4_rcu.statusx;
      boardRCUstatusStatusY       = boardStatus(boardNr).ap4_rcu.statusy;
      boardRCUstatusFFI0          = boardStatus(boardNr).ap4_rcu.ffi0;
      boardRCUstatusFFI1          = boardStatus(boardNr).ap4_rcu.ffi1;
      boardRCUstatusNofOverflowX  = boardStatus(boardNr).ap4_rcu.nof_overflowx;
      boardRCUstatusNofOverflowY  = boardStatus(boardNr).ap4_rcu.nof_overflowy;
      LOG_INFO(formatString("BoardRCUStatus ap4:statusX:%d:statusY:%d:ffi0:%d:ffi1:%d:nofOverflowX:%d:nofOverflowY:%d",boardRCUstatusStatusX,boardRCUstatusStatusY,boardRCUstatusFFI0,boardRCUstatusFFI1,boardRCUstatusNofOverflowX,boardRCUstatusNofOverflowY));
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_BoardRCUStatus,rackNr,subRackNr,relativeBoardNr,apNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusFFI0,boardRCUstatusFFI1,timestamp);
      rcuNr=0;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusX,boardRCUstatusNofOverflowX,timestamp);
      rcuNr++;
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackNr,subRackNr,relativeBoardNr,apNr,rcuNr);
      updateBoardRCUproperties(scopeString,boardRCUstatusStatusY,boardRCUstatusNofOverflowY,timestamp);
    }

    for(int rcuNr=rcuStatus.lbound(blitz::firstDim); rcuNr <= rcuStatus.ubound(blitz::firstDim); ++rcuNr)
    {
      uint8   rcuStatusBits = rcuStatus(rcuNr).status;
      LOG_INFO(formatString("UpdStatus:RCU[%d] status:0x%x",rcuNr,rcuStatusBits));
      
      int rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr;
      getRCURelativeNumbers(rcuNr,rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr);

      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr);
      LOG_DEBUG(formatString("RCU[%d]= %s",rcuNr,scopeString));
      updateRCUproperties(scopeString,rcuStatusBits,timestamp);
    }
  }
  
  return status;
}

GCFEvent::TResult RegisterAccessTask::handleUpdStats(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  {
    RSPUpdstatsEvent updStatsEvent(e);

    time_t curTime=(time_t)updStatsEvent.timestamp.sec();
    LOG_INFO(formatString("UpdStats:time:%s:status:%d:handle:%d", 
        ctime(&curTime),
        updStatsEvent.status,
        updStatsEvent.handle));

    _addStatistics(updStatsEvent.stats(), updStatsEvent.handle);
  }
  return status;
}
void RegisterAccessTask::updateBoardProperties(string scope,
                                               uint8  voltage_15,
                                               uint8  voltage_33,
                                               uint8  ffi0,
                                               uint8  ffi1,
                                               double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
  	LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    double v15 = static_cast<double>(voltage_15) * (2.5/192.0);
    GCFPVDouble pvDouble15(v15);
    it->second->setValueTimed(string(PROPNAME_VOLTAGE15),pvDouble15,timestamp);
    
    double v33 = static_cast<double>(voltage_33) * (5.0/192.0);
    GCFPVDouble pvDouble33(v33);
    it->second->setValueTimed(string(PROPNAME_VOLTAGE33),pvDouble33, timestamp);

    GCFPVUnsigned pvTemp;
    pvTemp.setValue(ffi0);
    it->second->setValueTimed(string(PROPNAME_FFI0),pvTemp, timestamp);

    pvTemp.setValue(ffi1);
    it->second->setValueTimed(string(PROPNAME_FFI1),pvTemp, timestamp);
  }
}

void RegisterAccessTask::updateETHproperties(string scope,
                                             uint32 frames,
                                             uint32 errors,
                                             uint8  lastError,
                                             uint8  ffi0,
                                             uint8  ffi1,
                                             uint8  ffi2,
                                             double timestamp)
{
  // layout eth status: 
  // 31......24  23.....16  15........8  7........0       
  // #RX[15..8]  #RX[7..0]  #Err[15..8]  #Err[7..0]  
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
  	LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    GCFPVUnsigned pvTemp(frames);
    it->second->setValueTimed(string(PROPNAME_FRAMESRECEIVED),pvTemp, timestamp);
    
    pvTemp.setValue(errors);
    it->second->setValueTimed(string(PROPNAME_FRAMESERROR),pvTemp, timestamp);

    pvTemp.setValue(lastError);
    it->second->setValueTimed(string(PROPNAME_LASTERROR),pvTemp, timestamp);

    pvTemp.setValue(ffi0);
    it->second->setValueTimed(string(PROPNAME_FFI0),pvTemp, timestamp);

    pvTemp.setValue(ffi1);
    it->second->setValueTimed(string(PROPNAME_FFI1),pvTemp, timestamp);

    pvTemp.setValue(ffi2);
    it->second->setValueTimed(string(PROPNAME_FFI2),pvTemp, timestamp);
  }
}

/**
 * update MEP status properties 
 */
void RegisterAccessTask::updateMEPStatusProperties(string scope,uint32 seqnr,
                                                                uint8  error,
                                                                uint8  ffi0,
                                                                double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
  	LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    GCFPVUnsigned pvTemp(seqnr);
    it->second->setValueTimed(string(PROPNAME_SEQNR),pvTemp, timestamp);
    
    pvTemp.setValue(error);
    it->second->setValueTimed(string(PROPNAME_ERROR),pvTemp, timestamp);

    pvTemp.setValue(ffi0);
    it->second->setValueTimed(string(PROPNAME_FFI0),pvTemp, timestamp);
  }
}

void RegisterAccessTask::updateSYNCStatusProperties(string scope,uint32 sample_count,
                                                                 uint32 sync_count,
                                                                 uint32 error_count,
                                                                 double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
  	LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    GCFPVUnsigned pvTemp(sample_count);
    it->second->setValueTimed(string(PROPNAME_SAMPLECOUNT),pvTemp, timestamp);
    
    pvTemp.setValue(sync_count);
    it->second->setValueTimed(string(PROPNAME_SYNCCOUNT),pvTemp, timestamp);

    pvTemp.setValue(error_count);
    it->second->setValueTimed(string(PROPNAME_ERRORCOUNT),pvTemp, timestamp);
  }
}
                                             
void RegisterAccessTask::updateFPGAboardProperties(string scope, double /*timestamp*/)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
    LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
  }
}

void RegisterAccessTask::updateFPGAproperties(string scope, uint8 status, 
                                                            uint8 temp,
                                                            double timestamp)
{
  // layout fpga status: 
  // 15..9 8       7........0       
  // ----- alive   temperature
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
    LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    LOG_INFO(formatString("FPGA status field is not yet used: %s.status=0x%x",scope.c_str(),status));
//    GCFPVUnsigned pvUns(status);
    GCFPVUnsigned pvUns(0);
    it->second->setValueTimed(string(PROPNAME_STATUS),pvUns, timestamp);
    
    GCFPVDouble pvDouble(static_cast<double>(temp));
    it->second->setValueTimed(string(PROPNAME_TEMPERATURE),pvDouble, timestamp);
  }
}

void RegisterAccessTask::updateBoardRCUproperties(string scope,uint8  ffi0,
                                                               uint8  ffi1,
                                                               double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
    LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    GCFPVUnsigned pvUns(ffi0);
    it->second->setValueTimed(string(PROPNAME_FFI0),pvUns, timestamp);
    pvUns.setValue(ffi1);
    it->second->setValueTimed(string(PROPNAME_FFI1),pvUns, timestamp);
  }
}

void RegisterAccessTask::updateRCUproperties(string scope,uint8 status, double timestamp)
{
  // layout rcu status (for both statusX and statusY): 
  // 7         6         5         4        3        2       1          0
  // VDDVCCEN VHENABLE VLENABLE FILSEL1 FILSEL0 BANDSEL HBAENABLE LBAENABLE
    
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
    LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    unsigned int tempStatus = (status >> 7 ) & 0x01;
    GCFPVBool pvBoolVddVccEn(tempStatus);
    if(pvBoolVddVccEn.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_VDDVCCEN)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_VDDVCCEN),pvBoolVddVccEn, timestamp);
    }
    
    tempStatus = (status >> 6) & 0x01;
    GCFPVBool pvBoolVhEnable(tempStatus);
    if(pvBoolVhEnable.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_VHENABLE)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_VHENABLE),pvBoolVhEnable, timestamp);
    }
    
    tempStatus = (status >> 5) & 0x01;
    GCFPVBool pvBoolVlEnable(tempStatus);
    if(pvBoolVlEnable.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_VLENABLE)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_VLENABLE),pvBoolVlEnable, timestamp);
    }
    
    tempStatus = (status >> 4) & 0x01;
    GCFPVBool pvBoolFilSel1(tempStatus);
    if(pvBoolFilSel1.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_FILSEL1)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_FILSEL1),pvBoolFilSel1, timestamp);
    }
    
    tempStatus = (status >> 3) & 0x01;
    GCFPVBool pvBoolFilSel0(tempStatus);
    if(pvBoolFilSel0.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_FILSEL0)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_FILSEL0),pvBoolFilSel0, timestamp);
    }
    
    tempStatus = (status >> 2) & 0x01;
    GCFPVBool pvBoolBandSel(tempStatus);
    if(pvBoolBandSel.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_BANDSEL)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_BANDSEL),pvBoolBandSel, timestamp);
    }
    
    tempStatus = (status >> 1) & 0x01;
    GCFPVBool pvBoolHBAEnable(tempStatus);
    if(pvBoolHBAEnable.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_HBAENABLE)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_HBAENABLE),pvBoolHBAEnable, timestamp);
    }
    
    tempStatus = (status >> 0) & 0x01;
    GCFPVBool pvBoolLBAEnable(tempStatus);
    if(pvBoolLBAEnable.getValue() != boost::shared_ptr<GCFPVBool>(static_cast<GCFPVBool*>(it->second->getOldValue(PROPNAME_LBAENABLE)))->getValue())
    {
      it->second->setValueTimed(string(PROPNAME_LBAENABLE),pvBoolLBAEnable, timestamp);
    }
      
    if(!pvBoolVddVccEn.getValue() ||
       (!pvBoolVhEnable.getValue() && !pvBoolVlEnable.getValue()) ||
       (!pvBoolHBAEnable.getValue() && !pvBoolLBAEnable.getValue()))
    {
      GCFPVUnsigned pvUnsignedStatus(STATUS_ERROR);
      it->second->setValueTimed(string(PROPNAME_STATUS),pvUnsignedStatus, timestamp);
    }       
    else
    {
      GCFPVUnsigned pvUnsignedStatus(STATUS_OK);
      it->second->setValueTimed(string(PROPNAME_STATUS),pvUnsignedStatus, timestamp);
    }       
  }
}

void RegisterAccessTask::updateBoardRCUproperties(string scope,uint8 /*status*/, uint32 nof_overflow, double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
    LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    LOG_WARN("ignoring status field in BoardRCUStatus");
    GCFPVUnsigned pvUns(nof_overflow);
    it->second->setValueTimed(string(PROPNAME_NOFOVERFLOW),pvUns, timestamp);
  }
}

void RegisterAccessTask::updateVersion(string scope, string version, double timestamp)
{
  TMyPropertySetMap::iterator it=m_myPropertySetMap.find(scope);
  if(it == m_myPropertySetMap.end())
  {
  	LOG_FATAL(formatString("PropertySet not found: %s",scope.c_str()));
  }
  else
  {
    GCFPVString pvString(version);
    it->second->setValueTimed(string(PROPNAME_VERSION),pvString,timestamp);
  }    
}

void RegisterAccessTask::handleMaintenance(string propName, const GCFPValue& value)
{
  GCFPVBool pvBool;
  pvBool.copy(value);
  bool maintenanceFlag(pvBool.getValue());
  
  // strip last part of the property name to get the resource name.
  int pos=propName.find_last_of("_.");
  string resource = propName.substr(0,pos);
  m_physicalModel.inMaintenance(maintenanceFlag,resource);
}

void RegisterAccessTask::handleRCUSettings(string propName, const int bitnr, const GCFPValue& value)
{
  boost::shared_ptr<GCFPVBool> pvLBAEnable;
  boost::shared_ptr<GCFPVBool> pvHBAEnable;
  boost::shared_ptr<GCFPVBool> pvBandSel;
  boost::shared_ptr<GCFPVBool> pvFilSel0;
  boost::shared_ptr<GCFPVBool> pvFilSel1;
  boost::shared_ptr<GCFPVBool> pvVLEnable;
  boost::shared_ptr<GCFPVBool> pvVHEnable;
  boost::shared_ptr<GCFPVBool> pvVddVccEn;
  
  TMyPropertySetMap::iterator propsetIt = getPropertySetFromScope(propName);
  if(propsetIt != m_myPropertySetMap.end())
  {
    // get old register values
    pvLBAEnable.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_LBAENABLE))); // bit 0
    pvHBAEnable.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_HBAENABLE))); // bit 1
    pvBandSel.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_BANDSEL)));   // bit 2
    pvFilSel0.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_FILSEL0)));   // bit 3
    pvFilSel1.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_FILSEL1)));   // bit 4
    pvVLEnable.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_VLENABLE)));  // bit 5
    pvVHEnable.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_VHENABLE)));  // bit 6
    pvVddVccEn.reset(static_cast<GCFPVBool*>(propsetIt->second->getValue(PROPNAME_VDDVCCEN)));  // bit 7
    
    // modify the new value
    switch(bitnr)
    {
      case BIT_LBAENABLE:
        pvLBAEnable->copy(value);
        break;
      case BIT_HBAENABLE:
        pvHBAEnable->copy(value);
        break;
      case BIT_BANDSEL:
        pvBandSel->copy(value);
        break;
      case BIT_FILSEL0:
        pvFilSel0->copy(value);
        break;
      case BIT_FILSEL1:
        pvFilSel1->copy(value);
        break;
      case BIT_VLENABLE:
        pvVLEnable->copy(value);
        break;
      case BIT_VHENABLE:
        pvVHEnable->copy(value);
        break;
      case BIT_VDDVCCEN:
        pvVddVccEn->copy(value);
        break;
      default:
        break;
    }
    
    int rcucontrol = 0;
    if(pvLBAEnable->getValue())
      rcucontrol |= 0x01;
    if(pvHBAEnable->getValue())
      rcucontrol |= 0x02;
    if(pvBandSel->getValue())
      rcucontrol |= 0x04;
    if(pvFilSel0->getValue())
      rcucontrol |= 0x08;
    if(pvFilSel1->getValue())
      rcucontrol |= 0x10;
    if(pvVLEnable->getValue())
      rcucontrol |= 0x20;
    if(pvVHEnable->getValue())
      rcucontrol |= 0x40;
    if(pvVddVccEn->getValue())
      rcucontrol |= 0x80;
    
    int rcu = getRCUHardwareNr(propName);
    if(rcu>=0)
    {
      RSPSetrcuEvent setrcu;
      setrcu.timestamp = RTC::Timestamp(0,0);
      setrcu.rcumask = 0;
      setrcu.rcumask.set(rcu);
          
      setrcu.settings().resize(1);
      setrcu.settings()(0).value = rcucontrol;
    
      m_RSPclient.send(setrcu);
    }
    else
    {
      LOG_FATAL(formatString("rcu for property %s not found in local administration",propName.c_str()));
    }
  }
  else
  {
    LOG_FATAL(formatString("property %s not found in local administration",propName.c_str()));
  }
}

void RegisterAccessTask::getBoardRelativeNumbers(int boardNr,int& rackNr,int& subRackNr,int& relativeBoardNr)
{
  rackNr          = boardNr / (m_n_subracks_per_rack*m_n_boards_per_subrack);
  subRackNr       = (boardNr / m_n_boards_per_subrack) % m_n_subracks_per_rack;
  relativeBoardNr = boardNr % m_n_boards_per_subrack;
}

void RegisterAccessTask::getRCURelativeNumbers(int rcuNr,int& rackRelativeNr,int& subRackRelativeNr,int& boardRelativeNr,int& apRelativeNr,int& rcuRelativeNr)
{
  rackRelativeNr    = rcuNr / (m_n_rcus_per_ap*m_n_aps_per_board*m_n_boards_per_subrack*m_n_subracks_per_rack);
  subRackRelativeNr = ( rcuNr / (m_n_rcus_per_ap*m_n_aps_per_board*m_n_boards_per_subrack) ) % m_n_subracks_per_rack;
  boardRelativeNr   = ( rcuNr / (m_n_rcus_per_ap*m_n_aps_per_board) ) % m_n_boards_per_subrack;
  apRelativeNr      = ( rcuNr / (m_n_rcus_per_ap) ) % m_n_aps_per_board;
  rcuRelativeNr     = ( rcuNr % m_n_rcus_per_ap);
}

int RegisterAccessTask::getRCUHardwareNr(const string& property)
{
  int rcu=-1;
  // strip property and systemname, propertyset name remains
  int posBegin=property.find_first_of(":")+1;
  int posEnd=property.find_last_of(".");
  string propertySetName = property.substr(posBegin,posEnd-posBegin);
  
  bool rcuFound=false;
  map<string,int>::iterator it = m_propertySet2RCUMap.begin();
  while(!rcuFound && it != m_propertySet2RCUMap.end())
  {
    if(propertySetName == it->first)
    {
      rcuFound=true;
      rcu = it->second;
    }
    else
    {
      ++it;
    }
  }
  return rcu;
}

RegisterAccessTask::TMyPropertySetMap::iterator RegisterAccessTask::getPropertySetFromScope(const string& property)
{
  // strip property and systemname, propertyset name remains
  int posBegin=property.find_first_of(":")+1;
  int posEnd=property.find_last_of(".");
  string propertySetName = property.substr(posBegin,posEnd-posBegin);
  
  bool rcuFound=false;
  TMyPropertySetMap::iterator it = m_myPropertySetMap.begin();
  while(!rcuFound && it != m_myPropertySetMap.end())
  {
    if(propertySetName == it->first)
    {
      rcuFound=true;
    }
    else
    {
      ++it;
    }
  }
  return it;
  
}


void RegisterAccessTask::_addStatistics(TStatistics& statistics, uint32 statsHandle)
{
  if(statsHandle == m_subStatsHandleSubbandPower)
  {
    if(m_integrationTime == 0)
    {
      _writeStatistics(statistics, statsHandle);
    }
    else
    {
      if(m_integratingStatisticsSubband.size() == 0)
      {
        m_integratingStatisticsSubband.resize(statistics.shape());
        m_integratingStatisticsSubband = 0;
      }
    
      m_integratingStatisticsSubband += statistics;
      m_numStatisticsSubband++;

      stringstream statisticsStream;
      statisticsStream << m_integratingStatisticsSubband(blitz::Range(0,2),blitz::Range(0,2));
      LOG_DEBUG(formatString("subband: n_stats:%d; statistics:%s",m_numStatisticsSubband, statisticsStream.str().c_str()));
    }
  }
  else if(statsHandle == m_subStatsHandleBeamletPower)
  {
    if(m_integrationTime == 0)
    {
      _writeStatistics(statistics, statsHandle);
    }
    else
    {
      if(m_integratingStatisticsBeamlet.size() == 0)
      {
        m_integratingStatisticsBeamlet.resize(statistics.shape());
        m_integratingStatisticsBeamlet = 0;
      }
    
      m_integratingStatisticsBeamlet += statistics;
      m_numStatisticsBeamlet++;

      stringstream statisticsStream;
      statisticsStream << m_integratingStatisticsBeamlet(blitz::Range(0,2),blitz::Range(0,2));
      LOG_DEBUG(formatString("beamlet: n_stats:%d; statistics:%s",m_numStatisticsBeamlet, statisticsStream.str().c_str()));
    }
  }
}

void RegisterAccessTask::_integrateStatistics()
{
  TStatistics statisticsSubband;
  TStatistics statisticsBeamlet;
  
  if(m_numStatisticsSubband != 0)
  {
    statisticsSubband.resize(m_integratingStatisticsSubband.shape());
    statisticsSubband = 0;
    
    switch(m_integrationMethod)
    {
      case 0: // average
      default:
        statisticsSubband = m_integratingStatisticsSubband/static_cast<double>(m_numStatisticsSubband);
        break;
  //    case 1: // NYI
  //      break;
    }
  
    stringstream statisticsStream;
    statisticsStream << statisticsSubband(blitz::Range(0,2),blitz::Range(0,2));
    LOG_DEBUG(formatString("subband integrated: n_stats:%d; statistics:%s",m_numStatisticsSubband, statisticsStream.str().c_str()));

    m_integratingStatisticsSubband = 0;
    m_numStatisticsSubband=0;

    _writeStatistics(statisticsSubband, m_subStatsHandleSubbandPower);
  }
  if(m_numStatisticsBeamlet != 0)
  {
    statisticsBeamlet.resize(m_integratingStatisticsBeamlet.shape());
    statisticsBeamlet = 0;
    
    switch(m_integrationMethod)
    {
      case 0: // average
      default:
        statisticsBeamlet = m_integratingStatisticsBeamlet/static_cast<double>(m_numStatisticsBeamlet);
        break;
  //    case 1: // NYI
  //      break;
    }

    stringstream statisticsStream;
    statisticsStream << statisticsSubband(blitz::Range(0,2),blitz::Range(0,2));
    LOG_DEBUG(formatString("beamlet integrated: n_stats:%d; statistics:%s",m_numStatisticsBeamlet, statisticsStream.str().c_str()));

    m_integratingStatisticsBeamlet.free();
    m_numStatisticsBeamlet=0;
    _writeStatistics(statisticsBeamlet, m_subStatsHandleBeamletPower);
  }
}

void RegisterAccessTask::_writeStatistics(TStatistics& statistics, uint32 statsHandle)
{
  int maxRCUs     = statistics.ubound(blitz::firstDim) - statistics.lbound(blitz::firstDim) + 1;
  int maxSubbands = statistics.ubound(blitz::secondDim) - statistics.lbound(blitz::secondDim) + 1;
  LOG_DEBUG(formatString("maxRCUs:%d maxSubbands:%d",maxRCUs,maxSubbands));
    
  if(m_centralized_stats)
  {
    LOG_DEBUG("Writing statistics to a dynamic array of doubles");
    // build a vector of doubles that will be stored in one datapoint
    GCFPValueArray valuePointerVector;
  
    // first elements indicate the length of the array
    valuePointerVector.push_back(new GCFPVDouble(maxRCUs));
    valuePointerVector.push_back(new GCFPVDouble(maxSubbands));
  
    // then for each rcu, the statistics of the beamlets or subbands
    int rcu,subband;
    for(rcu=statistics.lbound(blitz::firstDim);rcu<=statistics.ubound(blitz::firstDim);rcu++)
    {
      for(subband=statistics.lbound(blitz::secondDim);subband<=statistics.ubound(blitz::secondDim);subband++)
      {
        double stat = statistics(rcu,subband);
        valuePointerVector.push_back(new GCFPVDouble(stat));
      }
    }

    // convert the vector of unsigned values to a dynamic array
    GCFPVDynArr dynamicArray(LPT_DOUBLE,valuePointerVector);
    
    // set the property
    TMyPropertySetMap::iterator propSetIt=m_myPropertySetMap.find(string(SCOPE_PIC));
    if(propSetIt != m_myPropertySetMap.end())
    {
      if(statsHandle == m_subStatsHandleSubbandPower)
      {
        propSetIt->second->setValue(string(PROPNAME_STATISTICSSUBBANDPOWER),dynamicArray);
      }
      else if(statsHandle == m_subStatsHandleBeamletPower)
      {
        propSetIt->second->setValue(string(PROPNAME_STATISTICSBEAMLETPOWER),dynamicArray);
      }
    }
  
    // cleanup
    GCFPValueArray::iterator it=valuePointerVector.begin();
    while(it!=valuePointerVector.end())
    {
      delete *it;
      valuePointerVector.erase(it);
      it=valuePointerVector.begin();
    }
  }
  else
  {
    LOG_DEBUG("Writing statistics to one string");
    // statistics will be stored as a string in an element of each rcu
    // then for each rcu, the statistics of the beamlets or subbands
    int rcu,subband;
    for(rcu=statistics.lbound(blitz::firstDim);rcu<=statistics.ubound(blitz::firstDim);rcu++)
    {
      LOG_DEBUG(formatString("rcu:%d",rcu));
      
      stringstream statisticsStream;
      statisticsStream.setf(ios_base::fixed);
      for(subband=statistics.lbound(blitz::secondDim);subband<=statistics.ubound(blitz::secondDim);subband++)
      {
        double stat = statistics(rcu,subband);
        statisticsStream << subband << " ";
        statisticsStream << stat << endl;
      }
      LOG_DEBUG(formatString("first part of statistics:%s",statisticsStream.str().substr(0,30).c_str()));
      
      GCFPVString statisticsString(statisticsStream.str());
      // set the property
      char scopeString[300];
      int rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr;
      getRCURelativeNumbers(rcu,rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr);
      sprintf(scopeString,SCOPE_PIC_RackN_SubRackN_BoardN_APN_RCUN,rackRelativeNr,subRackRelativeNr,boardRelativeNr,apRelativeNr,rcuRelativeNr);
      TMyPropertySetMap::iterator propSetIt=m_myPropertySetMap.find(scopeString);
      if(propSetIt != m_myPropertySetMap.end())
      {
        if(statsHandle == m_subStatsHandleSubbandPower)
        {
          TGCFResult res = propSetIt->second->setValue(string(PROPNAME_STATISTICSSUBBANDPOWER),statisticsString);
          LOG_DEBUG(formatString("Writing subband statistics to %s returned %d",propSetIt->second->getScope().c_str(),res));
        }
        else if(statsHandle == m_subStatsHandleBeamletPower)
        {
          TGCFResult res = propSetIt->second->setValue(string(PROPNAME_STATISTICSBEAMLETPOWER),statisticsString);
          LOG_DEBUG(formatString("Writing beamlet statistics to %s returned %d",propSetIt->second->getScope().c_str(),res));
        }
      }
    }
  }
}

} // namespace ARA


} // namespace LOFAR

