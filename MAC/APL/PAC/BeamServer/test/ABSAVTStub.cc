//#
//#  AVTStub.cc: implementation of AVTStub class
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

#include <Suite/suite.h>
#define DECLARE_SIGNAL_NAMES
#include "ABS_Protocol.ph"
#include "ABSDirection.h"

#include "ABSAVTStub.h"

#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#undef PACKAGE
#undef VERSION
#include <lofar_config.h>
#include <Common/LofarLogger.h>

//
// using half of the beamlets should allow two AVTStub clients
// to connect to the BeamServer
//
#define TEST_N_BEAMLETS ( N_BEAMLETS / 2 )

using namespace ABS;
using namespace std;
using namespace LOFAR;

AVTStub::AVTStub(string name)
  : GCFTask((State)&AVTStub::initial, name), Test("AVTStub")
{
  registerProtocol(ABS_PROTOCOL, ABS_PROTOCOL_signalnames);

  beam_server.init(*this, "beam_server", GCFPortInterface::SAP, ABS_PROTOCOL);
}

AVTStub::~AVTStub()
{}

GCFEvent::TResult AVTStub::initial(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int disconnect_count = 0;

  LOG_DEBUG(formatString("initial received event on port %s", port.getName().c_str()));

  switch(e.signal)
  {
      case F_INIT:
      {
      }
      break;

      case F_ENTRY:
      {
	  beam_server.open();
      }
      break;

      case F_CONNECTED:
      {
	  LOG_DEBUG(formatString("port %s connected", port.getName().c_str()));
	  TRAN(AVTStub::test001);
      }
      break;

      case F_DISCONNECTED:
      {
	  // only do 5 reconnects
	  if (disconnect_count++ > 5)
	  {
	      FAIL("timeout");
	      TRAN(AVTStub::done);
	  }
	  port.setTimer((long)2);
	  port.close();
      }
      break;

      case F_TIMER:
      {
	  // try again
	  beam_server.open();
      }
      break;

      default:
      {
	  status = GCFEvent::NOT_HANDLED;
      }
      break;
  }

  return status;
}

GCFEvent::TResult AVTStub::test001(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int timerid = 0;

  switch (e.signal)
  {
      case F_ENTRY:
      {
	LOG_INFO("running test001");
  
	// start test timer, test should finish
	// within 2 seconds
	timerid = beam_server.setTimer((long)2);

	// start test by sending beam alloc
	ABSBeamallocEvent alloc;
	alloc.spectral_window = 0;
	alloc.n_subbands = 1;
	memset(alloc.subbands, 0, sizeof(alloc.subbands));

	TESTC(beam_server.send(alloc));
      }
      break;

      case ABS_BEAMALLOC_ACK:
      {
	ABSBeamallocAckEvent ack(e);

	TESTC(ABS_Protocol::SUCCESS == ack.status);
	if (ABS_Protocol::SUCCESS == ack.status)
	{
	  TESTC(0 <= ack.handle);

	  // beam allocated, now free it
	  ABSBeamfreeEvent beamfree;
	  beamfree.handle = ack.handle;

	  TESTC(beam_server.send(beamfree));
	}
	else
	{
	  TRAN(AVTStub::test002);
	}
      }
      break;

      case ABS_BEAMFREE_ACK:
      {
	ABSBeamfreeAckEvent ack(e);
	TESTC(ABS_Protocol::SUCCESS == ack.status);
	TESTC(0 <= ack.handle);

	// test completed, next test
	TRAN(AVTStub::test002);
      }
      break;

      case F_TIMER:
      {
	// abort test
	beam_server.close();
	FAIL("timeout");
	TRAN(AVTStub::done);
      }
      break;

      case F_DISCONNECTED:
      {
	FAIL("disconnected");
	port.close();
	TRAN(AVTStub::done);
      }
      break;

      case F_EXIT:
      {
	// before leaving, cancel the timer
	beam_server.cancelTimer(timerid);
      }
      break;

      default:
      {
	  status = GCFEvent::NOT_HANDLED;
      }
      break;
  }

  return status;
}

GCFEvent::TResult AVTStub::test002(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int timerid = 0;
  
  switch (e.signal)
  {
    case F_ENTRY:
    {
      LOG_INFO("running test002");

      // start test timer, test should finish
      // within 2 seconds
      timerid = beam_server.setTimer((long)2);

      // start test by sending beam alloc
      ABSBeamallocEvent alloc;
      alloc.spectral_window = 0;
      alloc.n_subbands = 1;
      memset(alloc.subbands, 0, sizeof(alloc.subbands));

      TESTC(beam_server.send(alloc));
    }
    break;

    case ABS_BEAMALLOC_ACK:
    {
      ABSBeamallocAckEvent ack(e);

      TESTC(ABS_Protocol::SUCCESS == ack.status);
      if (ABS_Protocol::SUCCESS == ack.status)
      {
	// send pointto command
	ABSBeampointtoEvent pointto;
	pointto.handle = ack.handle;
	pointto.time = time(0) + 15;
	pointto.type=(int)Direction::LOFAR_LMN;
	pointto.angle[0]=0.0;
	pointto.angle[1]=-1.0;

	TESTC(beam_server.send(pointto));

	// beam pointed, now free it
	ABSBeamfreeEvent beamfree;
	beamfree.handle = ack.handle;

	TESTC(beam_server.send(beamfree));
      }
      else TRAN(AVTStub::test003);
    }
    break;

    case ABS_BEAMFREE_ACK:
    {
      ABSBeamfreeAckEvent ack(e);
      TESTC(ABS_Protocol::SUCCESS == ack.status);

      // test completed, next test
      TRAN(AVTStub::test003);
    }
    break;

    case F_TIMER:
    {
      // abort test
      beam_server.close();
      FAIL("timeout");
      TRAN(AVTStub::done);
    }
    break;

    case F_DISCONNECTED:
    {
      FAIL("disconnected");
      port.close();
      TRAN(AVTStub::done);
    }
    break;

    case F_EXIT:
    {
      // before leaving, cancel the timer
      beam_server.cancelTimer(timerid);
    }
    break;

    default:
    {
      status = GCFEvent::NOT_HANDLED;
    }
    break;
  }

  return status;
}

GCFEvent::TResult AVTStub::test003(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int timerid = 0;
  
  switch (e.signal)
  {
    case F_ENTRY:
    {
      LOG_INFO("running test003");

      // start test timer, test should finish
      // within 2 seconds
      timerid = beam_server.setTimer((long)2);

      // send wgenable
      ABSWgsettingsEvent wgs;
      wgs.frequency=1e6;
      wgs.amplitude=0x8000;

      TESTC(beam_server.send(wgs));
    }
    break;

    case ABS_WGSETTINGS_ACK:
    {
      // check acknowledgement
      ABSWgsettingsAckEvent wgsa(e);

      TESTC(ABS_Protocol::SUCCESS == wgsa.status);
      if (ABS_Protocol::SUCCESS == wgsa.status)
      {
	  
	// send WGENABLE
	ABSWgenableEvent wgenable;
	TESTC(beam_server.send(wgenable));

	// start test by sending beam alloc
	ABSBeamallocEvent alloc;
	alloc.spectral_window = 0;
	alloc.n_subbands = 1;
	memset(alloc.subbands, 0, sizeof(alloc.subbands));

	TESTC(beam_server.send(alloc));
      }
      else TRAN(AVTStub::test004);
    }
    break;

    case ABS_BEAMALLOC_ACK:
    {
      ABSBeamallocAckEvent ack(e);

      TESTC(ABS_Protocol::SUCCESS == ack.status);
      if (ABS_Protocol::SUCCESS == ack.status)
      {
	// send pointto command
	ABSBeampointtoEvent pointto;
	pointto.handle = ack.handle;
	pointto.time = time(0) + 15;
	pointto.type=(int)Direction::LOFAR_LMN;
	pointto.angle[0]=0.0;
	pointto.angle[1]=-1.0;

	TESTC(beam_server.send(pointto));

	// beam pointed, now free it
	ABSBeamfreeEvent beamfree;
	beamfree.handle = ack.handle;

	TESTC(beam_server.send(beamfree));
      }
      else TRAN(AVTStub::test004);
    }
    break;

    case ABS_BEAMFREE_ACK:
    {
      ABSBeamfreeAckEvent ack(e);
      TESTC(ABS_Protocol::SUCCESS == ack.status);

      // send wgdisable
      ABSWgdisableEvent wgdisable;
      TESTC(beam_server.send(wgdisable));

      // test completed, next test
      TRAN(AVTStub::test004);
    }
    break;

    case F_TIMER:
    {
      // abort test
      beam_server.close();
      FAIL("timeout");
      TRAN(AVTStub::done);
    }
    break;

    case F_DISCONNECTED:
    {
      FAIL("disconnected");
      port.close();
      TRAN(AVTStub::done);
    }
    break;

    case F_EXIT:
    {
      // before leaving, cancel the timer
      beam_server.cancelTimer(timerid);
    }
    break;

    default:
    {
      status = GCFEvent::NOT_HANDLED;
    }
    break;
  }

  return status;
}

GCFEvent::TResult AVTStub::test004(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int timerid = 0;
  static int loop = 0;
  
  switch (e.signal)
  {
      case F_ENTRY:
      {
	LOG_INFO("running test004");

	// start test timer, test should finish
	// within 2 seconds
	timerid = beam_server.setTimer((long)2);

	// invalid n_subbands in beam alloc
	ABSBeamallocEvent alloc;
	alloc.spectral_window = 0;
	alloc.n_subbands = TEST_N_BEAMLETS + 1;
	memset(alloc.subbands, 0, sizeof(alloc.subbands));

	TESTC(beam_server.send(alloc));
      }
      break;

      case ABS_BEAMALLOC_ACK:
      {
	ABSBeamallocAckEvent ack(e);

	TESTC(ABS_Protocol::SUCCESS != ack.status);

	if (loop == 0)
	{
	  // send invalid spectral window index
	  ABSBeamallocEvent alloc;
	  alloc.spectral_window = -1;
	  alloc.n_subbands = 1;
	  memset(alloc.subbands, 0, sizeof(alloc.subbands));

	  TESTC(beam_server.send(alloc));

	  loop++;
	}
	else if (loop == 1)
	{
	  // send invalid n_subbands (-1)
	  ABSBeamallocEvent alloc;
	  alloc.spectral_window = 0;
	  alloc.n_subbands = -1;
	  memset(alloc.subbands, 0, sizeof(alloc.subbands));

	  TESTC(beam_server.send(alloc));

	  loop++;
	}
	else if (loop == 2)
	{
	  // send invalid index in subbands array
	  ABSBeamallocEvent alloc;
	  alloc.spectral_window = 0;
	  alloc.n_subbands = 1;
	  memset(alloc.subbands, 0, sizeof(alloc.subbands));
	  alloc.subbands[0] = -1;

	  TESTC(beam_server.send(alloc));

	  loop++;
	}
	else
	{
	    // done => next test
	    TRAN(AVTStub::test005);
	}

	LOG_DEBUG(formatString("loop=%d", loop));
      }
      break;

      case F_TIMER:
      {
	// abort test
	beam_server.close();
	FAIL("timeout");
	TRAN(AVTStub::done);
      }
      break;

      case F_DISCONNECTED:
      {
	FAIL("disconnected");
	port.close();
	TRAN(AVTStub::done);
      }
      break;

      case F_EXIT:
      {
	// before leaving, cancel the timer
	beam_server.cancelTimer(timerid);
      }
      break;

      default:
      {
	  status = GCFEvent::NOT_HANDLED;
      }
      break;
  }

  return status;
}

GCFEvent::TResult AVTStub::test005(GCFEvent& e, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  static int timerid = 0;
  static int beam_handle = -1;
  
  switch (e.signal)
  {
    case F_ENTRY:
    {
      LOG_INFO("running test005");

      // send beam allocation, select all subbands
      ABSBeamallocEvent alloc;
      alloc.spectral_window = 0;
      alloc.n_subbands = TEST_N_BEAMLETS;
      memset(alloc.subbands, 0, sizeof(alloc.subbands));
      for (int i = 0; i < TEST_N_BEAMLETS; i++)
      {
	alloc.subbands[i] = i;
      }

      TESTC(beam_server.send(alloc));
    }
    break;

    case ABS_BEAMALLOC_ACK:
    {
      ABSBeamallocAckEvent ack(e);

      TESTC(ABS_Protocol::SUCCESS == ack.status);
      if (ABS_Protocol::SUCCESS == ack.status)
      {
	beam_handle = ack.handle;
	LOG_DEBUG(formatString("got beam_handle=%d", beam_handle));

	// send pointto command (zenith)
	ABSBeampointtoEvent pointto;
	pointto.handle = ack.handle;
	pointto.time = time(0) + 20;
	pointto.type=(int)Direction::LOFAR_LMN;
	pointto.angle[0]=0.0;
	pointto.angle[1]=1.0;

	TESTC(beam_server.send(pointto));

	// send pointto command (northern horizon)
	pointto.time = time(0) + 25;
	pointto.angle[0]=1.0;
	pointto.angle[1]=0.0;

	TESTC(beam_server.send(pointto));

	// let the beamformer compute for 30 seconds
	timerid = beam_server.setTimer((long)30);
      }
      else TRAN(AVTStub::done);
    }
    break;

    case F_TIMER:
    {
      // done => send BEAMFREE
      ABSBeamfreeEvent beamfree;
      beamfree.handle = beam_handle;

      TESTC(beam_server.send(beamfree));
    }
    break;

    case ABS_BEAMFREE_ACK:
    {
      ABSBeamfreeAckEvent ack(e);
      TESTC(ABS_Protocol::SUCCESS == ack.status);
      TESTC(beam_handle == ack.handle);

      // test completed, next test
      TRAN(AVTStub::done);
    }
    break;

    case F_DISCONNECTED:
    {
      FAIL("disconnected");
      port.close();
      TRAN(AVTStub::done);
    }
    break;

    case F_EXIT:
    {
      // before leaving, cancel the timer
      beam_server.cancelTimer(timerid);
    }
    break;

    default:
    {
      status = GCFEvent::NOT_HANDLED;
    }
    break;
  }

  return status;
}

GCFEvent::TResult AVTStub::done(GCFEvent& e, GCFPortInterface& /*port*/)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  switch(e.signal)
  {
  case F_ENTRY:
    GCFTask::stop();
    break;
  }

  return status;
}

void AVTStub::run()
{
  start(); // make initial transition
  GCFTask::run();
}

int main(int argc, char** argv)
{
#if 0
  char prop_path[PATH_MAX];
  const char* mac_config = getenv("MAC_CONFIG");

  snprintf(prop_path, PATH_MAX-1,
	   "%s/%s", (mac_config?mac_config:"."),
	   "log4cplus.properties");
  INIT_LOGGER(prop_path);
#endif
  LOG_INFO(formatString("Program %s has started", argv[0]));

  GCFTask::init(argc, argv);

  Suite s("Beam Server Process Test Suite", &cerr);
  s.addTest(new AVTStub("AVTStub"));
  s.run();
  long nFail = s.report();
  s.free();

  LOG_INFO("Normal termination of program");

  return nFail;
}
