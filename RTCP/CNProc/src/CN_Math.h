#ifndef LOFAR_CNPROC_CN_MATH_H
#define LOFAR_CNPROC_CN_MATH_H

#include <cmath>

#if defined HAVE_MASS

extern "C"
{
  // the return conventions for std::complex<double> and double _Complex differ!
  double _Complex cosisin(double);
}

#else

inline static dcomplex cosisin(double x)
{
  return makedcomplex(std::cos(x), std::sin(x));
}

#endif

#endif
