//#  WH_Random.h: a random generator for BG/L correlator
//#
//#  Copyright (C) 2002-2004
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, sweastron.nl
//#
//#
//#  $Id$

#ifndef BG_CORRELATOR_WH_RANDOM_H
#define BG_CORRELATOR_WH_RANDOM_H

#include <sys/time.h>

#include <tinyCEP/WorkHolder.h>

namespace LOFAR
{
  
  class WH_Random: public WorkHolder
  {
  public:
    explicit WH_Random (const string& name,
			const int nelemenents,
			const int nsamples,
			const int nchannels,
			const int npolarisations);

    virtual ~WH_Random();

    static WorkHolder* construct (const string& name,
				  const int nelements, 
				  const int nsamples, 
				  const int nchannels,
				  const int npolarisations);
    
    virtual WH_Random* make (const string& name);
    
    //    virtual void preprocess();
    virtual void process();
    virtual void dump() const;

    float getBandwidth();

  private:
    WH_Random (const WH_Random&);
    WH_Random& operator= (const WH_Random&);

    int itsIntegrationTime;
    int itsIndex;

    const int itsNelements;
    const int itsNsamples;
    const int itsNchannels;
    const int itsNpolarisations;

    struct timeval starttime;
    struct timeval stoptime;

    float bandwidth;   // stores the 'measured' bandwidth in bytes/sec
  };


  inline float WH_Random::getBandwidth() { return bandwidth; }
} // namespace LOFAR

#endif
