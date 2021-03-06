//# SubbandMetaData.h
//# Copyright (C) 2012-2013  ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O. Box 2, 7990 AA Dwingeloo, The Netherlands
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

#ifndef LOFAR_INTERFACE_SUBBAND_META_DATA_H
#define LOFAR_INTERFACE_SUBBAND_META_DATA_H

#include <vector>

#include <Common/LofarLogger.h>
#include <Stream/Stream.h>
#include <CoInterface/SparseSet.h>

namespace LOFAR
{
  namespace Cobalt
  {

    struct SubbandMetaData
    {
    public:
      typedef SparseSet<unsigned> flags_type;

      SubbandMetaData(size_t nrTABs = 0);

      struct beamInfo {
        double delayAtBegin;
        double delayAfterEnd;
      };

      // delays for all directions
      struct beamInfo stationBeam;
      std::vector<struct beamInfo> TABs;

      // flag set.
      flags_type flags;

      void read(Stream *str);
      void write(Stream *str) const;

      // Maximum size of the buffer to marshall flags
      static const size_t MAXFLAGSIZE     = 8192 + 4;

      // Maximum number of TABs we'll support when marshalling
      static const size_t MAXNRTABS       = 512;

      // Maximum number of bytes write() will produce
      static const size_t MAXMARSHALLSIZE = MAXFLAGSIZE
                                          + sizeof(struct beamInfo)
                                          + sizeof(size_t)
                                          + MAXNRTABS * sizeof(struct beamInfo);
    };


    inline SubbandMetaData::SubbandMetaData(size_t nrTABs)
      :
      TABs(nrTABs)
    {
    }


    inline void SubbandMetaData::read(Stream *str)
    {
      // read station beam
      str->read(&stationBeam, sizeof stationBeam);

      // read TABs
      size_t nrTABs;
      str->read(&nrTABs, sizeof nrTABs);
      TABs.resize(nrTABs);
      if (nrTABs > 0 ) {
        str->read(&TABs[0], nrTABs * sizeof TABs[0]);
      }

      // read flags
      std::vector<char> flagsBuffer(MAXFLAGSIZE);
      str->read(&flagsBuffer[0], flagsBuffer.size());
      flags.unmarshall(&flagsBuffer[0]);
    }


    inline void SubbandMetaData::write(Stream *str) const
    {
      // write station beam
      str->write(&stationBeam, sizeof stationBeam);

      // write TABs
      size_t nrTABs = TABs.size();
      str->write(&nrTABs, sizeof nrTABs);
      if (nrTABs > 0) {
        ASSERTSTR(nrTABs < MAXNRTABS, "Metadata buffers support up to " << MAXNRTABS << " TABs, but specification contains " << nrTABs);

        str->write(&TABs[0], nrTABs * sizeof TABs[0]);
      }

      // write flags
      std::vector<char> flagsBuffer(MAXFLAGSIZE);

      ssize_t size = flags.marshall(&flagsBuffer[0], flagsBuffer.size());
      if (size < 0) {
        LOG_DEBUG_STR("Error marshalling flags into buffer of size " << MAXFLAGSIZE << ", compressing flags");

        // Span one flag set from the first to the last entry
        const flags_type::Ranges &ranges = flags.getRanges();
        const flags_type::range first = ranges[0];
        const flags_type::range last  = ranges[ranges.size()-1];

        flags_type newFlags;
        newFlags.include(first.begin,last.end);

        size = newFlags.marshall(&flagsBuffer[0], flagsBuffer.size());

        ASSERTSTR(size >= 0, "Cannot marshall the compressed flags.");
      }

      str->write(&flagsBuffer[0], flagsBuffer.size());
    }

  } // namespace Cobalt
} // namespace LOFAR

#endif

