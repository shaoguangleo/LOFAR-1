//#  ACRequest.h: small structure used for comm. with ACDaemon
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
//#  $Id$

#ifndef ACC_FILENAME_H
#define ACC_FILENAME_H

//# Never #include <config.h> or #include <lofar_config.h> in a header file!

//# Includes
//#include <otherpackage/file.h>

namespace LOFAR {
  namespace ACC {

#define	ACREQUESTNAMESIZE 80

// The ACRequest structure is exchanged with the ACDaemon to request an
// Application Controller.
struct ACRequest
{
	// Uniq request information sent by the client
	char	itsRequester [ACREQUESTNAMESIZE];

	// Address of the machine the AP is started on (htonl format)
	uint32	itsAddr;			// in_addr_t

	// portnr the AP will be listening on (htons format)
	uint16	itsPort;			// in_port_t
};

  } // namespace ACC
} // namespace LOFAR

#endif
