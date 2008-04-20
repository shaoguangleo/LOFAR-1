#ifndef LOFAR_APPL_CEP_CS1_CS1_BGL_PROC_TRANSPOSED_DATA_H
#define LOFAR_APPL_CEP_CS1_CS1_BGL_PROC_TRANSPOSED_DATA_H

#include <Common/lofar_complex.h>
#include <CS1_Interface/Allocator.h>
#include <CS1_Interface/CS1_Config.h>
#include <CS1_Interface/SparseSet.h>

#include <boost/multi_array.hpp>


namespace LOFAR {
namespace CS1 {

class TransposedData
{
  public:
    TransposedData(const Arena &, unsigned nrStations, unsigned nrSamplesToBGLProc);
    ~TransposedData();

    static size_t requiredSize(unsigned nrStations, unsigned nrSamplesToBGLProc);

    typedef INPUT_SAMPLE_TYPE SampleType;

  private:
    SparseSetAllocator allocator;

  public:
    boost::multi_array_ref<SampleType, 3> samples; //[itsNrStations][itsCS1PS->nrSamplesToBGLProc()][NR_POLARIZATIONS]

    SparseSet<unsigned> *flags; //[itsNrStations]

    typedef struct {
      float delayAtBegin, delayAfterEnd;
    } DelayIntervalType;
    
    DelayIntervalType *delays; // [itsNrStations]
    unsigned          *alignmentShifts; // [itsNrStations]
};


inline TransposedData::TransposedData(const Arena &arena, unsigned nrStations, unsigned nrSamplesToBGLProc)
:
  allocator(arena),
  samples(static_cast<SampleType *>(allocator.allocate(requiredSize(nrStations, nrSamplesToBGLProc), 32)), boost::extents[nrStations][nrSamplesToBGLProc][NR_POLARIZATIONS]),
  flags(new SparseSet<unsigned>[nrStations]),
  delays(new DelayIntervalType[nrStations]),
  alignmentShifts(new unsigned[nrStations])
{
}


inline TransposedData::~TransposedData()
{
  allocator.deallocate(samples.origin());
  delete [] flags;
  delete [] alignmentShifts;
  delete [] delays;
}


inline size_t TransposedData::requiredSize(unsigned nrStations, unsigned nrSamplesToBGLProc)
{
  return sizeof(SampleType) * nrStations * nrSamplesToBGLProc * NR_POLARIZATIONS;
}

} // namespace CS1
} // namespace LOFAR

#endif
