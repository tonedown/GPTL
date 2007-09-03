#include <stdio.h>
#include <stdlib.h>  /* atoi,exit */
#include <unistd.h>  /* getopt */
#include <string.h>  /* memset */
#if ( defined HAVE_LIBMPI ) || ( defined HAVE_LIBMPICH )
#include <mpi.h>
#endif

#include "../gptl.h"

#ifdef HAVE_PAPI
#include <papi.h>
#endif

static int iam = 0;
static int nproc = 1;
static int nthreads;

double sub (int);

int main (int argc, char **argv)
{
  int iter;
  int papiopt;
  int c;
  int comm = 0;

  double ret;

  extern char *optarg;

#if ( defined HAVE_LIBMPI ) || ( defined HAVE_LIBMPICH )
  (void) MPI_Init (&argc, &argv);
  comm = MPI_COMM_WORLD;
#endif

  while ((c = getopt (argc, argv, "p:")) != -1) {
    switch (c) {
    case 'p':
      if ((papiopt = GPTL_PAPIname2id (optarg)) >= 0) {
	printf ("Failure from GPTL_PAPIname2id\n");
	exit (1);
      }
      if (GPTLsetoption (papiopt, 1) < 0) {
	printf ("Failure from GPTLsetoption (%s,1)\n", optarg);
	exit (1);
      }
      break;
    default:
      printf ("unknown option %c\n", c);
      printf ("Usage: %s [-p papi_option_name]\n", argv[0]);
      exit (2);
    }
  }
  
  (void) GPTLsetoption (GPTLabort_on_error, 1);
  (void) GPTLsetoption (GPTLoverhead, 1);
  (void) GPTLsetoption (GPTLnarrowprint, 1);

  GPTLinitialize ();
  GPTLstart ("total");
	 
#if ( defined HAVE_LIBMPI ) || ( defined HAVE_LIBMPICH )
  ret = MPI_Comm_rank (MPI_COMM_WORLD, &iam);
  ret = MPI_Comm_size (MPI_COMM_WORLD, &nproc);
#endif
  if (iam == 0) {
    printf ("Purpose: test behavior of summary stats\n");
    printf ("Include PAPI and OpenMP, respectively, if enabled\n");
  }
  nthreads = omp_get_max_threads ();

#pragma omp parallel for private (iter, ret)
      
  for (iter = 1; iter <= nthreads; iter++) {
    ret = sub (iter);
  }

  GPTLstop ("total");
  GPTLpr (iam);
  GPTLpr_summary (comm);
  if (GPTLfinalize () < 0)
    exit (6);

#if ( defined HAVE_LIBMPI ) || ( defined HAVE_LIBMPICH )
  MPI_Finalize ();
#endif
  return 0;
}

double sub (int iter)
{
  unsigned long usec;
  unsigned long looplen = iam*iter*10000;
  unsigned long i;
  double sum;

  GPTLstart ("sub");
  usec = nproc * nthreads - (iam * iter);
  printf ("sleeping %ld usec\n", usec);

  GPTLstart ("sleep");
  usleep (usec);
  GPTLstop ("sleep");

  GPTLstart ("work");
  sum = 0.;
  GPTLstart ("add");
  for (i = 0; i < looplen; ++i) {
    sum += i;
  }
  GPTLstop ("add");

  GPTLstart ("madd");
  for (i = 0; i < looplen; ++i) {
    sum += i*1.1;
  }
  GPTLstop ("madd");

  GPTLstart ("div");
  for (i = 0; i < looplen; ++i) {
    sum /= 1.1;
  }
  GPTLstop ("div");
  GPTLstop ("work");
  GPTLstop ("sub");
  return sum;
}