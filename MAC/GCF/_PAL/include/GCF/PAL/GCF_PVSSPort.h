//#  GCF_PVSSPort.h: PVSS connection to a remote process
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

#ifndef GCF_PVSSPORT_H
#define GCF_PVSSPORT_H

#include <GCF/TM/GCF_RawPort.h>
#include <GCF/GCF_PVString.h>
#include <Common/lofar_string.h>
#include <set>

using namespace std;
// forward declaration
class GCFTask;
class GCFEvent;
class GSAPortService;
class GCFPVBlob;
class GCFPVSSUIMConverter;

/**
 * This is the class, which implements the special port with the PVSS message 
 * transport protocol. 
 */
class GCFPVSSPort : public GCFRawPort
{
 public:

    /// Construction methods
    /** @param protocol NOT USED */    
    explicit GCFPVSSPort (GCFTask& task,
          	    string name,
          	    TPortType type,
                int protocol, 
                bool transportRawData = false);
    explicit GCFPVSSPort ();
  
    virtual ~GCFPVSSPort ();
  
  public:

    void setConverter(GCFPVSSUIMConverter& converter);
    /**
     * open/close functions
     */
    virtual bool open ();
    virtual bool close ();
  
    /**
     * send/recv functions
     */
    virtual ssize_t send (GCFEvent& event);
    virtual ssize_t recv (void* buf,
                          size_t count);
           
  public: // pvss port specific methods

    void setDestAddr (const string& destDpName);
    const string& getPortID() {return _portId.getValue();}
    const string& getPortAddr() {return _portAddr.getValue();}
    virtual bool accept(GCFPVSSPort& newPort);

  private:
    /**
     * Don't allow copying this object.
     */
    GCFPVSSPort (const GCFPVSSPort&);
    GCFPVSSPort& operator= (const GCFPVSSPort&);
    
    friend class GSAPortService;
    void serviceStarted(bool successfull);
    void setService(GSAPortService& service);
            
    static unsigned int claimPortNr();
    static void releasePortNr(const string& portId);
    
  private:
    GSAPortService*   _pPortService;
    string            _destDpName;
    GCFPVString       _portId;
    GCFPVString       _portAddr;
    GCFPVSSUIMConverter* _pConverter;
    
    bool _acceptedPort;
    
    typedef set<unsigned int> TPVSSPortNrs;
    static TPVSSPortNrs _pvssPortNrs;        
};

class GCFPVSSUIMConverter
{
  public:
    virtual bool uimMsgToGCFEvent(unsigned char* buf, unsigned int length, GCFPVBlob& gcfEvent) = 0;
    virtual bool gcfEventToUIMMsg(GCFPVBlob& gcfEvent, GCFPVBlob& uimMsg) = 0;
};

#endif
