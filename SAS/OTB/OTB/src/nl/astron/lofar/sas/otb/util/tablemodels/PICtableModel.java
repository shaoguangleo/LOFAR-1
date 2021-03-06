/*
 * PICtableModel.java
 *
 *  Copyright (C) 2002-2007
 *  ASTRON (Netherlands Foundation for Research in Astronomy)
 *  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, softwaresupport@astron.nl
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

package nl.astron.lofar.sas.otb.util.tablemodels;

import java.rmi.RemoteException;
import java.util.ArrayList;
import nl.astron.lofar.sas.otb.jotdb3.jOTDBtree;
import nl.astron.lofar.sas.otb.util.*;
import org.apache.log4j.Logger;

/**
 * Implements the data model behind the PIC table 
 *
 * @created 31-01-2006, 11:11
 *
 * @author blaakmeer
 *
 * @version $Id$
 *
 * @updated
 */
public final class PICtableModel extends javax.swing.table.AbstractTableModel {
    
    private String headers[] = {"ID","Status","Classification","Creator","CreationTime","ObsoleteTime","Description"};
    private OtdbRmi otdbRmi;
    private Object data[][];
    
    static Logger logger = Logger.getLogger(PICtableModel.class);
    static String name = "PICtableModel";

    /** Creates a new instance of PICtableModel */
    public PICtableModel(OtdbRmi otdbRmi) {

        this.otdbRmi = otdbRmi;
        // this is the first tab, so we will present a filled tables list. all other tables are filled on demand
        fillTable();
    }
    
    /** Refreshes 1 row from table out of the database
     *
     * @param   row     Rownr that needs to be refreshed
     * @return  suceeded or failed status
     */ 
    public boolean refreshRow(int row) {
        
        try {
            if (! OtdbRmi.getRemoteOTDB().isConnected()) {
                logger.error("No open connection available");
                return false;
            }

            // get TreeID that needs 2b refreshed
            int aTreeID=((Integer)data[row][0]).intValue();
            jOTDBtree tInfo=OtdbRmi.getRemoteOTDB().getTreeInfo(aTreeID, false);
            if ( tInfo == null) {
                logger.warn("Unable to get treeInfo for tree with ID: " + aTreeID);
                return false;
            }
            data[row][0]=new Integer(tInfo.treeID());	   
            data[row][1]=OtdbRmi.getTreeState().get(tInfo.state);
            data[row][2]=OtdbRmi.getClassif().get(tInfo.classification);
	    data[row][3]=tInfo.creator;
	    data[row][4]=tInfo.starttime.replace("T", " ");
	    data[row][5]=tInfo.stoptime.replace("T", " ");
	    data[row][6]=tInfo.description;
            fireTableDataChanged();
        } catch (RemoteException e) {
            logger.error("Remote OTDB getTreeInfo failed: " + e);
        } 
        return true;
    }
    
    /** Fills the table from the database */
    public boolean fillTable() {
        
       if (otdbRmi == null) {
            logger.error("No active otdbRmi connection");
            return false;
        }
        try {
            if (OtdbRmi.getRemoteOTDB() != null && ! OtdbRmi.getRemoteOTDB().isConnected()) {
                logger.error("No open connection available");
                return false;
            }
            // Get a Treelist of all available PIC's
            ArrayList<jOTDBtree> aTreeList=new ArrayList<>(OtdbRmi.getRemoteOTDB().getTreeList(OtdbRmi.getRemoteTypes().getTreeType("hardware"),(short)0));
            data = new Object[aTreeList.size()][headers.length];
            logger.debug("Treelist downloaded. Size: "+aTreeList.size());
            int k=0;
            for (jOTDBtree tInfo:aTreeList) {
                if (tInfo == null) {
                    logger.warn("No such tree found!");
                } else {
                    logger.debug("Gathered info for ID: "+tInfo.treeID());
                    data[k][0]=new Integer(tInfo.treeID());	   
	            data[k][1]=OtdbRmi.getTreeState().get(tInfo.state);
                    data[k][2]=OtdbRmi.getClassif().get(tInfo.classification);
	            data[k][3]=tInfo.creator;
	            data[k][4]=tInfo.starttime.replace("T", " ");
	            data[k][5]=tInfo.stoptime.replace("T", " ");
	            data[k][6]=tInfo.description;
                    k++;
                }
            }
            fireTableDataChanged();
        } catch (RemoteException e) {
            logger.debug("Remote OTDB getTreeList failed: " + e);
	} 
        return true;
    }

    /** Returns the number of rows 
     *  @return Nr of rows 
     */
    public int getRowCount() {
        if (data != null) {
            return data.length;
        } else {
            return 0;
        }
    }

    /** Returns the column names 
     * @param    c   Column Number
     * @return  the name for this column    
     */
    @Override 
    public String getColumnName(int c) {
        try {
            return headers[c];
        }
        catch(ArrayIndexOutOfBoundsException e) {
            logger.error("ArrayIndex out of bound exception for getColumnName("+c+"): "+e);
            return null;
        }
        
    }

    /** Returns the number of columns 
     * @return  The number of columns
     */
    public int getColumnCount() {
        return headers.length;
    }

    /** Returns value from table
     * @param    r   rownumber
     * @param    c   columnnumber
     * 
     * @return  the value at row,column
     */
    public Object getValueAt(int r, int c) {
        try {
            if (data != null && data.length > 0) {
                return data[r][c];
            } else {
                return null;
            }
        }
        catch(ArrayIndexOutOfBoundsException e) {
            logger.error("ArrayIndex out of bound exception for getValueAt("+r+","+c+"): "+e);
            return null;
        }
    }

    @Override
    public Class getColumnClass(int c) {
        Object value=this.getValueAt(0,c);
        return (value==null?Object.class:value.getClass());
    }
}
