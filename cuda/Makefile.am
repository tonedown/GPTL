# This automake file generates the Makefile to build CUDA support into GPTL
# Jim Rosinski 11/3/19

libgptl_la_CPPFLAGS = -I$(top_srcdir)/include 

# This is our output. The GPTL library.
lib_LTLIBRARIES = libgptl_cuda.la
libgptl_cuda_la_LDFLAGS = -version-info 1:0:0

# These are the source files.
libgptl_cuda_la_SOURCES = gptl_device.cu util_device.cu

if HAVE_FORTRAN
libgptl_cuda_la_SOURCES += f_wrappers_device.cu
endif
