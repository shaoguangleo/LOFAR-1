//# DH_Vis.h: Vis DataHolder
//#
//# Copyright (C) 2000, 2001
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, swe@astron.nl
//#
//# $Id$

#ifndef ONLINEPROTO_DH_VIS_H
#define ONLINEPROTO_DH_VIS_H

#include <Transport/DataHolder.h>
#include <complex>

using std::complex;

namespace LOFAR
{
class DH_Vis: public DataHolder
{
public:
  typedef double  BufferPrimitive;
  typedef complex<BufferPrimitive> BufferType;

  explicit DH_Vis (const string& name, const int stations, const int channel, const int polarisations);

  DH_Vis(const DH_Vis&);

  virtual ~DH_Vis();

  DataHolder* clone() const;


  /// Allocate the buffers.
  virtual void init();

  /// Deallocate the buffers.
  virtual void postprocess();

  /// Get write access to the Buffer in the DataPacket.
  BufferType* getBuffer();
  /// Get read access to the Buffer in the DataPacket.
  const BufferType* getBuffer() const;
  BufferType*       getBufferElement(int station1, int station2, int channel, int polarisation);
  void              addBufferElementVal(int station1, int station2, int channel, int polarisation, BufferType value);
  void              setBufferElement(int station1, int station2, int channel, int polarisation, BufferType* valueptr);
  const int         getFBW() const;
  const unsigned int getBufSize() const;

private:
  /// Forbid assignment.
  DH_Vis& operator= (const DH_Vis&);

  BufferType*  itsBuffer;    // array containing frequency spectrum.
  unsigned int itsBufSize;
  int          itsFBW; // number of frequency channels within this beamlet

  int nstations;
  int nchannels;
  int npolarisations;

  void fillDataPointers();
};

inline DH_Vis::BufferType* DH_Vis::getBuffer()
  { return itsBuffer; }

inline const DH_Vis::BufferType* DH_Vis::getBuffer() const
  { return itsBuffer; }

#define VISADDRESS(station1, station2, channel, polarisation)          \
                         (station1)*nstations*nchannels*npolarisations \
                       + (station2)*nchannels*npolarisations           \
                       + (channel)*npolarisations                      \
                       + (polarisation) 

inline DH_Vis::BufferType* DH_Vis::getBufferElement(int station1, int station2, int channel, int polarisation)
  { 
    return itsBuffer + VISADDRESS(station1, station2, channel, polarisation);
  }
 
inline void DH_Vis::setBufferElement(int station1, int station2, int channel, int polarisation, BufferType* valueptr)
{
  *( itsBuffer + VISADDRESS(station1, station2, channel, polarisation)  ) = *valueptr;
};

inline void DH_Vis::addBufferElementVal(int station1, int station2, int channel, int polarisation, BufferType value)
{
  *( itsBuffer + VISADDRESS(station1, station2, channel, polarisation) ) += value;
};

inline const int DH_Vis::getFBW() const
  { return itsFBW; }

inline const unsigned int DH_Vis::getBufSize() const 
  { return itsBufSize; }  

}

#endif 
