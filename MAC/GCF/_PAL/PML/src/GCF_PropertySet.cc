//#  GCF_PropertySet.cc: 
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

#include <GCF/PAL/GCF_PropertySet.h>
#include <GCF/PAL/GCF_ExtPropertySet.h>
#include <GCF/PAL/GCF_Property.h>
#include <GCF/Utils.h>
#include <GPM_Defines.h>

GCFPropertySet::GCFPropertySet (const char* name,
                                const TPropertySet& typeInfo,
                                GCFAnswer* pAnswerObj) : 
  _pAnswerObj(pAnswerObj),
  _scope(name),
  _propSetInfo(typeInfo),
  _dummyProperty("", this)
  _isBusy(false),  
{
  if (!Utils::isValidPropName(scope.c_str()))
  {
    LOFAR_LOG_WARN(PML_STDOUT_LOGGER, ( 
        "Scope %s meets not the name convention! Set to \"\"",
        scope.c_str()));
    _scope = "";
  }
}

GCFPropertySet::~GCFPropertySet()
{
  clearAllProperties();
  _dummyProperty.resetPropSetRef();
}

void GCFMyPropertySet::loadPropSetIntoRam()
{
  GCFProperty* pProperty;
  const char* propName;
  for (unsigned int i = 0; i < _propSetInfo.nrOfProperties; i++)
  { 
    propName = _propSetInfo.properties[i].propName;
    if (Utils::isValidPropName(propName))
    {
      pProperty = createPropObject(_propSetInfo.properties[i]);
      addProperty(propName, *pProperty);
    }
    else
    {
      LOG_WARN(LOFAR::formatString ( 
          "Property %s meets not the name convention! NOT CREATED",
          propName));      
    }
  }
  if (_pAnswerObj)
  {
    setAnswer(_pAnswerObj);
  }
}

GCFProperty* GCFPropertySet::getProperty (const string propName) const
{
  string shortPropName(propName);
  cutScope(shortPropName);
  
  TPropertyList::const_iterator iter = _properties.find(shortPropName);
  
  if (iter != _properties.end())
  {
    return iter->second;
  }
  else
  {
    return 0;
  }
}

GCFProperty& GCFPropertySet::operator[] (const string propName)
{ 
  GCFProperty* pProperty = getProperty(propName);
  if (!pProperty)
  {
    pProperty = &_dummyProperty;
  }
  return *pProperty;
}

TGCFResult GCFPropertySet::setValue (const string propName, 
                                         const GCFPValue& value)
{
  GCFProperty* pProperty = getProperty(propName);
  if (pProperty)
  {
    return pProperty->setValue(value);    
  }
  else 
  {
    return GCF_PROP_NOT_IN_SET;
  }
}
                             
void GCFPropertySet::setAnswer (GCFAnswer* pAnswerObj)
{
  GCFProperty* pProperty;
  for (TPropertyList::iterator iter = _properties.begin(); 
        iter != _properties.end(); ++iter) 
  {
    pProperty = iter->second;
    assert(pProperty);
    pProperty->setAnswer(pAnswerObj);
  }
  _pAnswerObj = pAnswerObj;
}

bool GCFPropertySet::exists (const string propName) const
{
  GCFProperty* pProperty = getProperty(propName);
  if (pProperty)
  {
    return pProperty->exists();    
  }
  else 
  {
    return false;
  }
}

void GCFPropertySet::configure(const string apcName)
{
  LOG_INFO(LOFAR::formatString ( 
      "REQ: Configure prop. set '%s' with apc '%s'",
      getScope().c_str(), 
      apcName.c_str()));
  GPMController* pController = GPMController::instance();
  assert(pController);
  TPMResult pmResult = pController->configurePropSet(*this, apcName);
  assert(pmResult == PM_NO_ERROR);
}

void GCFPropertySet::configured(TGCFResult result, const string& apcName)
{
  if (_pAnswerObj != 0)
  {
    GCFConfAnswerEvent e(sig);
    e.pScope = getScope().c_str();
    e.pApcName = apcName.c_str();
    e.result = result;
    _pAnswerObj->handleAnswer(e);
  }
}

void GCFPropertySet::addProperty(const string& propName, GCFProperty& prop)
{
  assert(propName.length() > 0);
  
  string shortPropName(propName);
  cutScope(shortPropName);
  
  TPropertyList::iterator iter = _properties.find(shortPropName);
  if (iter == _properties.end())
  {
    _properties[shortPropName] = &prop;
  }
}

bool GCFPropertySet::cutScope(string& propName) const 
{
  bool scopeFound(false);
  
  if (propName.find(_scope) == 0)
  {
    // plus 1 means erase the GCF_PROP_NAME_SEP after scope too
    propName.erase(0, _scope.size() + 1); 
    scopeFound = true;
  }
  
  return scopeFound;
}

void GCFPropertySet::clearAllProperties()
{
  GCFProperty* pProperty;
  for (TPropertyList::iterator iter = _properties.begin(); 
        iter != _properties.end(); ++iter) 
  {
    pProperty = iter->second;
    assert(pProperty);
    pProperty->resetPropSetRef();
    delete pProperty;
  }
}

void GCFPropertySet::dispatchAnswer(unsigned short sig, TGCFResult result)
{
  if (_pAnswerObj != 0)
  {
    GCFPropSetAnswerEvent e(sig);
    e.pScope = getScope().c_str();
    e.result = result;
    _pAnswerObj->handleAnswer(e);
  }
}

