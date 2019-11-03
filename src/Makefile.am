# This automake file generates the Makefile to build the GPTL C library.
# Ed Hartnett 5/18/18

libgptl_la_CPPFLAGS = -I$(top_srcdir)/include 

# This is our output. The GPTL library.
lib_LTLIBRARIES = libgptl.la
libgptl_la_LDFLAGS = -version-info 1:0:0

# These are the source files.
libgptl_la_SOURCES = gptl.cpp getoverhead.cpp hashstats.cpp memstats.cpp memusage.cpp util.cpp

if HAVE_FORTRAN
libgptl_la_SOURCES += f_wrappers.cpp
endif

if HAVE_PAPI
libgptl_la_SOURCES += gptl_papi.cpp
endif

if HAVE_LIBMPI
libgptl_la_SOURCES += pr_summary.cpp
if ENABLE_PMPI
libgptl_la_SOURCES += pmpi.cpp
if HAVE_FORTRAN
libgptl_la_SOURCES += f_wrappers_pmpi.cpp
endif
endif
endif
