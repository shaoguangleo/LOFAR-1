//#  RSP: RSP data format
//#
//#  Copyright (C) 2008
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

#ifndef LOFAR_CS1_ION_PROC_RSP_H
#define LOFAR_CS1_ION_PROC_RSP_H


namespace LOFAR {
namespace CS1 {

#include <cstddef>


// All data is in Little Endian format!

struct RSP {
  struct header {
    uint16_t version;
    uint16_t configuration;
    uint16_t station;
    uint8_t  nrBeamlets;
    uint8_t  nrBlocks;
    uint32_t timestamp;
    uint32_t blockSequenceNumber;
  } header;

  char       data[8130];
};

} // namespace CS1
} // namespace LOFAR

#endif
