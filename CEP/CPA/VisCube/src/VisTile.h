#ifndef VisCube_VisTile_h
#define VisCube_VisTile_h 1

#define HAVE_BLITZ 1
    
// ColumnarTableTile
#include "VisCube/ColumnarTableTile.h"
    
#include "Common/Lorrays.h"
#include "Common/Thread/Mutex.h"
#include "DMI/TypeInfo.h"



// check for sanity
#if !LORRAYS_USE_BLITZ && !LORRAYS_USE_AIPSPP
  #error No array support defined
#endif
#if LORRAYS_USE_BLITZ && LORRAYS_USE_AIPSPP
  #error Conflicting array support defined
#endif
    
class VisTile;
class VisCube;

#pragma aidgroup VisCube
#pragma type #VisTile

// This macro defines all the columns available in a VisTile
// New columns should be added at the end of the list
#define DoForAllVisTileColumns(Do) \
  Do(double,0,time,TIME); \
  Do(double,0,interval,INTERVAL); \
  Do(int,0,rowflag,ROWFLAG); \
  Do(int,0,seqnr,SEQNR); \
  Do(float,0,weight,WEIGHT); \
  Do(double,1,uvw,UVW); \
  Do(fcomplex,2,data,DATA); \
  Do(fcomplex,2,predict,PREDICT); \
  Do(int,2,flags,FLAGS);


DefineRefTypes(VisTile,VisTileRef);

//##ModelId=3DB964F20079
class VDSID : public HIID
{
  public: 
    // default constructor
    //##ModelId=3DB964F40176
    VDSID (int segid=0,int beamid=0,int obsid=0);
  
    // construct from HIID (checks for correct length)
    //##ModelId=3DB964F40177
    VDSID (const HIID &id);
    
    // returns individual components
    //##ModelId=3DF9FDCD008A
    int segment     () const      { return (*this)[2]; }
    //##ModelId=3DB964F4017E
    int beam        () const      { return (*this)[1]; }
    //##ModelId=3DF9FDCD008E
    int observation () const      { return (*this)[0]; }
    
    // length of a VDSID
    //##ModelId=3DF9FDCD0090
    static uint Length() { return 3; }
};


//##ModelId=3DB964F200EE
//##Documentation
//## VisTile represents one tile (= a number of contiguous timeslots) of
//## visibility data. 
//## VisTile is essentially a ColumnarTableTile with a predefined format. This
//## format defines at least the following columns:
//## 
//##    TIME      (double)
//##    DATA      (Ncorr x Nfreq fcomplex)
//##    FLAGS     (Ncorr x Nfreq int)
//##    ROWFLAG   (int)
//##    UVW       (3 doubles)
//##    WEIGHT    (float)
//##    SEQNR     (int)
//## 
//## Optional columns are:
//## 
//##    PREDICT   (Ncorr x Nfreq fcomplex)
//## 
//## Accessor methods to column data in array format are provided, as well as
//## iterators.
//## 
//## Note that each tile usually has an associated VisTile::Format (=
//## TableFormat) object. This object is not part of the tile itself (for
//## example, in a VisCube, all tiles share the same format object), and must
//## be explictitly attached to a tile before any column-wise accessors are
//## called.
class VisTile : public ColumnarTableTile  //## Inherits: <unnamed>%3D9978030166
{
  public:
    //##ModelId=3DB964F200F4
    //##Documentation
    //## This enum lists all the possible columns in a visibility dataset.
      typedef enum {
// time centroid
        TIME = 0,
// time interval
        INTERVAL,
// observed data (Ncorr x Nchan)
        DATA,
// data flags (Ncorr x Nchan)
        FLAGS,
// row flag 
        ROWFLAG,
// UVW coordinates (3 values)
        UVW,
// weight
        WEIGHT,
// application-defined sequence number
// this is the row number when tile originated in an AIPS++ MS
        SEQNR,
// predicted data (Ncorr x Nchan)
        PREDICT,
            
        MAXCOL
      } Columns;
        
    friend class VisCube;
    
    class ConstIterator;
    class Iterator;
    //##ModelId=3DF9FDC90222
    typedef ConstIterator const_iterator;
    //##ModelId=3DF9FDC9024D
    typedef Iterator iterator;
      

    //##ModelId=3DB964F900AF
    //##Documentation
    //## Construct empty tile
      VisTile();

    //##ModelId=3DB964F900B0
    //##Documentation
    //## Copy constructor. Default is to copy by reference (i.e. data is
    //## shared). Useful flags are DMI::PRIVATIZE (+DMI::WRITE if needed) to
    //## force a private by-value copy, DMI::READONLY to make a read-only copy,
    //## or DMI::PRESERVE_RW to make a copy that preserves the r/w permissions
    //## of the original.
      VisTile (const VisTile &right,
        //##Documentation
        //## flags|DMI::DEEP are passed to CountedRef::copy() when copying the
        //## data block; this copied-and-privatizes the datablock ref
        int flags = DMI::WRITE,
        //##Documentation
        //## depth is ignored (copy is always deep), but is present here
        //## for consistency
        int depth = 1);

    //##ModelId=3DB964F900BF
    //##Documentation
    //## Initializes an empty tile of NC correlations, NF channels, NT
    //## timeslots, using the makeDefaultFormat() method (below) to obtain a
    //## format.
      VisTile (int nc, int nf, int nt);

    //##ModelId=3DB964F900D5
    //##Documentation
    //## Initializes a tile with NT timeslots, using the given format.
      VisTile (const FormatRef &form, int nt);

    //##ModelId=3DB964F900E4
    //##Documentation
    //## Creates a new tile by appending tile B to tile A. A and B must have
    //## compatible formats.
      VisTile (const VisTile &a, const VisTile &b);

    //##ModelId=3DB964F900F6
      ~VisTile();

    //##ModelId=3DB964F900F7
    //##Documentation
    //## Assigns by value. Tile will have read-write permissions
      VisTile & operator=(const VisTile &right);

    //##ModelId=3DB964F90100
    //##Documentation
    //## Initializes a default tile format for NC correlations and NF frequency
    //## channels.
      static void makeDefaultFormat (Format &form, int nc, int nf);

    //##ModelId=3DB964F90117
    //##Documentation
    //## Initializes an empty tile of NC correlations, NF channels, NT
    //## timeslots, using the makeDefaultFormat() method to obtain a format.
      void init (int nc, int nf, int nt);

    //##ModelId=3DB964F9012E
    //##Documentation
    //## Initializes a tile with NT timeslots, using the given format.
      void init (const FormatRef &form, int nt);

    //##ModelId=3DB964F9013E
    //##Documentation
    //## Resets tile to empty state (no data, no format)
      void reset ();

    //##ModelId=3DB964F9013F
    //##Documentation
    //## Attaches the given format to the tile. Will fail if the tile already
    //## has a different format attached, or if the format is incompatible with
    //## the data layout.
      void applyFormat (const FormatRef &form);

    //##ModelId=3DB964F90147
    //##Documentation
    //## Changes the tile format, usually re-formatting the data in memory
    //## accordingly. This may only be used to insert or remove columns, not to
    //## change column shapes. 
      void changeFormat (const FormatRef &form);

    //##ModelId=3DB964F9014F
    //##Documentation
    //## Copies the data from a segment of the other tile over to a segment of
    //## this tile. A segment is a whole number of rows (=timeslots). Will fail
    //## if the tile formats mismatch, or there is not enough rows in the tile
    //## for the copy.
      void copy (
        //##Documentation
        //## starting timeslot to copy to (within this tile)
        int it0,
        //##Documentation
        //## tile to copy from
        const VisTile &other,
        //##Documentation
        //## starting  timeslot to copy from
        int other_it0 = 0,
        //##Documentation
        //## number of timeslots to copy. If <0 (default), copies until end of
        //## source tile.
        int nt = -1);

    //##ModelId=3DB964F9016D
    //##Documentation
    //## Version of copy() implies it0 = 0
      void copy (const VisTile &other, int other_it0 = 0, int nt = -1);

    //##ModelId=3DB964F90184
    //##Documentation
    //## Adds rows (i.e. timeslots) at the end of the tile. This can re-format
    //## data in memory.
      void addRows (int nr);

    //##ModelId=3DD2594A034B
    //##Documentation
    //## Returns a const iterator pointing at the beginning of the tile. If
    //## flags are non-0 (which is default), the iterator will be attached to
    //## the tile via a counted ref constructed with the specified flags.
    VisTile::ConstIterator begin(int flags = DMI::ANON|DMI::READONLY) const;
      
    //##ModelId=3DD25962035B
    //##Documentation
    //## Like const begin(), but returns a non-const iterator.
    VisTile::Iterator begin(int flags = DMI::ANONWR);
      
    //##ModelId=3DD2598802C0
    //##Documentation
    //## Returns an STL-style "end" iterator  (actually just an invalid
    //## iterator), which gives "true" when compared with an iterator that has
    //## gone beyond the end of a tile.
    const VisTile::ConstIterator &end() const;

      //##ModelId=3DB964F901E6
    //##Documentation
    //## ncorr/nfreq/ntime: return the layout of the tile
      int ncorr () const;
    //##ModelId=3DB964F901E8
      int nfreq () const;
    //##ModelId=3DB964F9018D
      int ntime () const;
      
    // sets the tile ID from antenna1, antenna2 & a VDSID
    //##ModelId=3DF9FDD4016A
      void setTileId (int ant1,int ant2,const VDSID &vdsid)
      { 
        ColumnarTableTile::setTileId(vdsid|ant1|ant2); 
      }
    // extracts components of the tile ID
    //##ModelId=3DF9FDD4029A
      int antenna1 () const
      {
        return tileId().size()>VDSID::Length() ? tileId()[VDSID::Length()].id() : -1;
      }
    //##ModelId=3DF9FDD402E1
      int antenna2 () const
      {
        return tileId().size()>VDSID::Length()+1 ? tileId()[VDSID::Length()+1].id() : -1;
      }
    //##ModelId=3DF9FDD40328
      VDSID vdsId () const
      {
        return tileId().size() >= VDSID::Length()
            ? VDSID(tileId()) : VDSID();
      }

      // Standard BlockableObject implementation follows.
      // Note that toBlock() and privatize() are inherited from parent class.
    //##ModelId=3DB964F901CD
      int fromBlock (BlockSet& set);
    //##ModelId=3DB964F901D6
      CountedRefTarget* clone (int flags = 0, int  = 0) const;
    //##ModelId=3DB964F901E4
      TypeId objectType () const { return TpVisTile; }
      
    //##ModelId=3DB964F901EB
      DefineRefTypes(VisTile,Ref);
      
    //##ModelId=3DD3C6CB02E9
    //##Documentation
    //## standard debug info method, depending on level includes:
    //## 0: class name & object address
    //## 1: no. of rows
    //## 2: format (if attached) @level 1
    //## 3: format (if attached) @level 2
      string sdebug ( int detail = 1,const string &prefix = "",
                      const char *name = 0 ) const;

  protected:
      // a VisTile ID is Antenna1+Antenna2+VDSID
    //##ModelId=3DF9FDD40370
      virtual int maxIdSize () const
      { return 2 + VDSID::Length(); }
        
      
  private:
    //##ModelId=3DB964F901F6
    //##Documentation
    //## Helper method: reinitializes internal arrays to point at column data.
      void initArrays ();
      
    //##ModelId=3DB964F901F8
    //##Documentation
    //## helper method: checks that the given column is of the right type and
    //## dimensionality, fails if not
      void assertColumn (int icol,TypeId type,int ndim) const
      {
        FailWhen( !cdata(icol),
          Debug::ssprintf("column %d not defined",icol));
        FailWhen( format().type(icol) != type || 
                  format().ndims(icol) != ndim,
          Debug::ssprintf("type or shape mismatch in column %d\n",
            "expecting %dD %s, have %dD %s",icol,ndim+1,type.toString().c_str(),
            format().ndims(icol)+1,format().type(icol).toString().c_str()));
      }

// The templates getElement/setElement methods are there for VisCube to use.
            
// (the dummy T* parameter is to help template instantiation. g++-3.1 
// seems to have trouble with the obj.member<T>() syntax (when T is a template
// argument, i.e., when calling a templated member from another template)
// so we add a T* parameter to specify type instead.
      template<class T>
      T getElement (int icol,int it,T* = 0) const
      {
#ifdef USE_DEBUG
        assertColumn(icol,typeIdOf(T),0);
#endif
        return static_cast<const T*>(cdata(icol))[it];
      }
      
      template<class T>
      T setElement (int icol,int it,T value)
      {
#ifdef USE_DEBUG
        assertColumn(icol,typeIdOf(T),0);
#endif
        return static_cast<T*>(cwdata(icol))[it] = value;
      }
      
    //##ModelId=3DB964F90070
      int ncorr_;
    //##ModelId=3DB964F90077
      int nfreq_;
      
    //##ModelId=3DF9FDD40016
    //## mutex for thread-safety, locked for the duration of most tile operations
      Thread::Mutex mutex_; 
      
  public:
    // Re-open the public section because we really want to define accessor
    // methods as inlines.
      
    // Individual column accessors, inlined right here for brevity.
    // When more columns are defined, corresponding accessors should be 
    // added here.
      
    //    define some handy macros for column accessors. CheckWR checks
    //    the tile for writability, fails if not writable. 
    //    "wreturn" invokes CheckWR, followed by a return statement. 
    #define CheckWR FailWhen(!isWritable(),"r/w access violation");
    #define wreturn CheckWR; return

    // We use macros to declare accessors. This helps insure consistency,
    // and makes adding new columns easier

    #define DefineColumn(type,dim,name,id) DefineColumn_##dim(type,name)
    #define ALL LoRange::all()

    // The scalarColumn(T,name) macro defines a scalar column, plus
    // four inlined accessor methods: name() and wname() to access as a vector,
    // and name(i) and set_name(i,val) to access individual elements.
    #define DefineColumn_0(type,name) \
      private: LoVec_##type name##_array_; \
      public:  const LoVec_##type & name () const \
               { return name##_array_; } \
               type name (int it) const \
               { return name##_array_(it); } \
               LoVec_##type & w##name () \
               { wreturn name##_array_; } \
               void set_##name (int it,type val) \
               { CheckWR; name##_array_(it) = val; } 
    // The vectorColumn(T,name) macro defines a vector column, plus
    // four inlined accessor methods: name() and wname() to access as a matrix,
    // and name(i) and wname(i) to access individual vectors.
    #define DefineColumn_1(type,name) \
      private: LoMat_##type name##_array_; \
      public:  const LoMat_##type & name () const \
               { return name##_array_; } \
               LoMat_##type & w##name () \
               { wreturn name##_array_; } \
               const LoVec_##type name (int it) const \
               { return name##_array_(ALL,it); } \
               LoVec_##type w##name (int it) \
               { wreturn name##_array_(ALL,it); } 
    // The matrixColumn(T,name) macro defines a 2D column, plus
    // four inlined accessor methods: name() and wname() to access as a cube,
    // and tf_name(i) and wtf_name(i,val) to access as TF-planes.
    #define DefineColumn_2(type,name) \
      private: LoCube_##type name##_array_; \
      public:  const LoCube_##type & name () const \
               { return name##_array_; } \
               LoCube_##type & w##name () \
               { wreturn name##_array_; } \
               const LoMat_##type tf_##name (int icorr) const \
               { return name##_array_(icorr,ALL,ALL); } \
               LoMat_##type wtf_##name (int icorr) \
               { wreturn name##_array_(icorr,ALL,ALL); } 

    //##ModelId=3DF9FDD40042
    DoForAllVisTileColumns(DefineColumn);
          
    #undef DefineColumn
    #undef DefineColumn_0
    #undef DefineColumn_1
    #undef DefineColumn_2
    #undef wreturn
      
    //##ModelId=3DB964F200F9
    //##Documentation
    //## ConstIterator implements iteration over the time axis in a VisTile,
    //## and provides const accessor methods to the data cells for each
    //## timeslot. STL-like semantics are supported:
    //## 
    //## for( VisTile::ConstIterator iter = mytile.begin();     
    //##     iter != mytile.end();
    //##     iter++ )
    //## 
    //## ...as well as alternative iteration methods (see next(), end(),
    //## reset()). An iterator can also hold a CountedRef to a tile, thus
    //## guaranteeing automatic clean-up if the caller releases the tile before
    //## destroying the iterator.
    //## 
    //## Note that for performance reasons, accessors to array columns (data,
    //## flags, uvw) will return sub-arrays that reference the original data.
    //## Theoretically, this can be abused to violate constness, but such
    //## behaviour is severely discouraged and will be persecuted to the full
    //## extent of negative peer pressure.
    class ConstIterator
    {
      public:
        //##ModelId=3DB964F701E6
        //##Documentation
        //## Creates unattached iterator
          ConstIterator();

        //##ModelId=3DB964F701FA
        //##Documentation
        //## Copy constructor. New iterator will point at same tile, same
        //## position.
          ConstIterator(const ConstIterator &right);

        //##ModelId=3DB964F70241
        //##Documentation
        //## Initializes iterator for given tile via attach(tile) below.
          ConstIterator (const VisTile &tile);

        //##ModelId=3DB964F70288
        //##Documentation
        //## Initializes iterator for given tile via attach(ref) below.
          ConstIterator (const VisTileRef &ref);

        //##ModelId=3DB964F702D0
        //##Documentation
        //## Assignment. Detaches from current tile( if any) and makes copy of
        //## r.h.s. iterator.
          ConstIterator & operator=(const ConstIterator &right);
          
        //##ModelId=3DF9FDD2025D
          ~ConstIterator();

        //##ModelId=3DD25CA501CD
        //##Documentation
        //## Comparison operator. Note that all iterators pointing past the end
        //## of a tile (as well as all detached or uninitialized iterators) are
        //## equal.
          bool operator == (const ConstIterator &right);
        //##ModelId=3DD25CB20000
          bool operator != (const ConstIterator &right);
        //##ModelId=3DD25CC20012
        //##Documentation
        //## Prefix increment, same as calling next()
          ConstIterator & operator ++ ();
        //##ModelId=3DD25CD90071
        //##Documentation
        //## Postfix increment. !NOTE!: for performance reasons, this is
        //## identical to prefix increment.
          ConstIterator & operator ++ (int);

        //##ModelId=3DB964F70322
        //##Documentation
        //## Detaches from current tile (if any), then initializes iterator for
        //## start of new tile. It is up to the caller to insure that the tile
        //## object remains valid for the lifetime of the iterator.
          void attach (const VisTile &tile);

        //##ModelId=3DB964F70368
        //##Documentation
        //## Detaches from current tile (if any), then initializes iterator for
        //## new tile, keeping  a copy of the tile ref. The internal ref is
        //## released in the destructor. Tthis insures that the tile object
        //## persists for the lifetime of the iterator even if the caller has
        //## released all other refs to it.
          void attach (const VisTileRef &ref);

        // define macros for implementing per-column accessors
          #define accessor_2(type,name) \
                     LoMat_##type name () const \
                     { return ptile->name()(ALL,ALL,itime); }
          #define accessor_1(type,name) \
                     LoVec_##type name () const \
                     { return ptile->name()(ALL,itime); }
          #define accessor_0(type,name) \
                     type name () const \
                     { return ptile->name()(itime); }

          #define DefineAccessor(type,dim,name,id) accessor_##dim(type,name)
                     
        //##ModelId=3DF9FDD201A5
          DoForAllVisTileColumns(DefineAccessor);
          
          #undef DefineAccessor
          #undef accessor_0
          #undef accessor_1
          #undef accessor_2
          
          
          // specialized accessors for slicing along the frequency axis
          // (to get a spectrum)
        //##ModelId=3DF9FDD202A3
          LoVec_fcomplex f_data    (int icorr = 0) const
          { return ptile->data()(icorr,ALL,itime); }
          
        //##ModelId=3DF9FDD203B0
          LoVec_fcomplex f_predict (int icorr = 0) const
          { return ptile->predict()(icorr,ALL,itime); }
        //##ModelId=3DF9FDD300DC
          LoVec_int f_flags        (int icorr = 0) const
          { return ptile->flags()(icorr,ALL,itime); }

        //##ModelId=3DB964F800F1
        //##Documentation
        //## True if iterator has gone past the end of the tile (also when
        //## uninitialized)
          bool end () const;

        //##ModelId=3DB964F8010A
        //##Documentation
        //## Advances iterator to next timeslot. Will return true as long as
        //## the iterator has not gone past the end of the tile.
          bool next ();

        //##ModelId=3DB964F8011E
        //##Documentation
        //## Resets iterator to first timeslot in tile
          void reset ();

        // Additional Public Declarations
        //##ModelId=3DB964F80132
        //##Documentation
        //## Detaches iterator from current tile, if any. If tile was attached
        //## via a ref, the ref is released at this point.
          void detach();
          
        //##ModelId=3DD3CB0003D0
        //##Documentation
        //## standard debug info method, depending on level includes:
        //## 0: class name & object address
        //## 1+: iteration position, and current tile at same level
        //## 2+: counted ref (if any) at level 1
          string sdebug ( int detail = 1,const string &prefix = "",
                          const char *name = 0 ) const;
          
        //##ModelId=3DD3CB0201B1
          const char * debug ( int detail = 1,const string &prefix = "",
                               const char *name = 0 ) const
          { return Debug::staticBuffer(sdebug(detail,prefix,name)); }
          
      protected:
        //##ModelId=3DB964F70134
          VisTile *ptile;
      
        //##ModelId=3DB964F70140
          VisTileRef tileref;
          
          // go with an inefficient implementation for now -- a simple index --
          // since arrays suck anyway. Should be re-written when we have better 
          // arrays.
        //##ModelId=3DB964F70179
          int itime;
        //##ModelId=3DB964F701B0
          int ntime;
          
        //##ModelId=3DF9FDD2023F
        //##Documentation
        //## keep a lock on the tile being iterated on
          Thread::Mutex::Lock tilelock;
          
    };

    //##ModelId=3DB964F200FB
    //##Documentation
    //## A writable iterator. This is based on ConstIterator, but adds
    //## write-accessors. This Iterator may only be attached to a non-const,
    //## writable tile. 
    class Iterator : virtual public ConstIterator  //## Inherits: <unnamed>%3D888C1F00C5
    {

      public:
        //##ModelId=3DB964F8019C
          Iterator();
        //##ModelId=3DB964F801BF
          Iterator(const Iterator &right);
        //##ModelId=3DB964F80204
          Iterator (VisTile &tile);
        //##ModelId=3DB964F8025C
          Iterator (const VisTileRef &ref);
        //##ModelId=3DB964F8029F
          Iterator & operator=(const Iterator &right);

        //##ModelId=3DB964F802EF
          void attach (VisTile &tile);
        //##ModelId=3DB964F80334
          void attach (const VisTileRef &ref);

          #define accessor_2(type,name) \
             void set_##name (const blitz::Array<type,2> &x) const \
             { ptile->w##name()(ALL,ALL,itime) = x; }
          #define accessor_1(type,name) \
             void set_##name (const blitz::Array<type,1> &x) const  \
             { ptile->w##name()(ALL,itime) = x; }
          #define accessor_0(type,name) \
             void set_##name (type x) const \
             { ptile->w##name()(itime) = x; }
             
          #define DefineAccessor(type,ndim,name,id) accessor_##ndim(type,name)
                     
        //##ModelId=3DF9FDD30323
          DoForAllVisTileColumns(DefineAccessor);
          
          #undef DefineAccessor
          #undef accessor_0
          #undef accessor_1
          #undef accessor_2
          #undef ALL
          
        //##ModelId=3DD3CB0302B8
        //##Documentation
        //## standard debug info method, see ConstIterator above
          string sdebug ( int detail = 1,const string &prefix = "",
                          const char *name = 0 ) const
          { return ConstIterator::sdebug(detail,prefix,name?name:"I:VisTile"); }
          
        //##ModelId=3DD3CB04022A
          const char * debug ( int detail = 1,const string &prefix = "",
                               const char *name = 0 ) const
          { return Debug::staticBuffer(sdebug(detail,prefix,name)); }

      private:
        //##ModelId=3DB964F80160
        //##Documentation
        //## This hides ConsIterator's attach() methods
          ConstIterator::attach;

    };
    
      
};



//##ModelId=3DD25CA501CD
inline bool VisTile::ConstIterator::operator ==(const ConstIterator &right)
{
  if( end() && right.end() )
    return True;
  return ptile == right.ptile && itime == right.itime;
}

//##ModelId=3DD25CB20000
inline bool VisTile::ConstIterator::operator !=(const ConstIterator &right)
{
  return ! ( (*this) == right );
}

//##ModelId=3DD25CC20012
inline VisTile::ConstIterator & VisTile::ConstIterator::operator ++()
{
  next();
  return *this;
}

//##ModelId=3DD25CD90071
inline VisTile::ConstIterator & VisTile::ConstIterator::operator ++(int)
{
  next();
  return *this;
}

//##ModelId=3DB964F800F1
inline bool VisTile::ConstIterator::end () const
{
  return !ptile || itime >= ntime;
}

//##ModelId=3DB964F8010A
inline bool VisTile::ConstIterator::next ()
{
  return ++itime < ntime;
}

//##ModelId=3DB964F8011E
inline void VisTile::ConstIterator::reset ()
{
  itime = 0;
}

// Class VisTile::Iterator 

          
//##ModelId=3DB964F9016D
inline void VisTile::copy (const VisTile &other, int other_it0, int nt)
{
  copy(0,other,other_it0,nt);
}

//##ModelId=3DB964F9018D
inline int VisTile::ntime () const
{
  return nrow();
}

//##ModelId=3DB964F901E6
inline int VisTile::ncorr () const
{
  return ncorr_;
}

//##ModelId=3DB964F901E8
inline int VisTile::nfreq () const
{
  return nfreq_;
}

#undef CheckWR

//##ModelId=3DD2594A034B
inline VisTile::ConstIterator VisTile::begin (int flags) const
{
  return flags ? ConstIterator(VisTileRef(this,flags)) : ConstIterator(*this);
}

//##ModelId=3DD25962035B
inline VisTile::Iterator VisTile::begin (int flags)
{
  FailWhen( !isWritable(),"r/w access violation" );
  return flags ? Iterator(VisTileRef(this,flags)) : Iterator(*this);
}

//##ModelId=3DD2598802C0
inline const VisTile::ConstIterator & VisTile::end () const
{
  // return an iterator that is invalid by default
  static ConstIterator dum; 
  return dum;
}


#endif
