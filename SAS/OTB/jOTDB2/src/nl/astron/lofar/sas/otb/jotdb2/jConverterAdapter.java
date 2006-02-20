//#  jConverterAdapter.java: 
//#
//#  Copyright (C) 2002-2007
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
  
package nl.astron.lofar.sas.otb.jotdb2;


import java.rmi.server.UnicastRemoteObject;
import java.rmi.RemoteException;
import java.util.HashMap;

public class jConverterAdapter extends UnicastRemoteObject implements jConverterInterface
{
   // Constructor
   public jConverterAdapter (jConverter adaptee) throws RemoteException
     {
	this.adaptee = adaptee;
     }
   
    public long getClassif (String aConv) throws RemoteException
    {
	return adaptee.getClassif (aConv);
    }
    
    public String getClassif (long aConv) throws RemoteException
    {
	return adaptee.getClassif (aConv);
    }
    
    public HashMap getClassif() throws RemoteException
    {
        return adaptee.getClassif();
    }
    
    public long getParamType (String aConv) throws RemoteException
    {
	return adaptee.getParamType (aConv);
    }

    public String getParamType (long aConv) throws RemoteException
    {
	return adaptee.getParamType (aConv);
    }
    
    public HashMap getParamType() throws RemoteException
    {
        return adaptee.getParamType();
    }

    public long getTreeState (String aConv) throws RemoteException
    {
	return adaptee.getTreeState (aConv);
    }
    
    public String getTreeState (long aConv) throws RemoteException
    {
	return adaptee.getTreeState (aConv);
    }
    
    public HashMap getTreeState() throws RemoteException
    {
        return adaptee.getTreeState();
    }

    public long getTreeType (String aConv) throws RemoteException
    {
	return adaptee.getTreeType (aConv);
    }
    
    public String getTreeType (long aConv) throws RemoteException
    {
	return adaptee.getTreeType (aConv);
    }
    
    public HashMap getTreeType() throws RemoteException
    {
        return adaptee.getTreeType();
    }

    public long getUnit (String aConv) throws RemoteException
    {
	return adaptee.getUnit (aConv);
    }
    
    public String getUnit (long aConv) throws RemoteException
    {
	return adaptee.getUnit (aConv);
    }
    
    public HashMap getUnit() throws RemoteException
    {
        return adaptee.getUnit();
    }

   
    protected jConverter adaptee;   
}
