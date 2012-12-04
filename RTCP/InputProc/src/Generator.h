#ifndef __GENERATOR__
#define __GENERATOR__

#include <Common/LofarLogger.h>
#include <Stream/Stream.h>
#include <Interface/RSPTimeStamp.h>
#include <Interface/SmartPtr.h>
#include <IONProc/RSP.h>
#include <IONProc/WallClockTime.h>
#include "StationSettings.h"
#include "StationData.h"
#include <string>
#include <vector>

namespace LOFAR {
namespace RTCP {

template<typename T> class Generator: public StationStreams {
public:
  Generator( const StationSettings &settings, const std::vector<std::string> &streamDescriptors );

protected:
  void processBoard( size_t nr );

  virtual void makePacket( size_t boardNr, struct RSP &packet, const TimeStamp &timestamp );
};

template<typename T> Generator<T>::Generator( const StationSettings &settings, const std::vector<std::string> &streamDescriptors )
:
  StationStreams(str(boost::format("[station %s %s] [Generator] ") % settings.station.stationName % settings.station.antennaSet), settings, streamDescriptors)
{
  LOG_INFO_STR( logPrefix << "Initialised" );
}

template<typename T> void Generator<T>::makePacket( size_t boardNr, struct RSP &packet, const TimeStamp &timestamp )
{
  // configure the packet header
  packet.header.version = 3; // we emulate BDI 6.0

  packet.header.sourceInfo1 =
      (nr & 0x1F) | (settings.station.clock == 200 * 1000 * 1000 ? 1 << 7 : 0);

  switch (settings.station.bitmode) {
    case 16:
      packet.header.sourceInfo2 = 0;
      break;

    case 8:
      packet.header.sourceInfo2 = 1;
      break;

    case 4:
      packet.header.sourceInfo2 = 2;
      break;
  }

  packet.header.nrBeamlets = settings.nrBeamlets / settings.nrBoards;
  packet.header.nrBlocks = 16;

  packet.header.timestamp = timestamp.getSeqId();
  packet.header.blockSequenceNumber = timestamp.getBlockId();

  // insert data that is different for each packet
  int64 data = timestamp;

  memset(packet.data, data & 0xFF, sizeof packet.data);

  // verify whether the packet really reflects what we intended
  ASSERT(packet.rspBoard()     == nr);
  ASSERT(packet.payloadError() == false);
  ASSERT(packet.bitMode()      == settings.station.bitmode);
  ASSERT(packet.clockMHz()     == settings.station.clock / 1000000);
}

template<typename T> void Generator<T>::processBoard( size_t nr )
{
  const std::string logPrefix(str(boost::format("[station %s %s board %u] [Generator] ") % settings.station.stationName % settings.station.antennaSet % nr));

  try {
    LOG_INFO_STR( logPrefix << "Connecting to " << streamDescriptors[nr] );
    SmartPtr<Stream> s = createStream(streamDescriptors[nr], false);

    LOG_INFO_STR( logPrefix << "Start" );

    TimeStamp current(time(0L) + 1, 0, settings.station.clock);
    for(;;) {
      struct RSP packet;

      // generate packet
      makePacket( nr, packet, current );

      ASSERT(packet.packetSize() <= sizeof packet);
    
      // wait until it is due
      if (!waiter.waitUntil(current))
        break;

      // send packet
      try {
        s->write(&packet, packet.packetSize());
      } catch (SystemCallException &ex) {
        // UDP can return ECONNREFUSED or EINVAL if server does not have its port open
        if (ex.error != ECONNREFUSED && ex.error != EINVAL)
          throw;
      }

      current += packet.header.nrBlocks;
    }
  } catch (Stream::EndOfStreamException &ex) {
    LOG_INFO_STR( logPrefix << "End of stream");
  } catch (SystemCallException &ex) {
    if (ex.error == EINTR)
      LOG_INFO_STR( logPrefix << "Aborted: " << ex.what());
    else
      LOG_ERROR_STR( logPrefix << "Caught Exception: " << ex);
  } catch (Exception &ex) {
    LOG_ERROR_STR( logPrefix << "Caught Exception: " << ex);
  }

  LOG_INFO_STR( logPrefix << "End");
}

}
}

#endif