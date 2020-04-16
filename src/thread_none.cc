#include "config.h" // Must be first include
#include "thread.h"
#include "util.h"

int threadid = -1;   // This probably isn't needed

namespace gptl_thread {
  volatile int maxthreads = -1;       // max threads
  volatile int nthreads = -1;         // num threads. Init to bad value

  extern "C" {
    int threadinit (void)
    {
      static const char *thisfunc = "threadinit";

      if (nthreads != -1)
	return gptl_util::error ("GPTL: Unthreaded %s: MUST only be called once", thisfunc);

      nthreads = 0;
      maxthreads = 1;
      return 0;
    }

    void threadfinalize () {threadid = -1;}

    inline int get_thread_num ()
    {
#ifdef HAVE_PAPI
      static const char *thisfunc = "get_thread_num";
      /*
      ** When HAVE_PAPI is true, if 1 or more PAPI events are enabled,
      ** create and start an event set for the new thread.
      */
      if (threadid == -1 && gptl_papi::npapievents > 0) {
	if (create_and_start_events (0) < 0)
	  return gptl_util::error ("GPTL: Unthreaded %s: error from GPTLcreate_and_start_events for thread %0\n",
				   thisfunc);

	threadid = 0;
      }
#endif

      nthreads = 1;
      return 0;
    }

    void print_threadmapping (FILE *fp)
    {
      fprintf (fp, "\n");
      fprintf (fp, "threadid[0] = 0\n");
    }
  }
}
