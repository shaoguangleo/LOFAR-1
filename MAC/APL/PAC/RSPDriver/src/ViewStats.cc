//#
//#  ViewStats.cc: implementation of ViewStats class
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

// this include needs to be first!
#define DECLARE_SIGNAL_NAMES

#include "RSP_Protocol.ph"
#include "EPA_Protocol.ph"

#include "ViewStats.h"

#include <Suite/suite.h>
#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <blitz/array.h>
#include <gnuplot_i.h>

#undef PACKAGE
#undef VERSION
#include <lofar_config.h>
#include <Common/LofarLogger.h>

using namespace std;
using namespace LOFAR;
using namespace blitz;
using namespace EPA_Protocol;
using namespace RSP_Protocol;

//
// These are the parameters for the MAC-EPA increment 2
//
#define N_RSPBOARDS 3
#define N_BLPS      2

#define START_TEST(_test_, _descr_) \
  setCurSubTest(#_test_, _descr_)

#define STOP_TEST() \
  reportSubTest()

#define FAIL_ABORT(_txt_, _final_state_) \
do { \
    FAIL(_txt_);  \
    TRAN(_final_state_); \
} while (0)

#define TESTC_ABORT(cond, _final_state_) \
do { \
  if (!TESTC(cond)) \
  { \
    TRAN(_final_state_); \
    break; \
  } \
} while (0)

#define TESTC_DESCR_ABORT(cond, _descr_, _final_state_) \
do { \
  if (!TESTC_DESCR(cond, _descr_)) \
  { \
    TRAN(_final_state_); \
    break; \
  } \
} while(0)

ViewStats::ViewStats(string name, int type, int rcu)
  : GCFTask((State)&ViewStats::initial, name), Test(name),
    m_type(type), m_rcu(rcu)
{
  registerProtocol(RSP_PROTOCOL, RSP_PROTOCOL_signalnames);

  m_server.init(*this, "server", GCFPortInterface::SAP, RSP_PROTOCOL);
}

ViewStats::~ViewStats()
{}

GCFEvent::TResult ViewStats::initial(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch(e.signal)
  {
    case F_INIT:
    {
    }
    break;

    case F_ENTRY:
    {
      if (!m_server.isConnected()) m_server.open();
    }
    break;

    case F_CONNECTED:
    {
      TRAN(ViewStats::enabled);
    }
    break;

    case F_DISCONNECTED:
    {
      port.setTimer((long)1);
      port.close();
    }
    break;

    case F_TIMER:
    {
      // try again
      m_server.open();
    }
    break;

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

GCFEvent::TResult ViewStats::enabled(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch (e.signal)
  {
    case F_ENTRY:
    {
      START_TEST("enabled", "test UPDSTATS");

      // subscribe to status updates
      RSPSubstatsEvent substats;

      substats.timestamp.setNow();
      substats.rcumask.reset();
      if (m_rcu >= 0)
      {
	substats.rcumask.set(m_rcu);
      }
      else
      {
	for (int i = 0; i < N_RSPBOARDS * N_BLPS; i++)
	  substats.rcumask.set(i);
      }
	
      substats.period = 1;
      substats.type = m_type;
      substats.reduction = SUM;
      
      if (!m_server.send(substats))
      {
	LOG_FATAL("failed to subscribe");
	exit(EXIT_FAILURE);
      }
    }
    break;

    case RSP_SUBSTATSACK:
    {
      RSPSubstatsackEvent ack(e);

      if (SUCCESS != ack.status)
      {
	LOG_FATAL("failed to subscribe");
	exit(EXIT_FAILURE);
      }
      LOG_INFO_STR("ack.time=" << ack.timestamp);
    }
    break;

    case RSP_UPDSTATS:
    {
      RSPUpdstatsEvent upd(e);

      if (SUCCESS != upd.status)
      {
	LOG_FATAL("invalid update");
	exit(EXIT_FAILURE);
      }

      LOG_INFO_STR("upd.time=" << upd.timestamp);
      LOG_INFO_STR("upd.handle=" << upd.handle);
      
      LOG_DEBUG_STR("upd.stats=" << upd.stats());

      plot_statistics(upd.stats());
    }
    break;
    
    case RSP_UNSUBSTATSACK:
    {
      RSPUnsubstatsackEvent ack(e);

      if (SUCCESS != ack.status)
      {
	LOG_FATAL("unsubscribe failure");
	exit(EXIT_FAILURE);
      }

      LOG_INFO_STR("ack.time=" << ack.timestamp);

      LOG_INFO_STR("ack.handle=" << ack.handle);

      port.close();
      TRAN(ViewStats::initial);
    }
    break;

    case F_DISCONNECTED:
    {
      port.close();
      TRAN(ViewStats::initial);
    }
    break;

    case F_EXIT:
    {
      STOP_TEST();
    }
    break;

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

void ViewStats::plot_statistics(Array<complex<double>, 3>& stats)
{
  static gnuplot_ctrl* handle = 0;
  int n_freqbands = stats.extent(thirdDim);

  Array<double, 1> freq(n_freqbands);
  Array<double, 1> value(n_freqbands);

  // initialize the freq array
  firstIndex i;
  
  if (!handle)
  {
    handle = gnuplot_init();
    if (!handle) return;

    gnuplot_setstyle(handle,   "steps");
    gnuplot_cmd(handle, "set grid x y");
    //gnuplot_cmd(handle, "set bmargin 1");
    gnuplot_cmd(handle, "set tmargin .05");
  }

#if 1
  gnuplot_cmd(handle, "set size 1,1");
  gnuplot_cmd(handle, "set origin 0,0");
  gnuplot_cmd(handle, "set multiplot");
  gnuplot_cmd(handle, "set size 1,%f", 1.0 / stats.extent(secondDim));
#endif
  for (int rcu = stats.lbound(secondDim);
       rcu <= stats.ubound(secondDim);
       rcu++)
  {
    gnuplot_cmd(handle, "set origin 0,%f", 1.0 * rcu / stats.extent(secondDim));
    if (rcu == stats.lbound(secondDim))
    {
      gnuplot_cmd(handle, "set xtics axis");
    }
    else
    {
      gnuplot_cmd(handle, "set noxtics");
    }
    
    if (Statistics::SUBBAND_POWER == m_type
	|| Statistics::BEAMLET_POWER == m_type)
    {
      // add real and imaginary part to get power
      value  = real(stats(0, rcu, Range::all()));
      value += imag(stats(0, rcu, Range::all()));

      // signal + 1e-6 and +10dB calibrated this to the Marconi signal generator
      value = log10((value + 1e-6) / (1.0*(1<<16))) * 10.0 + 10.0;

      // set yrange for power
      gnuplot_cmd(handle, "set ytics -100,20");
      gnuplot_cmd(handle, "set yrange [-100:20]");
      gnuplot_cmd(handle, "set xrange [0:%f]", 20.0);

      freq = i * (20.0 / n_freqbands); // calculate frequency in MHz
      gnuplot_cmd(handle, "set xlabel \"Frequency (MHz)\" 0, 1.5");
    }
    else // MEAN
    {
      value =  real(stats(0, rcu, Range::all())) * real(stats(0, rcu, Range::all()));
      value += imag(stats(0, rcu, Range::all())) * imag(stats(0, rcu, Range::all()));
      value /= (1<<16);
      value /= (1<<16);
      value -= 1.0;

      // set yrange for mean
      gnuplot_cmd(handle, "set ytics -1.25,.25");
      gnuplot_cmd(handle, "set yrange [-1.25:1.25]");
      gnuplot_cmd(handle, "set xrange [0:%f]", 1.0*n_freqbands);

      freq = i; // selected beamlet index
      gnuplot_cmd(handle, "set xlabel \"Beamlet Index\" 0, 1.5");
    }

    if (m_type < Statistics::BEAMLET_MEAN)
    {
    }
    else
    {
    }

    gnuplot_resetplot(handle);

    char title[128];
    switch (m_type)
    {
      case Statistics::SUBBAND_MEAN:
	snprintf(title, 128, "Subband Mean Value (RCU=%d)", rcu);
	break;
      case Statistics::SUBBAND_POWER:
	snprintf(title, 128, "Subband Power (RCU=%d)", rcu);
	break;
      case Statistics::BEAMLET_MEAN:
	snprintf(title, 128, "Beamlet Mean Value (RCU=%d)", rcu);
	break;
      case Statistics::BEAMLET_POWER:
	snprintf(title, 128, "Beamlet Power (RCU=%d)", rcu);
	break;
      default:
	snprintf(title, 128, "ERROR: Invalid m_type");
	break;
    }

    gnuplot_plot_xy(handle, freq.data(), value.data(), n_freqbands, title);
  }

  gnuplot_cmd(handle, "set nomultiplot");
}

void ViewStats::run()
{
  start(); // make initial transition
  GCFTask::run();
}

int main(int argc, char** argv)
{
  GCFTask::init(argc, argv);

  LOG_INFO(formatString("Program %s has started", argv[0]));

  char buf[32];

  cout << "Type of stat [0==SUBBAND_MEAN, 1==SUBBAND_POWER, 2=BEAMLET_MEAN, 3=BEAMLET_POWER]:";
  int type = atoi(fgets(buf, 32, stdin));
  if (type < 0 || type >= Statistics::N_STAT_TYPES)
  {
    LOG_FATAL(formatString("Invalid type of stat, should be >= 0 && < %d", Statistics::N_STAT_TYPES));
    exit(EXIT_FAILURE);
  }

  cout << "Which RCU? ";
  int rcu = atoi(fgets(buf, 32, stdin));
  if (rcu < -1 || rcu >= N_RSPBOARDS * N_BLPS)
  {
    LOG_FATAL(formatString("Invalid RCU index, should be >= -1 && < %d; -1 indicates all RCU's", N_RSPBOARDS * N_BLPS));
    exit(EXIT_FAILURE);
  }
  
  Suite s("ViewStats", &cerr);
  s.addTest(new ViewStats("ViewStats", type, rcu));
  s.run();
  long nFail = s.report();
  s.free();

  LOG_INFO("Normal termination of program");

  return nFail;
}
