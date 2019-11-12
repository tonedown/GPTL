## This is an automake file, part of the GPTL-fortran package.

# This is the automake file for GPTL CUDA tests
# Jim Rosinski, 11/3/19

NVCC = @NVCC@
CUFLAGS = @CUFLAGS@
HELPER_PATH = @HELPER_PATH@
if HAVE_HELPER
CUFLAGS += -I$(top_srcdir)/include -I$(HELPER_PATH)
endif

# Find include directory and src directory (for .mod file).

# Link to GPTL library.
AM_LDFLAGS = ${top_builddir}/src/libgptl.la ${top_builddir}/cuda/src/libgptl_cuda.la

LDADD = ${top_builddir}/cuda/src/libgptl_cuda.la -lstdc++

# Test programs that will be built for all configurations.
check_PROGRAMS = driver_serial test1
TESTS          = driver_serial test1

# Test dependencies needed.
driver_serial_SOURCES = driver_serial.cu getval.cu persist.cu sleep1.cu sleep.cu subs.cu
test1_SOURCES         = test1.cu

%.o: %.cu
	$(NVCC) -c $< $(CUFLAGS)

# Test output to be deleted.
CLEANFILES = *.ii *.ptx *.cudafe1.* *.fatbin* *.module_id *_dlink.*
