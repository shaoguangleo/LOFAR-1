/*
 * PICtableModel.java
 *
 * Created on January 31, 2006, 11:11 AM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

package nl.astron.lofar.sas.otb.util;

import java.util.Vector;
import nl.astron.lofar.sas.otb.jotdb2.jOTDBtree;
import org.apache.log4j.Logger;

/**
 * Implements the data model behind the PIC table 
 *
 * @author blaakmeer
 */
public class PICtableModel extends javax.swing.table.AbstractTableModel {
    
    private String headers[] = {"TreeID","Status","Classification","Creator","CreationTime","ObsoleteTime","Description"};
    private OtdbRmi otdbRmi;
    private Object data[][];
    
    static Logger logger = Logger.getLogger(PICtableModel.class);
    static String name = "PICtableModel";

    /** Creates a new instance of PICtableModel */
    public PICtableModel(OtdbRmi otdbRmi) {

        this.otdbRmi = otdbRmi;
        fillTable();
    }
    
    /** Refreshes 1 row from table out of the database
     *
     * @param   row     Rownr that needs to be refreshed
     * @return  suceeded or failed status
     */ 
    public boolean refreshRow(int row) {
        
        try {
            if (! otdbRmi.getRemoteOTDB().isConnected()) {
                logger.debug("No open connection available");
                return false;
            }

            // get TreeID that needs 2b refreshed
            int aTreeID=((Integer)data[row][0]).intValue();
            jOTDBtree tInfo=otdbRmi.getRemoteOTDB().getTreeInfo(aTreeID, false);
            if ( tInfo == null) {
                logger.debug("Unable to get treeInfo for tree with ID: " + aTreeID);
                return false;
            }
            data[row][0]=new Integer(tInfo.treeID());	   
            data[row][1]=new String(otdbRmi.getTreeState().get(tInfo.state));
            data[row][2]=new String(otdbRmi.getClassif().get(tInfo.classification));
	    data[row][3]=new String(tInfo.creator);
	    data[row][4]=new String(tInfo.starttime);
	    data[row][5]=new String(tInfo.stoptime);
	    data[row][6]=new String(tInfo.description);
            fireTableDataChanged();
        } catch (Exception e) {
            logger.debug("Remote OTDB via RMI and JNI failed: " + e);
        } 
        return true;
    }
    
    /** Fills the table from the database */
    public boolean fillTable() {
        
       if (otdbRmi == null) {
            logger.debug("No active otdbRmi connection");
            return false;
        }
        try {
            if (otdbRmi.getRemoteOTDB() != null && ! otdbRmi.getRemoteOTDB().isConnected()) {
                logger.debug("No open connection available");
                return false;
            }
            // Get a Treelist of all available PIC's
            Vector aTreeList=otdbRmi.getRemoteOTDB().getTreeList(otdbRmi.getRemoteTypes().getTreeType("hardware"),(short)0);
            data = new Object[aTreeList.size()][headers.length];
            logger.debug("Treelist downloaded. Size: "+aTreeList.size());
           
            for (int k=0; k< aTreeList.size();k++) {
                jOTDBtree tInfo = (jOTDBtree)aTreeList.elementAt(k);
                if (tInfo == null) {
                    logger.debug("No such tree found!");
                } else {
                    logger.debug("Gathered info for ID: "+tInfo.treeID());
                    data[k][0]=new Integer(tInfo.treeID());	   
	            data[k][1]=new String(otdbRmi.getTreeState().get(tInfo.state));
                    data[k][2]=new String(otdbRmi.getClassif().get(tInfo.classification));
	            data[k][3]=new String(tInfo.creator);
	            data[k][4]=new String(tInfo.starttime);
	            data[k][5]=new String(tInfo.stoptime);
	            data[k][6]=new String(tInfo.description);
                }
            }
            fireTableDataChanged();
        } catch (Exception e) {
            logger.debug("Remote OTDB via RMI and JNI failed: " + e);
	} 
        return true;
    }

    /** Returns the number of rows 
     *  @return Nr of rows 
     */
    public int getRowCount() {
        return data.length;
    }

    /** Returns the column names 
     * @param    c   Column Number
     * @return  the name for this column     
     */
    public String getColumnName(int c) {
        try {
            return headers[c];
        }
        catch(ArrayIndexOutOfBoundsException e) {
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
            return data[r][c];
        }
        catch(ArrayIndexOutOfBoundsException e) {
            return null;
        }
    }
}
