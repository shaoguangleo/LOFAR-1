//#  ACSyncClient.h: Synchrone version of a Appl. Control client
//#
//#  Copyright (C) 2004
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
//#	 This class implements the client API for using an Application 
//#  Controller in synchrone mode. 
//#
//#  $Id$

#ifndef ACC_ACSYNCCLIENT_H
#define ACC_ACSYNCCLIENT_H

#include <lofar_config.h>

//# Includes
#include <ACC/ApplControlClient.h>
#include <ACC/ACSyncClient.h>

namespace LOFAR {
  namespace ACC {



//# Description of class.
// The ACSyncClient class implements the interface of the Application Controller
// for synchroon communication.
//
class ACSyncClient : public ApplControlClient 
{
public:
	// When constructing an ApplControllerClient object an Application
	// Controller process is started on a runtime determined host.
	// The \c hostIDFrontEnd argument of th constructor is the hostname
	// of the machine on which the AC-daemon runs which launches the AC
	// for this ApplControlClient object. <br>
	// The returned AC object knows who its AC is and is already connected to 
	// it. 
	explicit ACSyncClient(const string&	hostIDFrontEnd);

	// Destructor;
	virtual ~ACSyncClient();

	// Always returns 'false'.
	inline bool isAsync() const 
		{ return (false);	}

private:
	// NOT default constructable;
	ACSyncClient();

	// Copying is also not allowed.
	ACSyncClient(const ACSyncClient& that);

	// Copying is also not allowed.
	ACSyncClient& 	operator=(const ACSyncClient& that);

};


} // namespace ACC
} // namespace LOFAR

#endif
