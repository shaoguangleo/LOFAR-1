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

#ifndef AOREMOTE__FORMAT_H
#define AOREMOTE__FORMAT_H

#include <stdint.h>
#include <string>

#define AO_REMOTE_PROTOCOL_VERSION 1

namespace aoRemote {

enum BlockId
{
	InitialId = 0x414F , // letters 'AO'
	InitialResponseId = 2,
	RequestId = 3,
	ReadQualityTablesResponseHeaderId = 10
};

enum ErrorCode { NoError = 0, UnexpectedExceptionOccured=1, ProtocolNotUnderstoodError = 10 } ;

enum RequestType { StopClientRequest = 0, ReadQualityTablesRequest = 1 };

struct InitialBlock
{
	int16_t blockSize;
	int16_t blockIdentifier;
	int16_t protocolVersion;
	int16_t options;
};
struct InitialResponseBlock
{
	int16_t blockSize;
	int16_t blockIdentifier;
	int16_t negotiatedProtocolVersion;
	int16_t errorCode;
	int16_t hostNameSize;
	// std::string hostname will follow
};
struct RequestBlock
{
	int16_t blockSize;
	int16_t blockIdentifier;
	int16_t request;
	int16_t dataSize;
};
struct ReadQualityTablesResponseHeader
{
	int16_t blockSize;
	int16_t blockIdentifier;
	int16_t errorCode;
	int64_t dataSize;
};

#define READ_QTABLES_OPTION_COLLECT_IF_REQUIRED  0x0001
#define READ_QTABLES_OPTION_SAVE_COLLECTED       0x0002

struct ReadQualityTablesRequestOptions
{
	int32_t flags;
	std::string msFilename;
};

}

#endif
