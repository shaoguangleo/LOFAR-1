//#  -*- mode: c++ -*-
//#  CalInterface.h: class definition for the Beam Server task.
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

#ifndef CALINTERFACE_H_
#define CALINTERFACE_H_

namespace CAL
{
  class CalInterface
  {
    public:
      //CalInterface() = 0;
      virtual ~CalInterface();
      
      virtual void setPos() = 0;
      virtual void getPos() = 0;
      virtual void setSPW() = 0;
      virtual void getSPW() = 0;
      
      virtual void calibrate() = 0;
  };
};

#endif /* CALINTERFACE_H_ */

