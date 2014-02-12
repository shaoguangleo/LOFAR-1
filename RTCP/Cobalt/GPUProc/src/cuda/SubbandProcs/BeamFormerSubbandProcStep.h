//# BeamFormerSubbandProcStep.h
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

#ifndef LOFAR_GPUPROC_CUDA_BEAMFORMER_SUBBAND_PROC_STEP_H
#define LOFAR_GPUPROC_CUDA_BEAMFORMER_SUBBAND_PROC_STEP_H

#include <CoInterface/Parset.h>

#include <GPUProc/gpu_wrapper.h>

#include <CoInterface/BlockID.h>

#include "SubbandProc.h"

namespace LOFAR
{
  namespace Cobalt
  {

    //# Forward declarations
    struct BeamFormerFactories;

    class BeamFormerSubbandProcStep
    {
    public:

      virtual void initMembers(gpu::Context &context,
        BeamFormerFactories &factories) = 0;

      virtual void process(BlockID blockID,
        unsigned subband) = 0;

      virtual void printStats() =0;

      virtual void logTime() = 0;

       ~BeamFormerSubbandProcStep()
       {}

    protected:
      BeamFormerSubbandProcStep(const Parset &parset,
        gpu::Stream &i_queue)
        :
        ps(parset),
        queue(i_queue) 
        {} ;

      const Parset ps;
      gpu::Stream queue;

    };
  }
}

#endif