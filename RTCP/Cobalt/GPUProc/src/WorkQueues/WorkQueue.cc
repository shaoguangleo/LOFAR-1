#include "lofar_config.h"    
#include "CL/cl.hpp"

#include "Common/LofarLogger.h"
#include "global_defines.h"
#include "CoInterface/Parset.h"
#include "OpenCL_Support.h"

#include "WorkQueue.h"

namespace LOFAR
{
    namespace  RTCP 
    {      
                WorkQueue::WorkQueue(cl::Context &context, cl::Device		&device, unsigned gpuNumber, const Parset	&ps)
            :
        gpu(gpuNumber),
            device(device),
            ps(ps)
        {
#if defined __linux__ && defined USE_B7015
            set_affinity(gpu);
#endif

            queue = cl::CommandQueue(context, device, profiling ? CL_QUEUE_PROFILING_ENABLE : 0);
        }


        void WorkQueue::addCounter(const std::string &name)
        {
          counters[name] = new PerformanceCounter(name, profiling);
        }


        void WorkQueue::addTimer(const std::string &name)
        {
          timers[name] = new NSTimer(name, false, false);
        }

    }
}