//# DataManager.h: Data manager which incorporates asynchronous behaviour
//#
//# Copyright (C) 2000-2002
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//# $Id$ 

#ifndef CEPFRAME_DATAMANAGER_H
#define CEPFRAME_DATAMANAGER_H

//# Never #include <config.h> or #include <lofar_config.h> in a header file!

// \file
// Offers an interface to the data and takes care of buffering. 

#include <tinyCEP/TinyDataManager.h>
#include <Transport/DataHolder.h>
#include <Transport/Connection.h>
#include <Common/lofar_vector.h>

namespace LOFAR
{

// \addtogroup CEPFrame
// @{

typedef struct 
{
  DataHolder* currentDH;
  int         id;             // id of dataholder in DHPoolManager
  Connection* connection;     // Connection
} DH_info;

/**
  Class DataManager is the base class for all data managers
  in the CEPFrame environment. It main purpose is to offer a common interface
  to a class like WorkHolder. Apart from that it also offers some common
  functionality to the classes derived from it.
*/

class SynchronisityManager;

class DataManager : public TinyDataManager
{
public:
  /** The constructor with the number of input and output
      DataHolders as arguments.
  */
  DataManager (int inputs=0, int outputs=0);
  DataManager (const TinyDataManager&);
  virtual ~DataManager();
  
  DataHolder* getInHolder(int channel);
  DataHolder* getOutHolder(int channel);
  void readyWithInHolder(int channel);
  void readyWithOutHolder(int channel);
  
  void preprocess();
  void postprocess();

  // Get/set the in/out connection per channel
  Connection* getInConnection(int channel) const;
  void setInConnection(int channel, Connection* conn);
  Connection* getOutConnection(int channel) const;
  void setOutConnection(int channel, Connection* conn);

  const string& getInHolderName(int channel) const;
  const string& getOutHolderName(int channel) const; 

  DataHolder* getGeneralInHolder(int channel) const;//Use these functions only 
                                                    // for acquiring of general
                                                    // properties
  DataHolder* getGeneralOutHolder(int channel) const; 

  void initializeInputs();  ///Reads all inputs

  // N.B.The following methods should only be used when defining an application,
  // not during processing.

  // Set properties of a communication channel: synchronisity and sharing of DataHolders
  // by input and output
  // The following methods are deprecated, they do not allow setting the buffersize ande
  // they combine two different concepts (sharing and buffering)
  void setInBufferingProperties(int channel, bool synchronous, 
				bool shareDHs=false) const;
  void setOutBufferingProperties(int channel, bool synchronous, 
				 bool shareDHs=false) const;

  // Set buffer sizes on a channel. 
  // A buffer size of 1 is not the same as no buffer at all
  // therefore there are two arguments: bool synchonous and if synchronous = false then
  // also give the buffersize
  void setInBuffer(int channel, bool synchronous, int bufferSize = 0) const;
  void setOutBuffer(int channel, bool synchronous, int bufferSize = 0) const;

  // limit the number of concurrent writers in a group of output channels
  void setOutRoundRobinPolicy(vector<int> channels, unsigned maxConcurrent);
  void setInRoundRobinPolicy(vector<int> channels, unsigned maxConcurrent);

  // Is data transport of input channel synchronous?
  bool isInSynchronous(int channel);
  // Is data transport of output channel synchronous?
  bool isOutSynchronous(int channel);

  // return usage of buffers on a scale between 0 (nothing used) and 1 (all
  // buffers used).
  float getAndResetMaxInBufferUsage(int channel);
  float getAndResetMaxOutBufferUsage(int channel);

private:
  /// Copy constructor (copy semantics).
  DataManager (const DataManager&);

  SynchronisityManager* itsSynMan;
  DH_info* itsInDHsinfo;        // Last requested inholders information
  DH_info* itsOutDHsinfo;       // Last requested outholders information

};

// @}

} // namespace
#endif
