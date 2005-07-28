//#  MaintenanceVIFactory.h: factory class for Maintenance Virtual Instruments.
//#
//#  Copyright (C) 2002-2005
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

#ifndef MaintenanceFactory_H
#define MaintenanceFactory_H

//# Includes
#include <APLCommon/LogicalDeviceFactory.h>

//# local includes
#include "MaintenanceVI.h"
//# Common Includes

// forward declaration

namespace LOFAR
{
  
namespace AVI // A)pplication layer V)irtual I)nstrument
{
  class MaintenanceVIFactory : public APLCommon::LogicalDeviceFactory
  {
    public:

      MaintenanceVIFactory() {}; 
      virtual ~MaintenanceVIFactory() {};
      
      virtual boost::shared_ptr<APLCommon::LogicalDevice> createLogicalDevice(const string& taskName, 
                                                                              const string& parameterFile,
                                                                              GCF::TM::GCFTask* pStartDaemon)
      {
        return boost::shared_ptr<APLCommon::LogicalDevice>(new MaintenanceVI(taskName, parameterFile, pStartDaemon));
      };

    protected:
      // protected copy constructor
      MaintenanceVIFactory(const MaintenanceVIFactory&);
      // protected assignment operator
      MaintenanceVIFactory& operator=(const MaintenanceVIFactory&);

    private:
    
  };
};//AVI
};//LOFAR
#endif
