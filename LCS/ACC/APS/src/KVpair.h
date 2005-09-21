//#  KVpair.h: Implements a KV pair as a pair<string, string>.
//#
//#  Copyright (C) 2002-2004
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
//#  Note: This source is read best with tabstop 4.
//#
//#  $Id$

#ifndef LOFAR_APS_KVPAIR_H
#define LOFAR_APS_KVPAIR_H

// \file KVpair.h
// Implements a KV pair as a pair<string, string>.

//# Never #include <config.h> or #include <lofar_config.h> in a header file!
//# Includes
#include <Common/lofar_map.h>

namespace LOFAR {
  namespace ACC {
    namespace APS {
// \addtogroup APS
// @{

// Implements a KV pair as a pair<string, string>.
class KVpair : public pair<string, string>
{
public:
	KVpair(const string& aKey, const string& aValue, bool genTimestamp = false);
	KVpair(const string& aKey, bool			 aValue, bool genTimestamp = false);
	KVpair(const string& aKey, int			 aValue, bool genTimestamp = false);
	KVpair(const string& aKey, double		 aValue, bool genTimestamp = false);
	KVpair(const string& aKey, float		 aValue, bool genTimestamp = false);
	KVpair(const string& aKey, time_t		 aValue, bool genTimestamp = false);

	~KVpair();

	// Copying is allowed
	KVpair(const KVpair&	that);
	KVpair& operator=(const KVpair& that);
};

// @} addgroup
    } // namespace APS
  } // namespace ACC
} // namespace LOFAR

#endif
