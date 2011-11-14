/***************************************************************************
 *   Copyright (C) 2008 by A.R. Offringa   *
 *   offringa@astro.rug.nl   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <AOFlagger/remote/serverconnection.h>

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <AOFlagger/remote/format.h>

#include <AOFlagger/util/autoarray.h>

#include <AOFlagger/quality/statisticscollection.h>

namespace aoRemote
{

ServerConnection::ServerConnection(boost::asio::io_service &ioService) :
	_socket(ioService)
{
}

void ServerConnection::Start()
{
	InitialBlock initialBlock;
	initialBlock.blockIdentifier = InitialId;
	initialBlock.blockSize = sizeof(initialBlock);
	initialBlock.options = 0;
	initialBlock.protocolVersion = AO_REMOTE_PROTOCOL_VERSION;
	
	boost::asio::write(_socket, boost::asio::buffer(&initialBlock, sizeof(initialBlock)));
	
	InitialResponseBlock initialResponse;
	boost::asio::read(_socket, boost::asio::buffer(&initialResponse, sizeof(initialResponse)));
	enum ErrorCode errorCode = (enum ErrorCode) initialResponse.errorCode;
	if(initialResponse.blockIdentifier != InitialResponseId || initialResponse.blockSize != sizeof(initialResponse))
		throw std::runtime_error("Bad response from client during initial response");
	if(errorCode != NoError)
		throw std::runtime_error(std::string("Error reported by client during initial response: ") + GetErrorStr(errorCode));
	if(initialResponse.negotiatedProtocolVersion != AO_REMOTE_PROTOCOL_VERSION)
		throw std::runtime_error("Client seems to run different protocol version");
	if(initialResponse.hostNameSize == 0 || initialResponse.hostNameSize > 65536)
		throw std::runtime_error("Client did not send proper hostname");
	
	char hostname[initialResponse.hostNameSize + 1];
	boost::asio::read(_socket, boost::asio::buffer(hostname, initialResponse.hostNameSize));
	hostname[initialResponse.hostNameSize] = 0;
	_hostname = hostname;
	
	_onAwaitingCommand(*this);
}

void ServerConnection::StopClient()
{
	RequestBlock requestBlock;
	requestBlock.blockIdentifier = RequestId;
	requestBlock.blockSize = sizeof(requestBlock);
	requestBlock.dataSize = 0;
	requestBlock.request = StopClientRequest;
	boost::asio::write(_socket, boost::asio::buffer(&requestBlock, sizeof(requestBlock)));
}

void ServerConnection::ReadQualityTables(const std::string &msFilename, StatisticsCollection &collection)
{
	std::stringstream reqBuffer;
	
	RequestBlock requestBlock;
	ReadQualityTablesRequestOptions options;
	
	requestBlock.blockIdentifier = RequestId;
	requestBlock.blockSize = sizeof(requestBlock);
	requestBlock.dataSize = sizeof(options.flags) + msFilename.size();
	requestBlock.request = ReadQualityTablesRequest;
	reqBuffer.write(reinterpret_cast<char *>(&requestBlock), sizeof(requestBlock));
	
	options.flags = 0;
	options.msFilename = msFilename;
	reqBuffer.write(reinterpret_cast<char *>(&options.flags), sizeof(options.flags));
	reqBuffer.write(reinterpret_cast<const char *>(options.msFilename.c_str()), options.msFilename.size());
	
	boost::asio::write(_socket, boost::asio::buffer(reqBuffer.str()));
	
	ReadQualityTablesResponseHeader responseHeader;
	boost::asio::read(_socket, boost::asio::buffer(&responseHeader, sizeof(responseHeader)));
	if(responseHeader.blockIdentifier != ReadQualityTablesResponseHeaderId || responseHeader.blockSize != sizeof(responseHeader))
		throw std::runtime_error("Bad response from client upon read tables request");
	if(responseHeader.errorCode != NoError)
		throw std::runtime_error("Error reported by client upon read tables request");
	
	AutoArray<char> buffer(new char[responseHeader.dataSize]);
	
	boost::asio::read(_socket, boost::asio::buffer(&*buffer, responseHeader.dataSize));
	std::istringstream stream;
	if(stream.rdbuf()->pubsetbuf(&*buffer, responseHeader.dataSize) == 0)
	{
		throw std::runtime_error("Could not set string buffer");
	}
	
	std::cout << "Unserializing stream of size " << responseHeader.dataSize << "..." << std::endl;
	
	std::cout << "Stream pos before: " << stream.tellg() << std::endl;
	collection.Unserialize(stream);
	std::cout << "Stream pos after: " << stream.tellg() << std::endl;

	_onFinishReadQualityTables(*this, collection);
	_onAwaitingCommand(*this);
}

std::string ServerConnection::GetErrorStr(enum ErrorCode errorCode)
{
	switch(errorCode)
	{
		case NoError: return "No error";
			break;
		case UnexpectedExceptionOccured: return "Unexpected exception occured";
			break;
		case ProtocolNotUnderstoodError: return "Protocol not understood";
			break;
		default: return "Unknown error code";
			break;
	}
	
}


}

