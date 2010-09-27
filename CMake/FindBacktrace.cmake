# - Check whether the C library provides support for backtrace information.
# The backtrace() function provides you with stack frame return addresses. 
# In order to translate these return addresses to filename, line number
# and function name, we need support from the binutils:
#  - libbfd contains functions to do the address translation
#  - libiberty contains a function to demangle C++ function names.
# The following non-cached variables will always be set:
#  BACKTRACE_FOUND     - true if system has backtrace support
#  BACKTRACE_LIBRARIES - libraries needed to link in backtrace support
# The following cached variables may be set:
#  HAVE_EXECINFO_H     - true if header file execinfo.h exists
#                        (contains declaration of backtrace())
#  HAVE_BACKTRACE      - true if backtrace() function exists
#  HAVE_BFD_H          - true if header file bfd.h exists
#                        (contains declaration of bfd_* functions)
#  BFD_LIBRARY         - full path to libbfd, if found
#  IBERTY_LIBRARY      - full path to libiberty, if found
#  HAVE_DEMANGLE_H     - true if header file demangle.h exists
#                        (contains declaration of cplus_demangle())
#  HAVE_DECL_BASENAME  - true if a declaration for basename() exists
#  HAVE_CPLUS_DEMANGLE - true if function demangle() exists
#  Z_LIBRARY           - full path to libz, if found

# Copyright (C) 2009-2010
# ASTRON (Netherlands Institute for Radio Astronomy)
# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
#
# This file is part of the LOFAR software suite.
# The LOFAR software suite is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The LOFAR software suite is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
#
# $Id$

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(FindPackageHandleStandardArgs)

if(NOT BACKTRACE_FOUND)
  set(BACKTRACE_LIBRARIES)
  check_include_file(execinfo.h HAVE_EXECINFO_H)
  if(HAVE_EXECINFO_H)
    check_function_exists(backtrace HAVE_BACKTRACE)
    if(HAVE_BACKTRACE)
      check_include_file(bfd.h HAVE_BFD_H)
      if(HAVE_BFD_H)
        find_library(BFD_LIBRARY bfd)
        if(BFD_LIBRARY)
          set(HAVE_BFD 1 CACHE INTERNAL "Have bfd library")
          list(APPEND BACKTRACE_LIBRARIES ${BFD_LIBRARY})
          find_library(IBERTY_LIBRARY iberty)
          if(IBERTY_LIBRARY)
            list(APPEND BACKTRACE_LIBRARIES ${IBERTY_LIBRARY})
            check_include_file(demangle.h HAVE_DEMANGLE_H)
            if(HAVE_DEMANGLE_H)
              # Check for basename() avoids redeclaration error with g++ >= 4.4
              check_symbol_exists(basename demangle.h HAVE_DECL_BASENAME)
              set(CMAKE_REQUIRED_LIBRARIES ${IBERTY_LIBRARY})
              check_function_exists(cplus_demangle HAVE_CPLUS_DEMANGLE)
            endif(HAVE_DEMANGLE_H)
          endif(IBERTY_LIBRARY)
          # Newer version of libbfd also depend on libz.
          find_library(Z_LIBRARY z)
          if(Z_LIBRARY)
            list(APPEND BACKTRACE_LIBRARIES ${Z_LIBRARY})
          endif(Z_LIBRARY)
        endif(BFD_LIBRARY)
      endif(HAVE_BFD_H)
    endif(HAVE_BACKTRACE)
  endif(HAVE_EXECINFO_H)
  mark_as_advanced(BFD_LIBRARY IBERTY_LIBRARY Z_LIBRARY)
  find_package_handle_standard_args(Backtrace DEFAULT_MSG HAVE_BACKTRACE)
endif(NOT BACKTRACE_FOUND)
