// navigator.ctl
//
//  Copyright (C) 2002-2004
//  ASTRON (Netherlands Foundation for Research in Astronomy)
//  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, softwaresupport@astron.nl
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  $Id$
//
///////////////////////////////////////////////////////////////////
// global functions for the navigator
///////////////////////////////////////////////////////////////////
//
// Functions and procedures
//
// navigator_handleEventInitialize	: initializes the navigator
// navigator_handleEventTerminate	: terminates the navigator
// navigator_handleEventClose		: closes the navigator
// navigator_initializing               : returns false if init ready, else true
// navigator_clearWorkDPs               : clear the work Datapoints


#uses "GCFLogging.ctl"
#uses "GCFCWD.ctl"
#uses "claimManager.ctl"
#uses "GCFCommon.ctl"
#uses "GCFAlarm.ctl"
#uses "navCtrl.ctl"
#uses "navConfig.ctl"
#uses "navFunct.ctl"
#uses "navTabCtrl.ctl"
#uses "navProgressCtrl.ctl"

global bool       g_objectReady           = true;     // Can be used for timing by objects

// store the current datapoint
global string     g_currentDatapoint          = MainDBName+"LOFAR_PIC_Europe";

// store the last chosen panel for each tab
global string     g_lastHardwareDatapoint     = MainDBName+"LOFAR_PIC_Europe";
global string     g_lastProcessesDatapoint    = MainDBName+"LOFAR_PermSW";
global string     g_lastObservationsDatapoint = MainDBName+"LOFAR_ObsSW";
global string     g_lastPipelinesDatapoint    = MainDBName+"LOFAR_ObsSW";

// store the current active panel
global string     g_activePanel               = "main.pnl";

// store the last chosen antenna type
global string     g_activeAntennaType         = "HBA"; 


global dyn_string g_observationsList;  // holds active observations
global dyn_string g_pipelinesList;     // holds active pipelines
global dyn_string g_processesList;     // holds active software
global mapping    g_observations;      //
global dyn_string g_involved_stations; // holds stations that were involved in a distchange

global dyn_string g_stationList;       // holds valid stations for choices in the viewBox
// Station based globals
global dyn_int    g_cabinetList;       // holds valid cabinets for choices in the viewBox
global dyn_int    g_subrackList;       // holds valid subracks for choices in the viewBox
global dyn_int    g_uriBoardList;      // holds valid uriBoards for choices in the viewBox
global dyn_int    g_uniBoardList;      // holds valid uniBoards for choices in the viewBox
global dyn_int    g_FPGAList;          // holds valid fpgas for choices in the viewBox
global dyn_int    g_RSPList;           // holds valid RSP's for choices in the viewBox
global dyn_int    g_TBBList;           // holds valid TBB's for choices in the viewBox
global dyn_int    g_RCUList;           // holds valid RCU's for choices in the viewBox
global dyn_int    g_HBAList;           // holds valid HBAAntenna's for choices in the viewBox
global dyn_int    g_LBAList;           // holds valid LBAAntenna's for choices in the viewBox
// CEP based globals
global dyn_int    g_OSRackList;        // holds valid Offline/Storageracks for choices in view
global dyn_int    g_locusNodeList;     // holds valid storagenodes for choices in view
global dyn_int    g_cobaltRackList;    // holds valid cobaltracks for choices in viewBox
global dyn_int    g_cobaltNodeList;    // holds valid cobaltnodes for choices in viewBox
global dyn_int    g_cobaltNICList;     // holds valid cobaltNICs for choices in viewBox


global dyn_string strPlannedObs;
global dyn_string strHighlight;        // contains highlight info for mainpanels
global dyn_string highlight;           // contains highlight info for objects
global string     panelSelection       = ""; // holds selected panel

global dyn_string coreStations;
global dyn_string remoteStations;
global dyn_string europeStations;
global dyn_string superTerpStations;
global dyn_string cs0nnCoreStations;
global dyn_string csx01CoreStations;

global string savePath = "./";


//======================== Action Handlers =======================================
//
// The following functions are used to handle the 'actions' that are 
// generated by the mainpanel.
//

void navigator_handleEventInitialize()
{
  LOG_TRACE("navigator.ctl:navigator_handleEventInitialize|entered");
  g_initializing = true;
  
    MainDBName         = getSystemName();
    MainDBID           = getSystemId();
  // first we need to check if we are on the MainCU or a stationDB, if we are on a stationDB we are in standalone mode
  if (strpos(MainDBName,"MCU") >= 0) {
    CEPDBName          = MainDBName;
    strreplace(CEPDBName,"MCU","CCU");
    } else {
    g_standAlone       = true;    // can be used to check if we are in standalone mode (== station only mode)
  }
  g_currentDatapoint          = MainDBName+"LOFAR";
  g_lastHardwareDatapoint     = MainDBName+"LOFAR";
  g_lastProcessesDatapoint    = MainDBName+"LOFAR_PermSW";
  g_lastObservationsDatapoint = MainDBName+"LOFAR_ObsSW";
  g_lastPipelinesDatapoint    = MainDBName+"LOFAR_ObsSW";
    
  // Set the global statecolors/colornames, we need to do this before we 
  //start the rest of the framework, because the other processes need these
  initLofarColors();
  
  // first thing to do: get a new navigator ID,
  // this also clears all navigator_instance points from previous runs
  // check the commandline parameter:
  int navID = 0;
  if (isDollarDefined("$ID")) {
    navID = $ID;
  }
  
  // make sure there is a __navigator<id> datapoint of the type GCFNavigatorInstance.
  navConfig_setNavigatorID(navID); 

  // initialize the logSystem
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".logger")) {
    initLog(DPNAME_NAVIGATOR + g_navigatorID + ".logger");
  } else {
    DebugN("ERROR: Logsystem hasn't been found.");
  }
  
  
  // Do a dpQueryConnectSingle() so that we get a permanent list of claims
  // we can use this to translate a claimed name into a real datapoint name
  claimManager_queryConnectClaims();
  
  

  // we need to wait until the claim system was able to finish all connections and callbacks
  if (!waitInitProcess("connectClaimsFinished")) {
    LOG_FATAL("navigator.ctl:navigator_handleEventInitialize|Couldn't finish claimManager_queryConnectClaims() , leaving");
  }
  
  // fill global stations lists
  navFunct_fillStationLists();
  
  // Init the connection Watchdog
  GCFCWD_Init();


  // we need to wait until the connection watchdog has been initialised
  if (!waitInitProcess("GCFCWDFinished")) {
    LOG_FATAL("navigator.ctl:navigator_handleEventInitialize|Couldn't finish GCFCWD_Init() , leaving");
  }


  // set user to root for now, has to be taken from PVSS login later
  // since the names are caseinsensitive, convert to lowercase for 
  // database point conveniance later
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".user")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".user",getUserName());
    ACTIVE_USER=strtolower(getUserName());
  }

  // Clear the workDatapoints
  navigator_clearWorkDPs();  
        
  // Initilaize the alarm system
  initNavigatorAlarms();


  // we need to wait until the alarmSystem has been initialised
  if (!waitInitProcess("initNavigatorAlarmsFinished")) {
    LOG_FATAL("navigator.ctl:navigator_handleEventInitialize|Couldn't finish initNavigatorAlarmsFinished() , leaving");
  }
 
  // Do a dpQueryConnectSingle() so that we get a list of observations
  // so that we can easily populate our tables with 'Planned', 'Running' and 'Finished' observations
  navFunct_queryConnectObservations();
  
  // we need to wait until all known observations have been initialized
  if (!waitInitProcess("queryConnectObservationsFinished")) {
    LOG_FATAL("navigator.ctl:navigator_handleEventInitialize|Couldn't finish queryConnectObservationsFinished() , leaving");
  }


    // set initialized ready
  g_initializing = false;

  LOG_TRACE("navigator.ctl:navigator_handleEventInitialize|end");
  
}

///////////////////////////////////////////////////////////////////////////
//
// Function navigator_handleEventTerminate()
//
// NOTE: it is NOT possible to call dpGet in the terminate handler!
//
///////////////////////////////////////////////////////////////////////////
void navigator_handleEventTerminate()
{
  LOG_TRACE("navigator.ctl:navigator_handleEventTerminate|entered");
}

///////////////////////////////////////////////////////////////////////////
//
// Function navigator_handleEventClose()
//
// de-initializes the navigator
// NOTE: it is NOT possible to call dpGet in the terminate handler!
//
///////////////////////////////////////////////////////////////////////////
void navigator_handleEventClose()
{
  LOG_TRACE("navigator.ctl:navigator_handleEventClose|entered");

    
  PanelOff();
}

///////////////////////////////////////////////////////////////////////////
//
// Function navigator_initializing()
//
// returns false if init is ready, else false
//
///////////////////////////////////////////////////////////////////////////
bool navigator_initializing() {
  return g_initializing;
}

///////////////////////////////////////////////////////////////////////////
//
// Function navigator_clearWorkDPs()
//
// clears all workpoints that can contain residues from previous runs.
//
///////////////////////////////////////////////////////////////////////////
void navigator_clearWorkDPs() {
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".observationsList")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".observationsList",makeDynString(",LOFAR,LOFAR"));
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".hardwareList")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".hardwareList",makeDynString(",LOFAR,LOFAR"));
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".processesList")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".processesList",makeDynString(""));
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".trigger")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".trigger",false);
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".objectTrigger")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".objectTrigger",false);
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_viewBox.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_viewBox.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_viewSelection.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_viewSelection.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_topDetailSelection.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_topDetailSelection.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_bottomDetailSelection.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_bottomDetailSelection.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_locator.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_locator.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_progressBar.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_progressBar.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_headLines.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_headLines.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_alerts.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_alerts.action","");
  }
  if (dpExists(DPNAME_NAVIGATOR + g_navigatorID + ".fw_fastJumper.action")) {
    dpSet(DPNAME_NAVIGATOR + g_navigatorID + ".fw_fastJumper.action","");
  }

}

