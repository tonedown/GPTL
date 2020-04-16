#ifndef UTIL_H
#define UTIL_H
#ifdef HAVE_LIBMPI
#include <mpi.h>
#endif

namespace gptl_util {
  extern int num_warn;
  extern int num_errors;
  extern bool abort_on_error;
  extern "C" {
    int error (const char *, ...);
    void warn (const char *, ...);
    void note (const char *, ...);
    void *allocate (const int, const char *);
#ifdef HAVE_LIBMPI
    int GPTLbarrier (MPI_Comm, const char *);
#endif
  }
}
#endif
