//#
//#  RSPDriver.cc: implementation of RSPDriver class
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

#include <lofar_config.h>
#include <Common/LofarLogger.h>
#include <Common/LofarLocators.h>

#include <GCF/GCF_ServiceInfo.h>

#include <APL/RTCCommon/PSAccess.h>
#include <APL/RTCCommon/daemonize.h>
#include <APS/ParameterSet.h>

#include <APL/RSP_Protocol/RSP_Protocol.ph>
#include <APL/RSP_Protocol/EPA_Protocol.ph>
#include <APL/RSP_Protocol/MEPHeader.h>

#include <blitz/array.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "RSPDriver.h"
#include "Command.h"
#include "StationSettings.h"
#include "SetWeightsCmd.h"
#include "GetWeightsCmd.h"
#include "SetSubbandsCmd.h"
#include "GetSubbandsCmd.h"
#include "UpdSubbandsCmd.h"
#include "GetStatusCmd.h"
#include "UpdStatusCmd.h"
#include "SetRCUCmd.h"
#include "GetRCUCmd.h"
#include "UpdRCUCmd.h"
#include "SetHBACmd.h"
#include "GetHBACmd.h"
#include "UpdHBACmd.h"
#include "SetRSUCmd.h"
#include "SetWGCmd.h"
#include "GetWGCmd.h"
#include "GetVersionsCmd.h"
#include "GetStatsCmd.h"
#include "UpdStatsCmd.h"
#include "GetXCStatsCmd.h"
#include "UpdXCStatsCmd.h"
#include "SetClocksCmd.h"
#include "GetClocksCmd.h"
#include "UpdClocksCmd.h"
#include "GetRegisterStateCmd.h"
#include "UpdRegisterStateCmd.h"

#include "RSUWrite.h"
#include "BSWrite.h"
#include "BWWrite.h"
#include "BWRead.h"
#include "SSWrite.h"
#include "SSRead.h"
#include "RCUWrite.h"
#include "RCUProtocolWrite.h"
#include "RCUResultRead.h"
#include "HBAProtocolWrite.h"
#include "HBAResultRead.h"
#include "RCURead.h"
#include "StatusRead.h"
#include "SstRead.h"
#include "BstRead.h"
#include "XstRead.h"
#include "WGWrite.h"
#include "WGRead.h"
#include "VersionsRead.h"
#include "WriteReg.h"
#include "ReadReg.h"
#include "CDOWrite.h"
#include "TDSProtocolWrite.h"
#include "TDSResultRead.h"
#include "RADWrite.h"

#include "Cache.h"
#include "RawEvent.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#ifdef _POSIX_PRIORTY_SCHEDULING
#include <sched.h>
#endif
#endif
#ifdef HAVE_SYS_MMAN_H
#ifdef _POSIX_MEMLOCK
#include <sys/mman.h>
#endif
#endif

#define ETHERTYPE_EPA 0x10FA

using namespace blitz;
using namespace std;
using namespace LOFAR;
using namespace GCFCommon;
using namespace RSP;
using namespace RTC;

static const uint8 g_CR_SOFTCLEAR   = 0x1;
static const uint8 g_CR_SOFTSYNC    = 0x1;
static const uint8 g_CR_SYNCDISABLE = 0x1;

static const EPA_Protocol::RSUReset  g_RSU_RESET_SYNC  = { 1, 0, 0, 0 }; // Send SYNC pulse to all FPGA's
static const EPA_Protocol::RSUReset  g_RSU_RESET_CLEAR = { 0, 1, 0, 0 }; // Clear all FPGA's
static const EPA_Protocol::RSUReset  g_RSU_RESET_RESET = { 0, 0, 1, 0 }; // Reset all FPGA's and reconfigure BP with factory image

static int32    g_instancenr = -1;
static bool     g_daemonize  = false;

//
// parseOptions
//
void parseOptions(int           argc,
                  char**        argv)
{
  static struct option long_options[] = {
    { "instance",   required_argument, 0, 'I' },
    { "daemon",     no_argument,       0, 'd' },
    { 0, 0, 0, 0 },
  };

  optind = 0; // reset option parsing
  for(;;) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "dI:", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 'I':   // --instance
      g_instancenr = atoi(optarg);
      break;
    case 'd':   // --daemon
      g_daemonize = true;
      break;
    default:
      LOG_FATAL (formatString("Unknown option %c", c));
      ASSERT(false);
    } // switch
  } // for loop

}

//
// RSPDriver(name)
//
RSPDriver::RSPDriver(string name)
  : GCFTask((State)&RSPDriver::initial, name), m_board(0), m_scheduler(),
    m_update_counter(0), m_n_updates(0), m_elapsed(0)
#ifdef HAVE_SYS_TIMEPPS_H
  , m_ppsfd(-1), m_ppshandle(0)
#endif
{
#ifdef HAVE_SYS_TIMEPPS_H
  memset(&m_ppsinfo, 0, sizeof(pps_info_t));
#endif

  // first initialize the global settins
  LOG_DEBUG("Setting up station settings");
  StationSettings*      ssp = StationSettings::instance();
  ssp->setMaxRspBoards  (GET_CONFIG("RS.N_RSPBOARDS", i));
  ssp->setNrRspBoards   (GET_CONFIG("RS.N_RSPBOARDS", i));
  ssp->setNrBlpsPerBoard(GET_CONFIG("RS.N_BLPS", i));
  if (GET_CONFIG("RSPDriver.OPERATION_MODE", i) == MODE_SUBSTATION) {
    ssp->setNrRspBoards(1);
  };
  ASSERTSTR (g_instancenr <= StationSettings::instance()->maxRspBoards(),
                 "instancenumber larger than MAX_RSPBOARDS");
  ASSERTSTR ((GET_CONFIG("RSPDriver.OPERATION_MODE" ,i) == MODE_SUBSTATION) == 
        (g_instancenr != -1), 
        "--instance option does not match OPERATION_MODE setting");
  LOG_DEBUG_STR (*ssp);

  int mode = GET_CONFIG("RSPDriver.SYNC_MODE", i);
  if (mode < SYNC_SOFTWARE || mode > SYNC_PPS) {
    LOG_FATAL_STR("Invalid SYNC_MODE: " << mode);
    exit(EXIT_FAILURE);
  }

  // tell broker we are here
  LOG_DEBUG("Registering protocols");
  registerProtocol(RSP_PROTOCOL, RSP_PROTOCOL_signalnames);
  registerProtocol(EPA_PROTOCOL, EPA_PROTOCOL_signalnames);

#if 0
  // connect to clock
  LOG_DEBUG("Connecting to clock");
  m_clock.init(*this, "spid", GCFPortInterface::SAP, 0 /*don't care*/, true /*raw*/);
#endif

  // open client port
  LOG_DEBUG("Opening listener for clients");
//   string  acceptorID;
//   if (g_instancenr>=0) {
//     acceptorID = formatString("(%d)", g_instancenr);
//   }
  m_acceptor.init(*this, MAC_SVCMASK_RSPDRIVER, GCFPortInterface::MSPP, RSP_PROTOCOL);

  // open port with RSP board
  LOG_DEBUG("Connecting to RSPboards");
  m_board = new GCFETHRawPort[StationSettings::instance()->nrRspBoards()];
  ASSERT(m_board);

  //
  // Attempt access of RSPDriver.MAC_BASE, if it fails use the RSPDriver.ADDR0
  // parameters.
  //
  bool bUseMAC_BASE = true;
  try         { (void)GET_CONFIG("RSPDriver.MAC_BASE",i); }
  catch (...) { bUseMAC_BASE = false;                     }

  char boardname[64];
  char paramname[64];
  char macaddrstr[64];
  for (int boardid = 0; boardid < StationSettings::instance()->nrRspBoards(); boardid++)
  {
    snprintf(boardname, 64, "board%d", boardid);

    if (bUseMAC_BASE)
    {
      snprintf(macaddrstr, 64, "00:00:00:00:00:%02x", boardid + GET_CONFIG("RSPDriver.MAC_BASE", i));
    }
    else
    {
      snprintf(paramname, 64, "RSPDriver.MAC_ADDR_%d", boardid);
      strncpy(macaddrstr, GET_CONFIG_STRING(paramname), 64);
    }

    LOG_DEBUG_STR("initializing board " << boardname << ":" << macaddrstr);
    m_board[boardid].init(*this, boardname, GCFPortInterface::SAP, EPA_PROTOCOL,true /*raw*/);
    m_board[boardid].setAddr(GET_CONFIG_STRING("RSPDriver.IF_NAME"), macaddrstr);

    // set ethertype to 0x10FA so Ethereal can decode EPA messages
    m_board[boardid].setEtherType(ETHERTYPE_EPA);
  }

  addAllSyncActions();
}

//
// ~RSPDriver()
//
RSPDriver::~RSPDriver()
{
  delete [] m_board;
}

//
// isEnabled()
//
bool RSPDriver::isEnabled()
{
  bool enabled = true;
  for (int boardid = 0; boardid < StationSettings::instance()->nrRspBoards(); boardid++)
  {
    if (!m_board[boardid].isConnected())
    {
      enabled = false;
      break;
    }
  }

#if 0
  // m_clock is only used in SYNC_PARALLEL
  if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PARALLEL)
  {
    enabled = enabled && m_clock.isConnected();
  }
#endif
  
  return enabled;
}

/**
 * Add all synchronization actions per board.
 * Order is:
 * - BS:      write syncrhonization settings    // BSWrite
 * - STATUS (RSP Status): read RSP status info  // StatusRead
 * - BF:      write beamformer weights          // BWWrite
 * - SS:      write subband selection settings  // SSWrite
 * - RCU:     write RCU control settings        // RCUWrite/RCUProtocolWrite/RCUResultRead
 * - HBA:     write HBA control settings        // HBAProtocolWrite/HBAResultRead
 * - TDS:     write TDS control settings        // TDSProtocolWrite/TDSResultRead
 * - SST:     read subband statistics           // SstRead
 * - BST:     read beamlet statistics           // BstRead
 * - XST:     read crosslet statistics          // XstRead
 * - WG:      write waveform generator settings // WGWrite
 * - VERSION: read version info                 // VersionsRead
 *
 * For testing purposes, read back register that have just been written
 * - BF:  read beamformer weights          // BWRead
 * - SS:  read subbands selection settings // SSRead
 * - RCU: read RCU control settings        // RCURead
 * - WG:  read waveform generator settings // WGRead
 */
void RSPDriver::addAllSyncActions()
{
  /**
   * For each board a separate BWSync instance is created which handles
   * the synchronization of data between the board and the cache for that board.
   */
  for (int boardid = 0; boardid < StationSettings::instance()->nrRspBoards(); boardid++)
  {
    /*
     * Schedule register writes for soft PPS if configured.
     *
     * - This means disabling the external sync on all FPGA's
     *   by broadcasting a CR_CONTROL write.
     * - Requesting a soft PPS by writing a 1 in the SYNC bit
     *   of the RSU_RESET register.
     */  
    if (1 == GET_CONFIG("RSPDriver.SOFTPPS", i))
    {
      WriteReg* writereg = 0;

      // Disable External Sync for all FPGA's
      writereg = new WriteReg(m_board[boardid], boardid,
                              MEPHeader::DST_ALL,
                              MEPHeader::CR,
                              MEPHeader::CR_SYNCDISABLE,
                              MEPHeader::CR_CONTROL_SIZE);
      ASSERT(writereg);
      writereg->setSrcAddress((void*)&g_CR_SYNCDISABLE);
      m_scheduler.addSyncAction(writereg);

      // Send PPS to BP which sends signal to configuration device
      // which in turn sends a PPS to the AP's
      writereg = new WriteReg(m_board[boardid], boardid,
                              MEPHeader::DST_RSP,
                              MEPHeader::RSU,
                              MEPHeader::RSU_RESET,
                              MEPHeader::RSU_RESET_SIZE);
      ASSERT(writereg);
      writereg->setSrcAddress((void*)&g_RSU_RESET_SYNC);
      m_scheduler.addSyncAction(writereg);
    }

    /*
     * Clear the board if needed (this needs to be first after SOFT_PPS)
     */
    if (1 == GET_CONFIG("RSPDriver.WRITE_RSU", i))
    {
      RSUWrite* rsuwrite = new RSUWrite(m_board[boardid], boardid, m_scheduler);
      ASSERT(rsuwrite);
      m_scheduler.addSyncAction(rsuwrite);
    }

    /*
     * Set correct number of samples per interval
     * This needs to be done before RCU and WG registers.
     */
    if (1 == GET_CONFIG("RSPDriver.WRITE_BS", i))
    {
      for (int blp = 0; blp < StationSettings::instance()->nrBlpsPerBoard(); blp++) {
        BSWrite* bswrite = new BSWrite(m_board[boardid], boardid, blp);
        ASSERT(bswrite);
        m_scheduler.addSyncAction(bswrite);
      }
    }

    // order is important; TDSProtocolWrite should be added before TDSResultRead
    if (1 == GET_CONFIG("RSPDriver.WRITE_TDS_PROTOCOL", i))
    {
      TDSProtocolWrite* tdsprotocolwrite = new TDSProtocolWrite(m_board[boardid], boardid);
      ASSERT(tdsprotocolwrite);
      m_scheduler.addSyncAction(tdsprotocolwrite);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_TDS_RESULT", i))
    {
      TDSResultRead* tdsresultread = new TDSResultRead(m_board[boardid], boardid);
      ASSERT(tdsresultread);
      m_scheduler.addSyncAction(tdsresultread);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_STATUS", i))
    {
      StatusRead* statusread = new StatusRead(m_board[boardid], boardid);
      ASSERT(statusread);
      m_scheduler.addSyncAction(statusread);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_VERSION", i))
    {
      VersionsRead* versionread = new VersionsRead(m_board[boardid], boardid);
      ASSERT(versionread);
      m_scheduler.addSyncAction(versionread);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_BST", i))
    {
      BstRead* bstread = 0;

      bstread = new BstRead(m_board[boardid], boardid);
      ASSERT(bstread);
      m_scheduler.addSyncAction(bstread);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_XST", i))
    {
      XstRead* xstread = 0;

      for (int regid = MEPHeader::XST_STATS; regid < MEPHeader::XST_NR_STATS; regid++)
      {
        xstread = new XstRead(m_board[boardid], boardid, regid);
        ASSERT(xstread);
        m_scheduler.addSyncAction(xstread);
      }
    }

    if (1 == GET_CONFIG("RSPDriver.WRITE_CDO", i))
    {
      char dstip[64];
      char srcip[64];
      char dstmac[64];
      snprintf(srcip,  64, "RSPDriver.SRC_IP_ADDR_%d", boardid);
      snprintf(dstip,  64, "RSPDriver.DST_IP_ADDR_%d", boardid);
      snprintf(dstmac, 64, "RSPDriver.DST_MAC_ADDR_%d", boardid);

      CDOWrite* cdowrite = new CDOWrite(m_board[boardid], boardid,
                                        GET_CONFIG_STRING(srcip),
                                        GET_CONFIG_STRING(dstip),
                                        GET_CONFIG_STRING(dstmac));

      m_scheduler.addSyncAction(cdowrite);

      if (1 == GET_CONFIG("RSPDriver.READ_CDO", i)) {
	// Read back CDO header contents
	ReadReg* readreg = new ReadReg(m_board[boardid], boardid,
				       MEPHeader::DST_RSP,
				       MEPHeader::CDO,
				       MEPHeader::CDO_HEADER,
				       MEPHeader::CDO_HEADER_SIZE);
	ASSERT(readreg);
	m_scheduler.addSyncAction(readreg);
      }
    }

    if (1 == GET_CONFIG("RSPDriver.WRITE_RAD", i))
    {
      RADWrite* radwrite = new RADWrite(m_board[boardid], boardid);
      m_scheduler.addSyncAction(radwrite);
    }

    for (int action = 0; action < 2; action++)
    {
      if (action == GET_CONFIG("RSPDriver.LOOPBACK_MODE", i))
      {
        if (1 == GET_CONFIG("RSPDriver.WRITE_WG", i))
        {
          WGWrite* wgwrite = new WGWrite(m_board[boardid], boardid);
          ASSERT(wgwrite);
          m_scheduler.addSyncAction(wgwrite);
        }
      }
      else
      {
        if (1 == GET_CONFIG("RSPDriver.READ_WG", i))
        {
          WGRead* wgread = new WGRead(m_board[boardid], boardid);
          ASSERT(wgread);
          m_scheduler.addSyncAction(wgread);
        }
      }
    }

    for (int action = 0; action < 2; action++)
    {
      if (action == GET_CONFIG("RSPDriver.LOOPBACK_MODE", i))
      {
        if (1 == GET_CONFIG("RSPDriver.WRITE_SS", i))
        {
          SSWrite* sswrite = new SSWrite(m_board[boardid], boardid);
          ASSERT(sswrite);
          m_scheduler.addSyncAction(sswrite);
        }
      }
      else
      {
        if (1 == GET_CONFIG("RSPDriver.READ_SS", i))
        {
          SSRead* ssread = new SSRead(m_board[boardid], boardid);
          ASSERT(ssread);
          m_scheduler.addSyncAction(ssread);
        }
      }
    }

    //
    // Depending on the value of RSPDriver.LOOPBACK_MODE either the 
    // WRITE is done first or the READ is done first.
    //
    // If LOOPBACK_MODE == 0, the WRITE is done first.
    // In this mode you can check with Ethereal that what was
    // written is correctly read back from the board. This can
    // be used to check that the RSP hardware or the EPAStub
    // function correctly.
    //
    // If LOOPBACK_MODE == 1, the READ is done first.
    // In this mode you can check with Ethereal that what was
    // read from the EPAStub is written back in the same way.
    // This is used to check whether the RSPDriver stores the
    // information at the correct location in its cache.
    //
    // This is done in the same way for all read/write registers.
    //
    for (int action = 0; action < 2; action++)
    {
      if (action == GET_CONFIG("RSPDriver.LOOPBACK_MODE", i))
      {
        if (1 == GET_CONFIG("RSPDriver.WRITE_BF", i))
        {
          BWWrite* bwsync = 0;

          for (int blp = 0; blp < StationSettings::instance()->nrBlpsPerBoard(); blp++) {
            bwsync = new BWWrite(m_board[boardid], boardid, blp, MEPHeader::BF_XROUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWWrite(m_board[boardid], boardid, blp, MEPHeader::BF_XIOUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWWrite(m_board[boardid], boardid, blp, MEPHeader::BF_YROUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWWrite(m_board[boardid], boardid, blp, MEPHeader::BF_YIOUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
          }
        }
      }
      else
      {
        if (1 == GET_CONFIG("RSPDriver.READ_BF", i))
        {
          BWRead* bwsync = 0;

          for (int blp = 0; blp < StationSettings::instance()->nrBlpsPerBoard(); blp++) {
            bwsync = new BWRead(m_board[boardid], boardid, blp, MEPHeader::BF_XROUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWRead(m_board[boardid], boardid, blp, MEPHeader::BF_XIOUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWRead(m_board[boardid], boardid, blp, MEPHeader::BF_YROUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
            bwsync = new BWRead(m_board[boardid], boardid, blp, MEPHeader::BF_YIOUT);
            ASSERT(bwsync);
            m_scheduler.addSyncAction(bwsync);
          }
        }
      }
    }
    
    if (1 == GET_CONFIG("RSPDriver.READ_SST", i))
    {
      SstRead* sstread = 0;

      sstread = new SstRead(m_board[boardid], boardid);
      ASSERT(sstread);
      m_scheduler.addSyncAction(sstread);

    }

    for (int action = 0; action < 2; action++)
    {
      if (action == GET_CONFIG("RSPDriver.LOOPBACK_MODE", i))
      {
        if (1 == GET_CONFIG("RSPDriver.WRITE_RCU", i))
        {
          RCUWrite* rcuwrite = new RCUWrite(m_board[boardid], boardid);
          ASSERT(rcuwrite);
          m_scheduler.addSyncAction(rcuwrite);
        }
      }
      else
      {
        if (1 == GET_CONFIG("RSPDriver.READ_RCU", i))
        {
          RCURead* rcuread = new RCURead(m_board[boardid], boardid);
          ASSERT(rcuread);
          m_scheduler.addSyncAction(rcuread);
        }
      }
    }

    // order is important; RCUProtocolWrite should go before RCUResultRead
    if (1 == GET_CONFIG("RSPDriver.WRITE_RCU_PROTOCOL", i))
    {
      RCUProtocolWrite* rcuprotocolwrite = new RCUProtocolWrite(m_board[boardid], boardid);
      ASSERT(rcuprotocolwrite);
      m_scheduler.addSyncAction(rcuprotocolwrite);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_RCU_RESULT", i))
    {
      RCUResultRead* rcuresultread = new RCUResultRead(m_board[boardid], boardid);
      ASSERT(rcuresultread);
      m_scheduler.addSyncAction(rcuresultread);
    }

    // order is important; HBAProtocolWrite should go before HBAResultRead,
    // these two should go before RCUProtocolWrite/RCUResultRead
    if (1 == GET_CONFIG("RSPDriver.WRITE_HBA_PROTOCOL", i))
    {
      HBAProtocolWrite* hbaprotocolwrite = new HBAProtocolWrite(m_board[boardid], boardid);
      ASSERT(hbaprotocolwrite);
      m_scheduler.addSyncAction(hbaprotocolwrite);
    }

    if (1 == GET_CONFIG("RSPDriver.READ_HBA_RESULT", i))
    {
      HBAResultRead* hbaresultread = new HBAResultRead(m_board[boardid], boardid);
      ASSERT(hbaresultread);
      m_scheduler.addSyncAction(hbaresultread);
    }

  } // for (boardid...)
}

//
// openBoards()
//
void RSPDriver::openBoards()
{
  for (int boardid = 0; boardid < StationSettings::instance()->nrRspBoards(); boardid++)
  {
    if (!m_board[boardid].isConnected()) m_board[boardid].open();
  }
}

//
// Fetch the PPS signal, retry if needed
//
int RSPDriver::fetchPPS()
{
  int result = 0;
#ifdef HAVE_SYS_TIMEPPS_H
  do {
    result = time_pps_fetch(m_ppshandle, PPS_TSFMT_TSPEC, &m_ppsinfo, NULL);
  } while ((result < 0) && (EINTR == errno || EAGAIN == errno));
#else
  LOG_FATAL("fetchPPS should not be called when HAVE_SYS_TIMEPPS_H is not defined");
  exit(EXIT_FAILURE);
#endif
  return result;
}

//
// initial(event, port)
//
GCFEvent::TResult RSPDriver::initial(GCFEvent& event, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;
  
  switch(event.signal)
  {
    case F_INIT:
    {
      if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PPS) {
#ifdef HAVE_SYS_TIMEPPS_H
        pps_params_t parm;

        // standard time format and trigger on rising edge, API version 1
        memset(&parm, 0, sizeof(pps_params_t));
        parm.mode = PPS_TSFMT_TSPEC;
        parm.mode |= PPS_CAPTUREASSERT; // trigger on ASSERT
        parm.api_version = PPS_API_VERS_1;

        if ((m_ppsfd = open(GET_CONFIG_STRING("RSPDriver.PPS_DEVICE"), O_RDWR)) < 0) {
          LOG_FATAL_STR("Error opening '" << GET_CONFIG_STRING("RSPDriver.PPS_DEVICE") << "': " << strerror(errno));
          exit(EXIT_FAILURE);
        }

        if (time_pps_create(m_ppsfd, &m_ppshandle) < 0) {
          LOG_FATAL_STR("PPS device does not support PPS API?: " << strerror(errno));
          exit(EXIT_FAILURE);
        }

        int caps = 0;
        if (time_pps_getcap(m_ppshandle, &caps) < 0) {
          LOG_FATAL_STR("Cannot get PPS device capabilities:" << strerror(errno));
          exit(EXIT_FAILURE);
        }
        LOG_INFO(formatString("PPS device capabilities are 0x%x\n", caps));

#if 0
        if (!(caps & PPS_CANWAIT & PPS_CAPTUREASSERT)) {
          LOG_FATAL("PPS device does not support PPS_CANWAIT & PPS_CAPTUREASSERT. PPS device unsuitable.");
          exit(EXIT_FAILURE);
        }
#endif

        if (time_pps_setparams(m_ppshandle, &parm) < 0) {
          LOG_FATAL_STR("Error settings parameters on PPS device:" << strerror(errno));
          exit(EXIT_FAILURE);
        }

        // now we're setup to use time_pps_fetch...
#else
        LOG_FATAL("HAVE_SYS_TIMEPPS_H not defined. Platform doesn't support PPSkit interface.");
        exit(EXIT_FAILURE);
#endif
      }
    }
    break;

    case F_ENTRY:
    {
#if 0
      if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PARALLEL)
      {
        if (!m_clock.isConnected()) m_clock.open();
      }
#endif
      openBoards();
    }
    break;

    case F_CONNECTED:
    {
      LOG_INFO(formatString("CONNECTED: port '%s'", port.getName().c_str()));
      if (isEnabled())
      {
        TRAN(RSPDriver::enabled);
      }
    }
    break;

    case F_DISCONNECTED:
    {
      port.setTimer((long)3); // try again in 3 seconds
      LOG_WARN(formatString("port '%s' disconnected, retry in 3 seconds...", port.getName().c_str()));
      port.close();
    }
    break;

    case F_TIMER:
    {
      LOG_DEBUG(formatString("port '%s' retry of open...", port.getName().c_str()));
      port.open();
    }
    break;

    case F_DATAIN:
    {
#if 0
      if (&port == &m_clock)
      {
        /**
         * We don't need the clock here yet, simply read the value
         * and ignore
         */
        uint8 count = 0;
        (void)port.recv(&count, sizeof(uint8));
      }
      else
      {
#endif
        // ignore in this state
        static char buf[ETH_DATA_LEN];
        (void)port.recv(buf, ETH_DATA_LEN);
#if 0
      }
#endif
    }
    break;

    case F_EXIT:
    {
      // cancel timers
      m_board[0].cancelAllTimers();
    }
    break;

    default:
      status = GCFEvent::NOT_HANDLED;
      break;
  }

  return status;
}

//
// undertaker()
//
void RSPDriver::undertaker()
{
  for (list<GCFPortInterface*>::iterator it = m_dead_clients.begin();
       it != m_dead_clients.end();
       it++)
  {
    delete (*it);
  }
  m_dead_clients.clear();
}

//
// enabled(event, port)
//
GCFEvent::TResult RSPDriver::enabled(GCFEvent& event, GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::HANDLED;

  undertaker(); // destroy dead clients

  switch (event.signal)
  {
    case F_ENTRY:
    {
      // start waiting for clients
      if (!m_acceptor.isConnected()) m_acceptor.open();

      if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_SOFTWARE)
      {
        /* Start the update timer after 1 second */
        m_board[0].setTimer(1.0,
                            GET_CONFIG("RSPDriver.SYNC_INTERVAL", f)); // update SYNC_INTERVAL seconds
      }
      else if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_FAST)
      {
        //
        // single timeout after 1 second to set
        // off as-fast-as-possible update mode
        //
        m_board[0].setTimer(1.0);

        //
        // periodic timeout on m_acceptor port
        // to print average nr of updates per second
        //
        m_acceptor.setTimer(1.0, 1.0); // every second after 1.0 second
      }
      else if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PPS)
      {
#ifdef HAVE_SYS_TIMEPPS_H
        //
        // read away most recent timestamp..
        //
        if ( fetchPPS() < 0) {
            LOG_FATAL_STR("Error clearing startup PPS: " << strerror(errno));
            exit(EXIT_FAILURE);
        }

        //
        // and wait for next timestamp to get in sync immediately
        //
        if ( fetchPPS() < 0 ) {
            LOG_FATAL_STR("Error fetching first PPS: " << strerror(errno));
            exit(EXIT_FAILURE);
        }

        // start a single shot timer that is slightly shorter that 1 second
        // when the timer expires, wait for the true PPS using time_pps_fetch
        m_board[0].setTimer(0.99); // 1st event after 1 second
#else
        LOG_WARN("HAVE_SYS_TIMEPPS_H not defined. Platform doesn't support PPSkit interface. Using software timer.");
        m_board[0].setTimer((long)1); // next event in one (software) second
#endif
      }
    }
    break;

    case F_ACCEPT_REQ:
    {
      GCFTCPPort* client = new GCFTCPPort();
      client->init(*this, "client", GCFPortInterface::SPP, RSP_PROTOCOL);
      m_acceptor.accept(*client);
      m_client_list.push_back(client);

      LOG_INFO(formatString("NEW CLIENT CONNECTED: %d clients connected", m_client_list.size()));
    }
    break;

    case F_CONNECTED:
    {
      LOG_INFO(formatString("CONNECTED: port '%s'", port.getName().c_str()));
    }
    break;

    case F_DATAIN:
    {
#if 0
      if (&port == &m_clock)
      {
        status = clock_tick(port);
      }
      else
      {
#endif
        status = RawEvent::dispatch(*this, port);
#if 0
      }
#endif
    }
    break;
    
    case RSP_SETWEIGHTS:
      rsp_setweights(event, port);
      break;

    case RSP_GETWEIGHTS:
      rsp_getweights(event, port);
      break;

    case RSP_SETSUBBANDS:
      rsp_setsubbands(event, port);
      break;

    case RSP_GETSUBBANDS:
      rsp_getsubbands(event, port);
      break;

    case RSP_SUBSUBBANDS:
      rsp_subsubbands(event, port);
      break;
      
    case RSP_UNSUBSUBBANDS:
      rsp_unsubsubbands(event, port);
      break;

    case RSP_SETRCU:
      rsp_setrcu(event, port);
      break;

    case RSP_GETRCU:
      rsp_getrcu(event, port);
      break;
      
    case RSP_SUBRCU:
      rsp_subrcu(event, port);
      break;
      
    case RSP_UNSUBRCU:
      rsp_unsubrcu(event, port);
      break;
      
    case RSP_SETHBA:
      rsp_sethba(event, port);
      break;

    case RSP_GETHBA:
      rsp_gethba(event, port);
      break;
      
    case RSP_SUBHBA:
      rsp_subhba(event, port);
      break;
      
    case RSP_UNSUBHBA:
      rsp_unsubhba(event, port);
      break;
      
    case RSP_SETRSU:
      rsp_setrsu(event, port);
      break;

    case RSP_SETWG:
      rsp_setwg(event, port);
      break;
      
    case RSP_GETWG:
      rsp_getwg(event, port);
      break;
      
    case RSP_SUBSTATUS:
      rsp_substatus(event, port);
      break;
      
    case RSP_UNSUBSTATUS:
      rsp_unsubstatus(event, port);
      break;

    case RSP_GETSTATUS:
      rsp_getstatus(event, port);
      break;
      
    case RSP_SUBSTATS:
      rsp_substats(event, port);
      break;
      
    case RSP_UNSUBSTATS:
      rsp_unsubstats(event, port);
      break;
      
    case RSP_GETSTATS:
      rsp_getstats(event, port);
      break;

    case RSP_SUBXCSTATS:
      rsp_subxcstats(event, port);
      break;

    case RSP_UNSUBXCSTATS:
      rsp_unsubxcstats(event, port);
      break;

    case RSP_GETXCSTATS:
      rsp_getxcstats(event, port);
      break;

    case RSP_GETVERSION:
      rsp_getversions(event, port);
      break;

    case RSP_GETCONFIG:
      rsp_getconfig(event, port);
      break;

    case RSP_SETCLOCK:
      rsp_setclock(event, port);
      break;

    case RSP_GETCLOCK:
      rsp_getclock(event, port);
      break;

    case RSP_SUBCLOCK:
      rsp_subclock(event, port);
      break;

    case RSP_UNSUBCLOCK:
      rsp_unsubclock(event, port);
      break;
     
    case RSP_GETREGISTERSTATE:
      rsp_getregisterstate(event, port);
      break;
    
    case RSP_SUBREGISTERSTATE:
      rsp_subregisterstate(event, port);
      break;
    
    case RSP_UNSUBREGISTERSTATE:
      rsp_unsubregisterstate(event, port);
      break;

    case F_TIMER:
    {
      if (&port == &m_board[0])
      {
        //
        // If SYNC_MODE == SOFTWARE|FAST then run the scheduler
        // directly on the software timer.
        //
        if (   (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_SOFTWARE)
            || (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_FAST))
        {
          (void)clock_tick(m_acceptor); // force clock tick
        }
        else if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PPS)
        {
          GCFTimerEvent timer;

#ifdef HAVE_SYS_TIMEPPS_H
          const GCFTimerEvent* timeout = static_cast<const GCFTimerEvent*>(&event);
          pps_info_t prevppsinfo = m_ppsinfo;

          if ( fetchPPS() < 0 ) {

            LOG_WARN_STR("Error fetching PPS: " << strerror(errno));
            
            // print time, ugly
            char timestr[32];
            strftime(timestr, 32, "%T", gmtime(&timeout->sec));
            LOG_INFO(formatString("TICK: time=%s.%06d UTC (not PPS)", timestr, timeout->usec));

            timer.sec  = timeout->sec;
            timer.usec = timeout->usec;
            timer.id   = 0;
            timer.arg  = 0;

            m_board[0].setTimer((long)1); // next event after exactly 1 second
          
          } else {

            // print time of day, ugly
            char timestr[32];
            strftime(timestr, 32, "%T", gmtime(&timeout->sec));
            LOG_INFO(formatString("TICK: time=%s.%06d UTC (timeout)", timestr, timeout->usec));

            // print time of day, ugly
            strftime(timestr, 32, "%T", gmtime(&m_ppsinfo.assert_timestamp.tv_sec));
            LOG_INFO(formatString("TICK: PPS_time=%s.%06d UTC", timestr, m_ppsinfo.assert_timestamp.tv_nsec / 1000));
            
            /* construct a timer event */
            timer.sec  = m_ppsinfo.assert_timestamp.tv_sec;
            timer.usec = m_ppsinfo.assert_timestamp.tv_nsec / 1000;
            timer.id   = 0;
            timer.arg  = 0;

            /* check for missed PPS */
            if (prevppsinfo.assert_sequence + 1 != m_ppsinfo.assert_sequence) {
              LOG_WARN_STR("Missed " << m_ppsinfo.assert_sequence - prevppsinfo.assert_sequence - 1 << " PPS events.");
            }         
          }
          m_board[0].setTimer(0.95); // next event in just under 1 second
#else
          m_board[0].setTimer((long)1); // next event in one (software) second
#endif

          /* run the scheduler with the timer event */
          status = m_scheduler.run(timer, port);
        }
      }
      else if (&port == &m_acceptor)
      {
        // print average number of updates
        cerr << "Updates per second = " << m_update_counter << endl;

        if (m_update_counter > 0)
        {
          m_n_updates += m_update_counter;
          m_elapsed++;
          
          cerr << "Average number of updates per second = " << (double)m_n_updates / m_elapsed << endl;
        }
        
        m_update_counter = 0;
      }
    }
    break;

    case F_DISCONNECTED:
    {
      LOG_INFO(formatString("DISCONNECTED: port '%s'", port.getName().c_str()));
      port.close();

      if (&port == &m_acceptor)
      {
        LOG_FATAL("Failed to start listening for client connections.");
        exit(EXIT_FAILURE);
      }
      else if (&port == &m_board[0] || &port == &m_board[1] || &port == &m_acceptor)
      {
        m_acceptor.close();
        TRAN(RSPDriver::initial);
      }
      else
      {
        /* cancel all commands for this port */
        (void)m_scheduler.cancel(port);

        m_client_list.remove(&port);
        m_dead_clients.push_back(&port);
      }
    }
    break;

    case F_EXIT:
    {
      // cancel timers
      m_acceptor.cancelAllTimers();
      m_board[0].cancelAllTimers();
    }
    break;

    default:
      if (isBoardPort(port))
      {
        status = m_scheduler.dispatch(event, port);

        //
        // if SYNC_FAST mode and sync has completed
        // send new clock_tick
        //
        if ((GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_FAST) &&
            m_scheduler.syncHasCompleted())
        {
          m_board[0].setTimer(0.0); // immediate
          m_update_counter++;
        }
      }
      break;
  }

  return status;
}

//
// isBoardPort(port)
//
bool RSPDriver::isBoardPort(GCFPortInterface& port)
{
  /**
   * The addresses of the elements of the m_board array
   * are consecutive in memory, therefor we can do a range
   * check on the address to determine whether it is a port
   * to a board.
   */
  if (   &port >= &m_board[0]
      && &port <= &m_board[StationSettings::instance()->nrRspBoards()])
    return true;
  
  return false;
}

//
// clock_tick(port)
//
GCFEvent::TResult RSPDriver::clock_tick(GCFPortInterface& port)
{
  GCFEvent::TResult status = GCFEvent::NOT_HANDLED;

#if 0
  uint8 count = '0';
  if (GET_CONFIG("RSPDriver.SYNC_MODE", i) == SYNC_PARALLEL)
  {
    if (port.recv(&count, sizeof(uint8)) != 1)
    {
      LOG_WARN("We got a signal, but there is no clock pulse!");
    }

    count -= '0'; // convert to integer
    if (count > 1)
    {
      LOG_WARN("Got more than one clock pulse: missed real-time deadline");
    }
  }
#endif
          
  struct timeval now;
  (void)gettimeofday(&now, 0);

  // print time, ugly
  char timestr[32];
  strftime(timestr, 32, "%T", gmtime(&now.tv_sec));
  LOG_INFO(formatString("TICK: time=%s.%06d UTC", timestr, now.tv_usec));

  /* construct a timer event */
  GCFTimerEvent timer;
  timer.sec  = now.tv_sec;
  timer.usec = now.tv_usec;
  timer.id   = 0;
  timer.arg  = 0;
          
  /* run the scheduler with the timer event */
  status = m_scheduler.run(timer, port);

  return status;
}

//
// rsp_setweights(event,port)
//
void RSPDriver::rsp_setweights(GCFEvent& event, GCFPortInterface& port)
{
  /**
   * Create a separate command for each timestep for which
   * weights are contained in the event.
   */

  /* unpack the event */
  RSPSetweightsEvent* sw_event = new RSPSetweightsEvent(event);

  /* range check on parameters */
  if ((sw_event->weights().dimensions() != BeamletWeights::NDIM)
      || (sw_event->weights().extent(firstDim) < 1)
      || (sw_event->weights().extent(secondDim) > StationSettings::instance()->nrRcus())
      || (sw_event->weights().extent(thirdDim) != MEPHeader::N_BEAMLETS))
  {
    LOG_ERROR("SETWEIGHTS: invalid parameter");
    
    delete sw_event;
    
    RSPSetweightsackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  for (int timestep = 0; timestep < sw_event->weights().extent(firstDim); timestep++)
  {
    Ptr<SetWeightsCmd> command = new SetWeightsCmd(*sw_event, port, Command::WRITE, timestep);

    if (0 == timestep)
    {
      command->ack(Cache::getInstance().getFront());
    }
        
    command->setWeights(sw_event->weights()(Range(timestep, timestep),
                                            Range::all(),
                                            Range::all()));
    
    //
    // if weights for only one timestep are given
    // and the timestamp == Timestamp(0,0)
    // then apply the weights immediately
    //
    if ((1 == sw_event->weights().extent(firstDim))
        && (Timestamp(0,0) == sw_event->timestamp))
    {
      LOG_INFO("applying beamforming weights immediately");
      command->apply(Cache::getInstance().getFront(), true);
      command->apply(Cache::getInstance().getBack(), false);
    }
    else
    {
      (void)m_scheduler.enter(Ptr<Command>(&(*command)));
    }
  }

  /* cleanup the event */
  delete sw_event;
}

//
// rsp_getweights(event,port)
//
void RSPDriver::rsp_getweights(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetWeightsCmd> command = new GetWeightsCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETWEIGHTS: invalid parameter");
    
    RSPGetweightsackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}


//
// rsp_setsubbands(event,port)
//
void RSPDriver::rsp_setsubbands(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetSubbandsCmd> command = new SetSubbandsCmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETSUBBANDS: invalid parameter");
    
    RSPSetsubbandsackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) then apply the changes immediately
  if (Timestamp(0,0) == command->getTimestamp())
  {
    LOG_INFO("applying setsubbands immediately");
    command->apply(Cache::getInstance().getFront(), true);
    command->apply(Cache::getInstance().getBack(), false);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_getsubbands(event,port)
//
void RSPDriver::rsp_getsubbands(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetSubbandsCmd> command = new GetSubbandsCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETSUBBANDS: invalid parameter");
    
    RSPGetsubbandsackEvent ack;
    ack.subbands().resize(1,1);
    ack.subbands() = 0;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subsubbands(event,port)
//
void RSPDriver::rsp_subsubbands(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdSubbandsCmd in the periodic queue
  Ptr<UpdSubbandsCmd> command = new UpdSubbandsCmd(event, port, Command::READ);
  RSPSubsubbandsackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBSUBBANDS: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubsubbands(event,port)
//
void RSPDriver::rsp_unsubsubbands(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubsubbandsEvent unsub(event);

  RSPUnsubsubbandsackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBSUBBANDS: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_setrcu(event,port)
//
void RSPDriver::rsp_setrcu(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetRCUCmd> command = new SetRCUCmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETRCU: invalid parameter");
    
    RSPSetrcuackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) apply changes immediately
  if (Timestamp(0,0) == command->getTimestamp())
  {
    LOG_INFO("applying RCU control immediately");
    command->apply(Cache::getInstance().getFront(), true);
    command->apply(Cache::getInstance().getBack(), false);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_getrcu(event,port)
//
void RSPDriver::rsp_getrcu(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetRCUCmd> command = new GetRCUCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETRCU: invalid parameter");
    
    RSPGetrcuackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    ack.settings().resize(1);
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subrcu(event,port)
//
void RSPDriver::rsp_subrcu(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdRCUCmd in the periodic queue
  Ptr<UpdRCUCmd> command = new UpdRCUCmd(event, port, Command::READ);
  RSPSubrcuackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBRCU: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubrcu(event,port)
//
void RSPDriver::rsp_unsubrcu(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubrcuEvent unsub(event);

  RSPUnsubrcuackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBRCU: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_sethba(event,port)
//
void RSPDriver::rsp_sethba(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetHBACmd> command = new SetHBACmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETHBA: invalid parameter");
    
    RSPSethbaackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) apply changes immediately
  if (Timestamp(0,0) == command->getTimestamp())
  {
    LOG_INFO("applying HBA control immediately");
    command->apply(Cache::getInstance().getFront(), true);
    command->apply(Cache::getInstance().getBack(), false);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_gethba(event,port)
//
void RSPDriver::rsp_gethba(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetHBACmd> command = new GetHBACmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETHBA: invalid parameter");
    
    RSPGethbaackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    ack.settings().resize(1, 1); // create something to pack
    ack.settings() = 0;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subhba(event,port)
//
void RSPDriver::rsp_subhba(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdHBACmd in the periodic queue
  Ptr<UpdHBACmd> command = new UpdHBACmd(event, port, Command::READ);
  RSPSubhbaackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBHBA: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubhba(event,port)
//
void RSPDriver::rsp_unsubhba(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubhbaEvent unsub(event);

  RSPUnsubhbaackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBHBA: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_setrsu(event,port)
//
void RSPDriver::rsp_setrsu(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetRSUCmd> command = new SetRSUCmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETRSU: invalid parameter");
    
    RSPSetrsuackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) apply changes immediately
  if (Timestamp(0,0) == command->getTimestamp()) {
    LOG_INFO("applying RSU control to front buffer");
    command->apply(Cache::getInstance().getFront(), true);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_setwg(event, port)
//
void RSPDriver::rsp_setwg(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetWGCmd> command = new SetWGCmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETWG: invalid parameter");
    
    RSPSetwgackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) then apply the changes immediately
  if (Timestamp(0,0) == command->getTimestamp())
  {
    LOG_INFO("applying WG settings immediately");
    command->apply(Cache::getInstance().getFront(), true);
    command->apply(Cache::getInstance().getBack(), false);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_getwg(event, port)
//
void RSPDriver::rsp_getwg(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetWGCmd> command = new GetWGCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETWG: invalid parameter");
    
    RSPGetwgackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_substatus(event,port)
//
void RSPDriver::rsp_substatus(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdStatusCmd in the periodic queue
  Ptr<UpdStatusCmd> command = new UpdStatusCmd(event, port, Command::READ);
  RSPSubstatusackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBSTATUS: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubstatus(event,port)
//
void RSPDriver::rsp_unsubstatus(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubstatusEvent unsub(event);

  RSPUnsubstatusackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBSTATUS: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_getstatus (event, port)
//
void RSPDriver::rsp_getstatus(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetStatusCmd> command = new GetStatusCmd(event, port, Command::READ);

  if (!command->validate())
  {
    command->ack_fail();
    return;
  }

  // if null timestamp get value from the cache and acknowledge immediately
  if ((Timestamp(0,0) == command->getTimestamp())
      && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_substats (event, port)
//
void RSPDriver::rsp_substats(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdStatsCmd in the periodic queue
  Ptr<UpdStatsCmd> command = new UpdStatsCmd(event, port, Command::READ);
  RSPSubstatsackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBSTATS: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubstats (event, port)
//
void RSPDriver::rsp_unsubstats(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubstatsEvent unsub(event);

  RSPUnsubstatsackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBSTATS: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_getstats (event, port)
//
void RSPDriver::rsp_getstats(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetStatsCmd> command = new GetStatsCmd(event, port, Command::READ);

  if (!command->validate())
  {
    command->ack_fail();
    return;
  }

  // if null timestamp get value from the cache and acknowledge immediately
  if ((Timestamp(0,0) == command->getTimestamp())
      && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subxcstats (event, port)
//
void RSPDriver::rsp_subxcstats(GCFEvent& event, GCFPortInterface& port)
{
  // subscription is done by entering a UpdXCStatsCmd in the periodic queue
  Ptr<UpdXCStatsCmd> command = new UpdXCStatsCmd(event, port, Command::READ);
  RSPSubxcstatsackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBXCSTATS: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubxcstats (event, port)
//
void RSPDriver::rsp_unsubxcstats(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubxcstatsEvent unsub(event);

  RSPUnsubxcstatsackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBXCSTATS: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_getxcstats (event, port)
//
void RSPDriver::rsp_getxcstats(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetXCStatsCmd> command = new GetXCStatsCmd(event, port, Command::READ);

  if (!command->validate())
  {
    command->ack_fail();
    return;
  }

  // if null timestamp get value from the cache and acknowledge immediately
  if ((Timestamp(0,0) == command->getTimestamp())
      && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_getversions (event, port)
//
void RSPDriver::rsp_getversions(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetVersionsCmd> command = new GetVersionsCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETVERSIONS: invalid parameter");
    
    RSPGetversionackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_getconfig (event, port)
//
void RSPDriver::rsp_getconfig(GCFEvent& event, GCFPortInterface& port)
{
  RSPGetconfigEvent get(event);
  RSPGetconfigackEvent ack;

  ack.n_rcus        = StationSettings::instance()->nrRcus(); 
  ack.n_rspboards   = StationSettings::instance()->nrRspBoards();
  ack.max_rspboards = StationSettings::instance()->maxRspBoards();

  port.send(ack);
}

//
// rsp_setclock (event, port)
//
void RSPDriver::rsp_setclock(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<SetClocksCmd> command = new SetClocksCmd(event, port, Command::WRITE);

  if (!command->validate())
  {
    LOG_ERROR("SETCLOCK: invalid parameter");
    
    RSPSetclockackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }

  // if timestamp == Timestamp(0,0) apply changes immediately
  if (Timestamp(0,0) == command->getTimestamp())
  {
    LOG_INFO("applying Clock control immediately");
    command->apply(Cache::getInstance().getFront(), true);
    command->apply(Cache::getInstance().getBack(), false);
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
  command->ack(Cache::getInstance().getFront());
}

//
// rsp_getclock (event, port)
//
void RSPDriver::rsp_getclock(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetClocksCmd> command = new GetClocksCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETCLOCK: invalid parameter");
    
    RSPGetclockackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subclock (event, port)
//
void RSPDriver::rsp_subclock(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<UpdClocksCmd> command = new UpdClocksCmd(event, port, Command::READ);

  RSPSubclockackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBCLOCK: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubclock (event, port)
//
void RSPDriver::rsp_unsubclock(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubclockEvent unsub(event);

  RSPUnsubclockackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBCLOCK: failed to remove subscription");
  }

  port.send(ack);
}

//
// rsp_getregisterstate (event, port)
//
void RSPDriver::rsp_getregisterstate(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<GetRegisterStateCmd> command = new GetRegisterStateCmd(event, port, Command::READ);

  if (!command->validate())
  {
    LOG_ERROR("GETREGISTERSTATE: invalid parameter");
    
    RSPGetregisterstateackEvent ack;
    ack.timestamp = Timestamp(0,0);
    ack.status = FAILURE;
    port.send(ack);
    return;
  }
  
  // if null timestamp get value from the cache and acknowledge immediately
  if ( (Timestamp(0,0) == command->getTimestamp())
       && (true == command->readFromCache()))
  {
    command->setTimestamp(Cache::getInstance().getFront().getTimestamp());
    command->ack(Cache::getInstance().getFront());
  }
  else
  {
    (void)m_scheduler.enter(Ptr<Command>(&(*command)));
  }
}

//
// rsp_subregisterstate (event, port)
//
void RSPDriver::rsp_subregisterstate(GCFEvent& event, GCFPortInterface& port)
{
  Ptr<UpdRegisterStateCmd> command = new UpdRegisterStateCmd(event, port, Command::READ);

  RSPSubregisterstateackEvent ack;

  if (!command->validate())
  {
    LOG_ERROR("SUBREGISTERSTATE: invalid parameter");
    
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = FAILURE;
    ack.handle = 0;

    port.send(ack);
    return;
  }
  else
  {
    ack.timestamp = m_scheduler.getCurrentTime();
    ack.status = SUCCESS;
    ack.handle = (uint32)&(*command);
    port.send(ack);
  }

  (void)m_scheduler.enter(Ptr<Command>(&(*command)),
                          Scheduler::PERIODIC);
}

//
// rsp_unsubregisterstate (event, port)
//
void RSPDriver::rsp_unsubregisterstate(GCFEvent& event, GCFPortInterface& port)
{
  RSPUnsubregisterstateEvent unsub(event);

  RSPUnsubregisterstateackEvent ack;
  ack.timestamp = m_scheduler.getCurrentTime();
  ack.status = FAILURE;
  ack.handle = unsub.handle;

  if (m_scheduler.remove_subscription(port, unsub.handle) > 0)
  {
    ack.status = SUCCESS;
  }
  else
  {
    LOG_ERROR("UNSUBREGISTERSTATE: failed to remove subscription");
  }

  port.send(ack);
}

//
// main (argc, argv)
//
int main(int argc, char** argv)
{
  GCFTask::init(argc, argv);    // initializes log system
  
  LOG_INFO(formatString("Starting up %s", argv[0]));

  // adopt commandline switches
  LOG_DEBUG("Parsing options");
  parseOptions (argc, argv);

  /* daemonize if required */
  if (g_daemonize) {
    if (0 != daemonize(false)) {
      cerr << "Failed to background this process: " << strerror(errno) << endl;
      exit(EXIT_FAILURE);
    }
  }

  /* change to real-time priority */
#ifdef _POSIX_PRIORITY_SCHEDULING
  int min_priority = sched_get_priority_min(SCHED_FIFO);
  int max_priority = sched_get_priority_max(SCHED_FIFO);

  if (min_priority < 0 || max_priority < 0) {
    LOG_FATAL(formatString("Failed to get real-time priority range: %s", strerror(errno)));
    exit(EXIT_FAILURE);
  }

  /* set SCHED_FIFO priority at 50% */
  struct sched_param priority;
  memset(&priority, 0, sizeof(struct sched_param));
  priority.sched_priority = ((max_priority - min_priority) / 2) + min_priority;
  if (sched_setscheduler(0 /*this process*/, SCHED_FIFO, &priority) < 0) {
    LOG_FATAL(formatString("Failed to set scheduling policy SCHED_FIFO with priority %d: %s",
                           priority.sched_priority, strerror(errno)));
    exit(EXIT_FAILURE);
  }
#else
  LOG_WARN("System does not support real-time scheduling (SCHED_FIFO).");
#endif

  /* lock all current and future memory pages in memory */
#ifdef _POSIX_MEMLOCK
  if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
    LOG_FATAL(formatString("Failed to lock pages in memory: %s", strerror(errno)));
    exit(EXIT_FAILURE);
  }
#else
  LOG_WARN("System does not support locking pages in memory.");
#endif

  LOG_DEBUG ("Reading configuration files");
  try
  {
    ConfigLocator cl;
    globalParameterSet()->adoptFile(cl.locate("RemoteStation.conf"));
  }
  catch (Exception e)
  {
    LOG_ERROR_STR("Failed to load configuration files: " << e.text());
    exit(EXIT_FAILURE);
  }

  RSPDriver rsp("RSPDriver");

  rsp.start(); // make initial transition

  try
  {
    GCFTask::run();
  }
  catch (Exception e)
  {
    LOG_ERROR_STR("Exception: " << e.text());
    exit(EXIT_FAILURE);
  }

  LOG_INFO("Normal termination of program");

  return 0;
}
