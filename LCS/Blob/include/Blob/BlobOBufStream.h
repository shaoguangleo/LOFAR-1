//# BlobOBufStream.h: Output buffer for a blob using an ostream
//#
//# Copyright (C) 2003
//# ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
//#
//# This file is part of the LOFAR software suite.
//# The LOFAR software suite is free software: you can redistribute it and/or
//# modify it under the terms of the GNU General Public License as published
//# by the Free Software Foundation, either version 3 of the License, or
//# (at your option) any later version.
//#
//# The LOFAR software suite is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
//#
//# $Id$

#ifndef LOFAR_BLOB_BLOBOBUFSTREAM_H
#define LOFAR_BLOB_BLOBOBUFSTREAM_H

// \file
// Output buffer for a blob using an ostream

#include <Blob/BlobOBuffer.h>
#include <iosfwd>

namespace LOFAR {

// \ingroup %pkgname%
  // @{

  // This class is the BlobOBuffer that makes use of an ostream object.
  // The ostream can be any type (ofstream, ostringstream, ...)
  
  class BlobOBufStream : public BlobOBuffer
    {
    public:
      // Construct it with the underlying ostream object.
      explicit BlobOBufStream (std::ostream&);
      
      // Destructor.
      virtual ~BlobOBufStream();
      
      // Put the requested nr of bytes.
      virtual uint64 put (const void* buffer, uint64 nbytes);
      
      // Get the position in the stream.
      // -1 is returned if the stream is not seekable.
      virtual int64 tellPos() const;
      
      // Set the position in the stream.
      // It returns the new position which is -1 if the stream is not seekable.
      virtual int64 setPos (int64 pos);
      
    private:
      std::streambuf* itsStream;
    };

  // @}

} // end namespace

#endif
