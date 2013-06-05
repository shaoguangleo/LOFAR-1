//# CorrelatorPipeline.h
//# Copyright (C) 2012-2013  ASTRON (Netherlands Institute for Radio Astronomy)
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

#ifndef LOFAR_GPUPROC_CUDA_CORRELATOR_PIPELINE_H
#define LOFAR_GPUPROC_CUDA_CORRELATOR_PIPELINE_H

#include <CoInterface/Parset.h>
#include <CoInterface/SlidingPointer.h>

#include <GPUProc/gpu_wrapper.h>
#include <GPUProc/BestEffortQueue.h>
#include "Pipeline.h"
#include <GPUProc/FilterBank.h>
#include <GPUProc/WorkQueues/CorrelatorWorkQueue.h>
#include "CorrelatorPipelinePrograms.h"

namespace LOFAR
{
  namespace Cobalt
  {
    // Correlator pipeline, connect input, correlator WorkQueues and output in parallel (OpenMP).
    // Connect all parts of the pipeline together: set up connections with the input stream
    // each in a seperate thread. Start two WorkQueues for each GPU in the system.
    // These process independently, but can overlap each others compute with host/device I/O.
    // The WorkQueues are then filled with data from the input stream and started.
    // After all data is collected the output is written, again in parallel.
    // This class contains most CPU side parallelism.
    // It also contains two 'data' members that are shared between queues.
    class CorrelatorPipeline : public Pipeline
    {
    public:
      // subbandIndices is the list of subbands that are processed by this
      // pipeline, out of the range [0, ps.nrSubbands()).
      CorrelatorPipeline(const Parset &ps, const std::vector<size_t> &subbandIndices);

      // for each subband get data from input stream, sync, start the kernels to process all data, write output in parallel
      void doWork();

      // for each block, read all subbands from all stations, and divide the work over the workQueues
      template<typename SampleT> void receiveInput( size_t nrBlocks );

    private:
      const std::vector<size_t> subbandIndices; // [localSubbandIdx]

      struct Output {
        // synchronisation to write blocks in-order
        SlidingPointer<size_t> sync;

        // output data queue
        SmartPtr< BestEffortQueue< SmartPtr<CorrelatedDataHostBuffer> > > bequeue;
      };

      std::vector<struct Output> subbandPool; // [localSubbandIdx]

      FilterBank filterBank;

      std::vector< SmartPtr<CorrelatorWorkQueue> > workQueues;

      // process subbands on the GPU
      void processSubbands(CorrelatorWorkQueue &workQueue);

      // postprocess subbands on the CPU
      void postprocessSubbands(CorrelatorWorkQueue &workQueue);

      // send subbands to Storage
      void writeSubband(unsigned globalSubbandIdx, struct Output &output);
    };
  }
}

#endif

