#include "config.h" // Must be first include.
#include "defines.h"
#include "util.h"
#include "once.h"
#include "thread.h"
#include "gptl.h"
#ifdef HAVE_PAPI
#include "gptl_papi.h"
#endif

#include <sys/times.h>
#include <string.h>

// User-accessible "getter" methods. Must be outside namespace for C/Fortran callability

extern "C" {
  /*
  ** GPTLstamp: Compute timestamp of usr, sys, and wallclock time (seconds)
  **
  ** Output arguments:
  **   wall: wallclock
  **   usr:  user time
  **   sys:  system time
  **
  ** Return value: 0 (success) or gptl_util::error (failure)
  */
  int GPTLstamp (double *wall, double *usr, double *sys)
  {
    using gptl_private::ptr2wtimefunc;
    struct tms buf;   // argument to times

    if ( ! gptl_once::initialized)
      return gptl_util::error ("GPTLstamp: GPTLinitialize has not been called\n");

#ifdef HAVE_TIMES
    *usr = 0;
    *sys = 0;

    if (times (&buf) == -1)
      return gptl_util::error ("GPTLstamp: times() failed. Results bogus\n");

    *usr = buf.tms_utime / (double) gptl_once::ticks_per_sec;
    *sys = buf.tms_stime / (double) gptl_once::ticks_per_sec;
#endif
    *wall = (*ptr2wtimefunc) ();
    return 0;
  }

  /*
  ** GPTLquery: return current status info about a timer. If certain stats are not 
  ** enabled, they should just have zeros in them. If PAPI is not enabled, input
  ** counter info is ignored.
  ** 
  ** Input args:
  **   name:        timer name
  **   maxcounters: max number of PAPI counters to get info for
  **   t:           thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   count:            number of times this timer was called
  **   onflg:            whether timer is currently on
  **   wallclock:        accumulated wallclock time
  **   usr:              accumulated user CPU time
  **   sys:              accumulated system CPU time
  **   papicounters_out: accumulated PAPI counters
  */
  int GPTLquery (const char *name, int t, int *count, int *onflg, double *wallclock,
		 double *dusr, double *dsys, long long *papicounters_out, const int maxcounters)
  {
    gptl_private::Timer *ptr;  // linked list pointer
    unsigned int indx;         // linked list index returned from getentry (unused)
    static const char *thisfunc = "GPTLquery";
  
    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: get_thread_num failure\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }

    indx = gptl_private::genhashidx (name);
    ptr = gptl_private::getentry (gptl_private::hashtable[t], name, indx);
    if ( ! ptr)
      return gptl_util::error ("%s: requested timer %s does not have a name hash\n",
			       thisfunc, name);

    *onflg     = ptr->onflg;
    *count     = ptr->count;
    *wallclock = ptr->wall.accum;
    *dusr      = ptr->cpu.accum_utime / (double) gptl_once::ticks_per_sec;
    *dsys      = ptr->cpu.accum_stime / (double) gptl_once::ticks_per_sec;
#ifdef HAVE_PAPI
    gptl_papi::PAPIquery (&ptr->aux, papicounters_out, maxcounters);
#endif
    return 0;
  }

  /*
  ** GPTLget_wallclock: return wallclock accumulation for a timer.
  ** 
  ** Input args:
  **   timername: timer name
  **   t:         thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   value: current wallclock accumulation for the timer
  */
  int GPTLget_wallclock (const char *timername, int t, double *value)
  {
    gptl_private::Timer *ptr;          // linked list pointer
    unsigned int indx;   // hash index returned from getentry (unused)
    static const char *thisfunc = "GPTLget_wallclock";
  
    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    if ( ! gptl_private::wallstats.enabled)
      return gptl_util::error ("%s: wallstats not enabled\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: bad return from get_thread_num\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    indx = gptl_private::genhashidx (timername);
    ptr = gptl_private::getentry (gptl_private::hashtable[t], timername, indx);
    if ( ! ptr)
      return gptl_util::error ("%s: requested timer %s does not exist (or auto-instrumented?)\n",
			       thisfunc, timername);
    *value = ptr->wall.accum;
    return 0;
  }

  /*
  ** GPTLget_wallclock_latest: return most recent wallclock value for a timer.
  ** 
  ** Input args:
  **   timername: timer name
  **   t:         thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   value: most recent wallclock value for the timer
  */
  int GPTLget_wallclock_latest (const char *timername, int t, double *value)
  {
    gptl_private::Timer *ptr;          // linked list pointer
    unsigned int indx;   // hash index returned from getentry (unused)
    static const char *thisfunc = "GPTLget_wallclock_latest";
  
    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    if ( ! gptl_private::wallstats.enabled)
      return gptl_util::error ("%s: wallstats not enabled\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: bad return from get_thread_num\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    indx = gptl_private::genhashidx (timername);
    ptr = gptl_private::getentry (gptl_private::hashtable[t], timername, indx);
    if ( ! ptr)
      return gptl_util::error ("%s: requested timer %s does not exist\n", thisfunc, timername);
    *value = ptr->wall.latest;
    return 0;
  }

  /*
  ** GPTLget_threadwork: For a timer, across threads compute max work and imbalance
  **
  ** Input arguments:
  **   name: timer name
  **
  ** Output arguments:
  **   maxwork: maximum work across threads
  **   imbal:   imbalance vs. perfectly distributed workload
  **
  ** Return value: 0 (success) or -1 (failure)
  */
  int GPTLget_threadwork (const char *name, double *maxwork, double *imbal)
  {
    gptl_private::Timer *ptr;                  // linked list pointer
    int t;                       // thread number for this process
    int nfound = 0;              // number of threads which did work (must be > 0
    unsigned int indx;           // index into hash table
    double innermax = 0.;        // maximum work across threads
    double totalwork = 0.;       // total work done by all threads
    double balancedwork;         // time if work were perfectly load balanced
    static const char *thisfunc = "GPTLget_threadwork";

    if (gptl_private::disabled)
      return 0;

    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    if ( ! gptl_private::wallstats.enabled)
      return gptl_util::error ("%s: wallstats must be enabled to call this function\n", thisfunc);

    if (gptl_thread::get_thread_num () != 0)
      return gptl_util::error ("%s: Must be called by the master thread\n", thisfunc);

    indx = gptl_private::genhashidx (name);
    for (t = 0; t < gptl_thread::nthreads; ++t) {
      ptr = gptl_private::getentry (gptl_private::hashtable[t], name, indx);
      if (ptr) {
	++nfound;
	innermax = MAX (innermax, ptr->wall.accum);
	totalwork += ptr->wall.accum;
      }
    }

    // It's an error to call this routine for a region that does not exist
    if (nfound == 0)
      return gptl_util::error ("%s: No entries exist for name=%s\n", thisfunc, name);

    // A perfectly load-balanced calculation would take time=totalwork/nthreads
    // Therefore imbalance is slowest thread time minus this number
    balancedwork = totalwork / gptl_thread::nthreads;
    *maxwork = innermax;
    *imbal = innermax - balancedwork;
    return 0;
  }

  /*
  ** GPTLget_count: return number of start/stop calls for a timer.
  ** 
  ** Input args:
  **   timername: timer name
  **   t:         thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   count: current number of start/stop calls for the timer
  */
  int GPTLget_count (const char *timername, int t, int *count)
  {
    gptl_private::Timer *ptr;          // linked list pointer
    unsigned int indx;   // hash index returned from getentry (unused)
    static const char *thisfunc = "GPTLget_count";
  
    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);

    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: bad return from get_thread_num\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    indx = gptl_private::genhashidx (timername);
    ptr = gptl_private::getentry (gptl_private::hashtable[t], timername, indx);
    if ( ! ptr)
      return gptl_util::error ("%s: requested timer %s does not exist (or auto-instrumented?)\n",
			       thisfunc, timername);
    *count = ptr->count;
    return 0;
  }

  /*
  ** GPTLget_eventvalue: return PAPI-based event value for a timer. All values will be
  **   returned as doubles, even if the event is not derived.
  ** 
  ** Input args:
  **   timername: timer name
  **   eventname: event name (must be currently enabled)
  **   t:         thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   value: current value of the event for this timer
  */
  int GPTLget_eventvalue (const char *timername, const char *eventname, int t, double *value)
  {
    gptl_private::Timer *ptr;          // linked list pointer
    unsigned int indx;   // hash index returned from getentry (unused)
    static const char *thisfunc = "GPTLget_eventvalue";
  
    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: get_thread_num failure\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    indx = gptl_private::genhashidx (timername);
    ptr = gptl_private::getentry (gptl_private::hashtable[t], timername, indx);
    if ( ! ptr)
      return gptl_util::error ("%s: requested timer %s does not exist (or auto-instrumented?)\n",
			       thisfunc, timername);

#ifdef HAVE_PAPI
    return gptl_papi::PAPIget_eventvalue (eventname, &ptr->aux, value);
#else
    return gptl_util::error ("%s: PAPI not enabled\n", thisfunc); 
#endif
  }

  /*
  ** GPTLget_nregions: return number of regions (i.e. timer names) for this thread
  ** 
  ** Input args:
  **   t:    thread number (if < 0, the request is for the current thread)
  **
  ** Output args:
  **   nregions: number of regions
  */
  int GPTLget_nregions (int t, int *nregions)
  {
    gptl_private::Timer *ptr;     // walk through linked list
    const char *thisfunc = "GPTLget_nregions";

    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: get_thread_num failure\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    *nregions = 0;
    for (ptr = gptl_private::timers[t]->next; ptr; ptr = ptr->next) 
      ++*nregions;

    return 0;
  }

  /*
  ** GPTLget_regionname: return region name for this thread
  ** 
  ** Input args:
  **   t:      thread number (if < 0, the request is for the current thread)
  **   region: region number
  **   nc:     max number of chars to put in name
  **
  ** Output args:
  **   name    region name
  */
  int GPTLget_regionname (int t, int region, char *name, int nc)
  {
    int ncpy;    // number of characters to copy
    int i;       // index
    gptl_private::Timer *ptr;  // walk through linked list
    static const char *thisfunc = "GPTLget_regionname";

    if ( ! gptl_once::initialized)
      return gptl_util::error ("%s: GPTLinitialize has not been called\n", thisfunc);
  
    // If t is < 0, assume the request is for the current thread
    if (t < 0) {
      if ((t = gptl_thread::get_thread_num ()) < 0)
	return gptl_util::error ("%s: get_thread_num failure\n", thisfunc);
    } else {
      if (t >= gptl_thread::maxthreads)
	return gptl_util::error ("%s: requested thread %d is too big\n", thisfunc, t);
    }
  
    ptr = gptl_private::timers[t]->next;
    for (i = 0; i < region; i++) {
      if ( ! ptr)
	return gptl_util::error ("%s: timer number %d does not exist in thread %d\n", thisfunc, region, t);
      ptr = ptr->next;
    }

    if (ptr) {
      ncpy = MIN (nc, strlen (ptr->name));
      strncpy (name, ptr->name, ncpy);
    
      // Adding the \0 is only important when called from C or C++
      if (ncpy < nc)
	name[ncpy] = '\0';
    } else {
      return gptl_util::error ("%s: timer number %d does not exist in thread %d\n", thisfunc, region, t);
    }
    return 0;
  }

  // Whether GPTL has been initialized
  int GPTLis_initialized (void) {return (int) gptl_once::initialized;}

  // GPTLnum_errors: returns number of times GPTLerror() called
  int GPTLnum_errors (void) {return gptl_util::num_errors;}

  // GPTLnum_warn: returns number of times GPTLwarn() called
  int GPTLnum_warn (void) {return gptl_util::num_warn;}
}
