//#  OTDBaction.h: Action an operator takes on an OTDBevent.
//#
//#  Copyright (C) 2002-2004
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, softwaresupport@astron.nl
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

#ifndef LOFAR_OTDB_OTDBACTION_H
#define LOFAR_OTDB_OTDBACTION_H

// \file
// Action an operator takes on an OTDBevent.

//# Never #include <config.h> or #include <lofar_config.h> in a header file!
//# Includes
#include <OTDB/OTDBtypes.h>

namespace LOFAR {
  namespace OTDB {

// \addtogroup OTDB
// @{

//# --- Forward Declarations ---
//# classes mentioned as parameter or returntype without virtual functions.
//# class ...;


// Action an operator takes on an OTDBevent.
// ...
class OTDBaction
{
public:
	OTDBaction();
	~OTDBaction();

	// Copying is allowed
	OTDBaction(const OTDBaction&	that);

	OTDBaction& operator=(const OTDBaction& that);

private:
	//# --- Datamembers ---
	actionIDType	itsID;
	eventIDType		itsEventID;
	ptime			itsTimestamp;
	string			itsOriginator;
	actionType		itsType;
	string			itsDescription;
};

//# --- Inline functions ---



// @}
  } // namespace OTDB
} // namespace LOFAR

#endif
