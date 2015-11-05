/*
 * ObservationPanel.java
 *  Copyright (C) 2002-2007
 *  ASTRON (Netherlands Foundation for Research in Astronomy)
 *  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

package nl.astron.lofar.sas.otbcomponents.userpanels;

import java.awt.Component;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.rmi.RemoteException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Collection;
import java.util.Enumeration;
import java.util.Vector;
import javax.swing.DefaultListModel;
import javax.swing.JFileChooser;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.ListSelectionModel;
import javax.swing.border.TitledBorder;
import nl.astron.lofar.lofarutils.LofarUtils;
import nl.astron.lofar.sas.otb.MainFrame;
import nl.astron.lofar.sas.otb.jotdb3.jOTDBnode;
import nl.astron.lofar.sas.otb.jotdb3.jOTDBparam;
import nl.astron.lofar.sas.otb.jotdb3.jOTDBtree;
import nl.astron.lofar.sas.otb.objects.AnaBeam;
import nl.astron.lofar.sas.otb.objects.Beam;
import nl.astron.lofar.sas.otb.objects.TiedArrayBeam;
import nl.astron.lofar.sas.otb.util.IViewPanel;
import nl.astron.lofar.sas.otb.util.OtdbRmi;
import nl.astron.lofar.sas.otb.util.UserAccount;
import nl.astron.lofar.sas.otb.util.tablemodels.AnaBeamConfigurationTableModel;
import nl.astron.lofar.sas.otb.util.tablemodels.BeamConfigurationTableModel;
import nl.astron.lofar.sas.otb.util.tablemodels.BeamformerConfigurationTableModel;
import nl.astron.lofar.sas.otbcomponents.AnaBeamDialog;
import nl.astron.lofar.sas.otbcomponents.BeamDialog;
import nl.astron.lofar.sas.otbcomponents.BeamFileDialog;
import org.apache.log4j.Logger;

/**
 * Panel for Observation specific configuration
 *
 * @author  Coolen
 *
 * @created 17-02-2007, 15:07
 *
 * @version $Id$
 */
public class ObservationPanel extends javax.swing.JPanel implements IViewPanel{
    
    static Logger logger = Logger.getLogger(ObservationPanel.class);    
    static String name = "ObservationPanel";
    
   
     /** Creates new form BeanForm based upon aNode
     *
     * @params  aNode   node to obtain the info from
     *
     */
    public ObservationPanel(MainFrame aMainFrame,jOTDBnode aNode) {
        initComponents();
        itsMainFrame = aMainFrame;
        itsNode=aNode;
        initialize();
        initPanel();
    }
    
    /** Creates new form BeanForm */
    public ObservationPanel() {
        initComponents();
        itsMainFrame=null;
        itsNode=null;
        initialize();
    }
    
    @Override
    public void setMainFrame(MainFrame aMainFrame) {
        if (aMainFrame != null) {
            itsMainFrame=aMainFrame;
        } else {
            logger.error("No Mainframe supplied");
        }
    }
    
    @Override
    public String getShortName() {
        return name;
    }
    
    @Override
    public void setContent(Object anObject) {   
        itsNode=(jOTDBnode)anObject;
        if (itsNode != null) {
            try {
                //figure out the caller
                jOTDBtree aTree = OtdbRmi.getRemoteOTDB().getTreeInfo(itsNode.treeID(),false);
                itsTreeType=OtdbRmi.getTreeType().get(aTree.type);
            } catch (RemoteException ex) {
                String aS="ObservationPanel: Error getting treeInfo/treetype" + ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                itsTreeType="";
            }
         } else {
            logger.error("no node given");
            return;
        }
        
        //fire up filling for AntennaConfigPanel
        this.antennaConfigPanel.setMainFrame(this.itsMainFrame);
        this.antennaConfigPanel.setContent(this.itsNode);

        this.campaignInfoPanel.setMainFrame(this.itsMainFrame,false);

        jOTDBparam aParam=null;
        try {
            // get old tree description
            itsOldTreeDescription = OtdbRmi.getRemoteOTDB().getTreeInfo(itsNode.treeID(),false).description;
            inputTreeDescription.setText(itsOldTreeDescription);   
            
            //we need to get all the childs from this node.    
            ArrayList<jOTDBnode> childs = new ArrayList(OtdbRmi.getRemoteMaintenance().getItemList(itsNode.treeID(), itsNode.nodeID(), 1));

            for (jOTDBnode aNode: childs) {
                // get all the params per child
                aParam=null;
                            
                // We need to keep all the nodes needed by this panel
                // if the node is a leaf we need to get the pointed to value via Param.
                if (aNode.leaf) {
                    aParam = OtdbRmi.getRemoteMaintenance().getParam(aNode);
                    setField(itsNode,aParam,aNode);
                //we need to get all the childs from the following nodes as well.
                }else if (LofarUtils.keyName(aNode.name).contains("Beam") && 
                        !LofarUtils.keyName(aNode.name).contains("Beamformer") &&
                        !LofarUtils.keyName(aNode.name).contains("AnaBeam") &&
                        !LofarUtils.keyName(aNode.name).contains("TiedArrayBeam")
                        ) {
                    // Create a new Beam & TiedArrayBeamto add found childs to
                    itsActiveBeam = new Beam();
                    itsActiveTAB = new TiedArrayBeam();
                    itsTABList.clear();
                    itsBeams.add(aNode);
                    this.retrieveAndDisplayChildDataForNode(aNode);
                    // beam childs finished, add found TieadArrayList to Beam and add beam to BeamArrayList
                    itsActiveBeam.setTiedArrayBeams(itsTABList);
                    itsActiveBeam.setTreeType(itsTreeType);
                    // Bug1641 Backwards compatibility
                    if (itsActiveBeam.getMaximizeDuration().equals("")) {
                        itsActiveBeam.setMaximizeDuration("Missing");
                    }
                    beamConfigurationPanel.setColumnSize("maxDur",0);
                    itsBeamList.add(itsActiveBeam);
                }else if (LofarUtils.keyName(aNode.name).contains("AnaBeam")) {
                    // Create a new AnaBeam to add found childs to
                    itsActiveAnaBeam = new AnaBeam();
                    itsAnaBeams.add(aNode);
                    this.retrieveAndDisplayChildDataForNode(aNode);
                    // Bug1641 Backwards compatibility
                    if (itsActiveAnaBeam.getMaximizeDuration().equals("")) {
                        itsActiveAnaBeam.setMaximizeDuration("Missing");
                        anaBeamConfigurationPanel.setColumnSize("maxDur",0);
                     }
                    // AnaBeam childs finished, add to AnaBeamArrayList
                    itsAnaBeamList.add(itsActiveAnaBeam);
                }else if (LofarUtils.keyName(aNode.name).contains("Beamformer")) {
                    itsBeamformers.addElement(aNode);
                    this.retrieveAndDisplayChildDataForNode(aNode);
                } else if (LofarUtils.keyName(aNode.name).equals("VirtualInstrument")) {
                    this.retrieveAndDisplayChildDataForNode(aNode);
                }
            }
        } catch (RemoteException ex) {
            String aS="Error during getComponentParam: "+ ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            return;
        }
        
        
        initPanel();
    }
    @Override
    public boolean isSingleton() {
        return false;
    }
    
    @Override
    public JPanel getInstance() {
        return new ObservationPanel();
    }
    @Override
    public boolean hasPopupMenu() {
        return true;
    }
    
    
    /** create popup menu for this panel
     *
     *  // build up the menu
     *  aPopupMenu= new JPopupMenu();
     *  aMenuItem=new JMenuItem("Choice 1");        
     *  aMenuItem.addActionListener(new java.awt.event.ActionListener() {
     *      public void actionPerformed(java.awt.event.ActionEvent evt) {
     *          popupMenuHandler(evt);
     *      }
     *  });
     *  aMenuItem.setActionCommand("Choice 1");
     *  aPopupMenu.add(aMenuItem);
     *  aPopupMenu.setOpaque(true);
     *
     *
     *  aPopupMenu.show(aComponent, x, y );        
     */
    @Override
    public void createPopupMenu(Component aComponent,int x, int y) {
        JPopupMenu aPopupMenu=null;
        JMenuItem  aMenuItem=null;
        
        aPopupMenu= new JPopupMenu();
        //  Fill in menu as in the example above
        aMenuItem=new JMenuItem("Create ParSet File");        
        aMenuItem.addActionListener(new java.awt.event.ActionListener() {
            @Override
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                popupMenuHandler(evt);
            }
        });
        aMenuItem.setActionCommand("Create ParSet File");
        aPopupMenu.add(aMenuItem);
            
        
        aPopupMenu.setOpaque(true);
        aPopupMenu.show(aComponent, x, y );       
    }
    
    /** handles the choice from the popupmenu 
     *
     * depending on the choices that are possible for this panel perform the action for it
     *
     *      if (evt.getActionCommand().equals("Choice 1")) {
     *          perform action
     *      }  
     */
    @Override
    public void popupMenuHandler(java.awt.event.ActionEvent evt) {
         if (evt.getActionCommand().equals("Create ParSet File")) {
            logger.trace("Create ParSet File");
            int aTreeID=itsMainFrame.getSharedVars().getTreeID();
            if (fc == null) {
                fc = new JFileChooser();
                fc.setApproveButtonText("Save");
            }
            // try to get a new filename to write the parsetfile to
            if (fc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                try {
                    File aFile = fc.getSelectedFile();
                    
                    // create filename that can be used at the remote site    
                    String aRemoteFileName="/tmp/"+aTreeID+"-"+itsNode.name+"_"+itsMainFrame.getUserAccount().getUserName()+".ParSet";
                    
                    // write the parset
                    OtdbRmi.getRemoteMaintenance().exportTree(aTreeID,itsNode.nodeID(),aRemoteFileName,2,false); 
                    
                    //obtain the remote file
                    byte[] dldata = OtdbRmi.getRemoteFileTrans().downloadFile(aRemoteFileName);
                    try (BufferedOutputStream output = new BufferedOutputStream(new FileOutputStream(aFile))) {
                        output.write(dldata,0,dldata.length);
                        output.flush();
                    }
                    logger.trace("File written to: " + aFile.getPath());
                } catch (RemoteException ex) {
                    String aS="exportTree failed : " + ex;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                } catch (FileNotFoundException ex) {
                    String aS="Error during newPICTree creation: "+ ex;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                } catch (IOException ex) {
                    String aS="Error during newPICTree creation: "+ ex;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                }
            }
        }       
    }
    
     /** 
     * Helper method that retrieves the child nodes for a given jOTDBnode, 
     * and triggers setField() accordingly.
     * @param aNode the node to retrieve and display child data of.
     */
    private void retrieveAndDisplayChildDataForNode(jOTDBnode aNode){
        jOTDBparam aParam=null;
        try {
            ArrayList<jOTDBnode> HWchilds = new ArrayList<>(OtdbRmi.getRemoteMaintenance().getItemList(aNode.treeID(), aNode.nodeID(), 1));
            for (jOTDBnode n: HWchilds) {
                aParam=null;
                // We need to keep all the params needed by this panel
                if (n.leaf) {
                    aParam = OtdbRmi.getRemoteMaintenance().getParam(n);
                    setField(aNode,aParam,n);
                } else {
                    if (LofarUtils.keyName(n.name).contains("TiedArrayBeam") ) {
                        // Create a new TiedArrayBeam to add found childs to
                        itsActiveTAB = new TiedArrayBeam();
                        this.retrieveAndDisplayChildDataForNode(n);
                        // tiedArrayBeam childs finished, add to TiedArrayBeamList
                        itsTABList.add(itsActiveTAB);                    
                    }
                }
            }
        } catch (RemoteException ex) {
            String aS="Error during retrieveAndDisplayChildDataForNode: "+ ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            return;
        }
    }
    
    /**
     * Sets the different fields in the GUI, using the names of the nodes provided
     * @param parent the parent node of the node to be displayed
     * @param aParam the parameter of the node to be displayed if applicable
     * @param aNode  the node to be displayed
     */
    private void setField(jOTDBnode parent,jOTDBparam aParam, jOTDBnode aNode) {
        // Generic Observation
        if (aParam==null) {
            return;
        }
        boolean isRef = LofarUtils.isReference(aNode.limits);
        String aKeyName = LofarUtils.keyName(aNode.name);
        String parentName = LofarUtils.keyName(String.valueOf(parent.name));
        /* Set's the different fields in the GUI */

        logger.debug("setField for: "+ aNode.name);
        try {
            if (OtdbRmi.getRemoteTypes().getParamType(aParam.type).substring(0,1).equals("p")) {
               // Have to get new param because we need the unresolved limits field.
               aParam = OtdbRmi.getRemoteMaintenance().getParam(aNode.treeID(),aNode.paramDefID());                
            }
        } catch (RemoteException ex) {
            String aS="Error during getParam: "+ ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
        }
        
        if(parentName.equals("Observation")){        
            // Observation Specific parameters
            switch (aKeyName) {
                case "channelsPerSubband":
                    inputNrChannelsPerSubband.setToolTipText(aParam.description);
                    itsChannelsPerSubband=aNode;
                    if (isRef && aParam != null) {
                        inputNrChannelsPerSubband.setText(aNode.limits + " : " + aParam.limits);
                   } else {
                        inputNrChannelsPerSubband.setText(aNode.limits);
                   }break;
                case "nrSlotsInFrame":
                    inputNrSlotsInFrame.setToolTipText(aParam.description);
                    itsNrSlotsInFrame=aNode;
                    if (isRef && aParam != null) {
                        inputNrSlotsInFrame.setText(aNode.limits + " : " + aParam.limits);
                   } else {
                        inputNrSlotsInFrame.setText(aNode.limits);
                   }break;
                case "nrBeams":
                    itsNrBeams=aNode;
                    break;
                case "nrAnaBeams":
                    itsNrAnaBeams=aNode;
                    break;
                case "nrBeamformers":
                    itsNrBeamformers=aNode;
                    break;
            }
        } else if(parentName.contains("Beam") && !parentName.contains("Beamformer") && !parentName.contains("AnaBeam") && !parentName.contains("TiedArrayBeam")){
            // Observation Beam parameters
            switch (aKeyName) {
                case "target":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setTarget(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setTarget(aNode.limits);
                    }
                    break;
                case "angle1":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setAngle1(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setAngle1(aNode.limits);
                    }
                    itsActiveBeam.setCoordType("rad");
                    break;
                case "angle2":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setAngle2(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setAngle2(aNode.limits);
                    }
                    break;
                case "directionType":
                    itsActiveBeam.setDirectionTypeChoices(aParam.limits);
                    itsActiveBeam.setDirectionType(aNode.limits);
                    break;
                case "duration":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setDuration(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveBeam.setDuration(aNode.limits);
                   }break;
                case "maximizeDuration":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setMaximizeDuration(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveBeam.setMaximizeDuration(aNode.limits);
                   }break;
                case "startTime":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setStartTime(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveBeam.setStartTime(aNode.limits);
                   }break;
                case "beamletList":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setBeamletList(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveBeam.setBeamletList(aNode.limits);
                   }break;
                case "subbandList":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setSubbandList(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setSubbandList(aNode.limits);
                    }
                    break;
                case "momID":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setMomID(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setMomID(aNode.limits);
                    }
                    break;
                case "nrTiedArrayBeams":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setNrTiedArrayBeams(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setNrTiedArrayBeams(aNode.limits);
                    }
                    break;
                case "nrTabRings":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setNrTabRings(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setNrTabRings(aNode.limits);
                    }
                    break;
                case "tabRingSize":
                    if (isRef && aParam != null) {
                        itsActiveBeam.setTabRingSize(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveBeam.setTabRingSize(aNode.limits);
                    }
                    break;
            }
        } else if(parentName.contains("AnaBeam")){
            // Observation Analog Beam parameters
            switch (aKeyName) {
                case "target":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setTarget(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveAnaBeam.setTarget(aNode.limits);
                    }
                    break;
                case "angle1":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setAngle1(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveAnaBeam.setAngle1(aNode.limits);
                    }
                    itsActiveAnaBeam.setCoordType("rad");
                    break;
                case "angle2":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setAngle2(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveAnaBeam.setAngle2(aNode.limits);
                    }
                    break;
                case "directionType":
                    itsActiveAnaBeam.setDirectionTypeChoices(aParam.limits);
                    itsActiveAnaBeam.setDirectionType(aNode.limits);
                    break;
                case "duration":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setDuration(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveAnaBeam.setDuration(aNode.limits);
                   }break;
                case "maximizeDuration":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setMaximizeDuration(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveAnaBeam.setMaximizeDuration(aNode.limits);
                   }break;
                case "startTime":
                    if (isRef && aParam != null) {
                        itsActiveAnaBeam.setStartTime(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveAnaBeam.setStartTime(aNode.limits);
                   }break;
                case "rank":
                    itsActiveAnaBeam.setRankChoices(aParam.limits);
                    itsActiveAnaBeam.setRank(aNode.limits);
                    break;
            }
        } else if(parentName.contains("TiedArrayBeam")){
            // Observation TiedArray Beam parameters
            switch (aKeyName) {
                case "angle1":
                    if (isRef && aParam != null) {
                        itsActiveTAB.setAngle1(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveTAB.setAngle1(aNode.limits);
                    }
                    itsActiveTAB.setCoordType("rad");
                    break;
                case "angle2":
                    if (isRef && aParam != null) {
                        itsActiveTAB.setAngle2(aNode.limits + " : " + aParam.limits);
                    } else {
                        itsActiveTAB.setAngle2(aNode.limits);
                    }
                    break;
                case "directionType":
                    itsActiveTAB.setDirectionTypeChoices(aParam.limits);
                    itsActiveTAB.setDirectionType(aNode.limits);
                    break;
                case "dispersionMeasure":
                    if (isRef && aParam != null) {
                        itsActiveTAB.setDispersionMeasure(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveTAB.setDispersionMeasure(aNode.limits);
                   }break;
                case "coherent":
                    if (isRef && aParam != null) {
                        itsActiveTAB.setCoherent(aNode.limits + " : " + aParam.limits);
                   } else {
                        itsActiveTAB.setCoherent(aNode.limits);
                   }break;
            }
        } else if(parentName.contains("Beamformer")){        
            // Observation Beamformer parameters
            if (aKeyName.equals("stationList")) {        
                if (isRef && aParam != null) {
                    itsStations.add(aNode.limits + " : " + aParam.limits);
                } else {
                    itsStations.add(aNode.limits);
                }
                
                // add found stations to usedStationList
                String[] aList = aNode.limits.split("[,]");
                itsUsedBeamformStations.addAll(Arrays.asList(aList));
            }
        } else if(parentName.equals("VirtualInstrument")){        
            // Observation VirtualInstrument parameters

            if (aKeyName.equals("stationList")) {        
            }
        }
    }

    private void setStationList(jOTDBnode aNode) {
        this.itsStationList = aNode;
        this.stationList.setToolTipText(aNode.description);

        //set the checkbox correctly when no stations are provided in the data
        if(itsStationList.limits == null || itsStationList.limits.equals("[]")){
            stationList.setModel(new DefaultListModel());
        }else{
            TitledBorder aBorder = (TitledBorder)this.stationsPanel.getBorder();
            aBorder.setTitle("Station Names");
            LofarUtils.fillList(stationList,aNode.limits,false);
        }

    }

    /** Fill the Selectable stations for the Beamformer input
     * 
     * Every time the stationList changes we need to fill this list again.
     * while filling we have to check if the removed station are not filled
     * in a beamformer allready. If so show warning and remove the station from
     * that list.
     * Further we need to fill this list only with the station in the station list 
     * as far as they are not allready used in the beamformerlists 
     */
    private void fillBeamformerStationList() {
        
        itsAvailableBeamformStations.clear();
        int size = stationList.getModel().getSize();
        String[] tempList=new String[size];
        DefaultListModel itsModel = new DefaultListModel();
        beamformerStationList.setModel(itsModel);
        // get the  available virtual instrument station list
        for (int i=0; i< size; i++) {
            String station = stationList.getModel().getElementAt(i).toString();
            
            // Check if station is not allready used 
            if (itsUsedBeamformStations.indexOf(station) < 0) {
                tempList[i] = station; 
                itsModel.addElement(station);
                // check if station is not allready there
                if (itsAvailableBeamformStations.indexOf(station)<0){
                    itsAvailableBeamformStations.add(station);
                }
            }
        }
        LofarUtils.sortModel(itsModel);
        if(itsAvailableBeamformStations.isEmpty()) {
            addBeamformerButton.setEnabled(false);
        } else {
            addBeamformerButton.setEnabled(true);
        }
    }
    
    /* add a station to the active beamformer
     */
    void addBeamformerStation(String aStation) {
        // check if a selection in the beamformer table is active.
        if (aStation == null) {
            return;
        }
        int row=beamformerConfigurationPanel.getSelectedRow();
        if (row < 0  || aStation.equals("")) {
            return;
        }
        
        String selection = itsBeamformerConfigurationTableModel.getSelection(row);
        if (selection != null) {
            if (!selection.equals("")){
                selection = selection.concat(",");            
            }
            selection = selection.concat(aStation);
            itsUsedBeamformStations.add(aStation);
            itsAvailableBeamformStations.remove(aStation);
            itsBeamformerConfigurationTableModel.updateRow(selection,row);
            beamformerConfigurationPanel.setSelectedRow(row, row);
            fillBeamformerStationList();
        }
    }

    /** Restore original Values in  panel
     */
    private void restore() {

      setStationList(antennaConfigPanel.getStationList());
      // Observation Specific parameters
      inputNrChannelsPerSubband.setText(itsChannelsPerSubband.limits);
      inputNrSlotsInFrame.setText(itsNrSlotsInFrame.limits);
      inputDescription.setText("");
      inputTreeDescription.setText(itsOldTreeDescription);
    
      if (!itsTreeType.equals("VHtree")) {
         // Observation Beam parameters
         // create original Beamlet Bitset
         fillBeamletBitset();
      }
        // set tables back to initial values
        boolean fillTable = itsBeamConfigurationTableModel.fillTable(itsTreeType,itsBeamList,false);
      itsAnaBeamConfigurationTableModel.fillTable(itsTreeType,itsAnaBeamList,false);
      
      itsBeamformerConfigurationTableModel.fillTable(itsTreeType, itsStations);
      
      for (int i=0; i<itsStations.size();i++){
        String[] involvedStations = itsStations.get(i).split("[,]");
        for (int j = 0; j < involvedStations.length;j++) {
            if (!itsUsedBeamformStations.contains(involvedStations[j])) {
              itsUsedBeamformStations.add(involvedStations[j]);
            }
        }
      }
      
      
      // Observation VirtualInstrument parameters
      //set the checkbox correctly when no stations are provided in the data
      if(itsStationList.limits == null || itsStationList.limits.equals("[]")){
        stationList.setModel(new DefaultListModel());
      }else{
        TitledBorder aBorder = (TitledBorder)this.stationsPanel.getBorder();
        aBorder.setTitle("Station Names");
        LofarUtils.fillList(stationList,itsStationList.limits,false);
      }
      
      fillBeamformerStationList();

      if (beamConfigurationPanel.getTableModel().getRowCount() == 244) {
        this.addBeamButton.setEnabled(false);
      } else {
        this.addBeamButton.setEnabled(true);
      }
     // also restore antennaConfigPanel
      antennaConfigPanel.restore();
    }

    
    /** fill the Beamlet bitset to see what Beamlets have been set. To be able to determine later if a given Beamlet is indeed free.
     */
    private void fillBeamletBitset() {
        itsUsedBeamlets.clear();
        for (Beam b : itsBeamList) {
            BitSet aNewBitSet=b.getBeamletBitSet();
            // check if no duplication between the two bitsets
            if (itsUsedBeamlets.intersects(aNewBitSet)) {
                String errorMsg = "ERROR:  This BeamletList has beamlets defined that are allready used in a prior BeamConfiguration!!!!!  Beam: "+b;
                JOptionPane.showMessageDialog(this,errorMsg,"BeamletError",JOptionPane.ERROR_MESSAGE);
                logger.debug(errorMsg );
                return;
            }
            
            // No intersection, both bitsets can be or
            itsUsedBeamlets.or(aNewBitSet);
        }
    }
    
     
    private void initialize() {
        buttonPanel1.addButton("Restore");
        buttonPanel1.setButtonIcon("Restore",new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_undo.png")));
        buttonPanel1.addButton("Apply");
        buttonPanel1.setButtonIcon("Apply",new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_apply.png")));
        this.stationList.setModel(new DefaultListModel());
        this.beamformerStationList.setModel(new DefaultListModel());

        
      
        itsBeamConfigurationTableModel = new BeamConfigurationTableModel();
        beamConfigurationPanel.setTableModel(itsBeamConfigurationTableModel);
        beamConfigurationPanel.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        beamConfigurationPanel.setColumnSize("dirtype",20);
        beamConfigurationPanel.setColumnSize("angle 1",20);
        beamConfigurationPanel.setColumnSize("angle 2",20);
        beamConfigurationPanel.setColumnSize("coordtype",20);
        beamConfigurationPanel.setColumnSize("maxDur",20);
        beamConfigurationPanel.setColumnSize("#TAB",20);
        beamConfigurationPanel.setColumnSize("subbands",65);
        beamConfigurationPanel.setColumnSize("beamlets",65);
        beamConfigurationPanel.repaint();
        
        itsAnaBeamConfigurationTableModel = new AnaBeamConfigurationTableModel();
        anaBeamConfigurationPanel.setTableModel(itsAnaBeamConfigurationTableModel);
        anaBeamConfigurationPanel.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        anaBeamConfigurationPanel.setColumnSize("dirtype",40);
        anaBeamConfigurationPanel.setColumnSize("angle 1",40);
        anaBeamConfigurationPanel.setColumnSize("angle 2",40);
        anaBeamConfigurationPanel.setColumnSize("coordtype",30);
        anaBeamConfigurationPanel.setColumnSize("maxDur",20);
        anaBeamConfigurationPanel.setColumnSize("rank",30);
        anaBeamConfigurationPanel.repaint();

        itsBeamformerConfigurationTableModel = new BeamformerConfigurationTableModel();
        beamformerConfigurationPanel.setTableModel(itsBeamformerConfigurationTableModel);
        beamformerConfigurationPanel.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        beamformerConfigurationPanel.setColumnSize("Beamformer",20);
        beamformerConfigurationPanel.setColumnSize("Stations",200);
        beamformerConfigurationPanel.repaint();

    }
    
    private void initPanel() {

        itsMainFrame.setHourglassCursor();

        // check access
        UserAccount userAccount = itsMainFrame.getUserAccount();

        // for now:
            setAllEnabled(true);
        
        if(userAccount.isAdministrator()) {
            // enable/disable certain controls
        }
        if(userAccount.isAstronomer()) {
            // enable/disable certain controls
        }
        if(userAccount.isInstrumentScientist()) {
            // enable/disable certain controls
        }
        

         if (itsNode != null) {
            try {
                //figure out the caller
                jOTDBtree aTree = OtdbRmi.getRemoteOTDB().getTreeInfo(itsNode.treeID(),false);
                itsTreeType=OtdbRmi.getTreeType().get(aTree.type);
            } catch (RemoteException ex) {
                String aS="ObservationPanel: Error getting treeInfo/treetype" + ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                itsTreeType="";
            }
         } else {
            logger.error("no node given");
        }
        
        // set defaults
        // create initial beamletBitset
        // create initial table
        restore();
        
        for (int i=0; i < stationList.getModel().getSize();i++) {
          if (! itsUsedBeamformStations.contains(stationList.getModel().getElementAt(i).toString()) &&
                  itsAvailableBeamformStations.indexOf(stationList.getModel().getElementAt(i).toString())< 0) {
              itsAvailableBeamformStations.add(stationList.getModel().getElementAt(i).toString());
          }
        }

        
        if (itsTreeType.equals("VHtree")) {
            this.setButtonsVisible(false);
            this.setAllEnabled(false);
        }

        itsMainFrame.setNormalCursor();

    }
        
    /** saves the given node back to the database
     */
    private void saveNode(jOTDBnode aNode) {
        if (aNode == null) {
            return;
        }
        try {
            OtdbRmi.getRemoteMaintenance().saveNode(aNode); 
        } catch (RemoteException ex) {
            String aS="Error: saveNode failed : " + ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
        } 
    }
    
    /** Enables/disables the buttons
     *
     * @param   enabled     true/false enabled/disabled
     */
    public void enableButtons(boolean enabled) {
        loadBeamsButton.setEnabled(enabled);
        loadAnaBeamsButton.setEnabled(enabled);
        addBeamButton.setEnabled(enabled);
        editBeamButton.setEnabled(enabled);
        copyBeamButton.setEnabled(enabled);
        loadAnaBeamsButton.setEnabled(enabled);
        loadBeamsButton.setEnabled(enabled);
        deleteBeamButton.setEnabled(enabled);
        addAnaBeamButton.setEnabled(enabled);
        editAnaBeamButton.setEnabled(enabled);
        deleteAnaBeamButton.setEnabled(enabled);
    }
    
    /** Sets the buttons visible/invisible
     *
     * @param   visible     true/false visible/invisible
     */
    @Override
    public void setButtonsVisible(boolean visible) {
        loadBeamsButton.setVisible(visible);
        loadAnaBeamsButton.setVisible(visible);
        addBeamButton.setVisible(visible);
        editBeamButton.setVisible(visible);
        copyBeamButton.setVisible(visible);
        loadAnaBeamsButton.setVisible(visible);
        loadBeamsButton.setVisible(visible);
        deleteBeamButton.setVisible(visible);
        addAnaBeamButton.setVisible(visible);
        editAnaBeamButton.setVisible(visible);
        deleteAnaBeamButton.setVisible(visible);
    }        
    
    /** Enables/disables the complete form
     *
     * @param   enabled     true/false enabled/disabled
     */
    @Override
    public void setAllEnabled(boolean enabled) {
        this.inputDescription.setEnabled(enabled);
        this.inputNrChannelsPerSubband.setEnabled(enabled);
        this.inputTreeDescription.setEnabled(enabled);
    }
    
    private boolean saveInput() {
        // Digital Beam
        // keep default Beams
        boolean keep = true;
        jOTDBnode aDefaultBNode = null ;
        if (itsBeamConfigurationTableModel.changed()) {
           try {
              for (jOTDBnode n: itsBeams) {
                    if (keep) {
                        aDefaultBNode = n;
                        keep = false;
                    } else {
                        OtdbRmi.getRemoteMaintenance().deleteNode(n);
                    }
                }
           } catch (RemoteException ex) {
                String aS="Error during deletion beam node: "+ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                return false;
            }
        
         

            // now that all Nodes are deleted we should collect the tables input and create new Beams to save to the database.
            itsBeamList.clear();
            itsBeams.clear();
            // add the default node again
            itsBeams.add(aDefaultBNode);
            itsBeams.trimToSize();
            itsBeamList = itsBeamConfigurationTableModel.getTable();
            itsBeamList.trimToSize();
            short index=0;
            try {
               // for all elements except the first (default) one
                keep = true;
                for (Beam b : itsBeamList) {
                    if (keep) {
                        keep = false;
                        continue;
                    }    

                    // make a dupnode from the default node, give it the next number in the count,get the elements and fill all values from the elements
                    // with the values from the set fields and save the elements again
                   //
                    // Duplicates the given node (and its parameters and children)
                    int aN = OtdbRmi.getRemoteMaintenance().dupNode(itsNode.treeID(),aDefaultBNode.nodeID(),index++);
                    if (aN <= 0) {
                        String aS="Something went wrong with dupNode("+itsNode.treeID()+","+aDefaultBNode.nodeID()+") will try to save remainder";
                        logger.error(aS);
                        LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    } else {
                        // we got a new duplicate whos children need to be filled with the settings from the panel.
                        jOTDBnode aNode = OtdbRmi.getRemoteMaintenance().getNode(itsNode.treeID(),aN);
                        ArrayList<jOTDBnode> HWchilds = new ArrayList<>(OtdbRmi.getRemoteMaintenance().getItemList(aNode.treeID(), aNode.nodeID(), 1));
                        // get all the params per child
                        for (jOTDBnode n: HWchilds) {
                            String aKeyName = LofarUtils.keyName(n.name);
                            if (!n.leaf) {
                                if (LofarUtils.keyName(n.name).contains("TiedArrayBeam") ) {
                                    
                                    // for all TiedArrays except the first (default) one
                                    keep = true;
                                    short idx=0;
                                    for (TiedArrayBeam tab : b.getTiedArrayBeams()) {
                                        if (keep) {
                                            keep = false;
                                            continue;
                                        }    

                                        // Duplicates the given node (and its parameters and children)
                                        int aTabN = OtdbRmi.getRemoteMaintenance().dupNode(itsNode.treeID(),n.nodeID(),idx++);
                                        if (aTabN <= 0) {
                                            String aS="Something went wrong with dupNode("+itsNode.treeID()+","+n.nodeID()+") will try to save remainder";
                                            logger.error(aS);
                                            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                                        } else {
                                            // we got a new duplicate whos children need to be filled with the settings from the panel.
                                            jOTDBnode aTabNode = OtdbRmi.getRemoteMaintenance().getNode(itsNode.treeID(),aTabN);
                                            ArrayList<jOTDBnode> tabHWchilds = new ArrayList<>(OtdbRmi.getRemoteMaintenance().getItemList(aTabNode.treeID(), aTabNode.nodeID(), 1));
                                            for (jOTDBnode tabn: tabHWchilds) {
                                                String aTabKeyName = LofarUtils.keyName(tabn.name);
                                                switch (aTabKeyName) {
                                
                                                    case "directionType" :
                                                        tabn.limits=tab.getDirectionType();
                                                        break;
                                                    case "angle1" :
                                                        String aVal1=tab.getAngle1();
                                                    if (!tab.getCoordType().equals("rad") ) {
                                                        String tmp=tab.getCoordType();
                                                        switch (tmp) {
                                                            case "hmsdms":
                                                                tmp="hms";
                                                                break;
                                                            case "dmsdms":
                                                                tmp="dms";
                                                                break;
                                                            }
                                                            aVal1=LofarUtils.changeCoordinate(tmp, "rad", aVal1);
                                                        }
                                                        tabn.limits=aVal1;
                                                        break;
                                                    case "angle2" :
                                                        String aVal2=tab.getAngle2();
                                                    if (!tab.getCoordType().equals("rad") ) {
                                                        String tmp=tab.getCoordType();
                                                        switch (tmp) {
                                                            case "hmsdms":
                                                                tmp="hms";
                                                                break;
                                                            case "dmsdms":
                                                                tmp="dms";
                                                                break;
                                                            }
                                                            aVal2=LofarUtils.changeCoordinate(tmp, "rad", aVal2);
                                                        }
                                                        tabn.limits=aVal2;
                                                        break;
                                                    case "dispersionmeasure" :
                                                        tabn.limits=tab.getDispersionMeasure();
                                                        break;
                                                    case "coherent" :
                                                        tabn.limits=tab.getCoherent();
                                                        break;
                                                }
                                                saveNode(tabn);
                                            }
                                        }
                                    }
                                    // store new number of instances in baseSetting
                                    short tabbeams= (short)(b.getTiedArrayBeams().size()-1);
                                    n.instances = tabbeams; //
                                    saveNode(n);
                                }
                                continue;
                            }
                            // for leafs:
                            switch (aKeyName) {
                                
                                case "directionType" :
                                    n.limits=b.getDirectionType();
                                    break;
                                case "target" :
                                    n.limits=b.getTarget();
                                    break;
                                case "angle1" :
                                    String aVal1=b.getAngle1();
                                    if (!b.getCoordType().equals("rad") ) {
                                        String tmp=b.getCoordType();
                                        switch (tmp) {
                                        case "hmsdms":
                                            tmp="hms";
                                            break;
                                        case "dmsdms":
                                            tmp="dms";
                                            break;
                                        }
                                        aVal1=LofarUtils.changeCoordinate(tmp, "rad", aVal1);
                                    }
                                    n.limits=aVal1;
                                    break;
                                case "angle2" :
                                    String aVal2=b.getAngle2();
                                    if (!b.getCoordType().equals("rad") ) {
                                        String tmp=b.getCoordType();
                                        if (tmp.equals("hmsdms") || tmp.equals("dmsdms")) {
                                          tmp="dms";
                                        }
                                        aVal2=LofarUtils.changeCoordinate(tmp, "rad", aVal2);
                                    }
                                    n.limits=aVal2;
                                    break;
                                case "duration" :
                                    n.limits=b.getDuration();
                                    break;
                                case "maximizeDuration" :
                                    n.limits=b.getMaximizeDuration();
                                    break;
                                case "startTime" :
                                    n.limits=b.getStartTime();
                                    break;
                                case "subbandList" :
                                    n.limits=b.getSubbandList();
                                    break;
                                case "beamletList" :
                                    n.limits=b.getBeamletList();
                                    break;
                                case "momID" :
                                    n.limits=b.getMomID();
                                    break;
                                case "nrTabRings" :
                                    n.limits=b.getNrTabRings();
                                    break;
                                case "tabRingSize" :
                                    n.limits=b.getTabRingSize();
                                    break;
                                case "nrTiedArrayBeams" :
                                    n.limits=b.getNrTiedArrayBeams();
                                    break;
                                default :
                                    String aS="Wrong key found during duplication and save: " + aKeyName;
                                    logger.error(aS);
                                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                            }
                            saveNode(n);
                        }
                        // store new node in itsBeams.
                        itsBeams.add(aNode);
                    }
                }


            } catch (RemoteException ex) {
                String aS="Error during duplication and save : " + ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                return false;
            }

        }
        // store new number of instances in baseSetting
        short beams= (short)(itsBeams.size()-1);
        itsBeams.get(0).instances = beams; //
        saveNode(itsBeams.get(0));

        //   =====================   AnaBeams 
        
        
        // same for Analog Beams
        // delete all Analog Beams from the table (excluding the Default one);
        // Keep the 1st one, it's the default Analog Beam
        jOTDBnode aDefaultABNode = null;
        
        // save anabeams if changes, or delete all if anaBeamConfiguration isn't visible (LBA mode), keep the 1st one , its the defaultnode
        if (itsAnaBeamConfigurationTableModel.changed() || (!anaBeamConfiguration.isVisible())) {
            try {
                keep = true;
                for (jOTDBnode n : itsAnaBeams) {
                    if (keep) {
                        aDefaultABNode = n;
                        keep = false;
                    } else {
                        OtdbRmi.getRemoteMaintenance().deleteNode(n);
                    }
                }
            } catch (RemoteException ex) {
                String aS="Error during deletion of default analog beam node: "+ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                return false;
            }

            // now that all Nodes are deleted we should collect the tables input and create new AnaBeams to save to the database.
            itsAnaBeamList.clear();
            itsAnaBeams.clear();
            itsAnaBeams.trimToSize();
            // however, if anaBeamConfiguration is invisible, we can skip this and set nrAnabeams to 0

            if (anaBeamConfiguration.isVisible() && aDefaultABNode != null) {
                // add the default node again
                itsAnaBeams.add(aDefaultABNode);
                itsAnaBeamList = itsAnaBeamConfigurationTableModel.getTable();
                short index=0;
                try {
                    // for all elements except the default one
                    keep = true;
                    for (AnaBeam b : itsAnaBeamList) {
                        if (keep) {
                            keep = false;
                            continue;
                        }    
                        

                        // make a dupnode from the default node, give it the next number in the count,get the elements and fill all values from the elements
                        // with the values from the set fields and save the elements again
                        //
                        // Duplicates the given node (and its parameters and children)
                        int aN = OtdbRmi.getRemoteMaintenance().dupNode(itsNode.treeID(),aDefaultABNode.nodeID(),index++);
                        if (aN <= 0) {
                            String aS="Something went wrong with dupNode("+itsNode.treeID()+","+aDefaultABNode.nodeID()+") will try to save remainder";
                            logger.error(aS);
                            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                        } else {
                            // we got a new duplicate whos children need to be filled with the settings from the panel.
                            jOTDBnode aNode = OtdbRmi.getRemoteMaintenance().getNode(itsNode.treeID(),aN);

                            ArrayList<jOTDBnode> HWchilds = new ArrayList<>(OtdbRmi.getRemoteMaintenance().getItemList(aNode.treeID(), aNode.nodeID(), 1));
                            for (jOTDBnode n: HWchilds) {
                                String aKeyName = LofarUtils.keyName(n.name);
                                switch (aKeyName) {
                                    case "directionType" :
                                        n.limits=b.getDirectionType();
                                        break;
                                    case "target" :
                                        n.limits=b.getTarget();
                                        break;
                                    case "angle1" :
                                        String aVal1=b.getAngle1();
                                        if (!b.getCoordType().equals("rad") ) {
                                            String tmp=b.getCoordType();
                                            switch (tmp) {
                                            case "hmsdms":
                                                tmp="hms";
                                                break;
                                            case "dmsdms":
                                                tmp="dms";
                                                break;
                                            }
                                            aVal1=LofarUtils.changeCoordinate(tmp, "rad", aVal1);
                                        }
                                        n.limits=aVal1;
                                        break;
                                    case "angle2" :
                                        String aVal2=b.getAngle2();
                                        if (!b.getCoordType().equals("rad") ) {
                                            String tmp=b.getCoordType();
                                            if (tmp.equals("hmsdms") || tmp.equals("dmsdms")) {
                                                tmp="dms";
                                            }
                                            aVal2=LofarUtils.changeCoordinate(tmp, "rad", aVal2);
                                        }
                                        n.limits=aVal2;
                                        break;
                                    case "duration" :
                                        n.limits=b.getDuration();
                                        break;
                                    case "maximizeDuration" :
                                        n.limits=b.getMaximizeDuration();
                                        break;
                                    case "startTime" :
                                        n.limits=b.getStartTime();
                                        break;
                                    case "rank" :
                                        n.limits=b.getRank();
                                        break;
                                    }
                                    saveNode(n);
                                }
                                itsAnaBeams.add(aNode);
                            }
                        }

                } catch (RemoteException ex) {
                    String aS="Error during duplication and save : " + ex;
                  logger.error(aS);
                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    return false;
                }
                
                // store new number of instances in baseSetting
                short anaBeams= (short)(itsAnaBeams.size()-1);
                itsAnaBeams.get(0).instances = anaBeams; //
                saveNode(itsAnaBeams.get(0));
            } else {  
                if (itsAnaBeams != null  && itsAnaBeams.size() > 0) {
                    // Anabeams not visible, set to 0 instances
                    short anaBeams= (short)(0);
                    itsAnaBeams.get(0).instances = anaBeams; //
                    saveNode(itsAnaBeams.get(0));
                }
            }
        }




        jOTDBnode aDefaultBFNode= itsBeamformers.elementAt(0);
        // validate table
        // same for beamformer
        if (itsBeamformerConfigurationTableModel.changed()) {
            int i=0;
            //delete all Beamformers from the table (excluding the Default one);
        
            // Keep the 1st one, it's the default Beam
            try {
                for (i=1; i< itsBeamformers.size(); i++) {
                    OtdbRmi.getRemoteMaintenance().deleteNode(itsBeamformers.elementAt(i));
                }
            } catch (RemoteException ex) {
                String aS="Error during deletion of defaultNode: "+ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                return false;
            }
            boolean succes = itsBeamformerConfigurationTableModel.getTable(itsStations);
            // keep default save
            for (int j=0; j< itsStations.size(); j++) {
                if (itsStations.get(j).equals("")) {
                    itsStations.remove(j);
                }
            }
            try {
                // for all elements
                for (i=1; i < itsStations.size();i++) {
        
                    // make a dupnode from the default node, give it the next number in the count,get the elements and fill all values from the elements
                    // with the values from the set fields and save the elements again
                    //
                    // Duplicates the given node (and its parameters and children)
                    int aN = OtdbRmi.getRemoteMaintenance().dupNode(itsNode.treeID(),aDefaultBFNode.nodeID(),(short)(i-1));
                    if (aN <= 0) {
                        String aS="Something went wrong with dupNode("+itsNode.treeID()+","+aDefaultBFNode.nodeID()+") will try to save remainder";
                        logger.error(aS);
                        LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    } else {
                        // we got a new duplicate whos children need to be filled with the settings from the panel.
                        jOTDBnode aNode = OtdbRmi.getRemoteMaintenance().getNode(itsNode.treeID(),aN);
                        // store new duplicate in itsBeamformers.
                        itsBeamformers.add(aNode);

                        ArrayList<jOTDBnode> HWchilds = new ArrayList<>(OtdbRmi.getRemoteMaintenance().getItemList(aNode.treeID(), aNode.nodeID(), 1));
                        for (jOTDBnode n: HWchilds) {
                            String aKeyName = LofarUtils.keyName(n.name);
                            switch (aKeyName) {
                                case "stationList" :
                                    n.limits=itsStations.get(i);
                                    break;
                            }
                            saveNode(n);
                        }
                    }
                }
            

            } catch (RemoteException ex) {
                String aS="Error during duplication and save : " + ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                return false;
            }
        }
        // store new number of instances in baseSetting
        short bforms= (short)(itsStations.size()-1);
        aDefaultBFNode.instances=bforms;
        saveNode(aDefaultBFNode);


        
        // Generic Observation
        if (itsChannelsPerSubband != null && !inputNrChannelsPerSubband.getText().equals(itsChannelsPerSubband.limits)) {
            itsChannelsPerSubband.limits = inputNrChannelsPerSubband.getText();
            saveNode(itsChannelsPerSubband);
        }
        if (itsNrSlotsInFrame != null && !inputNrSlotsInFrame.getText().equals(itsNrSlotsInFrame.limits)) {
            itsNrSlotsInFrame.limits = inputNrSlotsInFrame.getText();
            saveNode(itsNrSlotsInFrame);
        }
        if (itsNrBeams != null && !Integer.toString(beamConfigurationPanel.getTableModel().getRowCount()).equals(itsNrBeams.limits)) {
            itsNrBeams.limits = Integer.toString(beamConfigurationPanel.getTableModel().getRowCount());
            saveNode(itsNrBeams);
        }

        if ((itsNrAnaBeams != null && !Integer.toString(anaBeamConfigurationPanel.getTableModel().getRowCount()).equals(itsNrAnaBeams.limits))
                || ! anaBeamConfiguration.isVisible()) {
            if (anaBeamConfiguration.isVisible()) {
                itsNrAnaBeams.limits = Integer.toString(anaBeamConfigurationPanel.getTableModel().getRowCount());
            } else {
                itsNrAnaBeams.limits="0";
            }
            saveNode(itsNrAnaBeams);
        }
        
        if (itsNrBeamformers != null && !Integer.toString(beamformerConfigurationPanel.getTableModel().getRowCount()).equals(itsNrBeamformers.limits)) {
            itsNrBeamformers.limits = Integer.toString(beamformerConfigurationPanel.getTableModel().getRowCount());
            saveNode(itsNrBeamformers);
        }
        // treeDescription
        if (itsOldTreeDescription != null && !inputTreeDescription.getText().equals(itsOldTreeDescription)) {
            try {
                if (!OtdbRmi.getRemoteMaintenance().setDescription(itsNode.treeID(), inputTreeDescription.getText())) {
                    String aS="Error during setDescription: "+OtdbRmi.getRemoteMaintenance().errorMsg();
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                } 
            } catch (RemoteException ex) {
                String aS="Error: saveNode failed : " + ex;
                logger.error(aS);
                LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            } 
            itsOldTreeDescription = inputTreeDescription.getText();
        }
        itsMainFrame.setChanged("Home",true);
        itsMainFrame.setChanged("Template_Maintenance("+itsNode.treeID()+")" ,true);
        itsMainFrame.checkChanged("Template_Maintenance("+itsNode.treeID()+")");


        return true;   
    }
    
    private void changeDescription(jOTDBnode aNode) {
        if (aNode == null) {
            return;
        }
        try {
            jOTDBparam aParam = OtdbRmi.getRemoteMaintenance().getParam(aNode);
            // check if the node changed, and if the description was changed, if so ask if the new description
            // should be saved.
            if (itsOldDescriptionParam == null | itsOldDescriptionParam !=aParam) {
                if (itsOldDescriptionParam != null) {
                    if (!inputDescription.getText().equals(itsOldDescriptionParam.description) && !inputDescription.getText().equals("")) {
                        int answer=JOptionPane.showConfirmDialog(this,"The old description was altered, do you want to save the old one ?","alert",JOptionPane.YES_NO_OPTION);
                        if (answer == JOptionPane.YES_OPTION) {
                            if (!OtdbRmi.getRemoteMaintenance().saveParam(itsOldDescriptionParam)) {
                                logger.error("Saving param "+itsOldDescriptionParam.nodeID()+","+itsOldDescriptionParam.paramID()+"failed: "+ OtdbRmi.getRemoteMaintenance().errorMsg());
                            }                          
                        }
                    }
                }
            }
            itsOldDescriptionParam=aParam;
            inputDescription.setText(aParam.description);
        } catch (RemoteException ex) {
            String aS="Error during getParam: "+ ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            return;
        }
    }

    private void deleteBeamformer() {
        String selection = itsBeamformerConfigurationTableModel.getSelection(beamformerConfigurationPanel.getSelectedRow());
        if (selection== null) {
            return;
        }
        String[] involvedStations = selection.split("[,]");
        for (int j = 0; j < involvedStations.length;j++) {
            itsUsedBeamformStations.remove(involvedStations[j]);
        }
        itsBeamformerConfigurationTableModel.removeRow(beamformerConfigurationPanel.getSelectedRow());
        ArrayList<String> sl=new ArrayList<>();
        itsBeamformerConfigurationTableModel.getTable(sl);
        itsBeamformerConfigurationTableModel.fillTable(itsTreeType, sl);
        fillBeamformerStationList();
    }

    private void deleteBeam() {
        int row = beamConfigurationPanel.getSelectedRow();
        if (JOptionPane.showConfirmDialog(this,"Are you sure you want to delete this Beam ?","Delete Beam",JOptionPane.YES_NO_OPTION,JOptionPane.WARNING_MESSAGE) == JOptionPane.YES_OPTION ) {
            // if removed then the old Beamlets's should be removed form the checklist also
            BitSet beamletSet = itsBeamConfigurationTableModel.getSelection(row).getBeamletBitSet();
            if (row > -1) {
                itsBeamConfigurationTableModel.removeRow(row);
                itsUsedBeamlets.xor(beamletSet);
                // No selection anymore after delete, so buttons disabled again
                this.editBeamButton.setEnabled(false);
                this.deleteBeamButton.setEnabled(false);
                this.copyBeamButton.setEnabled(false);


            }
        } 
        
      if (beamConfigurationPanel.getTableModel().getRowCount() == 244) {
        this.addBeamButton.setEnabled(false);
      } else {
        this.addBeamButton.setEnabled(true);
      }
    }
    
    private void deleteAnaBeam() {
        int row = anaBeamConfigurationPanel.getSelectedRow();

        if (JOptionPane.showConfirmDialog(this,"Are you sure you want to delete this Analog Beam ?","Delete Analog Beam",JOptionPane.YES_NO_OPTION,JOptionPane.WARNING_MESSAGE) == JOptionPane.YES_OPTION ) {
            if (row > -1) {
                itsAnaBeamConfigurationTableModel.removeRow(row);
                // No selection anymore after delete, so buttons disabled again
                this.editAnaBeamButton.setEnabled(false);
                this.deleteAnaBeamButton.setEnabled(false);


            }
        }

      if (anaBeamConfigurationPanel.getTableModel().getRowCount() == 1) {
        this.addAnaBeamButton.setEnabled(false);
      } else {
        this.addAnaBeamButton.setEnabled(true);
      }
    }

    private void addBeam() {
     
        BitSet aBS=itsUsedBeamlets;
        itsSelectedRow=-1;
        itsSelectedRow = beamConfigurationPanel.getSelectedRow();
        // set selection to defaults.
        Beam selection = itsBeamList.get(0);
        if (editBeam) {
            selection = itsBeamConfigurationTableModel.getSelection(itsSelectedRow);
                       
            BitSet oldBeamlets = selection.getBeamletBitSet();
            aBS.xor(oldBeamlets);
        }
        beamDialog = new BeamDialog(itsMainFrame,itsTreeType,true,aBS,selection.clone(),editBeam);
        
        beamDialog.setLocationRelativeTo(this);
        if (editBeam) {
            beamDialog.setBorderTitle("edit Beam");
        } else {
            beamDialog.setBorderTitle("add new Beam");            
        }
        beamDialog.setVisible(true);
        
        // check if something has changed 
        if (beamDialog.hasChanged()) {
            Beam newBeam = beamDialog.getBeam();
            itsUsedBeamlets=beamDialog.getBeamletList();
            // check if we are editting an entry or adding a new entry
            if (editBeam) {
                itsBeamConfigurationTableModel.updateRow(newBeam,itsSelectedRow);
                // set editting = false
                editBeam=false;
            } else {            
                boolean succes = itsBeamConfigurationTableModel.addRow(newBeam);
            }
        }
        beamDialog.dispose();
        
        this.editBeamButton.setEnabled(false);
        this.deleteBeamButton.setEnabled(false);
        this.copyBeamButton.setEnabled(false);
        if (beamConfigurationPanel.getTableModel().getRowCount() == 244 ) {
            this.addBeamButton.setEnabled(false);
        } else {
            this.addBeamButton.setEnabled(true);
        }
        
    }

    private void copyBeamToAnaBeam() {

        itsSelectedRow=-1;
        // set selection to defaults.
        AnaBeam defaultAnaBeam = itsAnaBeamList.get(0);

        itsSelectedRow = beamConfigurationPanel.getSelectedRow();
        if (itsSelectedRow < 0 ) return;
        Beam selection = itsBeamConfigurationTableModel.getSelection(itsSelectedRow);
        
        
        // set matching beam pars to anabeam
        defaultAnaBeam.setTarget(selection.getTarget());
        defaultAnaBeam.setAngle1(selection.getAngle1());
        defaultAnaBeam.setAngle2(selection.getAngle2());
        defaultAnaBeam.setCoordType(selection.getCoordType());
        defaultAnaBeam.setDuration(selection.getDuration());
        defaultAnaBeam.setMaximizeDuration(selection.getMaximizeDuration());
        defaultAnaBeam.setStartTime(selection.getStartTime());
        // Rank default to 1 in this case
        defaultAnaBeam.setRank("1");
        boolean succes = itsAnaBeamConfigurationTableModel.addRow(defaultAnaBeam);
    }

    private void addAnaBeam() {

        itsSelectedRow=-1;
        // set selection to defaults.
        AnaBeam selection = itsAnaBeamList.get(0);
        if (editAnaBeam) {
            itsSelectedRow = anaBeamConfigurationPanel.getSelectedRow();
            if (itsSelectedRow < 0) return;
            selection = itsAnaBeamConfigurationTableModel.getSelection(itsSelectedRow);
        }
        anaBeamDialog = new AnaBeamDialog(itsMainFrame,true,selection.clone(),editAnaBeam);
        anaBeamDialog.setLocationRelativeTo(this);
        if (editAnaBeam) {
            anaBeamDialog.setBorderTitle("edit Beam");
        } else {
            anaBeamDialog.setBorderTitle("add new Beam");
        }
        anaBeamDialog.setVisible(true);

        // check if something has changed
        if (anaBeamDialog.hasChanged()) {
            AnaBeam newBeam = anaBeamDialog.getAnaBeam();
            // check if we are editting an entry or adding a new entry
            if (editAnaBeam) {
                itsAnaBeamConfigurationTableModel.updateRow(newBeam,itsSelectedRow);
                // set editting = false
                editAnaBeam=false;
            } else {
                itsAnaBeamConfigurationTableModel.addRow(newBeam);
            }
        }

        this.editAnaBeamButton.setEnabled(false);
        this.deleteAnaBeamButton.setEnabled(false);
        if (anaBeamConfigurationPanel.getTableModel().getRowCount() == 1 ) {
            this.addAnaBeamButton.setEnabled(false);
        } else {
            this.addAnaBeamButton.setEnabled(true);
        }
    }

    private void loadBeamFile(String choice) {
        // Can read files with contents like:
        // #
        // beam0.target=test
        // beam0.angle1=1
        // beam0.angle2=2
        // beam0.coordType=rad
        // beam0.directionType=AZEL
        // beam0.duration=300
        // beam0.maximizeDuration=true
        // beam0.subbandList=[1,2,3,4,5]
        // beam0.beamletList[1,2,3,4,5]
        // beam0.tiedarraybeam0.angle1 = 1
        // #
        // beam1.target=test2
        // beam1.angle1=3
        // beam1.angle2=4
        // beam0.coordType=rad
        // beam1.directionType=LMN
        // beam1.duration=360
        // beam1.subbandList=[6..10]
        // beam1.beamletList[6..10]
        //
        if (choice.equals("AnaBeams") || choice.equals("Beams")) {
            File aNewFile=null;

            // show login dialog
            if (beamFileDialog == null ) {
                beamFileDialog = new BeamFileDialog(itsMainFrame,true,choice);
            } else {
                beamFileDialog.setType(choice);
            }
            beamFileDialog.setLocationRelativeTo(this);
            beamFileDialog.setVisible(true);
            if(beamFileDialog.isOk()) {
                aNewFile = beamFileDialog.getFile();
            } else {
                logger.debug("No File chosen");
                return;
            }
            if (aNewFile != null && aNewFile.exists()) {
                logger.info("File to load: " + aNewFile.getName());

                // read the file and split input into beam or anabeam components
                readFile(aNewFile,choice);
            }
        } else {
            String aS="Error: Wrong filetype chosen: "+ choice;
            logger.error(aS);
            LofarUtils.showErrorPanel(this,aS,new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            return;
        }

    }

    private void readFile(File aNewFile, String choice) {
        logger.trace("File to read: " + aNewFile.getPath());

        ArrayList<Beam> readBeams = new ArrayList<>();
        ArrayList<AnaBeam> readAnaBeams = new ArrayList<>();
        ArrayList<TiedArrayBeam> readTABs = new ArrayList<>();
        

        //line to read filelines into
        String inLine = "";
        String line = "";
        BufferedReader in = null;
        int oldIdx = -1;
        Boolean foundTAB = false;
        int tabIdx = -1;
        
        try (FileReader f = new FileReader(aNewFile.getPath())) {
            in = new BufferedReader(f);
            while ((inLine = in.readLine()) != null) {

                if (inLine == null || inLine.startsWith("#") || inLine.isEmpty()) {
                    continue;
                }
                line = inLine.toLowerCase();
                // split into key value
                String[] keyVal = line.split("[=]");
                if (keyVal == null || keyVal.length < 2 || keyVal[0].isEmpty() || keyVal[1].isEmpty()) {
                    String aS = "Error in input: " + line;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    continue;
                }

                // split in (ana)beam# and key
                String[] beamKey = keyVal[0].split("[.]");
                String[] TABkey = new String[2];

                if (beamKey == null || beamKey.length < 2 || beamKey[0].isEmpty() || beamKey[1].isEmpty()) {
                    String aS = "Error in input: " + line;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    continue;
                }

                // tiedArrayBeam have also a beam in the remaining beamKey
                if (beamKey[0].contains(".")) {
                    String[] aS = beamKey[0].split("[.]");
                    TABkey[0] = aS[0];
                    TABkey[1] = beamKey[1];
                    beamKey[0] = aS[0];
                    foundTAB = true;
                    tabIdx = Integer.parseInt(beamKey[0].toLowerCase().replace("tiedarraybeam", ""));
                } else {
                    foundTAB = false;
                }
                // check index and if not in list, add new default Beam or AnaBeam to all lists
                String check = "beam";
                if (choice.equals("AnaBeams")) {
                    check = "anabeam";
                }
                int idx = Integer.parseInt(beamKey[0].replace(check, ""));
                if (idx != oldIdx) {
                    oldIdx = idx;
                    if (foundTAB) {
                        // since this is a new beam we also need a an empty TAB array
                        // but write the total nr of tiedArrayBeams to the old Beam first
                        readBeams.get(idx+1).setNrTiedArrayBeams(Integer.toString(readTABs.size()-1));
                        readTABs.clear();
                    }
                }
                if (check.equals("beam") && (idx < 0 || idx > 243)) {
                    String aS = "Error in index, only beam 0-243 allowed : " + idx;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    continue;
                } else if (check.equals("anabeam") && idx != 0) {
                    String aS = "Error in index, only anabeam 0 allowed : " + idx;
                    logger.error(aS);
                    LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                    continue;
                } 

                // set Defaults
                switch (check) {
                    case "beam" :
                        while (readBeams.size() < idx + 2) {
                            readBeams.add(itsBeamList.get(0));
                        }
                        if (foundTAB) {
                            while (readTABs.size() < tabIdx + 2) {
                                readTABs.add(itsBeamList.get(0).getTiedArrayBeams().get(0));
                            }                            
                        }
                        break;
                    case "anabeam" :
                        while (readAnaBeams.size() < idx + 2) {
                            readAnaBeams.add(itsAnaBeamList.get(0));
                        }
                        break;
                }
                
                if (foundTAB) {
                    check = "tiedarraybeam";
                }


                // add new keyval
                // beams start with 0
                switch (beamKey[1]) {
                    case "target" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setTarget(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setTarget(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "angle1" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setAngle1(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setAngle1(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setAngle1(keyVal[1]);
                                break;
                        }
                        break;
                    case "angle2" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setAngle2(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setAngle2(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setAngle2(keyVal[1]);
                                break;
                        }
                        break;
                    case "coordtype" :
                        if (keyVal[1].toLowerCase().equals("rad") || keyVal[1].toLowerCase().equals("deg") || keyVal[1].toLowerCase().equals("hmsdms")
                                ||keyVal[1].toLowerCase().equals("dmsdms") ) {
                            switch (check) {
                                case "beam" :
                                    readBeams.get(idx+1).setCoordType(keyVal[1]);
                                    break;
                                case "anabeam" :
                                    readAnaBeams.get(idx+1).setCoordType(keyVal[1]);
                                    break;
                                case "tiedarraybeam" :
                                    readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setCoordType(keyVal[1]);
                                    break;
                            }
                        } else {
                            String aS = "Error in directionType value : " + line;
                            logger.error(aS);
                            LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                        }
                        break;
                    case "directiontype" :
                        if (keyVal[1].equals("J2000") || keyVal[1].equals("AZEL") || keyVal[1].equals("LMN")) {
                            switch (check) {
                                case "beam" :
                                    readBeams.get(idx+1).setDirectionType(keyVal[1]);
                                    break;
                                case "anabeam" :
                                    readAnaBeams.get(idx+1).setDirectionType(keyVal[1]);
                                    break;
                                case "tiedarraybeam" :
                                    readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setDirectionType(keyVal[1]);
                                    break;
                            }
                        } else {
                            String aS = "Error in directionType value : " + line;
                            logger.error(aS);
                            LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                        }
                        break;
                    case "starttime" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setStartTime(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setStartTime(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "duration" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setDuration(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setDuration(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "maximizeduration" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setMaximizeDuration(keyVal[1]);
                                break;
                            case "anabeam" :
                                readAnaBeams.get(idx+1).setMaximizeDuration(keyVal[1]);
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "subbandlist" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setSubbandList(keyVal[1]);
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "beamletlist" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setBeamletList(keyVal[1]);
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                        break;
                    case "nrtiedarraybeams" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setNrTiedArrayBeams(keyVal[1]);
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                    case "nrtabrings" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setNrTabRings(keyVal[1]);
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                    case "tabringsize" :
                        switch (check) {
                            case "beam" :
                                readBeams.get(idx+1).setTabRingSize(keyVal[1]);
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                break;
                        }
                    case "rank" :
                        if (keyVal[1].equals("1") || keyVal[1].equals("2") || keyVal[1].equals("3") || keyVal[1].equals("4") || keyVal[1].equals("5")) {
                            switch (check) {
                                case "beam" :
                                    break;
                                case "anabeam" :
                                    readAnaBeams.get(idx+1).setRank(keyVal[1]);
                                    break;
                                case "tiedarraybeam" :
                                    break;
                            }
                        } else {
                            String aS = "Error in ranks value : " + line;
                            logger.error(aS);
                            LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                        }
                        break;
                    case "dispersionmeasure" :
                        switch (check) {
                            case "beam" :
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setDispersionMeasure(keyVal[1]);
                                break;
                        }
                        break;                    
                    case "coherent" :
                        switch (check) {
                            case "beam" :
                                break;
                            case "anabeam" :
                                break;
                            case "tiedarraybeam" :
                                readBeams.get(idx+1).getTiedArrayBeams().get(tabIdx+1).setCoherent(keyVal[1]);
                                break;
                        }
                    default :
                        String aS = "Unknown key : " + line;
                        logger.error(aS);
                        LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
                        break;
                }
            }
            // all input read.  close file
            f.close();
        } catch (IOException ex) {
            String aS = "Error opening or reading file: " + ex;
            logger.error(aS);
            LofarUtils.showErrorPanel(this, aS, new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_warn.gif")));
            return;
        }

        // Bug1641 Backwards compatibility
        for (Beam b : readBeams ) {
            if (b.getMaximizeDuration().equals("")) b.setMaximizeDuration("Missing");
        }
        for (AnaBeam b : readAnaBeams ) {
            if (b.getMaximizeDuration().equals("")) b.setMaximizeDuration("Missing");
        }
        // fill table with all entries
        switch (choice) {
            case "Beams":
                itsBeamConfigurationTableModel.fillTable(itsTreeType,readBeams, true);
                break;
            case "AnaBeams":
                itsAnaBeamConfigurationTableModel.fillTable(itsTreeType, readAnaBeams, true);
                break;
        }
    }

    private void checkBeamformers(Object[] stations ){

        ArrayList<Integer> delrows=new ArrayList<>();
        ArrayList<String> aVS= new ArrayList<>();
        for(int k=0; k< stations.length;k++) {
            aVS.add((String)stations[k]);
        }
        this.itsBeamformerConfigurationTableModel.getTable(itsStations);
        
        for (int i = 1; i< itsStations.size();i++) {
            String sl = itsStations.get(i);
            Collection s = Arrays.asList(sl.split("[,]"));
            if (aVS.containsAll(s)) {
                break;
            }
            if (delrows != null && !delrows.contains(i-1)) {
                delrows.add(i-1);
            }                
        }
        // check if we found matches with stations that no longer exist, if so the beamformer involved
        // will be deleted.
        
        for (int i=0; i < delrows.size(); i++) {
            beamformerConfigurationPanel.setSelectedRow(delrows.get(i),delrows.get(i));
            deleteBeamformer();
        }        
    }

    private void setAnaBeamConfiguration(boolean flag) {
        this.anaBeamConfiguration.setVisible(flag);
        this.copyBeamButton.setVisible(flag);
   }

    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jPanel1 = new javax.swing.JPanel();
        jLabel1 = new javax.swing.JLabel();
        jTabbedPane1 = new javax.swing.JTabbedPane();
        antennaConfigPanel = new nl.astron.lofar.sas.otbcomponents.AntennaConfigPanel();
        jScrollPane1 = new javax.swing.JScrollPane();
        jPanel2 = new javax.swing.JPanel();
        treeDescriptionScrollPane = new javax.swing.JScrollPane();
        inputTreeDescription = new javax.swing.JTextArea();
        descriptionScrollPane = new javax.swing.JScrollPane();
        inputDescription = new javax.swing.JTextArea();
        jPanel10 = new javax.swing.JPanel();
        labelNrSlotsInFrame = new javax.swing.JLabel();
        inputNrSlotsInFrame = new javax.swing.JTextField();
        inputNrChannelsPerSubband = new javax.swing.JTextField();
        labelNrChannelsPerSubband = new javax.swing.JLabel();
        jPanel3 = new javax.swing.JPanel();
        beamConfigurationPanel = new nl.astron.lofar.sas.otbcomponents.TablePanel();
        addBeamButton = new javax.swing.JButton();
        editBeamButton = new javax.swing.JButton();
        deleteBeamButton = new javax.swing.JButton();
        loadBeamsButton = new javax.swing.JButton();
        copyBeamButton = new javax.swing.JButton();
        jPanel5 = new javax.swing.JPanel();
        stationsPanel = new javax.swing.JPanel();
        stationsScrollPane = new javax.swing.JScrollPane();
        stationList = new javax.swing.JList();
        stationsModPanel = new javax.swing.JPanel();
        stationsButtonPanel = new javax.swing.JPanel();
        jPanel4 = new javax.swing.JPanel();
        beamformerConfigurationPanel = new nl.astron.lofar.sas.otbcomponents.TablePanel();
        jLabel2 = new javax.swing.JLabel();
        addBeamformerButton = new javax.swing.JButton();
        deleteBeamformerButton = new javax.swing.JButton();
        jLabel3 = new javax.swing.JLabel();
        jLabel4 = new javax.swing.JLabel();
        beamformerStationsScrollPane = new javax.swing.JScrollPane();
        beamformerStationList = new javax.swing.JList();
        anaBeamConfiguration = new javax.swing.JPanel();
        anaBeamConfigurationPanel = new nl.astron.lofar.sas.otbcomponents.TablePanel();
        addAnaBeamButton = new javax.swing.JButton();
        editAnaBeamButton = new javax.swing.JButton();
        deleteAnaBeamButton = new javax.swing.JButton();
        loadAnaBeamsButton = new javax.swing.JButton();
        campaignInfoPanel = new nl.astron.lofar.sas.otbcomponents.CampaignInfo();
        buttonPanel1 = new nl.astron.lofar.sas.otbcomponents.ButtonPanel();

        setLayout(new java.awt.BorderLayout());

        jPanel1.setBorder(javax.swing.BorderFactory.createTitledBorder(""));
        jPanel1.setPreferredSize(new java.awt.Dimension(100, 25));
        jPanel1.setLayout(new java.awt.BorderLayout());

        jLabel1.setFont(new java.awt.Font("Tahoma", 1, 11));
        jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel1.setText("Observation Details");
        jPanel1.add(jLabel1, java.awt.BorderLayout.CENTER);

        add(jPanel1, java.awt.BorderLayout.NORTH);

        jTabbedPane1.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jTabbedPane1StateChanged(evt);
            }
        });

        antennaConfigPanel.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                antennaConfigPanelActionPerformed(evt);
            }
        });
        jTabbedPane1.addTab("Station", antennaConfigPanel);

        jPanel2.setBorder(javax.swing.BorderFactory.createTitledBorder(""));

        treeDescriptionScrollPane.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Observation Tree Description", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N

        inputTreeDescription.setColumns(20);
        inputTreeDescription.setRows(5);
        inputTreeDescription.setToolTipText("The description set here will go to the Tree Description");
        treeDescriptionScrollPane.setViewportView(inputTreeDescription);

        descriptionScrollPane.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Field Descriptions.", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N

        inputDescription.setColumns(20);
        inputDescription.setEditable(false);
        inputDescription.setRows(5);
        descriptionScrollPane.setViewportView(inputDescription);

        jPanel10.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Generic Observation Input", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N

        labelNrSlotsInFrame.setText("# Slots In Frame");

        inputNrSlotsInFrame.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                inputNrSlotsInFrameFocusGained(evt);
            }
        });

        inputNrChannelsPerSubband.setToolTipText("This field will be removes as soon as the input in the AntennaConfig tab wil be used for receiver selection.");
        inputNrChannelsPerSubband.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                inputNrChannelsPerSubbandFocusGained(evt);
            }
        });

        labelNrChannelsPerSubband.setText("# Channels per Subband");

        org.jdesktop.layout.GroupLayout jPanel10Layout = new org.jdesktop.layout.GroupLayout(jPanel10);
        jPanel10.setLayout(jPanel10Layout);
        jPanel10Layout.setHorizontalGroup(
            jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel10Layout.createSequentialGroup()
                .add(jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING, false)
                    .add(labelNrChannelsPerSubband, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                    .add(labelNrSlotsInFrame))
                .add(18, 18, 18)
                .add(jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING, false)
                    .add(inputNrSlotsInFrame)
                    .add(inputNrChannelsPerSubband, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 102, Short.MAX_VALUE))
                .addContainerGap(22, Short.MAX_VALUE))
        );
        jPanel10Layout.setVerticalGroup(
            jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel10Layout.createSequentialGroup()
                .add(jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(labelNrSlotsInFrame)
                    .add(inputNrSlotsInFrame, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                .add(12, 12, 12)
                .add(jPanel10Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(labelNrChannelsPerSubband)
                    .add(inputNrChannelsPerSubband, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)))
        );

        jPanel3.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Digital Beam Configuration", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N
        jPanel3.setPreferredSize(new java.awt.Dimension(200, 125));
        jPanel3.setRequestFocusEnabled(false);
        jPanel3.setVerifyInputWhenFocusTarget(false);

        beamConfigurationPanel.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                beamConfigurationPanelMouseClicked(evt);
            }
        });

        addBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_add.gif"))); // NOI18N
        addBeamButton.setText("add beam");
        addBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                addBeamButtonActionPerformed(evt);
            }
        });

        editBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_edit.gif"))); // NOI18N
        editBeamButton.setText("edit beam");
        editBeamButton.setEnabled(false);
        editBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                editBeamButtonActionPerformed(evt);
            }
        });

        deleteBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_delete.png"))); // NOI18N
        deleteBeamButton.setText("delete beam");
        deleteBeamButton.setEnabled(false);
        deleteBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                deleteBeamButtonActionPerformed(evt);
            }
        });

        loadBeamsButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_fileopen.gif"))); // NOI18N
        loadBeamsButton.setText("Load File");
        loadBeamsButton.setToolTipText("Load beams from a File");
        loadBeamsButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                loadBeamsButtonActionPerformed(evt);
            }
        });

        copyBeamButton.setText("Copy to Analog");
        copyBeamButton.setToolTipText("copy the selected beam to the analog table");
        copyBeamButton.setEnabled(false);
        this.setVisible(false);
        copyBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                copyBeamButtonActionPerformed(evt);
            }
        });

        org.jdesktop.layout.GroupLayout jPanel3Layout = new org.jdesktop.layout.GroupLayout(jPanel3);
        jPanel3.setLayout(jPanel3Layout);
        jPanel3Layout.setHorizontalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel3Layout.createSequentialGroup()
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(beamConfigurationPanel, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 1255, Short.MAX_VALUE)
                    .add(jPanel3Layout.createSequentialGroup()
                        .add(addBeamButton)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(editBeamButton)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(deleteBeamButton)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                        .add(loadBeamsButton)
                        .add(18, 18, 18)
                        .add(copyBeamButton, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 155, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)))
                .addContainerGap())
        );
        jPanel3Layout.setVerticalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel3Layout.createSequentialGroup()
                .add(beamConfigurationPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 123, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                        .add(editBeamButton)
                        .add(deleteBeamButton)
                        .add(loadBeamsButton)
                        .add(copyBeamButton))
                    .add(addBeamButton))
                .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        jPanel5.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Virtual Instrument", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N

        stationsPanel.setBorder(javax.swing.BorderFactory.createTitledBorder("Station Names"));
        stationsPanel.setToolTipText("Identifiers of the participating stations.");
        stationsPanel.setLayout(new java.awt.BorderLayout());

        stationList.setModel(new javax.swing.AbstractListModel() {
            String[] strings = { "1", "2", "3", "4", "5" };
            public int getSize() { return strings.length; }
            public Object getElementAt(int i) { return strings[i]; }
        });
        stationList.setSelectionMode(javax.swing.ListSelectionModel.SINGLE_SELECTION);
        stationList.setToolTipText("Names of the participating stations.");
        stationList.setEnabled(false);
        stationsScrollPane.setViewportView(stationList);

        stationsPanel.add(stationsScrollPane, java.awt.BorderLayout.CENTER);

        stationsModPanel.setLayout(new java.awt.BorderLayout());

        stationsButtonPanel.setLayout(new java.awt.GridBagLayout());
        stationsModPanel.add(stationsButtonPanel, java.awt.BorderLayout.SOUTH);

        stationsPanel.add(stationsModPanel, java.awt.BorderLayout.SOUTH);

        org.jdesktop.layout.GroupLayout jPanel5Layout = new org.jdesktop.layout.GroupLayout(jPanel5);
        jPanel5.setLayout(jPanel5Layout);
        jPanel5Layout.setHorizontalGroup(
            jPanel5Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel5Layout.createSequentialGroup()
                .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                .add(stationsPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 145, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .add(308, 308, 308))
        );
        jPanel5Layout.setVerticalGroup(
            jPanel5Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel5Layout.createSequentialGroup()
                .add(stationsPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 176, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addContainerGap(12, Short.MAX_VALUE))
        );

        jPanel4.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Beamformer Input", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N

        beamformerConfigurationPanel.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                beamformerConfigurationPanelMouseClicked(evt);
            }
        });

        jLabel2.setFont(new java.awt.Font("Tahoma", 1, 12));
        jLabel2.setText("Stations");

        addBeamformerButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_add.gif"))); // NOI18N
        addBeamformerButton.setText("add beamformer");
        addBeamformerButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                addBeamformerButtonActionPerformed(evt);
            }
        });

        deleteBeamformerButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_delete.png"))); // NOI18N
        deleteBeamformerButton.setText("delete beamformer");
        deleteBeamformerButton.setEnabled(false);
        deleteBeamformerButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                deleteBeamformerButtonActionPerformed(evt);
            }
        });

        jLabel3.setText("doubleclick on a station to ");

        jLabel4.setText("add it to the highlighted beamformer");

        beamformerStationList.setModel(new javax.swing.AbstractListModel() {
            String[] strings = { "1", "2", "3", "4", "5" };
            public int getSize() { return strings.length; }
            public Object getElementAt(int i) { return strings[i]; }
        });
        beamformerStationList.setToolTipText("Stations that can be used in the selected beamformer");
        beamformerStationList.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                beamformerStationListMouseClicked(evt);
            }
        });
        beamformerStationsScrollPane.setViewportView(beamformerStationList);

        org.jdesktop.layout.GroupLayout jPanel4Layout = new org.jdesktop.layout.GroupLayout(jPanel4);
        jPanel4.setLayout(jPanel4Layout);
        jPanel4Layout.setHorizontalGroup(
            jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel4Layout.createSequentialGroup()
                .addContainerGap()
                .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(beamformerConfigurationPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 397, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jPanel4Layout.createSequentialGroup()
                        .add(addBeamformerButton)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(deleteBeamformerButton)))
                .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(jPanel4Layout.createSequentialGroup()
                        .add(28, 28, 28)
                        .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(jLabel3)
                            .add(jLabel4)
                            .add(beamformerStationsScrollPane, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 132, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)))
                    .add(jPanel4Layout.createSequentialGroup()
                        .add(57, 57, 57)
                        .add(jLabel2)))
                .add(10, 10, 10))
        );
        jPanel4Layout.setVerticalGroup(
            jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel4Layout.createSequentialGroup()
                .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, jPanel4Layout.createSequentialGroup()
                        .addContainerGap()
                        .add(jLabel2)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(beamformerStationsScrollPane, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 109, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                    .add(beamformerConfigurationPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 128, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(jPanel4Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                        .add(addBeamformerButton)
                        .add(deleteBeamformerButton))
                    .add(jPanel4Layout.createSequentialGroup()
                        .add(jLabel3)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(jLabel4)))
                .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        anaBeamConfiguration.setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Analog Beam Configuration", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Tahoma", 1, 11))); // NOI18N
        anaBeamConfiguration.setEnabled(false);

        anaBeamConfigurationPanel.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                anaBeamConfigurationPanelMouseClicked(evt);
            }
        });

        addAnaBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_add.gif"))); // NOI18N
        addAnaBeamButton.setText("add beam");
        addAnaBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                addAnaBeamButtonActionPerformed(evt);
            }
        });

        editAnaBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_edit.gif"))); // NOI18N
        editAnaBeamButton.setText("edit beam");
        editAnaBeamButton.setEnabled(false);
        editAnaBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                editAnaBeamButtonActionPerformed(evt);
            }
        });

        deleteAnaBeamButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_delete.png"))); // NOI18N
        deleteAnaBeamButton.setText("delete beam");
        deleteAnaBeamButton.setEnabled(false);
        deleteAnaBeamButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                deleteAnaBeamButtonActionPerformed(evt);
            }
        });

        loadAnaBeamsButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/nl/astron/lofar/sas/otb/icons/16_fileopen.gif"))); // NOI18N
        loadAnaBeamsButton.setText("Load File");
        loadAnaBeamsButton.setToolTipText("load analogue beams from file");
        loadAnaBeamsButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                loadAnaBeamsButtonActionPerformed(evt);
            }
        });

        org.jdesktop.layout.GroupLayout anaBeamConfigurationLayout = new org.jdesktop.layout.GroupLayout(anaBeamConfiguration);
        anaBeamConfiguration.setLayout(anaBeamConfigurationLayout);
        anaBeamConfigurationLayout.setHorizontalGroup(
            anaBeamConfigurationLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(anaBeamConfigurationLayout.createSequentialGroup()
                .add(anaBeamConfigurationPanel, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 1163, Short.MAX_VALUE)
                .add(102, 102, 102))
            .add(anaBeamConfigurationLayout.createSequentialGroup()
                .add(addAnaBeamButton)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(editAnaBeamButton)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(deleteAnaBeamButton)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                .add(loadAnaBeamsButton)
                .add(839, 839, 839))
        );
        anaBeamConfigurationLayout.setVerticalGroup(
            anaBeamConfigurationLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(anaBeamConfigurationLayout.createSequentialGroup()
                .add(anaBeamConfigurationPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 123, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                .add(anaBeamConfigurationLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(anaBeamConfigurationLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                        .add(editAnaBeamButton)
                        .add(deleteAnaBeamButton)
                        .add(loadAnaBeamsButton))
                    .add(addAnaBeamButton)))
        );

        org.jdesktop.layout.GroupLayout jPanel2Layout = new org.jdesktop.layout.GroupLayout(jPanel2);
        jPanel2.setLayout(jPanel2Layout);
        jPanel2Layout.setHorizontalGroup(
            jPanel2Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel2Layout.createSequentialGroup()
                .add(jPanel2Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING, false)
                    .add(jPanel2Layout.createSequentialGroup()
                        .add(jPanel4, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(jPanel5, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 175, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .add(30, 30, 30)
                        .add(jPanel10, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                    .add(descriptionScrollPane)
                    .add(jPanel3, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 1277, Short.MAX_VALUE)
                    .add(anaBeamConfiguration, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                    .add(treeDescriptionScrollPane))
                .addContainerGap(30, Short.MAX_VALUE))
        );
        jPanel2Layout.setVerticalGroup(
            jPanel2Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel2Layout.createSequentialGroup()
                .add(jPanel3, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 180, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(anaBeamConfiguration, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .add(jPanel2Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(jPanel2Layout.createSequentialGroup()
                        .add(18, 18, 18)
                        .add(jPanel10, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                    .add(jPanel2Layout.createSequentialGroup()
                        .add(11, 11, 11)
                        .add(jPanel2Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(jPanel5, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                            .add(jPanel4, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                .add(descriptionScrollPane, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 54, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(treeDescriptionScrollPane, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 101, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .add(144, 144, 144))
        );

        jScrollPane1.setViewportView(jPanel2);

        jTabbedPane1.addTab("Generic", jScrollPane1);
        jTabbedPane1.addTab("Campaign", campaignInfoPanel);

        add(jTabbedPane1, java.awt.BorderLayout.CENTER);

        buttonPanel1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                buttonPanel1ActionPerformed(evt);
            }
        });
        add(buttonPanel1, java.awt.BorderLayout.SOUTH);
    }// </editor-fold>//GEN-END:initComponents

    private void deleteBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_deleteBeamButtonActionPerformed
        deleteBeam();
    }//GEN-LAST:event_deleteBeamButtonActionPerformed

    private void editBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_editBeamButtonActionPerformed
        editBeam=true;
        addBeam();
    }//GEN-LAST:event_editBeamButtonActionPerformed

    private void addBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_addBeamButtonActionPerformed
        editBeam=false;
        addBeam();
    }//GEN-LAST:event_addBeamButtonActionPerformed

    private void beamConfigurationPanelMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_beamConfigurationPanelMouseClicked
        editBeamButton.setEnabled(true);
        deleteBeamButton.setEnabled(true);
        copyBeamButton.setEnabled(true);
    }//GEN-LAST:event_beamConfigurationPanelMouseClicked

    private void buttonPanel1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_buttonPanel1ActionPerformed
        switch (evt.getActionCommand()) {
            case "Apply":
                itsMainFrame.setHourglassCursor();
                // save the input from the AntennaConfig Panel
                this.antennaConfigPanel.saveInput();
                // save the input from the Generic Panel
                saveInput();
                // reset all buttons, flags and tables to initial start position. So the panel now reflects the new, saved situation
                initPanel();
                itsMainFrame.setNormalCursor();
                break;
            case "Restore":
                itsMainFrame.setHourglassCursor();
                restore();
                itsMainFrame.setNormalCursor();
                break;
        }

    }//GEN-LAST:event_buttonPanel1ActionPerformed

    private void inputNrSlotsInFrameFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_inputNrSlotsInFrameFocusGained
        changeDescription(itsNrSlotsInFrame);
    }//GEN-LAST:event_inputNrSlotsInFrameFocusGained

    private void beamformerConfigurationPanelMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_beamformerConfigurationPanelMouseClicked
        deleteBeamformerButton.setEnabled(true);
    }//GEN-LAST:event_beamformerConfigurationPanelMouseClicked

    private void deleteBeamformerButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_deleteBeamformerButtonActionPerformed
        // delete beamformer, ask confirmation
        int answer=JOptionPane.showConfirmDialog(this,"Are you sure you want to delete this beamformer?","alert",JOptionPane.YES_NO_OPTION);
        if (answer == JOptionPane.NO_OPTION) {
            return;
        }
        // get stations involved and add them back to the available station list
        
        deleteBeamformer();
    }//GEN-LAST:event_deleteBeamformerButtonActionPerformed

    private void addBeamformerButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_addBeamformerButtonActionPerformed
        String selection="";
        if (this.itsAvailableBeamformStations.size()>0){  
          itsBeamformerConfigurationTableModel.addRow(selection);
          beamformerConfigurationPanel.setSelectedRow((itsBeamformerConfigurationTableModel.getRowCount()-1),(itsBeamformerConfigurationTableModel.getRowCount()-1));
          JOptionPane.showMessageDialog(this,"New Beamformer created, add stations via doubleclick in stationslist","New Beamformer",JOptionPane.YES_OPTION);
        }
    }//GEN-LAST:event_addBeamformerButtonActionPerformed

    private void jTabbedPane1StateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jTabbedPane1StateChanged
        switch (jTabbedPane1.getTitleAt(jTabbedPane1.getSelectedIndex())) {
            case "Station":
                break;
            case "Generic":
                LofarUtils.fillList(stationList, antennaConfigPanel.getUsedStations(),false);
                DefaultListModel aM = (DefaultListModel)stationList.getModel();
                checkBeamformers(aM.toArray());
                fillBeamformerStationList();
                if (this.antennaConfigPanel.isLBASelected()){
                    this.setAnaBeamConfiguration(false);
                } else {
                    this.setAnaBeamConfiguration(true);
                }
                break;
        }
    }//GEN-LAST:event_jTabbedPane1StateChanged

    private void anaBeamConfigurationPanelMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_anaBeamConfigurationPanelMouseClicked
    if (!this.antennaConfigPanel.isLBASelected()) {
        editAnaBeamButton.setEnabled(true);
        deleteAnaBeamButton.setEnabled(true);
    }
}//GEN-LAST:event_anaBeamConfigurationPanelMouseClicked

    private void addAnaBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_addAnaBeamButtonActionPerformed
        editAnaBeam=false;
        addAnaBeam();
}//GEN-LAST:event_addAnaBeamButtonActionPerformed

    private void editAnaBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_editAnaBeamButtonActionPerformed
        editAnaBeam=true;
        addAnaBeam();

}//GEN-LAST:event_editAnaBeamButtonActionPerformed

    private void deleteAnaBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_deleteAnaBeamButtonActionPerformed
        deleteAnaBeam();
}//GEN-LAST:event_deleteAnaBeamButtonActionPerformed

    private void inputNrChannelsPerSubbandFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_inputNrChannelsPerSubbandFocusGained
        changeDescription(itsChannelsPerSubband);
}//GEN-LAST:event_inputNrChannelsPerSubbandFocusGained

    private void loadBeamsButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_loadBeamsButtonActionPerformed
        loadBeamFile("Beams");
    }//GEN-LAST:event_loadBeamsButtonActionPerformed

    private void loadAnaBeamsButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_loadAnaBeamsButtonActionPerformed
        if (this.anaBeamConfiguration.isVisible()) {
            loadBeamFile("AnaBeams");
        }
    }//GEN-LAST:event_loadAnaBeamsButtonActionPerformed

    private void copyBeamButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_copyBeamButtonActionPerformed
        copyBeamToAnaBeam();
    }//GEN-LAST:event_copyBeamButtonActionPerformed

    private void beamformerStationListMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_beamformerStationListMouseClicked
        if (evt.getClickCount() > 1) {
            //doubleClick event
            addBeamformerStation((String) beamformerStationList.getSelectedValue());
        }
}//GEN-LAST:event_beamformerStationListMouseClicked

    private void antennaConfigPanelActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_antennaConfigPanelActionPerformed
       if (evt.getActionCommand().equals("validInput")) {
           buttonPanel1.setButtonEnabled("Apply", true);
       }
       if (evt.getActionCommand().equals("invalidInput")) {
           buttonPanel1.setButtonEnabled("Apply", false);
       }
    }//GEN-LAST:event_antennaConfigPanelActionPerformed
    
    private jOTDBnode                         itsNode = null;
    private MainFrame                         itsMainFrame;
    private jOTDBparam                        itsOldDescriptionParam;
    private String                            itsOldTreeDescription;
    private String                            itsTreeType="";
    private BeamConfigurationTableModel       itsBeamConfigurationTableModel = null;
    private AnaBeamConfigurationTableModel    itsAnaBeamConfigurationTableModel = null;
    private BeamformerConfigurationTableModel itsBeamformerConfigurationTableModel = null;
    private JFileChooser                      fc = null;
    private BeamDialog                        beamDialog = null;
    private AnaBeamDialog                     anaBeamDialog = null;
    private BeamFileDialog                    beamFileDialog = null;
    
    // Observation Specific parameters
    private jOTDBnode itsChannelsPerSubband=null;
    private jOTDBnode itsNrSlotsInFrame=null;
    private jOTDBnode itsNrBeams=null;
    private jOTDBnode itsNrAnaBeams=null;
    private jOTDBnode itsNrBeamformers=null;
  
    
    // Beams
    private ArrayList<Beam>     itsBeamList       = new ArrayList<>();
    private Beam                itsActiveBeam;
    // keep nodes for deletion
    private ArrayList<jOTDBnode> itsBeams = new ArrayList<>();
    
    // AnalogBeams
    private ArrayList<AnaBeam>  itsAnaBeamList    = new ArrayList<>();
    private AnaBeam             itsActiveAnaBeam;
    // keep nodes for deletion
    private ArrayList<jOTDBnode> itsAnaBeams = new ArrayList<>();

    // TiedArrayBeams
    private ArrayList<TiedArrayBeam>  itsTABList    = new ArrayList<>();
    private TiedArrayBeam       itsActiveTAB;
    
    // Beamformers
    private Vector<jOTDBnode> itsBeamformers    = new Vector<>();
    private ArrayList<String>    itsStations       = new ArrayList<>();
   
    // Observation Virtual Instrument parameters
    private jOTDBnode itsStationList=null;

    // keeps lists of available (unused)  and all used stations for Beamformer creation
    private ArrayList<String>    itsAvailableBeamformStations       = new ArrayList<>();
    private ArrayList<String>    itsUsedBeamformStations            = new ArrayList<>();
    // each beamlet has its bit in the bitset
    private BitSet   itsUsedBeamlets = new BitSet(216);
    private boolean  editBeam = false;
    private boolean  editAnaBeam = false;
    private int      itsSelectedRow = -1;


    // Temp
    private Vector<String>    itsUsedStorageNodes      = new Vector<>();



    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton addAnaBeamButton;
    private javax.swing.JButton addBeamButton;
    private javax.swing.JButton addBeamformerButton;
    private javax.swing.JPanel anaBeamConfiguration;
    private nl.astron.lofar.sas.otbcomponents.TablePanel anaBeamConfigurationPanel;
    private nl.astron.lofar.sas.otbcomponents.AntennaConfigPanel antennaConfigPanel;
    private nl.astron.lofar.sas.otbcomponents.TablePanel beamConfigurationPanel;
    private nl.astron.lofar.sas.otbcomponents.TablePanel beamformerConfigurationPanel;
    private javax.swing.JList beamformerStationList;
    private javax.swing.JScrollPane beamformerStationsScrollPane;
    private nl.astron.lofar.sas.otbcomponents.ButtonPanel buttonPanel1;
    private nl.astron.lofar.sas.otbcomponents.CampaignInfo campaignInfoPanel;
    private javax.swing.JButton copyBeamButton;
    private javax.swing.JButton deleteAnaBeamButton;
    private javax.swing.JButton deleteBeamButton;
    private javax.swing.JButton deleteBeamformerButton;
    private javax.swing.JScrollPane descriptionScrollPane;
    private javax.swing.JButton editAnaBeamButton;
    private javax.swing.JButton editBeamButton;
    private javax.swing.JTextArea inputDescription;
    private javax.swing.JTextField inputNrChannelsPerSubband;
    private javax.swing.JTextField inputNrSlotsInFrame;
    private javax.swing.JTextArea inputTreeDescription;
    private javax.swing.JLabel jLabel1;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JLabel jLabel3;
    private javax.swing.JLabel jLabel4;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JPanel jPanel10;
    private javax.swing.JPanel jPanel2;
    private javax.swing.JPanel jPanel3;
    private javax.swing.JPanel jPanel4;
    private javax.swing.JPanel jPanel5;
    private javax.swing.JScrollPane jScrollPane1;
    private javax.swing.JTabbedPane jTabbedPane1;
    private javax.swing.JLabel labelNrChannelsPerSubband;
    private javax.swing.JLabel labelNrSlotsInFrame;
    private javax.swing.JButton loadAnaBeamsButton;
    private javax.swing.JButton loadBeamsButton;
    private javax.swing.JList stationList;
    private javax.swing.JPanel stationsButtonPanel;
    private javax.swing.JPanel stationsModPanel;
    private javax.swing.JPanel stationsPanel;
    private javax.swing.JScrollPane stationsScrollPane;
    private javax.swing.JScrollPane treeDescriptionScrollPane;
    // End of variables declaration//GEN-END:variables
    
    
    
    /**
     * Utility field used by event firing mechanism.
     */
    private javax.swing.event.EventListenerList myListenerList =  null;

    /**
     * Registers ActionListener to receive events.
     * @param listener The listener to register.
     */
    public synchronized void addActionListener(java.awt.event.ActionListener listener) {

        if (myListenerList == null ) {
            myListenerList = new javax.swing.event.EventListenerList();
        }
        myListenerList.add (java.awt.event.ActionListener.class, listener);
    }

    /**
     * Removes ActionListener from the list of listeners.
     * @param listener The listener to remove.
     */
    public synchronized void removeActionListener(java.awt.event.ActionListener listener) {

        myListenerList.remove (java.awt.event.ActionListener.class, listener);
    }

    /**
     * Notifies all registered listeners about the event.
     * 
     * @param event The event to be fired
     */
    private void fireActionListenerActionPerformed(java.awt.event.ActionEvent event) {

        if (myListenerList == null) return;
        Object[] listeners = myListenerList.getListenerList ();
        for (int i = listeners.length - 2; i >= 0; i -= 2) {
            if (listeners[i]==java.awt.event.ActionListener.class) {
                ((java.awt.event.ActionListener)listeners[i+1]).actionPerformed (event);
            }
        }
    }    
}
