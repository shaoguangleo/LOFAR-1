//# createProgram.h
//# Copyright (C) 2013  ASTRON (Netherlands Institute for Radio Astronomy)
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

#ifndef LOFAR_GPUPROC_CUDA_CREATE_PROGRAM_H
#define LOFAR_GPUPROC_CUDA_CREATE_PROGRAM_H

#include <string>
#include <vector>

#include <CoInterface/Parset.h>

#include "gpu_wrapper.h"

namespace LOFAR
{
  namespace Cobalt
  {
    /*
     * For CUDA, context is ignored, but note that creating such an object
     * makes it the active context.
     * srcFilename cannot be an absolute path.
     */
    gpu::Module createProgram(const Parset &ps, gpu::Context &context, std::vector<std::string> &targets, const std::string &srcFilename);
  }
}

#endif

