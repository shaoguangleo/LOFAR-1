//#  ParmWriter.h: Writes/updates parameters in ParmTables
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

#ifndef LOFAR_BBSCONTROL_PARMWRITER_H
#define LOFAR_BBSCONTROL_PARMWRITER_H

//# Never #include <config.h> or #include <lofar_config.h> in a header file!

// \file
// Writes/updates parameters in ParmTables.

//# Includes

namespace LOFAR
{
  //# Forward Declarations
  class ParmDataInfo;
  class MeqMatrix;
  namespace ParmDB { class ParmValueRep; }

  namespace BBS
  {
    // \addtogroup BBS
    // @{

    // Description of class.
    class ParmWriter
    {
    public:
      // Constructor
      ParmWriter();

      // Destructor
      ~ParmWriter();

      void write (const ParmDataInfo& pData,
		  double fStart, double fEnd, 
		  double tStart, double tEnd);

    private:
      void setCoeff (ParmDB::ParmValueRep& pval, const MeqMatrix& coeff);
    };

    // @}

  } // namespace BBS

} // namespace LOFAR

#endif
