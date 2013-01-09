#include <lofar_config.h>
#include "PacketReader.h"
#include <Common/LofarLogger.h>
#include <Stream/SocketStream.h>
#include <Interface/RSPTimeStamp.h>
#include <Interface/Stream.h>
#include <boost/format.hpp>

namespace LOFAR {
namespace RTCP {


PacketReader::PacketReader( const std::string &logPrefix, const std::string &streamDescriptor, const struct BufferSettings &settings )
:
  logPrefix(str(boost::format("%s [PacketReader] ") % logPrefix)),
  settings(settings),

  nrReceived(0),
  nrBadSize(0),
  nrBadTime(0),
  nrBadData(0),
  nrBadMode(0),
  hadSizeError(false),
  hadModeError(false)
{
  inputStream = createStream(streamDescriptor, true);

  SocketStream *asSocket = dynamic_cast<SocketStream *>(inputStream.get());
  bool isUDP = asSocket && asSocket->protocol == SocketStream::UDP;

  supportPartialReads = !isUDP;
}


bool PacketReader::readPacket( struct RSP &packet )
{
  if (supportPartialReads) {
    // read header first
    inputStream->read(&packet.header, sizeof packet.header);

    // read rest of packet
    inputStream->read(&packet.payload.data, packet.packetSize() - sizeof packet.header);

    ++nrReceived;
  } else {
    // read full packet at once -- numbytes will tell us how much we've actually read
    size_t numbytes = inputStream->tryRead(&packet, sizeof packet);

    ++nrReceived;

    if( numbytes < sizeof(struct RSP::Header)
     || numbytes != packet.packetSize() ) {

      if (!hadSizeError) {
        LOG_ERROR_STR( logPrefix << "Packet is " << numbytes << " bytes, but should be " << packet.packetSize() << " bytes" );
        hadSizeError = true;
      }

      ++nrBadSize;
      return false;
    }
  }

  // illegal timestamp means illegal packet
  if (packet.header.timestamp == ~0U) {
    ++nrBadTime;
    return false;
  }

  // check sanity of packet

  // discard packets with errors
  if (packet.payloadError()) {
    ++nrBadData;
    return false;
  }

  // check whether the station configuration matches ours
  if (packet.clockMHz() * 1000000 != settings.station.clock
   || packet.bitMode() != settings.station.bitmode) {

    if (!hadModeError) {
      LOG_ERROR_STR( logPrefix << "Packet has mode (" << packet.clockMHz() << " MHz, " << packet.bitMode() << " bit), but should be mode (" << settings.station.clock / 1000000 << " MHz, " << settings.station.bitmode << " bit)");
      hadModeError = true;
    }

    ++nrBadMode;
    return false;
  }

  return true;
}


void PacketReader::logStatistics()
{
  LOG_INFO_STR( logPrefix << "Received " << nrReceived << " packets: " << nrBadTime << " bad timestamps, " << nrBadSize << " bad sizes, " << nrBadData << " payload errors, " << nrBadMode << " clock/bitmode errors" );

  nrReceived = 0;
  nrBadTime = 0;
  nrBadSize = 0;
  nrBadData = 0;
  nrBadMode = 0;

  hadSizeError = false;
  hadModeError = false;
}


}
}