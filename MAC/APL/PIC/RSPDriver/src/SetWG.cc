//#
//#  SetWG.cc: implementation of SetWG class
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

#include "SetWG.h"

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

SetWG::SetWG(string name, int blp, uint16 ampl, double freq)
  : GCFTask((State)&SetWG::initial, name), Test(name),
    m_blp(blp), m_ampl(ampl), m_freq(freq)
{
  registerProtocol(RSP_PROTOCOL, RSP_PROTOCOL_signalnames);

  m_server.init(*this, "server", GCFPortInterface::SAP, RSP_PROTOCOL);
}

SetWG::~SetWG()
{}

GCFEvent::TResult SetWG::initial(GCFEvent& e, GCFPortInterface& port)
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
      TRAN(SetWG::enabled);
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

GCFEvent::TResult SetWG::enabled(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch (e.signal)
  {
    case F_ENTRY:
    {
      START_TEST("enabled", "SetWG");

      // subscribe to status updates
      RSPSetwgEvent wg;
 
      wg.timestamp.setNow();
      wg.blpmask.reset();
      if (m_blp >= 0)
      {
	wg.blpmask.set(m_blp);
      }
      else
      {
	for (int i = 0; i < N_RSPBOARDS * N_BLPS; i++)
	  wg.blpmask.set(i);
      }
	
      wg.settings().resize(1);

      if (m_freq > 10e-6)
      {
	wg.settings()(0).freq = (uint16)(((m_freq * (1 << 16)) / 80.0e6) + 0.5);
	wg.settings()(0).ampl = m_ampl;
	wg.settings()(0).nof_usersamples = 0;
	wg.settings()(0).mode = WGSettings::MODE_SINE;
	wg.settings()(0)._pad = 0; // keep valgrind happy
      }
      else
      {
	wg.settings()(0).freq = 0;
	wg.settings()(0).ampl = 0;
	wg.settings()(0).nof_usersamples = 0;
	wg.settings()(0).mode = WGSettings::MODE_OFF;
	wg.settings()(0)._pad = 0; // keep valgrind happy
      }
      
      if (!m_server.send(wg))
      {
	LOG_FATAL("failed to send RSPSetwgEvent");
	exit(EXIT_FAILURE);
      }
    }
    break;

    case RSP_SETWGACK:
    {
      RSPSetwgackEvent ack(e);

      LOG_INFO_STR("ack.time=" << ack.timestamp);

      if (SUCCESS != ack.status)
      {
	LOG_FATAL("negative ack");
	exit(EXIT_FAILURE);
      }
      else
      {
	// wait 5 seconds for the command to activate
	// before disconnecting
	m_server.setTimer(5.0);
      }
    }
    break;

    case F_TIMER:
    {
      // we're done
      TRAN(SetWG::final);
    }
    break;
	
    case F_DISCONNECTED:
    {
      port.close();
      TRAN(SetWG::initial);
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

GCFEvent::TResult SetWG::final(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch(e.signal)
  {
    case F_ENTRY:
      GCFTask::stop();
      break;
      
    case F_EXIT:
      break;

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

void SetWG::run()
{
  start(); // make initial transition
  GCFTask::run();
}

int main(int argc, char** argv)
{
  GCFTask::init(argc, argv);

  LOG_INFO(formatString("Program %s has started", argv[0]));

  char buf[32];

  //
  // Read amplitude
  //
  cout << "Amplitude (1e4 == 0dB):";
  double ampl = atof(fgets(buf, 32, stdin));
  if (ampl < 0 || ampl > 1.0*(1<<16))
  {
    LOG_FATAL(formatString("Invalid amplitude specification should be between 0 and %d", (1<<16)));
    exit(EXIT_FAILURE);
  }

  //
  // Read frequency
  //
  cout << "Frequency (in MHz, e.g. 10e6):";
  double freq = atof(fgets(buf, 32, stdin));
  if (freq < 0 || freq > 20e6)
  {
    LOG_FATAL(formatString("Invalid frequency, should be betwee 0 and 20e6"));
    exit(EXIT_FAILURE);
  }

  //
  // Read BLP
  //
  cout << "Which BLP? (-1 means all): ";
  int blp = atoi(fgets(buf, 32, stdin));
  if (blp < -1 || blp >= N_RSPBOARDS * N_BLPS)
  {
    LOG_FATAL(formatString("Invalid BLP index, should be >= -1 && < %d; -1 indicates all BLP's", N_RSPBOARDS * N_BLPS));
    exit(EXIT_FAILURE);
  }
  
  Suite s("SetWG", &cerr);
  s.addTest(new SetWG("SetWG", blp, (uint16)ampl, freq));
  s.run();
  long nFail = s.report();
  s.free();

  LOG_INFO("Normal termination of program");

  return nFail;
}
