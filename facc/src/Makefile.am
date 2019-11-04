# This automake file generates the Makefile to build the GPTL C library.
# Jim Rosinski 11/3/19

FFLAGS_ACC = @FFLAGS_ACC@
# This is our output: Add to GPTL device library.
lib_LTLIBRARIES = ../../cuda/src/libgptl_cuda.la
libgptl_cuda_la_LDFLAGS = -version-info 1:0:0

# These are the source files.
libgptl_cuda_la_SOURCES = gptl_acc.F90

%.o: %.F90
	$(FC) -c $< $(FFLAGS_ACC)
