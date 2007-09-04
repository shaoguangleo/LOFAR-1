//# gcfnav-tab.ctl
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
//#  You should have received a copy of the GNU General Public Licenlse
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//#  $Id$

//#
//# global functions for the Tabs of the Navigator. All event handlers are implemented here
//#

#uses "nav_fw/gcf-logging.ctl"
#uses "nav_fw/gcfnavigator.ctl"
#uses "nav_fw/gcfnav-configuration.ctl"

global string  VIEW_COMBOBOX_CTRL      = "ComboBoxViews";
global string  VIEW_TABS_CTRL          = "TabViews";
global string  VIEW_TABS_VIEW_NAME     = "View";
global string  VIEW_TABS_CONFIG_NAME   = "Config";

global mapping g_subViews;
global mapping g_subViewConfigs;
global string  g_datapoint;
global string  g_configPanelFileName;
global int     g_selectedView;
global string  g_selectedViewName;

///////////////////////////////////////////////////////////////////////////
//
// Function mappingClear
//  
// removes all items from a mapping
//
///////////////////////////////////////////////////////////////////////////
void mappingClear(mapping &map) {
  	LOG_DEBUG("mappingClear begin",map);
  	while (mappinglen(map)>0)
    	mappingRemove(map,mappingGetKey(map,1));
  	LOG_DEBUG("mappingClear end",map);
}

///////////////////////////////////////////////////////////////////////////
//
// Function NavTabInitialize
//  
// initializes the panel
// parameters:
//   datapoint        : string representing the selected datapoint, including system name and (if applicable) element name
//
///////////////////////////////////////////////////////////////////////////
void NavTabInitialize(string datapoint) {
  	LOG_DEBUG("NavTabInitialize(datapoint): ",datapoint);
  
  	dyn_errClass err;
  	shape viewsComboBoxCtrl = getShape(VIEW_COMBOBOX_CTRL);  
  	shape viewTabsCtrl      = getShape(VIEW_TABS_CTRL);  
  
  	// clear mappings and combobox
  	viewsComboBoxCtrl.deleteAllItems();
  	mappingClear(g_subViews);
  	mappingClear(g_subViewConfigs);
  	g_selectedView = 1;
  	g_selectedViewName = navConfigGetSelectedView();
  	LOG_DEBUG("NavTabInitialize: selView",g_selectedViewName);
  
  	g_datapoint = datapoint;
  	g_configPanelFileName = "";
  	string viewsPath = navConfigGetViewsPath();
  	string dpNameConfig = navConfigGetViewConfig(g_datapoint);
  
  	int selectedSubView;
  	int selectedViewUnused;
  	dyn_string views;
  	dyn_int  nrOfSubViews;
  	dyn_string subViews;
  	dyn_string configs;
  	if(navConfigGetViewConfigElements(dpNameConfig,
                                      selectedViewUnused,
                                      selectedSubView,
                                      views,
                                      nrOfSubViews,
                                      subViews,
                                      configs)) {
    	LOG_DEBUG("NavTabInitialize:",g_selectedView,selectedSubView,LOG_DYN(views),LOG_DYN(nrOfSubViews),LOG_DYN(subViews),LOG_DYN(configs));
    	for(int i=1;i<=dynlen(views);i++) {
      		if(navConfigGetViewCaption(views[i]) == g_selectedViewName)
        		g_selectedView = i;
    	}

    	// create the mapping
    	int beginSubViews=1;
    	if(g_selectedView < 1)
      		g_selectedView = 1;
    	if(selectedSubView < 1)
      		selectedSubView = 1;
    	if(selectedSubView > viewsComboBoxCtrl.itemCount())
     		selectedSubView=1;
    	for(int i=1;i<g_selectedView;i++) {
      		beginSubViews += nrOfSubViews[i];
    	}
    	for(int i=beginSubViews;i<beginSubViews+nrOfSubViews[g_selectedView];i++) {
      		// get subviews config
      		string subViewCaption;
      		string subViewFileName;
      		if(navConfigGetSubViewConfigElements(subViews[i],subViewCaption,subViewFileName)) {
        		LOG_DEBUG("subviewcaption,subviewfilename:",subViewCaption,viewsPath+subViewFileName);
        		g_subViews[subViewCaption] = subViewFileName;
        		g_subViewConfigs[subViewCaption] = configs[i];
      		}
    	}
    	LOG_TRACE("g_subViews = ",g_subViews);
    	LOG_TRACE("g_subViewConfigs = ",g_subViewConfigs);
    
    	// fill the combobox
    	for(int i=1;i<=mappinglen(g_subViews);i++) {
      		viewsComboBoxCtrl.appendItem(mappingGetKey(g_subViews,i));
    	}

    	// get the config panel filename    
    	g_configPanelFileName = navConfigGetViewConfigPanel(views[g_selectedView]);
    	g_configPanelFileName = g_configPanelFileName;
   
    	viewsComboBoxCtrl.selectedPos(selectedSubView);
    	ComboBoxViewsSelectionChanged();
  	}
}


///////////////////////////////////////////////////////////////////////////
//
// Function ComboBoxViewsSelectionChanged
//  
// called when the combobox changes
//
///////////////////////////////////////////////////////////////////////////
void ComboBoxViewsSelectionChanged() {
  	LOG_DEBUG("ComboBoxViewsSelectionChanged()");
  
  	shape viewsComboBoxCtrl = getShape(VIEW_COMBOBOX_CTRL);  
  	shape viewTabsCtrl      = getShape(VIEW_TABS_CTRL);  
  	string viewsPath = navConfigGetViewsPath();
  	// store the selected subview:
  	int selectedSubViewIndex = viewsComboBoxCtrl.selectedPos();
  	navConfigSetSelectedSubView(g_datapoint,selectedSubViewIndex);
  
  	// if config tab is selected, some more actions may be required
  
  	string selectedSubView = viewsComboBoxCtrl.selectedText();
  	string selectedPanel = "";
  	if(mappinglen(g_subViews)>0) {
  	  	selectedPanel= g_subViews[selectedSubView];
  	}

  	//if for selectedSubView a mapping has been found, use this name,
  	//otherwise parse an empty string to prevent WARNINGS
  	string insertSubViewConfigs="";
  	if(mappingHasKey(g_subViewConfigs, selectedSubView)) {
    	insertSubViewConfigs = g_subViewConfigs[selectedSubView];   
  	}
  	else {
    	insertSubViewConfigs = "";
  	}
  	//Get the name of the refDp is it exits
  	string datapointPath = buildPathFromNode(g_curSelNode);
  	string dpNameTemp = datapointPath;
  	bool isReference;
  	dyn_string reference;
  	string referenceDatapoint="";
  	checkForReference(dpNameTemp, reference, isReference);
  	if(isReference) {
    	referenceDatapoint=datapointPath;
  	}
  	dyn_string panelParameters = makeDynString(
    	"$datapoint:" + g_datapoint,
    	"$configDatapoint:" + insertSubViewConfigs,
    	"$referenceDatapoint:" +referenceDatapoint);
  	LOG_TRACE("selectedSubView,selectedPanel,panelParameters: ",selectedSubView,selectedPanel,panelParameters);
  
  	//---------------------------------------------------------------
  	//viewTabsCtrl.namedRegisterPanel divided in three possibilities.
  	//1. There is no subview configured for the DpType
  	//2. 
  	//3. panelfile present and accessable
  	//4. There is no panelfile configured for this DP-Type
  	//5. panelfile not present and accessable
  	if(viewsComboBoxCtrl.itemCount==0) {
    	viewTabsCtrl.namedRegisterPanel(VIEW_TABS_VIEW_NAME, viewsPath + "nosubview.pnl",panelParameters);
    	LOG_WARN("1. No subview configured for this datapoint", g_datapoint);
  	}
  	//  else if(!dpAccessable(g_datapoint))
  	//  {
  	//    viewTabsCtrl.namedRegisterPanel(VIEW_TABS_VIEW_NAME, viewsPath + "nodpfound.pnl",panelParameters);
  	//    LOG_DEBUG("2. Datapoint selected in tree not found.");
  	//  }
  	else if(access(getPath(PANELS_REL_PATH)+selectedPanel,F_OK) == 0 && selectedPanel!="")
  	{
    	viewTabsCtrl.namedRegisterPanel(VIEW_TABS_VIEW_NAME,selectedPanel,panelParameters);
    	LOG_INFO("3 selectedPanel:", selectedPanel);
  	} 
  	else if (selectedPanel=="0") {
    	selectedPanel = viewsPath + "nopanel.pnl";
    	viewTabsCtrl.namedRegisterPanel(VIEW_TABS_VIEW_NAME, viewsPath + "nopanel.pnl",panelParameters);
    	LOG_WARN("4 No panel configured for this subview",selectedSubView);
  	}
  	else {  //5. The configured panel file for this subview has not been found
    	string oldSelectedPanel = selectedPanel;
    	viewTabsCtrl.namedRegisterPanel(VIEW_TABS_VIEW_NAME, viewsPath + "nopanelfound.pnl",panelParameters);
    	LOG_WARN("5. File does not exist:",oldSelectedPanel);
  	}            
 
  	string datapointTypeName = "";
//  	if(dpAccessable(g_datapoint+"__enabled")) {
//    	datapointTypeName = getDpTypeFromEnabled(g_datapoint);
//    	LOG_INFO("get typename from enabled",g_datapoint,datapointTypeName);
//  	}
//  	else 
	if(dpAccessable(g_datapoint)) {
    	datapointTypeName = dpTypeName(g_datapoint);
    	LOG_INFO("get typename not from enabled but it is accessible ",g_datapoint,datapointTypeName);
  	}
  	else {
    	// cut system name myself. Necessary for datapoint parts that are not datapoints themselves
    	datapointTypeName = g_datapoint;
    	int sepPos = strpos(datapointTypeName, ":");
    	if (sepPos >= 0) {
      		datapointTypeName = substr(datapointTypeName, sepPos + 1);
    	}
    	LOG_INFO("datapoint is not accessible",g_datapoint,datapointTypeName);
  	}

  	// Load the config panel in the viewTabsCtrl
  	dyn_string configPanelParameters = makeDynString(
    	"$selectedView:" + g_selectedView,
    	"$viewName:" + g_selectedViewName,
    	"$selectedElementDpType:" + datapointTypeName,
    	"$datapoint:" + g_datapoint,
    	"$configDatapoint:"+insertSubViewConfigs,
    	"$referenceDatapoint:" +referenceDatapoint);
  
  	LOG_TRACE("configPanel,configParameters: ",g_configPanelFileName,configPanelParameters);
  
  	// check if the file exists:
  	if(access(getPath(PANELS_REL_PATH)+g_configPanelFileName,F_OK) == 0 && g_configPanelFileName!="") {
    	viewTabsCtrl.namedRegisterPanel(VIEW_TABS_CONFIG_NAME,g_configPanelFileName,configPanelParameters);
  	}
  	else {
    	LOG_WARN("File does not exist:",g_configPanelFileName);
  	}
}

///////////////////////////////////////////////////////////////////////////
//
// Function ConfigTabAddSubViewClicked
//  
// called when the "add subview" button is clicked
//
///////////////////////////////////////////////////////////////////////////
bool ConfigTabAddSubViewClicked(string viewName, int selectedView, string selectedElementDpType, string configDatapoint, int &nrOfSubViews) {
  	LOG_DEBUG("ConfigTabAddSubViewClicked: ", viewName, selectedView, selectedElementDpType, configDatapoint, nrOfSubViews);
  	bool success;
  	dyn_float resultFloat;
  	dyn_string resultString;
  	string viewsPath = "";
  	dpGet("__navigator." + ELNAME_VIEWSPATH, viewsPath);
  	string panelName = viewsPath + "navigator_newsubview.pnl";
  	ChildPanelOnCentralModalReturn(
    	panelName,
    	"Add Sub-view",
    	makeDynString("$addView:TRUE", 
                  	  "$viewName:" + viewName,
                	  "$selectedView:" + selectedView, 
                	  "$selectedElementDpType:" + selectedElementDpType, 
                	  "$configDatapoint:" + configDatapoint),
    	resultFloat,
    	resultString);
  	success = resultFloat[1];
  	nrOfSubViews = resultFloat[2];
  	if(success) {
    	navConfigTriggerNavigatorRefresh();
  	}
  	return success;
}

///////////////////////////////////////////////////////////////////////////
//
// Function ConfigTabRemoveSubViewClicked
//  
// called when the "remove subview" button is clicked
//
///////////////////////////////////////////////////////////////////////////
bool ConfigTabRemoveSubViewClicked(string viewName, int selectedView, string selectedElementDpType, string configDatapoint, int &nrOfSubViews) {
  	LOG_DEBUG("ConfigTabRemoveSubViewClicked: ", viewName, selectedView, selectedElementDpType, configDatapoint, nrOfSubViews);
  	bool success;
  	dyn_float resultFloat;
  	dyn_string resultString;
  	string viewsPath = "";
  	dpGet("__navigator." + ELNAME_VIEWSPATH, viewsPath);

  	ChildPanelOnCentralModalReturn(
    	viewsPath + "navigator_newsubview.pnl",
    	"Remove Sub-view",
    	makeDynString("$addView:FALSE", 
                  	  "$viewName:" + viewName,
                  	  "$selectedView:" + selectedView, 
                  	  "$selectedElementDpType:" + selectedElementDpType, 
                  	  "$configDatapoint:" + configDatapoint),
        resultFloat,
    	resultString);

  	success = resultFloat[1];
  	nrOfSubViews = resultFloat[2];
  	if(success) {
    	navConfigTriggerNavigatorRefresh();
  	}
  	return success;
}
