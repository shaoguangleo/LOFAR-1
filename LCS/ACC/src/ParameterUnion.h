//#  ParameterUnion.h: All parameters of a module with with dflt values and ranges
//#
//#  Copyright (C) 2002-2003
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
//#  Abstract:
//#
//#	 Defines a class the should contain fully filled ParamaterCollections
//#  to be used in runtime to feed the applications with information.
//#
//#  $Id$

#ifndef ACC_PARAMETERUNION_H
#define ACC_PARAMETERUNION_H

#include <lofar_config.h>

//# Includes
#include <ACC/ParameterCollection.h>

namespace LOFAR {
  namespace ACC {


//# Description of class.
// The ParameterUnion class is a ParameterCollection that contains all the
// runtime key-value pairs for an application.
// The restrictions of this collections are:<br>
// 1) The firstline must be a versionnr key with a valid versionnumber.<br>
// 2) No other versionumberkey should be present<br>
//
class ParameterUnion : public ParameterCollection
{
public:
	// Default constructable;
	ParameterUnion();
	~ParameterUnion();

	// Define a conversion function from base class to this class
	ParameterUnion(const ParameterCollection& that);

	// Allow reading a file for backwards compatibility
	explicit ParameterUnion(const string&	theFilename);

	// Copying is allowed.
	ParameterUnion(const ParameterUnion& that);
	ParameterUnion& 	operator=(const ParameterUnion& that);

	// Check if the contents of the ParameterUnion meets the restrictions.
	// The \c errorReport string contains the violations that were found.
	bool check(string&	errorReport) const;

	friend std::ostream& operator<<(std::ostream& os, const ParameterCollection &thePS);
};

} // namespace ACC
} // namespace LOFAR

#endif
