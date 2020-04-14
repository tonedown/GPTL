#ifndef GPTLMPI_H
#define GPTLMPI_H

#include <mpi.h>

extern "C" {
  // In pr_summary.cc:
  extern int GPTLpr_summary (MPI_Comm);
  extern int GPTLpr_summary_file (MPI_Comm, const char *);

  // In util.cc:
  extern int GPTLbarrier (MPI_Comm, const char *);
}
#endif
