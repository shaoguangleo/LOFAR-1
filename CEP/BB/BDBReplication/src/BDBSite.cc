//#  BDBSite.cc: Class that contains information about another Berkeley site
//#
//#  Copyright (C) 2002-2005
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

#include <lofar_config.h>

// General includes
#include <Common/LofarLogger.h>
#include <iostream>

#include <BDBSite.h>

// Application specific includes

using namespace LOFAR;
using namespace std;

INIT_TRACER_CONTEXT(BDBSite, "BDBReplication");

BDBSite::BDBSite(const char* hostName, const int port, Socket* socket)
  :     itsHostName(hostName), 
    itsPort(port),
    itsSocket(socket)
  //itsLock(itsMutex, false),

{
  itsConnectionDataBuffer = new char[sizeof(int) + 2 + itsHostName.size()];
  memset(itsConnectionDataBuffer, 0, sizeof(int) + 2 + itsHostName.size());
  memcpy(itsConnectionDataBuffer, &itsPort, sizeof(int));
  memcpy(itsConnectionDataBuffer + sizeof(int), itsHostName.c_str(), itsHostName.size());

  itsConnectionData.set_data(itsConnectionDataBuffer);
  itsConnectionData.set_size(sizeof(int) + 2 + itsHostName.size());
};

BDBSite:: ~BDBSite()
{
  delete itsConnectionDataBuffer;
  delete itsSocket;
}

bool BDBSite::operator==(BDBSite& other)
{
  return ((itsPort==other.itsPort) && (itsHostName == other.itsHostName));
}

ostream& operator<<(ostream& os, BDBSite& site)
{
  return os<<site.itsHostName<<":"<<site.itsPort;
}

void BDBSite::send(void* buffer, int bufferSize) {
  int send = 0;
  while (send < bufferSize) {
    int sendNow = itsSocket->writeBlocking((void*)((char*)buffer+send), bufferSize-send);
    if (sendNow < 0){
      cerr<<"Error while writing: "<<itsSocket->errstr()<<endl;
    } else {
      send += sendNow;
    }      
  }
}

int BDBSite::recv(void* buffer, int bufferSize) {
  int received = itsSocket->read(buffer, bufferSize);
  if (received == 0) {
    cerr<<"No data present on non-blocking receive"<<endl;
    return 0;
  } else if (received < 0) {    
    cerr<<"Error while receiving: "<<itsSocket->errstr()<<endl;
    return 0;
  } else {
    int recNow = itsSocket->readBlocking((void*)((char*)buffer+received), bufferSize-received);
    if (recNow < 0){
      cerr<<"Error while receiving(blocking): "<<itsSocket->errstr()<<endl;
      return 0;
    } else {
      return bufferSize;
    }      
  }
} 

int BDBSite::recvBlocking(void* buffer, int bufferSize) {
  int ret = itsSocket->readBlocking(buffer, bufferSize);
  if (ret != Socket::SK_OK){
    cerr<<"Error while receiving(non-Blocking): "<<itsSocket->errstr()<<endl;
  };
  return ret;
} 

BDBSiteMap::BDBSiteMap() :
  itsLastEnvId(2)
  //  itsLock(itsMutex, false),
{};
BDBSiteMap::~BDBSiteMap()
{};

void BDBSiteMap::print(ostream& os)
{
  iterator it;
  os<<"SiteMap"<<endl;
  boost::mutex::scoped_lock sl(itsMutex);

  //  lock();
  for (it = itsSiteMap.begin(); it != itsSiteMap.end(); it++) {
    os<<*it->second<<endl;
  }
  //  unlock();
};

void BDBSiteMap::addSite(BDBSite* newSite)
{
  bool found = false;
  boost::mutex::scoped_lock sl(itsMutex);
  //  lock();
  iterator it;
  for (it = this->begin(); it!=this->end(); it++) {
    if (*(it->second) == *newSite) found = true;
  }
  if (!found) {
    itsSiteMap[itsLastEnvId++] = newSite;
    LOG_TRACE_FLOW_STR("New site "<<*newSite);
    LOG_TRACE_FLOW_STR("site added: "<<*newSite);
  } else {
    LOG_TRACE_FLOW_STR("site already exists: "<<*newSite);
    LOG_TRACE_FLOW_STR("Site "<<*newSite<<"already exists");
    delete newSite;
  };
  //  unlock();
  LOG_TRACE_FLOW_STR("SITE ADDED!");
  //  print(cout);
}
