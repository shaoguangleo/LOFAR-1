//  WH_DataGenerator.cc: An empty WorkHolder (doing nothing)
//
//  Copyright (C) 2000, 2001
//  ASTRON (Netherlands Foundation for Research in Astronomy)
//  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//////////////////////////////////////////////////////////////////////

#include <StationSim/WH_DataGenerator.h>
#include <Common/Debug.h>


WH_DataGenerator::WH_DataGenerator (const string& name, unsigned int nrcu)
: WorkHolder       (1, 1, name,"WH_DataGenerator"),
  itsOutDataHolder ("out", nrcu, 1),
  itsNrcu          (nrcu)
{

}

WH_DataGenerator::~WH_DataGenerator()
{}

WorkHolder* WH_DataGenerator::construct (const string& name, int, int,
				 const ParamBlock&)
{
  return new WH_DataGenerator (name, 1);
}

WH_DataGenerator* WH_DataGenerator::make (const string& name) const
{
  return new WH_DataGenerator (name, itsNrcu);
}

void WH_DataGenerator::process()
{}

void WH_DataGenerator::dump() const
{
  TRACER2("WH_DataGenerator Dump");
}


DataHolder* WH_DataGenerator::getInHolder (int)
{
  return &itsInDataHolder; 
}

DataHolder* WH_DataGenerator::getOutHolder (int)
{
  return &itsOutDataHolder;
}


/**
   This example getMonitorValue method pruduces output as follows:
   "hi"  : std output text including the WorkHolder's name; return 0
   "one" : return 1
 */
int WH_DataGenerator::getMonitorValue(const char* name){
  TRACER2("Called WH_DataGenerator::getMonitorValue");
  int result =0;
  if (strcmp(name,"hi") == 0) {
    cout << "Hi I'm " << getName() << endl;  
  } else if (strcmp(name,"one") == 0) {
    result = 1;
  }
  TRACER2("WH_DataGenerator::getMonitorValue resturns " << result);
  return result;
}
