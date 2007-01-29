//#  ParameterSet.h: Implements a map of Key-Value pairs.
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
//#  Note: This source is read best with tabstop 4.
//#
//#  $Id$

#ifndef LOFAR_APS_PARAMETERSET_H
#define LOFAR_APS_PARAMETERSET_H

// \file
// Implements a map of Key-Value pairs.

//# Never #include <config.h> or #include <lofar_config.h> in a header file!
//# Includes
#include <APS/ParameterSetImpl.h>

namespace LOFAR {
  namespace ACC {
    namespace APS {
// \addtogroup APS
// @{

//# Description of class.
// The ParameterSet class is a key-value implementation of the type
// map<string, string>. 
// This means that values are stored as a string which allows easy merging and
// splitting of ParameterSets because no conversions have to be done.
// A couple of getXxx routines are provided to convert the strings to the 
// desired type.
//
class ParameterSet
{
public:
	typedef ParameterSetImpl::iterator			iterator;
	typedef ParameterSetImpl::const_iterator		const_iterator;

	// \name Construction and Destruction
	// A ParameterSet can be constructed as empty collection, can be
	// read from a file or copied from another collection. 
	// @{

	// Create an empty collection. The optional argument \a mode
	// determines how keys should be compared.
	explicit ParameterSet(KeyCompare::Mode	mode = KeyCompare::NORMAL);

	// Destroy the contents.
	~ParameterSet();

	// Construct a ParameterSet from the contents of \a theFilename. The
	// optional argument \a mode determines how keys should be compared.
 	explicit ParameterSet(const string&		theFilename,
			      KeyCompare::Mode	mode = KeyCompare::NORMAL);


	// Copying is allowed.
	ParameterSet(const ParameterSet& that);

	// Copying is allowed.
	ParameterSet& 	operator=(const ParameterSet& that);
	//@}

	// Iteration.
	//@{
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	//@}


	// Key comparison mode.
	KeyCompare::Mode keyCompareMode() const;

	// Clear the set.
	void clear();

	// \name Merging or appending collections
	// An existing collection can be extended/merged with another collection.
	// @{

	// Adds the Key-Values pair in the given file to the current
	// collection. Each key will be prefixed with the optional argument \a
	// thePrefix.
	void	adoptFile      (const string&               theFilename,
				const string&               thePrefix = "");

	// Adds the Key-Values pair in the given buffer to the current
	// collection. Each key will be prefixed with the optional argument \a
	// thePrefix.
	void	adoptBuffer    (const string&               theBuffer,
				const string&               thePrefix = "");

	// Adds the Key-Values pair in the given collection to the current 
	// collection. Each key will be prefixed with the optional argument \a
	// thePrefix.
	// \attention Do \b not pass \c *this as first argument. This will
	// result in undefined behaviour and likely cause a segmentation fault
	// or an infinite loop.
	void	adoptCollection(const ParameterSet&         theCollection,
				const string&               thePrefix = "");
	// @}


	// \name Saving the collection
	// The map of key-value pair can be saved in a file or a string.
	// @{

	// Writes the Key-Values pair from the current ParCollection to the file.
	void	writeFile   (const string& theFilename, bool append = false) const;

	// Writes the Key-Values pair from the current ParCollection to the 
	// string buffer.
	void	writeBuffer (      string& theBuffer) const;
	//@}

	// \name Make subsets
	// A subset from the current collection can be made based on the prefix
	// of the keys in the collection.
	// @{

	// Creates a subset from the current ParameterSet containing all the 
	// parameters that start with the given baseKey. The baseKey is cut off 
	// from the Keynames in the created subset, the optional prefix is put
	// before all keys in the subset.
	ParameterSet	makeSubset(const string& baseKey,
								   const string& prefix = "") const;
	// @}

	
	// \name Handling single key-value pairs
	// Single key-value pairs can ofcourse be added, replaced or removed from 
	// a collection.
	// @{

	// Add the given pair to the collection. When the \c aKey does not exist 
	// in the collection an exception is thrown.
	void	add    (const string& aKey, const string& aValue);
	void	add    (const KVpair& aPair);

	// Replaces the given pair in the collection. If \c aKey does not exist in
	// the collection the pair is just added to the collection.
	void	replace(const string& aKey, const string& aValue);
	void	replace(const KVpair& aPair);

	// Removes the pair with the given key. Removing a non-existing key is ok.
	void	remove (const string& aKey);
	// @}

	// \name Searching and retrieving
	// The following functions support searching the collection for existance
	// of given keys an the retrieval of the corresponding value. In the getXxx
	// retrieve functions the stored string-value is converted to the wanted
	// type.
	// @{

	// Checks if the given Key is defined in the ParameterSet.
	bool	isDefined (const string& searchKey) const;

	// Searches for a module whose short nam is given and returns it position
	// in the parameterSet.
	string	locateModule(const string&	shortName) const;

	// Returns the value as a boolean.
	bool	getBool  (const string& aKey) const;
	int16	getInt16 (const string& aKey) const;
	uint16	getUint16(const string& aKey) const;
	int32	getInt32 (const string& aKey) const;
	uint32	getUint32(const string& aKey) const;
#if HAVE_LONG_LONG
	int64	getInt64 (const string& aKey) const;
	uint64	getUint64(const string& aKey) const;
#endif
	float	getFloat (const string& aKey) const;
	double	getDouble(const string& aKey) const;
	string	getString(const string& aKey) const;
	// Returns the value as a time value (seconds since 1970).
	time_t	getTime  (const string& aKey) const;

	// Returns the value as an integer
	vector<bool>	getBoolVector  (const string& aKey) const;
	vector<int16>	getInt16Vector (const string& aKey) const;
	vector<uint16>	getUint16Vector(const string& aKey) const;
	vector<int32>	getInt32Vector (const string& aKey) const;
	vector<uint32>	getUint32Vector(const string& aKey) const;
#if HAVE_LONG_LONG
	vector<int64>	getInt64Vector (const string& aKey) const;
	vector<uint64>	getUint64Vector(const string& aKey) const;
#endif
	vector<float>	getFloatVector (const string& aKey) const;
	vector<double>	getDoubleVector(const string& aKey) const;
	vector<string>	getStringVector(const string& aKey) const;
	vector<time_t>	getTimeVector  (const string& aKey) const;

	// @}

	// \name Printing
	// Mostly for debug purposes the collection can be printed.
	// @{

	// Allow printing the whole parameter collection.
	friend std::ostream& operator<<(std::ostream& os, const ParameterSet &thePS);
	// @}

private:
	// Construct from an existing impl object.
	explicit ParameterSet (ParameterSetImpl* set)
	  { itsSet = set; }

	// Decrement the refcount and delete the impl if the count is zero.
	void unlink();

	ParameterSetImpl* itsSet;
};


// Make one instance of the parameterSet globally accessable.
ParameterSet* 	globalParameterSet();

//#
//# ---------- inline functions ----------
//#
inline ParameterSet::iterator	ParameterSet::begin      ()
{
	return itsSet->begin();
}
inline ParameterSet::iterator	ParameterSet::end      ()
{
	return itsSet->end();
}
inline ParameterSet::const_iterator	ParameterSet::begin      () const
{
	return itsSet->begin();
}
inline ParameterSet::const_iterator	ParameterSet::end      () const
{
	return itsSet->end();
}

inline KeyCompare::Mode	ParameterSet::keyCompareMode	() const
{
	return itsSet->keyCompareMode();
}

inline void	ParameterSet::clear      ()
{
	itsSet->clear();
}

inline void	ParameterSet::adoptFile	(const string& theFilename, const string& thePrefix)
{
	itsSet->adoptFile (theFilename, thePrefix);
}

inline void	ParameterSet::adoptBuffer	(const string& theBuffer, const string& thePrefix)
{
	itsSet->adoptBuffer (theBuffer, thePrefix);
}

inline void	ParameterSet::adoptCollection	(const ParameterSet& theCollection, const string& thePrefix)
{
	itsSet->adoptCollection (*theCollection.itsSet, thePrefix);
}

inline void	ParameterSet::writeFile   (const string& theFilename, bool append) const
{
	itsSet->writeFile (theFilename, append);
}

inline void	ParameterSet::writeBuffer (      string& theBuffer) const
{
	itsSet->writeBuffer (theBuffer);
}

inline ParameterSet	ParameterSet::makeSubset(const string& baseKey,
								   const string& prefix) const
{
	return ParameterSet (itsSet->makeSubset(baseKey, prefix));
}

inline void	ParameterSet::add    (const string& aKey, const string& aValue)
{
	itsSet->add (aKey, aValue);
}
inline void	ParameterSet::add    (const KVpair& aPair)
{
	itsSet->add (aPair);
}

inline void	ParameterSet::replace(const string& aKey, const string& aValue)
{
	itsSet->replace (aKey, aValue);
}
inline void	ParameterSet::replace(const KVpair& aPair)
{
	itsSet->replace (aPair);
}

inline void	ParameterSet::remove (const string& aKey)
{
	itsSet->remove (aKey);
}

inline bool	ParameterSet::isDefined (const string& searchKey) const
{
	return itsSet->isDefined (searchKey);
}

inline string	ParameterSet::locateModule(const string&	shortName) const
{
	return (itsSet->locateModule(shortName));
}

//#	getBool(key)
inline bool ParameterSet::getBool(const string& aKey) const
{
	return itsSet->getBool(aKey);
}

//#	getInt16(key)
inline int16 ParameterSet::getInt16(const string& aKey) const
{
	return itsSet->getInt16(aKey);
}

//#	getUint16(key)
inline uint16 ParameterSet::getUint16(const string& aKey) const
{
	return itsSet->getUint16(aKey);
}

//#	getInt32(key)
inline int32 ParameterSet::getInt32(const string& aKey) const
{
	return itsSet->getInt32(aKey);
}

//#	getUint32(key)
inline uint32 ParameterSet::getUint32(const string& aKey) const
{
	return itsSet->getUint32(aKey);
}

#if HAVE_LONG_LONG
//#	getInt64(key)
inline int64 ParameterSet::getInt64(const string& aKey) const
{
	return itsSet->getInt64(aKey);
}

//#	getUint64(key)
inline uint64 ParameterSet::getUint64(const string& aKey) const
{
	return itsSet->getUint64(aKey);
}
#endif

//#	getFloat(key)
inline float ParameterSet::getFloat (const string& aKey) const
{
	return itsSet->getFloat(aKey);
}

//#	getDouble(key)
inline double ParameterSet::getDouble(const string& aKey) const
{
	return itsSet->getDouble(aKey);
}
//#	getString(key)
inline string ParameterSet::getString(const string& aKey) const
{
	return itsSet->getString(aKey);
}

//#	getTime(key)
inline time_t ParameterSet::getTime(const string& aKey) const
{
	return itsSet->getTime(aKey);
}

//#	getBoolVector(key)
inline vector<bool> ParameterSet::getBoolVector(const string& aKey) const
{
	return itsSet->getBoolVector(aKey);
}

//#	getInt16Vector(key)
inline vector<int16> ParameterSet::getInt16Vector(const string& aKey) const
{
	return itsSet->getInt16Vector(aKey);
}

//#	getUint16Vector(key)
inline vector<uint16> ParameterSet::getUint16Vector(const string& aKey) const
{
	return itsSet->getUint16Vector(aKey);
}

//#	getInt32Vector(key)
inline vector<int32> ParameterSet::getInt32Vector(const string& aKey) const
{
	return itsSet->getInt32Vector(aKey);
}

//#	getUint32Vector(key)
inline vector<uint32> ParameterSet::getUint32Vector(const string& aKey) const
{
	return itsSet->getUint32Vector(aKey);
}

#if HAVE_LONG_LONG
//#	getInt64Vector(key)
inline vector<int64> ParameterSet::getInt64Vector(const string& aKey) const
{
	return itsSet->getInt64Vector(aKey);
}

//#	getUint64Vector(key)
inline vector<uint64> ParameterSet::getUint64Vector(const string& aKey) const
{
	return itsSet->getUint64Vector(aKey);
}
#endif

//#	getFloatVector(key)
inline vector<float> ParameterSet::getFloatVector(const string& aKey) const
{
	return itsSet->getFloatVector(aKey);
}

//#	getDoubleVector(key)
inline vector<double> ParameterSet::getDoubleVector(const string& aKey) const
{
	return itsSet->getDoubleVector(aKey);
}
//#	getStringVector(key)
inline vector<string> ParameterSet::getStringVector(const string& aKey) const
{
	return itsSet->getStringVector(aKey);
}

//#	getTimeVector(key)
inline vector<time_t> ParameterSet::getTimeVector(const string& aKey) const
{
	return itsSet->getTimeVector(aKey);
}

    } // namespace APS
  } // namespace ACC
} // namespace LOFAR

#endif
