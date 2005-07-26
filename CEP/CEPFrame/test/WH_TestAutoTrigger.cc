//  WH_TestAutoTrigger.cc:
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
//  $Id$
//
//
//////////////////////////////////////////////////////////////////////

#include <lofar_config.h>

#include <WH_TestAutoTrigger.h>

namespace LOFAR
{

  WH_TestAutoTrigger::WH_TestAutoTrigger (const string& name)
  : WorkHolder       (1, 1, name,"WH_TestAutoTrigger"),
    itsEventCount(0)
{
  getDataManager().addInDataHolder(0, new DH_Tester("in"));
  getDataManager().addOutDataHolder(0, new DH_Tester("out"));

  // switch both the input and output channel triggers off
  getDataManager().setAutoTriggerIn(0, false);
  getDataManager().setAutoTriggerOut(0, false);
}


WH_TestAutoTrigger::~WH_TestAutoTrigger()
{}

WorkHolder* WH_TestAutoTrigger::construct (const string& name, int, int,
					   const KeyValueMap&)
{
  return new WH_TestAutoTrigger (name);
}

WH_TestAutoTrigger* WH_TestAutoTrigger::make (const string& name)
{
  return new WH_TestAutoTrigger (name);
}

void WH_TestAutoTrigger::process()
{
  itsEventCount++;
  ((DH_Tester*)getDataManager().getOutHolder(0))->setCounter(itsEventCount);
  if (itsEventCount%3 == 0) {
    getDataManager().readyWithInHolder(0);
    getDataManager().readyWithOutHolder(0);
  } else {
    cout << endl;
  }
}

void WH_TestAutoTrigger::dump() const
{
   cout << "Cnt=" << itsEventCount << ",  Input buffer=";
   cout << ((DH_Tester*)getDataManager().getInHolder(0))->getCounter() << "   ";

}

}
