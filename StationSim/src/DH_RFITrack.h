//  DH_RFITrack.h: 
//
//  Copyright (C) 2002
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
//  $Log$
//
//////////////////////////////////////////////////////////////////////

#ifndef STATIONSIM_DH_RFITRACK_H
#define STATIONSIM_DH_RFITRACK_H

#include <Common/lofar_complex.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BaseSim/DataHolder.h"

/**
   This class is the DataHolder holding the RFI tracks for
   the beamformer.
*/

class DH_RFITrack: public DataHolder
{
public:
  typedef double BufferType;

  DH_RFITrack (const string& name, unsigned int maxNtrack);

  virtual ~DH_RFITrack();

  /// Aloocate the buffers.
  virtual void preprocess();

  /// Deallocate the buffers.
  virtual void postprocess();

  /// Get write access to the Buffer in the DataPacket.
  BufferType* getBuffer();
  /// Get read access to the Buffer in the DataPacket.
  const BufferType* getBuffer() const;

  int ntrack() const;

  void setNtrack (int ntrack);

protected:
  // Definition of the DataPacket type.
  class DataPacket: public DataHolder::DataPacket
  {
  public:
    DataPacket() {};

    int        itsNtrack;
    BufferType itsFill;         // to ensure alignment
  };

private:
  /// Forbid copy constructor.
  DH_RFITrack (const DH_RFITrack&);
  /// Forbid assignment.
  DH_RFITrack& operator= (const DH_RFITrack&);


  void*        itsDataPacket;
  BufferType*  itsBuffer;
  unsigned int itsMaxNtrack;
};


inline DH_RFITrack::BufferType* DH_RFITrack::getBuffer()
  { return itsBuffer; }

inline const DH_RFITrack::BufferType* DH_RFITrack::getBuffer() const
  { return itsBuffer; }

inline int DH_RFITrack::ntrack() const
    { return static_cast<DataPacket* const>(itsDataPacket)->itsNtrack; }

inline void DH_RFITrack::setNtrack (int ntrack)
    { static_cast<DataPacket*>(itsDataPacket)->itsNtrack = ntrack; }


#endif
