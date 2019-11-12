# This automake file generates the Makefile to build the GPTL C library.
# Jim Rosinski 11/3/19

# Small max register count for the library code to minimize performance impact on calling app
HELPER_PATH = @HELPER_PATH@
NVCC = @NVCC@
CUFLAGS = @CUFLAGS@ --maxrregcount 32
INC = -I$(top_srcdir) -I$(top_srcdir)/cuda/include -I$(top_srcdir)/include
if HAVE_HELPER
INC += -I$(HELPER_PATH)
endif

.cu.o:
	$(NVCC) $(INC) $(CUFLAGS) -o $@ -c $<

.cu.lo:
	$(top_srcdir)/cudalt.py $@ $(NVCC) -c $(INC) $(CUFLAGS) -c $<

# This is our output. The GPTL CUDA library.
lib_LTLIBRARIES = libgptlcuda.la

libgptlcuda_la_CPPFLAGS = -I$(top_srcdir)/cuda/include -I$(top_srcdir)/include
libgptlcuda_la_LDFLAGS = -version-info 1:0:0

# These are the source files.
libgptlcuda_la_SOURCES = gptl_device.cu globaldrivers.cu util_device.cu print_gpustats.cu

# Add Fortran wrappers if Fortran support was found
if HAVE_FC_ACC
libgptlcuda_la_SOURCES += f_wrappers_device.cu
endif

# Test output to be deleted.
CLEANFILES = *.ii *.ptx *.cudafe1.* *.fatbin* *.module_id *_dlink.*
