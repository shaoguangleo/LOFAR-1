//#  GPM_Property.h: 
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

#ifndef GPM_PROPERTY_H
#define GPM_PROPERTY_H

#include <SAL/GCF_PValue.h>
#include <GCFCommon/GCF_Defines.h>
#include "GPM_Defines.h"

class GPMPropertySet;

class GPMProperty
{
  public:
    GPMProperty(GCFPValue::TMACValueType type, string name = "");
    GPMProperty(const TProperty& propertyFields);
    ~GPMProperty();
    
  private:
    friend class GPMPropertySet;
    TPMResult setValue(const GCFPValue& value);
    TPMResult getValue(GCFPValue& value, bool curValue = true) const;
    GCFPValue* getValue(bool curValue = true) const;
    TPMResult setLink(bool link);
    inline bool isLinked() const {return _isLinked;}
    void setAccessMode(TAccessMode mode, bool on);
    bool testAccessMode(TAccessMode mode) const;   
    inline const string& getName() const {return _name;}
           
  private:
    TAccessMode _accessMode;
    GCFPValue* _pCurValue;
    GCFPValue* _pOldValue;
    bool  _isLinked;
    string _name;
};
#endif
