## This is an automake file, part of the GPTL-fortran package.

# This is the automake file for GPTL Fortran GPU tests.
# Jim Rosinski, 11/3/19

# Add include directories
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/fortran/src -I$(top_srcdir)/facc/src

# Link to GPTL fortran library.
AM_LDFLAGS = ${top_builddir}/fortran/src/libgptlf.la ${top_builddir}/src/libgptl.la \
             ${top_builddir}/fortran/src/libgptl_cuda.la

LDADD = ${top_builddir}/src/libgptl.la

# Test programs that will be built for all configurations.
check_PROGRAMS = simple
TESTS          = simple

MODFILES = ${top_builddir}/fortran/src/gptl.mod ${top_builddir}/facc/src/gptl_acc.mod
# Test dependencies needed.
simple_SOURCES = simple.F90 ${MODFILES}

# Test output to be deleted.
CLEANFILES = *.o
