//#  TPersistentObject.h: container class for making objects persistent.
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

#ifndef LCS_PL_TPERSISTENTOBJECT_H
#define LCS_PL_TPERSISTENTOBJECT_H

//# Includes
#include <PL/PersistentObject.h>
// #include <PL/Exception.h>
#include <loki/static_check.h>

namespace LCS 
{
  namespace PL
  {
    //# Forward Declarations
    class Query;
    template<typename T> class Collection;
    template<typename T> class TPersistentObject;
    template<typename T> class DBRep;

    //
    // This templated class acts as a surrogate container class for instances
    // of T. The container provides the functionality to make instances of T
    // persistent. 
    // \note We use the term surrogate container, because TPersistentObject
    // does not act as a real STL-like container. TPersistentObject stores
    // a \e pointer to the instance of T instead of storing a \e copy, which
    // is what STL-like containers do.
    //
    template<typename T>
    class TPersistentObject : public PersistentObject
    {
    public:

      // We need a default constructor, because we want to be able to
      // create e.g. a Collection of TPersistentObject<T>. The default
      // constructor of will create a default constructed object \c T on
      // the free store, which will be deleted when \c *this is destroyed.
      TPersistentObject() :
        itsObjectPtr(new T()), itsObjectSharedPtr(itsObjectPtr)
      {
	init();
      }

      // \c T is passed by refererence, not const reference, because this
      // documents more explicitly that \c t can be easily changed, using
      // the data() method, which also returns a reference, instead of a 
      // const reference. Internally we keep a pointer to the instance of T;
      // we need a pointer because pointer can have a null value, whereas 
      // a reference cannot.
      explicit TPersistentObject(T& t) : 
	itsObjectPtr(&t)
      { 
	init();
      }

      virtual ~TPersistentObject() 
      {
      }
      
      // Return a reference to the contained persistent object.
      // \note This is a \e non-const reference, because we want to allow
      // modification of *itsObjectPtr.
      T& data() { return *itsObjectPtr; }

      // \todo Maybe this should be done completely different. It will depend
      // on how we can best iterate over the query result. We might e.g.
      // instantiate our persistent objects using their unique object-ids
      // which we're returned by the query. Then we could call the
      // retrieve(ObjectId&) member function using to initialize an already 
      // existing PersistentObject. We will have at least one existing
      // TPersistentObject<T> anyway, because we can only add instances of
      // TPersistentObject<T> to our Collection.
      static Collection<TPersistentObject<T> > 
      retrieve(const Query& query, int maxObjects)
      {
	return doRetrieve(query, maxObjects);
      }

      static TPersistentObject<T>
      retrieve(const ObjectId& oid)
      {
	return doRetrieve(oid);
      }

    private:
      
      // @name Methods that must be implemented using template specialization
      // The implementation of the following methods will depend on the
      // class type \c T. Therefore, there is no default implementation 
      // available. 

      //@{
 
      // This method \e must be called by every constructor. It ensures that
      // the POContainer in the base class PersistentObject is initialized
      // properly and that the ownership relations between the
      // PersistentObjects in this container are properly set in their
      // associated MetaData objects.
      void init()
      {
  	STATIC_CHECK(0, _Use_Explicit_Member_Specialization_);
//  	THROW(NotImplemented, 
// 	      "Method should be implemented using template specialization"); 
      }

      // This method is responsible for actually erasing the \e primitive
      // data members of \c T.
      // \throw NotImplemented
      virtual void doErase() const 
      {
	STATIC_CHECK(0, _Use_Explicit_Member_Specialization_);
//  	THROW(NotImplemented, 
// 	      "Method should be implemented using template specialization"); 
      }

      // This method is responsible for actually inserting the \e primitive
      // data members of \c T.
      // \throw NotImplemented
      virtual void doInsert() const 
      {
	STATIC_CHECK(0, _Use_Explicit_Member_Specialization_);
//  	THROW(NotImplemented, 
// 	      "Method should be implemented using template specialization"); 
      }

      static TPersistentObject<T>
      doRetrieve(const ObjectId& oid)
      {
  	STATIC_CHECK(0, ERROR__Use_Explicit_Member_Specialization);
//   	THROW(NotImplemented, 
//  	      "Method should be implemented using template specialization"); 
      }

      // This method is responsible for actually erasing the \e primitive
      // data members of \c T.
      // \throw NotImplemented
      virtual void doUpdate() const 
      {
	STATIC_CHECK(0, _Use_Explicit_Member_Specialization_);
//  	THROW(NotImplemented, 
// 	      "Method should be implemented using template specialization"); 
      }

      // This method is responsible for actually retrieving the \e primitive
      // data members of \c T.
      // \throw NotImplemented
      static Collection<TPersistentObject<T> > 
      doRetrieve(const Query& query, int maxObjects)
      {
  	STATIC_CHECK(0, ERROR__Use_Explicit_Member_Specialization);
//   	THROW(NotImplemented, 
//  	      "Method should be implemented using template specialization"); 
      }

      //@}

      // Here we keep a pointer to the instance of T.
      T* itsObjectPtr;

      // The boost::shared_ptr will be used to keep track of instances of T
      // that we've created ourselves. Because copying a TPersistentObject 
      // implies copying the pointer to the instance of T, we must somehow
      // keep track of the number of pointers pointing to this object. When
      // the count drops to zero, boost::shared_ptr will delete the object.
      // Obviously, we do not want this behaviour for instances of T that we
      // did not create ourselves. If we're not the creator, then we're not
      // the owner, hence we should never ever delete the object! Therefore,
      // we will only transfer ownership of itsObjectPtr to the
      // boost::shared_ptr when \e we are the creator/owner of the object
      // that itsObjectPtr points to.
      boost::shared_ptr<T> itsObjectSharedPtr;

      // @name The DBRep classes.
      //@{

      // The DBRep<T> struct needs to have access to our private data members,
      // because it will update them when e.g. data is read form the database.
      friend struct DBRep<T>;

      // Convert the data in our persistent object class to DBRep format,
      // which stores all data members contiguously in memory.
      template<typename U>
      void toDatabaseRep(DBRep<U>& dest) const;

      // Convert the data from DBRep format to our persistent object.
      template<typename U>
      void fromDatabaseRep(const DBRep<U>& org);

      //@}

    };

  }

}

#endif
