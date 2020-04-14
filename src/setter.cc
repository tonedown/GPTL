// User-callable setter functions

#include "once.h"
#include "private.h"
#include "thread.h"
#include "util.h"
#ifdef HAVE_PAPI
#include "gptl_papi.h"
#endif

#include <string.h>    // memset

extern "C" {
  // GPTLenable: enable timers 
  int GPTLenable (void)
  {
    gptl_private::disabled = false;
    return 0;
  }
  
  // GPTLdisable: disable timers
  int GPTLdisable (void)
  {
    gptl_private::disabled = true;
    return 0;
  }

  // GPTLreset_timer: reset a timer to 0
  // Return value: 0 (success) or GPTLerror (failure)
  int GPTLreset_timer (char *name)
  {
    int t;             // index over threads
    gptl_private::Timer *ptr;        // linked list index
    unsigned int indx; // hash table index
    static const char *thisfunc = "GPTLreset_timer";

    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    if (gptl_thread::get_thread_num () != 0)
      return gptl_util::error ("%s: Must be called by the master thread\n", thisfunc);

    indx = gptl_private::genhashidx (name);
    for (t = 0; t < gptl_thread::nthreads; ++t) {
      ptr = gptl_private::getentry (gptl_private::hashtable[t], name, indx);
      if (ptr) {
	ptr->onflg = false;
	ptr->count = 0;
	memset (&ptr->wall, 0, sizeof (ptr->wall));
	memset (&ptr->cpu, 0, sizeof (ptr->cpu));
#ifdef HAVE_PAPI
	memset (&ptr->aux, 0, sizeof (ptr->aux));
#endif
      }
    }
    return 0;
  }

  // GPTLreset: reset all timers to 0
  // Return value: 0 (success) or GPTLerror (failure)
  int GPTLreset (void)
  {
    gptl_private::Timer *ptr;        // linked list index
    static const char *thisfunc = "GPTLreset";

    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    for (int t = 0; t < gptl_thread::nthreads; t++) {
      for (ptr = gptl_private::timers[t]; ptr; ptr = ptr->next) {
	ptr->onflg = false;
	ptr->count = 0;
	memset (&ptr->wall, 0, sizeof (ptr->wall));
	memset (&ptr->cpu, 0, sizeof (ptr->cpu));
#ifdef HAVE_PAPI
	memset (&ptr->aux, 0, sizeof (ptr->aux));
#endif
      }
    }
    if (gptl_once::verbose)
      printf ("%s: accumulators for all timers set to zero\n", thisfunc);

    return 0;
  }

  // GPTLreset_errors: reset error state to no errors
  int GPTLreset_errors (void) {gptl_util::num_errors = 0;}
}
