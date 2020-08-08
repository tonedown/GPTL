# This file contains macro settings which are used by the Makefile. Some require
# "yes" or "no", where various subflags are required if the value is
# "yes". The intent is for the user to edit whichever parts of this file are
# necessary, save to a file named "macros.make", then run "make" to build the
# GPTL library.

##########################################################################

# Where to install GPTL library and include files.
# if "git" is not available, set REVNO by hand
REVNO = $(shell git describe)
INSTALLDIR = $(HOME)/install

# Where to install man pages (if blank, defaults to $INSTALLDIR)
MANDIR = 

# C compiler
CC = nvcc

# CUDA compiler
CU = nvcc

# Whether to build debug lib or optimized lib, and associated flags
DEBUG = no
CUFLAGS_ALL = $(UNDERSCORING) -g -ftz=true -gencode arch=compute_60,code=compute_60 -rdc=true -keep
ifeq ($(DEBUG),yes)
  CUFLAGS = -DDEBUG -O0 -G -lineinfo $(CUFLAGS_ALL)
else
  CUFLAGS = -UDEBUG -O2              $(CUFLAGS_ALL)
endif
# Add diagnostic info
#CUFLAGS += -Xptxas="-v"

# Is the /proc filesystem available? On most Linux systems it is. If available, get_memusage() 
# reads from it to find memory usage info. Otherwise get_memusage() will use get_rusage()
HAVE_SLASHPROC = yes

# GPTL can enable threading either via OPENMP=yes, or PTHREADS=yes. Since
# most OpenMP implementations are built on top of pthreads, OpenMP
# applications linked with GPTL as built with PTHREADS=yes should work fine.
# Thus COMPFLAG should be set to the compiler flag that enables OpenMP directives
# if either OPENMP=yes, or PTHREADS=yes. If OPENMP=no and PTHREADS=no, GPTL 
# will not be thread-safe.
OPENMP = no
COMPFLAG = -mp
# Set PTHREADS if available and OPENMP=no
ifeq ($(OPENMP),no)
  PTHREADS = no
endif

# Set ENABLE_NESTEDOMP=yes only if you will be profiling code regions inside of doubly-nested
# OpenMP regions. ENABLE_NESTEDOMP=yes can increase the cost of getting the thread number by
# about 50% even in regions which are not doubly-nested.
ifeq ($(OPENMP),yes)
  ENABLE_NESTEDOMP = no
endif

# For gcc, -Dinline=inline is a no-op. For other C compilers, things like 
# -Dinline=__inline__ may be required. To find your compiler's definition, try 
# running "./suggestions CC=<your_C_compiler>".
INLINEFLAG = -Dinline=inline

# To get some C compilers such as gcc to behave properly with -O0 and no inlining, 
# need to effectively delete the "inline" keyword
#For some reason nvcc doesn't like -Dinline=<nothing> (tons of multiple entry points)
ifeq ($(DEBUG),yes)
  INLINEFLAG = -Dinline=inline
endif

# To build the Fortran interface, set FORTRAN=yes and define the entries under
# ifeq ($(FORTRAN),yes). Otherwise, set FORTRAN=no and skip this section.
FORTRAN = yes
ifeq ($(FORTRAN),yes)
# Fortran name mangling: possibilities are: leave UNDERSCORING blank meaning none
# (e.g. xlf90), -DFORTRANDOUBLEUNDERSCORE (e.g. g77), and -DFORTRANUNDERSCORE 
# (e.g. gfortran, pathf95)
#
#  UNDERSCORING =
#  UNDERSCORING = -DFORTRANDOUBLEUNDERSCORE
  UNDERSCORING = -DFORTRANUNDERSCORE

# Set Fortran compiler, flags, and OpenMP compiler flag. Note that Fortran
# OpenMP tests are possible with OPENMP=no as long as PTHREADS=yes
# These settings are only used by the Fortran test applications in ftests/.
  FC     = nvfortran
  FFLAGS = -g -O2 -m64
  FOMPFLAG = -mp
endif

# Archiver: normally it's just ar
AR = ar

# Whether to build GPTL with MPI support. Set inc and lib info if needed. 
# If CC=mpicc, MPI_INCFLAGS and MPI_LIBFLAGS can be blank.
HAVE_MPI = no
ifeq ($(HAVE_MPI),yes)
# Hopefully MPI_Comm_f2c() exists, but if not, set HAVE_COMM_F2C = no
  HAVE_COMM_F2C = yes
  MPI_INCFLAGS = 
  MPI_LIBFLAGS = 
# Want 2 MPI tasks
  MPICMD = mpirun -n 2
endif

# clock_gettime() in librt.a is an option for gathering wallclock time stats
# on some machines. Setting HAVE_LIBRT=yes enables this, but will probably
# require linking applications with -lrt 
HAVE_LIBRT = no

# Only define HAVE_NANOTIME if this is a x86. It provides by far the finest grained,
# lowest overhead wallclock timer on that architecture.
# If HAVE_NANOTIME=yes, set BIT64=yes if this is an x86_64
HAVE_NANOTIME = yes
ifeq ($(HAVE_NANOTIME),yes)
  BIT64 = yes
endif

# Some old compilers don't support vprintf. Set to "no" in this case
HAVE_VPRINTF = yes

# Some old compilers don't support the C times() function. Set to "no" in this case
HAVE_TIMES = yes

# gettimeofday() should be available everywhere. But if not, set to "no"
HAVE_GETTIMEOFDAY = yes

# Whether to test auto-profiling (adds 2 tests to "make test"). If compiler is gcc or 
# pathscale, set INSTRFLAG to -finstrument-functions. PGI 12 and later provide 
# -Minstrument=functions.
TEST_AUTOPROFILE = no
ifeq ($(TEST_AUTOPROFILE),yes)
  INSTRFLAG = -Minstrument=functions
  CXX = nvcc
endif

# Whether system functions backtrace() and backtrace_symbols() exist. Setting HAVE_BACKTRACE=yes
# will enable auto-generation of function name when auto-profiling and GPTLdopr_memusage has been
# enabled at run-time. If HAVE_BACKTRACE=no, function address will be printed instead.
# GNU compilers: compile application code with -finstrument-functions -rdynamic
# Intel compilers: compile application code with -finstrument-functions -rdynamic -g
HAVE_BACKTRACE = no
ifeq ($(HAVE_BACKTRACE),yes)
  CUFLAGS += -DHAVE_BACKTRACE
endif

# Whether to enable wrappers (for user codes) and tests for OpenACC
# If set to "no" then no wrappers will be built or installed, nor will tests requiring
# OpenACC but built or run
ENABLE_ACC = yes

# Name of the CUDA-enabled library (not a good idea to change this)
GPULIB = gptl_cuda
