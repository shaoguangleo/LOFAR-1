//#  GetWeightsCmd.cc: implementation of the GetWeightsCmd class
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

#include "RSP_Protocol.ph"
#include "RSPConfig.h"
#include "GetWeightsCmd.h"

#include <blitz/array.h>

#undef PACKAGE
#undef VERSION
#include <lofar_config.h>
#include <Common/LofarLogger.h>

using namespace RSP;
using namespace LOFAR;
using namespace RSP_Protocol;
using namespace blitz;

GetWeightsCmd::GetWeightsCmd(GCFEvent& event, GCFPortInterface& port, Operation oper)
{
  m_event = new RSPGetweightsEvent(event);

  setOperation(oper);
  setPeriod(0);
  setPort(port);
}

GetWeightsCmd::~GetWeightsCmd()
{
  if (isOwner()) delete m_event;
}

void GetWeightsCmd::ack(CacheBuffer& cache)
{
  RSPGetweightsackEvent ack;

  ack.timestamp = getTimestamp();
  ack.status = SUCCESS;

  ack.weights().resize(BeamletWeights::SINGLE_TIMESTEP,
		       m_event->rcumask.count(),
		       cache.getBeamletWeights()().extent(thirdDim));

  int result_rcu = 0;
  for (int cache_rcu = 0; cache_rcu < GET_CONFIG("N_RCU", i); cache_rcu++)
  {
    if (m_event->rcumask[result_rcu])
    {
      if (result_rcu < GET_CONFIG("N_RCU", i))
      {
	ack.weights()(0, result_rcu, Range::all())
	  = cache.getBeamletWeights()()(0, cache_rcu, Range::all());
      }
      else
      {
	LOG_WARN(formatString("invalid RCU index %d, there are only %d RCU's",
			      result_rcu, GET_CONFIG("N_RCU", i)));
      }
      
      result_rcu++;
    }
  }
  
  getPort()->send(ack);
}

void GetWeightsCmd::apply(CacheBuffer& /*cache*/)
{
  // no-op
}

void GetWeightsCmd::complete(CacheBuffer& cache)
{
  ack(cache);
}

const Timestamp& GetWeightsCmd::getTimestamp() const
{
  return m_event->timestamp;
}

void GetWeightsCmd::setTimestamp(const Timestamp& timestamp)
{
  m_event->timestamp = timestamp;
}

bool GetWeightsCmd::validate() const
{
  return (m_event->rcumask.count() <= (unsigned int)GET_CONFIG("N_RCU", i));
}

bool GetWeightsCmd::readFromCache() const
{
  return m_event->cache;
}
