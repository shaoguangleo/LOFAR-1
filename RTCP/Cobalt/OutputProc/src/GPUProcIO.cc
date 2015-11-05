//# GPUProcIO.cc: Routines for communicating with GPUProc
//# Copyright (C) 2008-2014  ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O. Box 2, 7990 AA Dwingeloo, The Netherlands
//#
//# This file is part of the LOFAR software suite.
//# The LOFAR software suite is free software: you can redistribute it and/or
//# modify it under the terms of the GNU General Public License as published
//# by the Free Software Foundation, either version 3 of the License, or
//# (at your option) any later version.
//#
//# The LOFAR software suite is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
//#
//# $Id$

//# Always #include <lofar_config.h> first!
#include <lofar_config.h>

#include "GPUProcIO.h"

#include <cstring>
#include <vector>
#include <omp.h>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <Common/LofarLogger.h>
#include <Common/StringUtil.h>
#include <Common/Exceptions.h>
#include <Stream/PortBroker.h>
#include <ApplCommon/PVSSDatapointDefs.h>
#include <ApplCommon/StationInfo.h>
#include <MACIO/RTmetadata.h>
#include <CoInterface/Exceptions.h>
#include <CoInterface/OMPThread.h>
#include <CoInterface/Parset.h>
#include <CoInterface/FinalMetaData.h>
#include <CoInterface/Stream.h>
#include <CoInterface/SmartPtr.h>
#include "SubbandWriter.h"
#include "OutputThread.h"

using namespace LOFAR;
using namespace LOFAR::Cobalt;
using namespace std;
using boost::format;
using boost::lexical_cast;

namespace LOFAR {
  namespace Cobalt {

static string formatDataPointLocusName(const string& hostname)
{
  // For node name, strip a "locus" prefix and '0's if any (avoid remote octal interp).
  string nodeValue(hostname);
  const char* nvstr = nodeValue.c_str();
  if (strncmp(nvstr, "locus", sizeof("locus") - 1) == 0) {
    unsigned stripLen = sizeof("locus") - 1;
    while (nvstr[stripLen] == '0')
      stripLen += 1;
    nodeValue.erase(0, stripLen);
  }
  return nodeValue;
}

bool process(Stream &controlStream, unsigned myRank)
{
  bool success(true);
  Parset parset(&controlStream);

  const vector<string> &hostnames = parset.settings.outputProcHosts;
  ASSERT(myRank < hostnames.size());
  string myHostName = hostnames[myRank];

  // Send id string to the MAC Log Processor as context for further LOGs.
  // Also use it for MAC/PVSS data point logging as a key name prefix.
  // For outputProc there are no conv specifications (%xx) to be filled in.
  string mdKeyPrefix(createPropertySetName(PSN_COBALT_OUTPUT_PROC, "", parset.PVSS_TempObsName()));
  LOG_INFO_STR("MACProcessScope: " << mdKeyPrefix);
  mdKeyPrefix.push_back('.'); // keys look like: "keyPrefix.subKeyName[x]"

  const string mdRegisterName = string(PST_COBALT_OUTPUT_PROC) + ":" +
                                lexical_cast<string>(parset.settings.observationID) + "@" + myHostName;
  const string mdHostName = parset.getString("Cobalt.PVSSGateway.host", "");
  MACIO::RTmetadata mdLogger(parset.settings.observationID, mdRegisterName, mdHostName);
  mdLogger.start();

  {
    // make sure "parset" and "mdLogger" stay in scope for the lifetime of the SubbandWriters

    vector<SmartPtr<SubbandWriter> > subbandWriters;
    vector<SmartPtr<TABOutputThread> > tabWriters;

    /*
     * Construct writers
     */

    // Process correlated data
    if (parset.settings.correlator.enabled) {
      for (size_t fileIdx = 0; fileIdx < parset.settings.correlator.files.size(); ++fileIdx)
      {
        if (parset.settings.correlator.files[fileIdx].location.host != myHostName) 
          continue;

        mdLogger.log(mdKeyPrefix + PN_COP_LOCUS_NODE + '[' + lexical_cast<string>(fileIdx) + ']',
                     formatDataPointLocusName(myHostName));

        string logPrefix = str(format("[obs %u correlated stream %3u] ")
                               % parset.settings.observationID % fileIdx);

        SubbandWriter *writer = new SubbandWriter(parset, fileIdx, mdLogger, mdKeyPrefix, logPrefix);
        subbandWriters.push_back(writer);
      }
    }

    map<size_t, SmartPtr<Pool<TABTranspose::BeamformedData> > > outputPools;
    TABTranspose::Receiver::CollectorMap collectors;

    // Process beam-formed data
    if (parset.settings.beamFormer.enabled) {
      for (size_t fileIdx = 0; fileIdx < parset.settings.beamFormer.files.size(); ++fileIdx)
      {
        struct ObservationSettings::BeamFormer::File &file = parset.settings.beamFormer.files[fileIdx];

        if (file.location.host != myHostName) 
          continue;

        const unsigned allFileIdx = fileIdx + parset.settings.correlator.files.size();
        mdLogger.log(mdKeyPrefix + PN_COP_LOCUS_NODE + '[' + lexical_cast<string>(allFileIdx) + ']',
                     formatDataPointLocusName(myHostName));

        struct ObservationSettings::BeamFormer::StokesSettings &stokes =
          file.coherent ? parset.settings.beamFormer.coherentSettings
                        : parset.settings.beamFormer.incoherentSettings;

        const size_t nrSubbands = file.lastSubbandIdx - file.firstSubbandIdx;
        const size_t nrChannels = stokes.nrChannels;
        const size_t nrSamples = stokes.nrSamples;

        outputPools[fileIdx] = new Pool<TABTranspose::BeamformedData>(str(format("process::outputPool [file %u]") % fileIdx), parset.settings.realTime);

        // Create and fill an outputPool for this fileIdx
        for (size_t i = 0; i < 10; ++i) {
	         outputPools[fileIdx]->free.append(new TABTranspose::BeamformedData(
             boost::extents[nrSamples][nrSubbands][nrChannels],
             boost::extents[nrSubbands][nrChannels]
           ), false);
        }

        // Create a collector for this fileIdx
        collectors[fileIdx] = new TABTranspose::BlockCollector(
          *outputPools[fileIdx], fileIdx, nrSubbands, nrChannels, nrSamples, parset.settings.nrBlocks(), parset.settings.realTime ? 5 : 0);

        string logPrefix = str(format("[obs %u beamformed stream %3u] ")
                                                    % parset.settings.observationID % fileIdx);

        TABOutputThread *writer = new TABOutputThread(parset, fileIdx, *outputPools[fileIdx], mdLogger, mdKeyPrefix, logPrefix);
        tabWriters.push_back(writer);
      }
    }

    /*
     * PROCESS
     */

    FinalMetaData finalMetaData;

    // Set up receiver engine for 2nd transpose
    TABTranspose::MultiReceiver mr("2nd-transpose-", collectors);


#   pragma omp parallel sections num_threads(3)
    {
      // Done signal from controller, by sending the final meta data
#     pragma omp section
      {
        OMPThread::ScopedName sn("finalMetaData");

        // Add final meta data (broken tile information, etc)
        // that is obtained after the end of an observation.
        LOG_INFO_STR("Waiting for final meta data");

        try {
          finalMetaData.read(controlStream);
        } catch (LOFAR::Exception &err) {
          success = false;
          LOG_ERROR_STR("Failed to read broken tile information: " << err);
        }

        if (parset.settings.realTime) {
          // Real-time observations: stop now. MultiReceiver::kill
          // will stop the TABWriters.
          mr.kill(0);
        } else {
          // Non-real-time observations: wait until all data has been
          // processed. The TABWriters will stop once they received
          // the last block.
        }

        // SubbandWriters finish on their own once their InputThread
        // gets disconnected.
      }

      // SubbandWriters
#     pragma omp section
      {
        OMPThread::ScopedName sn("subbandWr");

#       pragma omp parallel for num_threads(subbandWriters.size())
        for (int i = 0; i < (int)subbandWriters.size(); ++i) {
          OMPThread::ScopedName sn(str(format("subbandWr %u") % subbandWriters[i]->streamNr()));

          subbandWriters[i]->process();
        }
      }

      // TABWriters
#     pragma omp section
      {
        OMPThread::ScopedName sn("tabWr");

#       pragma omp parallel for num_threads(tabWriters.size())       
        for (int i = 0; i < (int)tabWriters.size(); ++i) {
          OMPThread::ScopedName sn(str(format("tabWr %u") % tabWriters[i]->streamNr()));
          tabWriters[i]->process();
        }
      }
    }

    /*
     * FINAL META DATA
     */

    // Add final meta data (broken tile information, etc)
    // that is obtained after the end of an observation.
    LOG_DEBUG_STR("Processing final meta data");

    for (size_t i = 0; i < subbandWriters.size(); ++i)
      subbandWriters[i]->augment(finalMetaData);
    for (size_t i = 0; i < tabWriters.size(); ++i)
      tabWriters[i]->augment(finalMetaData);

    /*
     * LTA FEEDBACK
     */

    LOG_DEBUG_STR("Retrieving LTA feedback");
    Parset feedbackLTA;

    for (size_t i = 0; i < subbandWriters.size(); ++i)
      feedbackLTA.adoptCollection(subbandWriters[i]->feedbackLTA());
    for (size_t i = 0; i < tabWriters.size(); ++i)
      feedbackLTA.adoptCollection(tabWriters[i]->feedbackLTA());

    LOG_DEBUG_STR("Forwarding LTA feedback");
    try {
      feedbackLTA.write(&controlStream);
    } catch (LOFAR::Exception &err) {
      success = false;
      LOG_ERROR_STR("Failed to forward LTA feedback information: " << err);
    }

    return success;
  }
}

  } // namespace Cobalt
} // namespace LOFAR

