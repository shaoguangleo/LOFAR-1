//#  ABSBeam.h: interface of the Beam class
//#
//#  Copyright (C) 2002-2004
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

#ifndef ABSBEAM_H_
#define ABSBEAM_H_

#include "ABSDirection.h"
#include "ABSPointing.h"
#include "ABSSpectralWindow.h"
#include "ABSBeamlet.h"

#include <queue>
#include <set>
#include <map>

namespace ABS
{

  /**
   * Singleton class with exactly nbeams instances.
   */
  class Beam
      {
      public:
	  /**
	   * Get a beam with the specified beam index.
	   * @param beam The index of the beam to get.
	   * 0 <= beam < ninstances.
	   */
	  static Beam* getInstance(int beam);

	  /**
	   * Set the number of beam instances that
	   * should be created. After calling this method
	   * once you can not alter the number of 
	   * instances by calling it again.
	   * @param ninstances The total number of instances
	   * that should be created.
	   */
	  static int setNInstances(int ninstances);

	  /**
	   * Allocate the beam using the specified subband set
	   * within the specified spectral window.
	   * @param spw Spectral window in which to allocate.
	   * @param subbands Which set of subbands to allocate
	   * (indices in the spectral window).
	   */
	  int allocate(SpectralWindow const & spw, std::set<int> subbands);
	  int deallocate();
	  int addPointing();
	  int convertPointings();
	  int getPointings();

	  /**
	   * Get the mapping from input subbands to
	   * output beamlets.
	   * @note The map is NOT cleared.
	   * New pairs are simply added. This allows the
	   * caller to call this on a number of beams
	   * to get the total mapping for all beams.
	   */
	  void getSubbandSelection(std::map<int, int>& selection) const;

      protected:
	  Beam(); // no construction outside class
	  ~Beam();

      private:
	  /** is this beam in use? */
	  bool m_allocated;
	  
	  /** current direction of the beam */
	  Pointing m_pointing;

	  /** queue of future pointings */
	  std::priority_queue<Pointing> m_pointing_queue;

	  /**
	   * Set of beamlets belonging to this beam.
	   * It is a set because there should be no
	   * duplicate beamlet instances in the set.
	   */
	  std::set<Beamlet*> m_beamlets;

      private:
	  //@{
	  /** singleton implementation members */
	  static int   m_ninstances;
	  static Beam* m_beams; // array of ninstances beams
	  //@}

      private:
	  /**
	   * Don't allow copying this object.
	   */
	  Beam (const Beam&); // not implemented
	  Beam& operator= (const Beam&); // not implemented
      };
};
     
#endif /* ABSBEAM_H_ */
