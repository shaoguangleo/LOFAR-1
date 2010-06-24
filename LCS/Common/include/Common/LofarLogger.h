//# LofarLogger.h: Macro interface to the lofar logging package
//#
//# Copyright (C) 2004
//# ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
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

#ifndef LOFAR_COMMON_LOFARLOGGER_H
#define LOFAR_COMMON_LOFARLOGGER_H

// \file
// Macro interface to the lofar logging package.

//# Include the correct set of macros, depending on the availability of the
//# log4cplus or log4cxx packages.
#if defined(USE_LOG4CPLUS)
# include <Common/LofarLog4Cplus.h>
#elif defined(USE_LOG4CXX)
# include <Common/LofarLog4Cxx.h>
#else
# include <Common/LofarLogCout.h>
#endif

#include <Common/Exceptions.h>

namespace LOFAR 
{

// \ingroup Common
// \addtogroup LofarLogger
  // @{


//# --------------------- Common definitions ---------------
//
// \name Common definitions 
// @{

// The package name of the current package. This variable should be defined in
// the file lofar_config.h which is generated by the lofarconf script. Here,
// we will make sure that LOFARLOGGER_PACKAGE is always defined.
#ifndef LOFARLOGGER_PACKAGE
# define LOFARLOGGER_PACKAGE "Unknown_package"
#endif

// The full package name is defined as the concatenation of the package name
// and an optional sub-package name that may be defined by the user.
#ifdef LOFARLOGGER_SUBPACKAGE
# define LOFARLOGGER_FULLPACKAGE LOFARLOGGER_PACKAGE "." LOFARLOGGER_SUBPACKAGE
#else
# define LOFARLOGGER_FULLPACKAGE LOFARLOGGER_PACKAGE
#endif

// @}

//#------------------------ Assert and FailWhen -------------------------------
//#
//# NB: THROW is defined in package-dependant file.
//
// \name Assert and FailWhen
// \note The \c DBG version of \c ASSERT and \c FAILWHEN will only be in your
// compiled code when the (pre)compiler flag \c ENABLE_DBGASSERT is defined.
// @{

// If the condition of assert is NOT met a logrequest is sent to the logger
// <tt>\<module\>.EXCEPTION</tt> and an AssertError exception is thrown.
#define ASSERT(cond) do { \
	if (!(cond)) THROW(::LOFAR::AssertError, "Assertion: " #cond); \
	} while(0)


// If the condition of assert is NOT met, a logrequest is sent to the logger
// <tt>\<module\>.EXCEPTION</tt> and an AssertError exception is thrown.
#define ASSERTSTR(cond,stream) do { \
	if (!(cond)) THROW(::LOFAR::AssertError, \
			   "Assertion: " #cond "; " << stream); \
	} while(0)

// If the condition of failwhen is met, a logrequest is sent to the logger
// <tt>\<module\>.EXCEPTION</tt> and an AssertError exception is thrown.
#define FAILWHEN(cond) do { \
	if (cond) THROW(::LOFAR::AssertError, "Failtest: " #cond); \
	} while(0)

#ifdef ENABLE_DBGASSERT
#define DBGASSERT(cond)				ASSERT(cond)
#define DBGASSERTSTR(cond,stream)	ASSERTSTR(cond,stream)
#define DBGFAILWHEN(cond)			FAILWHEN(cond)
#else
#define DBGASSERT(cond)
#define DBGASSERTSTR(cond,stream)
#define DBGFAILWHEN(cond)
#endif

// @}
    
} // namespace LOFAR

#endif // file read before
