//#  APLInterTaskPort.cc: Direct communication between two tasks in the same process
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

#include <stdio.h>
#include <GCF/GCF_TMProtocols.h>
#include <GCF/GCF_Task.h>
#include <GCF/GCF_Event.h>
#include "APLInterTaskPort.h"
#include "APL_Defines.h"

using namespace std;

/**
 * ::APLInterTaskPort constructor
 */
APLInterTaskPort::APLInterTaskPort(GCFTask& slaveTask, GCFTask& task, string& name, TPortType type, int protocol) : 
    GCFRawPort(task, name, type, protocol), 
    m_slaveTask(slaveTask),
    m_toClientTimerId(),
    m_toServerTimerId()
{
}

/**
 * ::~APLInterTaskPort destructor
 */
APLInterTaskPort::~APLInterTaskPort()
{
}

/**
 * ::open
 */
int APLInterTaskPort::open()
{
  schedule_connected();
  return 0;
}

/**
 * ::close
 */
int APLInterTaskPort::close()
{
  schedule_disconnected();
  return 0;
}

/**
 * ::send
 */
ssize_t APLInterTaskPort::send(GCFEvent& e)
{
  ssize_t returnValue(0);
  
  if (SPP == _type)
  {
    if (F_EVT_INOUT(e) & F_IN)
    {
      LOFAR_LOG_ERROR(APL_LOGGER_ROOT, (
          "Trying to send IN event on SPP "
         "port '%s'; discarding this event.",
         _name.c_str()));
         
      returnValue=-1;
    }
  }
  else if (SAP == _type)
  {
    if (F_EVT_INOUT(e) & F_OUT)
    {
      LOFAR_LOG_ERROR(APL_LOGGER_ROOT, (
          "Trying to send OUT event on SAP "
          "port '%s'; discarding this event.",
         _name.c_str()));
      returnValue=-1;
    }
  }
  else if (MSPP == _type)
  {
    LOFAR_LOG_ERROR(APL_LOGGER_ROOT, (
      "Trying to send event by means of the portprovider: %s (MSPP). "
      "Not supported yet",
       _name.c_str()));
    returnValue=-1;
  }

  if(returnValue!=-1)
  {
    // send event using a timer event to exit the sending tasks event loop
    unsigned int requiredLength;
    char* packedBuffer = (char*)e.pack(requiredLength);
    char* pEventBuffer = new char[requiredLength]; // memory is freed in timer handler
    memcpy(pEventBuffer, packedBuffer, requiredLength);
    long timerId = setTimer(0, 0, 0, 0, (void*)pEventBuffer);
    m_toClientTimerId.insert(timerId);
    returnValue=timerId;
  }
  return returnValue;
}

/**
 * ::sendBack
 */
ssize_t APLInterTaskPort::sendBack(GCFEvent& e)
{
  ssize_t returnValue(0);
  
  // send event using a timer event to exit the sending tasks event loop
  unsigned int requiredLength;
  char* packedBuffer = (char*)e.pack(requiredLength);
  char* pEventBuffer = new char[requiredLength]; // memory is freed in timer handler
  memcpy(pEventBuffer, packedBuffer, requiredLength);
  long timerId = setTimer(0, 0, 0, 0, (void*)pEventBuffer);
  m_toServerTimerId.insert(timerId);
  returnValue=timerId;

  return returnValue;
}

/**
 * ::recv
 */
ssize_t APLInterTaskPort::recv(void* /*buf*/, size_t /*count*/)
{
  return 0;
}

GCFEvent::TResult APLInterTaskPort::dispatch(GCFEvent& event)
{
  GCFEvent::TResult status = GCFEvent::NOT_HANDLED;
  if(event.signal == F_TIMER)
  {
    GCFTimerEvent& timerEvent=static_cast<GCFTimerEvent&>(event);

    set<long>::iterator clientIt = m_toClientTimerId.find(timerEvent.id);
    set<long>::iterator serverIt = m_toServerTimerId.find(timerEvent.id);
    
    if(clientIt != m_toClientTimerId.end() || serverIt != m_toServerTimerId.end())
    {
      // allocate enough memory for the GCFEvent object and all member data
      char* packedBuffer = (char*)timerEvent.arg;
      unsigned short signal;
      unsigned int length;
      unsigned int gcfeventlen=sizeof(GCFEvent);
      memcpy(&signal,packedBuffer,sizeof(signal));
      memcpy(&length,packedBuffer+sizeof(signal),sizeof(length));
      char *pEventObject = new char[gcfeventlen+length];
      GCFEvent* pActualEvent = (GCFEvent*)pEventObject;
      if(pActualEvent!=0)
      {
        pActualEvent->signal = signal;
        pActualEvent->length = length;
        memcpy(pEventObject+gcfeventlen,packedBuffer+sizeof(signal)+sizeof(length),length);
        
        if(clientIt != m_toClientTimerId.end())
        {
          status = m_slaveTask.dispatch(*pActualEvent, *this);
          m_toClientTimerId.erase(timerEvent.id);
        }
        else if(serverIt != m_toServerTimerId.end())
        {
          status = _pTask->dispatch(*pActualEvent, *this);
          m_toServerTimerId.erase(timerEvent.id);
        }        
        delete[] pEventObject;
        delete[] packedBuffer;
      }
    }
  }
  if(status==GCFEvent::NOT_HANDLED)
  {
    status=GCFRawPort::dispatch(event);
  }
  return status;
}

