//  CycBufferManager.h: 
//
//  Copyright (C) 2000-2002
//  ASTRON (Netherlands Foundation for Research in Astronomy)
//  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
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
//
//////////////////////////////////////////////////////////////////////

#ifndef CEPFRAME_CYCBUFFERMANAGER_H
#define CEPFRAME_CYCBUFFERMANAGER_H

/*  #ifdef HAVE_CONFIG_H */
/*  #include <config.h> */
/*  #endif */

#include "CEPFrame/CyclicBuffer.h"

/**
  Class CycBufferManager is derived from class DHPoolManager. It main
  purpose is to manage DataHolders in a cyclic buffer.
*/

#include "CEPFrame/DataHolder.h"
#include "CEPFrame/DHPoolManager.h"

class CycBufferManager : public DHPoolManager
{
public:
  /** The constructor.
  */
  CycBufferManager();

  virtual ~CycBufferManager();

  // Returns a write-locked DataHolder pointer from the cyclic buffer
  virtual DataHolder* getWriteLockedDH(int* id);
  // Returns a read-locked DataHolder pointer from the cyclic buffer
  virtual DataHolder* getReadLockedDH(int* id);
  // Returns a read/write-locked DataHolder pointer from the cyclic buffer
  virtual DataHolder* getRWLockedDH(int* id);

  // Release locks
  virtual void writeUnlock(int id);
  virtual void readUnlock(int id);

  virtual void preprocess();
  
  virtual int getSize();

private:
  int                       size;     // Size of buffer
  CyclicBuffer<DataHolder*> itsBuf;   // Buffer containing DataHolders

};


#endif
