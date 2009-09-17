//#
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

//# Always #include <lofar_config.h> first!
#include <lofar_config.h>

//# Includes
#include <CN_Processing.h>
#include <CorrelatorAsm.h>
#include <FIR_Asm.h>

#include <Common/Timer.h>
#include <Interface/CN_Configuration.h>
#include <Interface/PencilCoordinates.h>
#include <Interface/CN_Mapping.h>
#include <complex>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

#if defined HAVE_BGP
#include <common/bgp_personality_inlines.h>
#include <spi/kernel_interface.h>
#endif


#if defined HAVE_BGP
//#define LOG_CONDITION	(itsLocationInfo.rankInPset() == 0)
#define LOG_CONDITION	(itsLocationInfo.rank() == 0)
//#define LOG_CONDITION	1
#else
#define LOG_CONDITION	1
#endif

namespace LOFAR {
namespace RTCP {


//static NSTimer transposeTimer("transpose()", true); // Unused --Rob
static NSTimer computeTimer("computing", true, true);
static NSTimer totalProcessingTimer("global total processing", true, true);


CN_Processing_Base::~CN_Processing_Base()
{
}

template <typename SAMPLE_TYPE> CN_Processing<SAMPLE_TYPE>::CN_Processing(Stream *str, const LocationInfo &locationInfo)
:
  itsStream(str),
  itsLocationInfo(locationInfo),
  itsInputData(0),
  itsInputSubbandMetaData(0),
  itsSubbandMetaData(0),
  itsTransposedData(0),
  itsFilteredData(0),
  itsCorrelatedData(0),
  itsBeamFormedData(0),
  itsStokesData(0),
  itsIncoherentStokesIData(0),
  itsStokesDataIntegratedChannels(0),
  itsMode(),
#if defined HAVE_BGP
  itsAsyncTranspose(0),
#endif
  itsPPF(0),
  itsBeamFormer(0),
  itsStokes(0),
  itsIncoherentStokesI(0),
  itsCorrelator(0)
{
}


template <typename SAMPLE_TYPE> CN_Processing<SAMPLE_TYPE>::~CN_Processing()
{
}


#if defined HAVE_MPI

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::printSubbandList() const
{
  std::stringstream logStr;
  
  logStr << "node " << itsLocationInfo.rank() << " filters and correlates subbands ";

  unsigned sb = itsCurrentSubband; 

  do {
    logStr << (sb == itsCurrentSubband ? '[' : ',') << sb;

    if ((sb += itsSubbandIncrement) >= itsLastSubband)
      sb -= itsLastSubband - itsFirstSubband;

  } while (sb != itsCurrentSubband);
  
  logStr << ']';
  LOG_DEBUG(logStr.str());
}

#endif // HAVE_MPI


template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::preprocess(CN_Configuration &configuration)
{
  //checkConsistency(parset);	TODO

#if defined HAVE_BGP
  unsigned myPset	    = itsLocationInfo.psetNumber();
  unsigned myCoreInPset	    = CN_Mapping::reverseMapCoreOnPset(itsLocationInfo.rankInPset(), myPset);
#else
  unsigned myPset	    = 0;
  unsigned myCoreInPset	    = 0;
#endif

  std::vector<unsigned> &inputPsets  = configuration.inputPsets();
  std::vector<unsigned> &outputPsets = configuration.outputPsets();
  
  std::vector<unsigned>::const_iterator inputPsetIndex  = std::find(inputPsets.begin(),  inputPsets.end(),  myPset);
  std::vector<unsigned>::const_iterator outputPsetIndex = std::find(outputPsets.begin(), outputPsets.end(), myPset);

  itsIsTransposeInput	     = inputPsetIndex  != inputPsets.end();
  itsIsTransposeOutput	     = outputPsetIndex != outputPsets.end();

  itsNrStations	             = configuration.nrStations();
  itsNrPencilBeams           = configuration.nrPencilBeams();
  itsNrSubbands              = configuration.nrSubbands();
  itsNrSubbandsPerPset       = configuration.nrSubbandsPerPset();
  itsMode                    = configuration.mode();
  itsOutputIncoherentStokesI = configuration.outputIncoherentStokesI();
  itsStokesIntegrateChannels = configuration.stokesIntegrateChannels();
  itsOutputPsetSize          = outputPsets.size();
  itsCenterFrequencies       = configuration.refFreqs();

  unsigned nrChannels			 = configuration.nrChannelsPerSubband();
  unsigned nrSamplesPerIntegration       = configuration.nrSamplesPerIntegration();
  unsigned nrSamplesPerStokesIntegration = configuration.nrSamplesPerStokesIntegration();
  unsigned nrSamplesToCNProc		 = configuration.nrSamplesToCNProc();
  std::vector<unsigned> station2BeamFormedStation = configuration.tabList();
  
  // We have to create the Beam Former first, it knows the number of beam-formed stations.
  // The number of baselines depends on this.
  // If beam forming is disabled, nrBeamFormedStations will be equal to nrStations.
  itsBeamFormer = new BeamFormer(itsNrPencilBeams, itsNrStations, nrChannels, nrSamplesPerIntegration, configuration.sampleRate() / nrChannels, station2BeamFormedStation);
  const unsigned nrBeamFormedStations = itsBeamFormer->getStationMapping().size();
  const unsigned nrBaselines = nrBeamFormedStations * (nrBeamFormedStations + 1) / 2;

  // Each phase (e.g., transpose, PPF, correlator) reads from an input data
  // set and writes to an output data set.  To save memory, two memory buffers
  // are used, and consecutive phases alternately use one of them as input
  // buffer and the other as output buffer.
  // Since some buffers (arenas) are used multiple times, we use multiple
  // Allocators for a single arena.
  //
  if (itsIsTransposeInput) {
    itsInputData = new InputData<SAMPLE_TYPE>(outputPsets.size(), nrSamplesToCNProc);
    itsInputSubbandMetaData = new SubbandMetaData( outputPsets.size(), itsNrPencilBeams, 32 );
  }

  if (itsIsTransposeOutput) {
    // create only the data structures that are used by the pipeline

    itsSubbandMetaData = new SubbandMetaData( itsNrStations, itsNrPencilBeams, 32 );
    itsTransposedData = new TransposedData<SAMPLE_TYPE>(itsNrStations, nrSamplesToCNProc);
    itsFilteredData   = new FilteredData(itsNrStations, nrChannels, nrSamplesPerIntegration);

    switch( itsMode.mode() ) {
      case CN_Mode::FILTER:
        // we have everything already
        break;

      case CN_Mode::CORRELATE:
        itsBeamFormedData = new BeamFormedData(itsNrPencilBeams, nrChannels, nrSamplesPerIntegration);
        itsCorrelatedData = new CorrelatedData(nrBaselines, nrChannels);
        break;

      case CN_Mode::COHERENT_COMPLEX_VOLTAGES:
        itsBeamFormedData = new BeamFormedData(itsNrPencilBeams, nrChannels, nrSamplesPerIntegration);
        break;

      case CN_Mode::COHERENT_STOKES_I:
      case CN_Mode::COHERENT_ALLSTOKES:
        itsBeamFormedData = new BeamFormedData(itsNrPencilBeams, nrChannels, nrSamplesPerIntegration);
        // fallthrough

      case CN_Mode::INCOHERENT_STOKES_I:
      case CN_Mode::INCOHERENT_ALLSTOKES:
        itsStokesData     = new StokesData(itsMode.isCoherent(), itsMode.nrStokes(), itsNrPencilBeams, nrChannels, nrSamplesPerIntegration, nrSamplesPerStokesIntegration);
        break;

      default:
	LOG_DEBUG_STR("Invalid mode: " << itsMode);
        break;
    }

    if (itsOutputIncoherentStokesI)
      itsIncoherentStokesIData = new StokesData(false, 1, 1, nrChannels, nrSamplesPerIntegration, nrSamplesPerStokesIntegration);

    if (itsStokesIntegrateChannels)
      itsStokesDataIntegratedChannels = new StokesDataIntegratedChannels(itsMode.isCoherent(), itsMode.nrStokes(), itsNrPencilBeams, nrSamplesPerIntegration, nrSamplesPerStokesIntegration);
  }

  itsMapping.addDataset(itsInputData,			 0);
  itsMapping.addDataset(itsTransposedData,		 1);
  itsMapping.addDataset(itsFilteredData,		 2);
  itsMapping.addDataset(itsBeamFormedData,		 1);
  itsMapping.addDataset(itsCorrelatedData,		 1);
  itsMapping.addDataset(itsStokesData,			 2);
  itsMapping.addDataset(itsIncoherentStokesIData,	 1);
  itsMapping.addDataset(itsStokesDataIntegratedChannels, 1);

  if (!itsMode.isCoherent()) {
    // for incoherent modes, the filtered data is used for stokes, so they cannot overlap.
    itsMapping.moveDataset(itsStokesData, 1);
    itsMapping.moveDataset(itsStokesDataIntegratedChannels, 2);
  }

  // create the arenas and allocate the data sets
  itsMapping.allocate();

  if (itsIsTransposeOutput) {
    std::vector<unsigned> usedCoresInPset  = configuration.usedCoresInPset();
    unsigned		  usedCoresPerPset = usedCoresInPset.size();
    unsigned		  myCoreIndex	   = std::find(usedCoresInPset.begin(), usedCoresInPset.end(), myCoreInPset) - usedCoresInPset.begin();
    unsigned		  logicalNode	   = usedCoresPerPset * (outputPsetIndex - outputPsets.begin()) + myCoreIndex;

    itsFirstSubband	 = (logicalNode / usedCoresPerPset) * itsNrSubbandsPerPset;
    itsLastSubband	 = itsFirstSubband + itsNrSubbandsPerPset;
    itsCurrentSubband	 = itsFirstSubband + logicalNode % usedCoresPerPset % itsNrSubbandsPerPset;
    itsSubbandIncrement	 = usedCoresPerPset % itsNrSubbandsPerPset;

#if defined HAVE_MPI
    printSubbandList();
#endif // HAVE_MPI

    itsPPF		 = new PPF<SAMPLE_TYPE>(itsNrStations, nrChannels, nrSamplesPerIntegration, configuration.sampleRate() / nrChannels, configuration.delayCompensation(), itsLocationInfo.rank() == 0);

    itsStokes            = new Stokes(itsMode.isCoherent(), itsMode.nrStokes(), nrChannels, nrSamplesPerIntegration, nrSamplesPerStokesIntegration);
    itsIncoherentStokesI = new Stokes(false, 1, nrChannels, nrSamplesPerIntegration, nrSamplesPerStokesIntegration);

    itsCorrelator	 = new Correlator(itsBeamFormer->getStationMapping(), nrChannels, nrSamplesPerIntegration, configuration.correctBandPass());
  }

#if defined HAVE_MPI
  if (itsIsTransposeInput || itsIsTransposeOutput) {
    itsAsyncTranspose = new AsyncTranspose<SAMPLE_TYPE>(itsIsTransposeInput, itsIsTransposeOutput, 
							myCoreInPset, itsLocationInfo, inputPsets, outputPsets );
  }
#endif // HAVE_MPI
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::transpose()
{
#if defined HAVE_MPI

  if (itsIsTransposeInput) {
    itsInputSubbandMetaData->read(itsStream); // sync read the meta data
  }

  if(itsIsTransposeOutput && itsCurrentSubband < itsNrSubbands) {
    NSTimer postAsyncReceives("post async receives", LOG_CONDITION, true);
    postAsyncReceives.start();
    itsAsyncTranspose->postAllReceives(itsSubbandMetaData,itsTransposedData);
    postAsyncReceives.stop();
  }

  // We must not try to read data from I/O node if our subband does not exist.
  // Also, we cannot do the async sends in that case.
  if (itsIsTransposeInput) { 
    static NSTimer readTimer("receive timer", true, true);

    if (LOG_CONDITION) {
      LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start reading at " << MPI_Wtime());
    }
    
    NSTimer asyncSendTimer("async send", LOG_CONDITION, true);

    for (unsigned i = 0; i < itsOutputPsetSize; i ++) {
      unsigned subband = (itsCurrentSubband % itsNrSubbandsPerPset) + (i * itsNrSubbandsPerPset);

      if (subband < itsNrSubbands) {
//	LOG_DEBUG("read subband " << subband << " from IO node");
	readTimer.start();
	itsInputData->readOne(itsStream, i); // Synchronously read 1 subband from my IO node.
	readTimer.stop();
	asyncSendTimer.start();
//	LOG_DEBUG("transpose: send subband " << subband << " to pset id " << i);

	itsAsyncTranspose->asyncSend(i, itsInputSubbandMetaData, itsInputData); // Asynchronously send one subband to another pset.
	asyncSendTimer.stop();
      }
    }
  } // itsIsTransposeInput

#else // ! HAVE_MPI

  if (itsIsTransposeInput) {
    static NSTimer readTimer("receive timer", true, true);
    readTimer.start();
    itsInputSubbandMetaData->read(itsStream);
    itsInputData->read(itsStream);
    readTimer.stop();
  }

#endif // HAVE_MPI
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::filter()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start processing at " << MPI_Wtime());

  NSTimer asyncReceiveTimer("wait for any async receive", LOG_CONDITION, true);

  for (unsigned i = 0; i < itsNrStations; i ++) {
    asyncReceiveTimer.start();
    const unsigned stat = itsAsyncTranspose->waitForAnyReceive();
    asyncReceiveTimer.stop();

//    LOG_DEBUG("transpose: received subband " << itsCurrentSubband << " from " << stat);

    computeTimer.start();
    itsPPF->computeFlags(stat, itsSubbandMetaData, itsFilteredData);
    itsPPF->filter(stat, itsCenterFrequencies[itsCurrentSubband], itsSubbandMetaData, itsTransposedData, itsFilteredData);
    computeTimer.stop();
  }
#else // NO MPI
  for (unsigned stat = 0; stat < itsNrStations; stat ++) {
    computeTimer.start();
    itsPPF->computeFlags(stat, itsSubbandMetaData, itsFilteredData);
    itsPPF->filter(stat, itsCenterFrequencies[itsCurrentSubband], itsSubbandMetaData, itsTransposedData, itsFilteredData);
    computeTimer.stop();
  }
#endif // HAVE_MPI
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::formBeams()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start beam forming at " << MPI_Wtime());
#endif // HAVE_MPI
  computeTimer.start();
  itsBeamFormer->formBeams(itsSubbandMetaData,itsFilteredData,itsBeamFormedData, itsCenterFrequencies[itsCurrentSubband]);
  computeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::calculateIncoherentStokesI()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start calculating incoherent Stokes I at " << MPI_Wtime());
#endif // HAVE_MPI
  computeTimer.start();
  itsIncoherentStokesI->calculateIncoherent(itsFilteredData,itsIncoherentStokesIData,itsNrStations);
  computeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::calculateIncoherentStokes()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start calculating incoherent Stokes at " << MPI_Wtime());
#endif // HAVE_MPI
  computeTimer.start();
  itsStokes->calculateIncoherent(itsFilteredData,itsStokesData,itsNrStations);
  computeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::calculateCoherentStokes()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start calculating coherent Stokes at " << MPI_Wtime());
#endif // HAVE_MPI
  computeTimer.start();
  itsStokes->calculateCoherent(itsBeamFormedData,itsStokesData,itsNrPencilBeams);
  computeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::correlate()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start correlating at " << MPI_Wtime());
#endif // HAVE_MPI
  computeTimer.start();
  itsCorrelator->computeFlagsAndCentroids(itsFilteredData, itsCorrelatedData);
  itsCorrelator->correlate(itsFilteredData, itsCorrelatedData);
  computeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::sendOutput( StreamableData *outputData )
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start writing at " << MPI_Wtime());
#endif // HAVE_MPI

  static NSTimer writeTimer("send timer", true, true);
  writeTimer.start();
  outputData->write(itsStream, false);
  writeTimer.stop();
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::finishSendingInput()
{
#if defined HAVE_MPI
  if (LOG_CONDITION)
    LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start waiting to finish sending input at " << MPI_Wtime());

  NSTimer waitAsyncSendTimer("wait for all async sends", LOG_CONDITION, true);
  waitAsyncSendTimer.start();
  itsAsyncTranspose->waitForAllSends();
  waitAsyncSendTimer.stop();
#endif
}

template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::process()
{
  totalProcessingTimer.start();
  NSTimer totalTimer("total processing", LOG_CONDITION, true);
  totalTimer.start();

  // transpose/obtain input data
  transpose();

  if (itsIsTransposeOutput && itsCurrentSubband < itsNrSubbands) {
    // the order and types of sendOutput have to match
    // what the IONProc and Storage expect to receive
    // (defined in Interface/PipelineOutput.h)
    filter();

    if( itsOutputIncoherentStokesI ) {
      calculateIncoherentStokesI();
      sendOutput(itsIncoherentStokesIData);
    }

    switch( itsMode.mode() ) {
      case CN_Mode::FILTER:
        sendOutput(itsFilteredData);
        break;

      case CN_Mode::CORRELATE:
        formBeams();
        correlate();
        sendOutput(itsCorrelatedData);
        break;

      case CN_Mode::COHERENT_COMPLEX_VOLTAGES:
        formBeams();
        sendOutput(itsBeamFormedData);
        break;

      case CN_Mode::COHERENT_STOKES_I:
      case CN_Mode::COHERENT_ALLSTOKES:
        formBeams();
        calculateCoherentStokes();
	if( itsStokesIntegrateChannels ) {
	  itsStokes->compressStokes(itsStokesData, itsStokesDataIntegratedChannels, itsNrPencilBeams);
          sendOutput(itsStokesDataIntegratedChannels);
	} else {
          sendOutput(itsStokesData);
	}
        break;

      case CN_Mode::INCOHERENT_STOKES_I:
      case CN_Mode::INCOHERENT_ALLSTOKES:
        calculateIncoherentStokes();
	if(itsStokesIntegrateChannels) {
	  itsStokes->compressStokes(itsStokesData, itsStokesDataIntegratedChannels, 1);
          sendOutput(itsStokesDataIntegratedChannels);
	} else {
          sendOutput(itsStokesData);
	}
        break;

      default:
	LOG_DEBUG_STR("Invalid mode: " << itsMode);
        break;
    }
  } // itsIsTransposeOutput

  // Just always wait, if we didn't do any sends, this is a no-op.
  if (itsIsTransposeInput)
    finishSendingInput();

#if defined HAVE_MPI
  if (itsIsTransposeInput || itsIsTransposeOutput) {
    if (LOG_CONDITION)
      LOG_DEBUG(std::setprecision(12) << "core " << itsLocationInfo.rank() << ": start idling at " << MPI_Wtime());
  }
#endif // HAVE_MPI

#if 0
  static unsigned count = 0;

  if (itsLocationInfo.rank() == 5 && ++ count == 9)
    for (double time = MPI_Wtime() + 4.0; MPI_Wtime() < time;)
      ;
#endif

  if ((itsCurrentSubband += itsSubbandIncrement) >= itsLastSubband) {
    itsCurrentSubband -= itsLastSubband - itsFirstSubband;
  }

  totalTimer.stop();
  totalProcessingTimer.stop();
}


template <typename SAMPLE_TYPE> void CN_Processing<SAMPLE_TYPE>::postprocess()
{
  delete itsBeamFormer;

  if (itsIsTransposeInput) {
    delete itsInputData;
    delete itsInputSubbandMetaData;
  }

  if (itsIsTransposeInput || itsIsTransposeOutput) {
#if defined HAVE_MPI
      delete itsAsyncTranspose;
#endif // HAVE_MPI
  }

  if (itsIsTransposeOutput) {
    delete itsSubbandMetaData;
    delete itsTransposedData;
    delete itsPPF;
    delete itsFilteredData;
    delete itsBeamFormedData;
    delete itsCorrelator;
    delete itsCorrelatedData;
    delete itsStokes;
    delete itsStokesData;
    delete itsIncoherentStokesI;
    delete itsIncoherentStokesIData;
  }
}


template class CN_Processing<i4complex>;
template class CN_Processing<i8complex>;
template class CN_Processing<i16complex>;

} // namespace RTCP
} // namespace LOFAR
