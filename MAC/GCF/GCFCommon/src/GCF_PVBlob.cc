//#  GCF_PVBlob.cc: 
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


#include <GCF/GCF_PVBlob.h>
#include <Common/DataConvert.h>

namespace LOFAR 
{
 namespace GCF 
 {
  namespace Common 
  {

GCFPVBlob::GCFPVBlob(unsigned char* val, uint16 length, bool clone) 
  : GCFPValue(LPT_BLOB), _value(val), _length(length), _isDataHolder(clone)
{
  if (clone)
  {
    _value = new unsigned char[length];
    memcpy(_value, val, length);
  }
}

unsigned int GCFPVBlob::unpackConcrete(const char* valBuf)
{
  unsigned int unpackedBytes(0);
  // first read length field
  memcpy((void *) &_length, valBuf, sizeof(_length));
  unpackedBytes += sizeof(_length);

  if (mustConvert()) LOFAR::dataConvert(LOFAR::dataFormat(), &_length, 1);
  
  // if it is the data holder the blob buffer space must be freed first
  if (_isDataHolder)
  {
    delete [] _value;
  }
  _isDataHolder = true;
  // create new blob buffer space (now it becames a data holder)
  _value = new unsigned char[_length];
  // copies the data
  memcpy(_value, (unsigned char*) valBuf + unpackedBytes, _length);   
  unpackedBytes += _length;
  
  return unpackedBytes;
}

unsigned int GCFPVBlob::packConcrete(char* valBuf) const
{
  unsigned int packedBytes(0);

  memcpy(valBuf, (void *) &_length, sizeof(_length)); // packs the length field
  memcpy(valBuf + sizeof(_length), (void *) _value, _length); // packs the blob data
  packedBytes += sizeof(_length) + _length;
  
  return packedBytes;
}

TGCFResult GCFPVBlob::setValue(unsigned char* value, uint16 length, bool clone)
{ 
  TGCFResult result(GCF_NO_ERROR);
  _length = length; 
  if (_isDataHolder)
  {
    delete [] _value;
  }
  if (clone && value && length)
  {
    _value = new unsigned char[_length];
    memcpy(_value, value, _length);   
  }
  else
  {
    _value = value; 
  }
  _isDataHolder = (clone && value && length);
  return result;
}
 
TGCFResult GCFPVBlob::setValue(const string& value)
{
  TGCFResult result(GCF_NO_ERROR);

  _length = value.length();
  if (_isDataHolder)
  {
    delete [] _value;
  }
  _value = new unsigned char[_length];
  memcpy(_value, value.c_str(), _length);   
 
  _isDataHolder = true;
  
  return result;
}

string GCFPVBlob::getValueAsString(const string& format)
{
}

GCFPValue* GCFPVBlob::clone() const
{
  GCFPValue* pNewValue = new GCFPVBlob(_value, _length, true);
  return pNewValue;
}

TGCFResult GCFPVBlob::copy(const GCFPValue& newVal)
{
  TGCFResult result(GCF_NO_ERROR);

  if (newVal.getType() == getType())
  {
    if (_isDataHolder)
    {
      delete [] _value;
    }
    _length = ((const GCFPVBlob *)&newVal)->getLen();
    _value = new unsigned char[_length];
    memcpy(_value, ((const GCFPVBlob *)&newVal)->getValue(), _length);   
    _isDataHolder = true;
  }
  else
    result = GCF_DIFFERENT_TYPES;
  
  return result;
}
  } // namespace Common
 } // namespace GCF
} // namespace LOFAR
