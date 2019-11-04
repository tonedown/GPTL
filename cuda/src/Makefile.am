# This automake file generates the Makefile to build the GPTL C library.
# Jim Rosinski 11/3/19

# Small max register count for the library code to minimize performance impact on calling app
NVCC = @NVCC@
CUFLAGS = @CUFLAGS@ --maxrregcount 32

# This is our output. The GPTL library.
lib_LTLIBRARIES = libgptl_cuda.la
libgptl_cuda_la_LDFLAGS = -version-info 1:0:0

# These are the source files.
libgptl_cuda_la_SOURCES = gptl_device.cu globaldrivers.cu util_device.cu

# Add Fortran wrappers if Fortran support was found
if HAVE_FC_ACC
libgptl_cuda_la_SOURCES += f_wrappers_device.cu
endif

%.o: %.cu
	$(NVCC) $< $(CUFLAGS)

# Test output to be deleted.
CLEANFILES = *.ii *.ptx *.cudafe1.* *.fatbin* *.module_id *_dlink.*
