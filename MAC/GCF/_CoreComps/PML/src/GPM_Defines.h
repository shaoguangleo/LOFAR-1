//#  GPM_Defines.h: preprocessor definitions of various constants
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
//#  $Id$

#ifndef GPM_DEFINES_H
#define GPM_DEFINES_H

#include <GCFCommon/GCF_Defines.h>

#define CORE_COMPS_PML_LOGGER  (GCF_LOGGER_ROOT + string(".CoreComps.PML"))

#define PML_STDOUT_LOGGER      (CORE_COMPS_SAL_LOGGER + string(".Logger"))

enum TPMResult {
  PM_NO_ERROR, 
  PM_UNKNOWN_ERROR,
  PM_PA_NOTCONNECTED,
  PM_IS_BUSY,
  PM_SCOPE_ALREADY_EXISTS,
  PM_SCOPE_NOT_EXISTS, 
  PM_PROP_NOT_EXISTS,
  PM_SCADA_ERROR,
  PM_PROP_NOT_VALID,
  PM_PROP_WRONG_TYPE,
  PM_PROP_LINK_NOT_IN_SYNC,
  PM_PROP_SET_BUSY,
  PM_PROP_LIST_FAILURE,
  PM_PROP_ALREADY_LINKED,
  PM_PROP_NOT_LINKED,
  PM_PROP_NOT_IN_SET,
  PM_SCOPE_NOT_FOUND
};

#endif
