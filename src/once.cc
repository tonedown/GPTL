#include "config.h" // must be 1st include
#include "once.h"
#include "thread.h"
#include "util.h"
#include "postprocess.h"
#include "private.h"
#include "gptl.h"   // user-visible prototypes
#ifdef HAVE_PAPI
#include "gptl_papi.h"
#endif

#include <stdio.h>
#include <ctype.h>     // isdigit
#include <time.h>      // time_t
#include <stdlib.h>    // atof
#include <string.h>
#include <unistd.h>    // sysconf

// Local to the file
static const int LEN = 4096;
static float get_clockfreq (void);

#ifdef HAVE_GETTIMEOFDAY
static int init_gettimeofday (void);
#endif

#ifdef HAVE_NANOTIME
int init_nanotime (void);
#endif

#ifdef HAVE_LIBMPI
static int init_mpiwtime (void);
#endif

#ifdef HAVE_LIBRT
static int init_clock_gettime (void);
#endif

#ifdef _AIX
static int init_read_real_time (void);
#endif

static int init_placebo (void);
static void set_abort_on_error (bool val);

// Namespace declarations of data/functions to be used only by GPTL
namespace gptl_once {
  float cpumhz = -1.;                 // clock freq. in mhz. Init to bad value
  volatile bool initialized = false;  // GPTLinitialize has been called
  bool percent = false;               // print wallclock also as percent of 1st timers[0]
  bool dopr_preamble = true;          // whether to print preamble info
  bool dopr_threadsort = true;        // whether to print sorted thread stats
  bool dopr_multparent = true;        // whether to print multiple parent info
  bool dopr_collision = false;        // whether to print hash collision info
  bool dopr_memusage = false;         // whether to include memusage print when auto-profiling
  bool verbose = false;               // output verbosity
  long ticks_per_sec;                 // clock ticks per second
  int depthlimit = 99999;             // limit of depth to go down call tree
  bool sync_mpi = false;              // auto-synchronize MPI calls when PMPI enabled
  bool onlypr_rank0 = false;          // flag says only print from MPI rank 0 (default false)
  time_t ref_gettimeofday = -1;       // ref start point for gettimeofday
  time_t ref_clock_gettime = -1;      // ref start point for clock_gettime
  int funcidx = 0;                    // default timer is gettimeofday
#ifdef HAVE_NANOTIME
  double cyc2sec = -1;                // convert cycles to seconds. init to bad value
  char *clock_source = "unknown";     // where clock found
#endif

  Funcentry funclist[] = {
#ifdef HAVE_GETTIMEOFDAY
    {GPTLgettimeofday,   gptl_private::utr_gettimeofday,   init_gettimeofday,  "gettimeofday"},
#endif
#ifdef HAVE_NANOTIME
    {GPTLnanotime,       gptl_private::utr_nanotime,       init_nanotime,      "nanotime"},
#endif
#ifdef HAVE_LIBMPI
    {GPTLmpiwtime,       gptl_private::utr_mpiwtime,       init_mpiwtime,      "MPI_Wtime"},
#endif
#ifdef HAVE_LIBRT
    {GPTLclockgettime,   gptl_private::utr_clock_gettime,  init_clock_gettime, "clock_gettime"},
#endif
#ifdef _AIX
    {GPTLread_real_time, gptl_private::utr_read_real_time, init_read_real_time,"read_real_time"},
#endif
    {GPTLplacebo,        gptl_private::utr_placebo,        init_placebo,       "placebo"}
  };
  const int nfuncentries = sizeof (funclist) / sizeof (Funcentry);
  // The following are the set of initializers for underlying timing routines which may or may
  // not be available. NANOTIME is only available on x86.
}

extern "C" {
  // Public entry points
  /**
   * Set option value
   *
   * @param option option to be set
   * @param val value to which option should be set (nonzero=true, zero=false)
   *
   * @return: 0 (success) or gptl_util::error (failure)
   */
  int GPTLsetoption (const int option, const int val)
  {
    using gptl_once::verbose;
    using gptl_postprocess::method;
    using gptl_postprocess::methodstr;
    static const char *thisfunc = "GPTLsetoption";
      
    if (gptl_once::initialized)
      return gptl_util::error ("%s: must be called BEFORE GPTLinitialize\n", thisfunc);
      
    if (option == GPTLabort_on_error) {
      set_abort_on_error ((bool) val);
      if (verbose)
	printf ("%s: boolean abort_on_error = %d\n", thisfunc, val);
      return 0;
    }
    
    switch (option) {
    case GPTLcpu:
#ifdef HAVE_TIMES
      gptl_private::cpustats.enabled = (bool) val; 
      if (verbose)
	printf ("%s: cpustats = %d\n", thisfunc, val);
#else
      if (val)
	return gptl_util::error ("%s: times() not available\n", thisfunc);
#endif
      return 0;
    case GPTLwall:     
      gptl_private::wallstats.enabled = (bool) val; 
      if (verbose)
	printf ("%s: boolean wallstats = %d\n", thisfunc, val);
      return 0;
    case GPTLoverhead: 
      gptl_private::overheadstats.enabled = (bool) val; 
      if (verbose)
	printf ("%s: boolean overheadstats = %d\n", thisfunc, val);
      return 0;
    case GPTLdepthlimit: 
      gptl_once::depthlimit = val; 
      if (verbose)
	printf ("%s: depthlimit = %d\n", thisfunc, val);
      return 0;
    case GPTLverbose: 
      verbose = (bool) val; 
#ifdef HAVE_PAPI
      (void) gptl_papi::PAPIsetoption (GPTLverbose, val);
#endif
      if (verbose)
	printf ("%s: boolean verbose = %d\n", thisfunc, val);
      return 0;
    case GPTLpercent: 
      gptl_once::percent = (bool) val; 
      if (verbose)
	printf ("%s: boolean percent = %d\n", thisfunc, val);
      return 0;
    case GPTLdopr_preamble: 
      gptl_once::dopr_preamble = (bool) val; 
      if (verbose)
	printf ("%s: boolean dopr_preamble = %d\n", thisfunc, val);
      return 0;
    case GPTLdopr_threadsort: 
      gptl_once::dopr_threadsort = (bool) val; 
      if (verbose)
	printf ("%s: boolean dopr_threadsort = %d\n", thisfunc, val);
      return 0;
    case GPTLdopr_multparent: 
      gptl_once::dopr_multparent = (bool) val; 
      if (verbose)
	printf ("%s: boolean dopr_multparent = %d\n", thisfunc, val);
      return 0;
    case GPTLdopr_collision: 
      gptl_once::dopr_collision = (bool) val; 
      if (verbose)
	printf ("%s: boolean dopr_collision = %d\n", thisfunc, val);
      return 0;
    case GPTLdopr_memusage: 
      gptl_once::dopr_memusage = (bool) val; 
      if (verbose)
	printf ("%s: boolean dopr_memusage = %d\n", thisfunc, val);
      return 0;
    case GPTLprint_method:
      method = (GPTL_Method) val; 
      if (verbose)
	printf ("%s: print_method = %s\n", thisfunc, methodstr (method));
      return 0;
    case GPTLtablesize:
      if (val < 1)
	return gptl_util::error ("%s: tablesize must be positive. %d is invalid\n", thisfunc, val);
      gptl_private::tablesize = val;
      gptl_private::tablesizem1 = val - 1;
      if (verbose)
	printf ("%s: tablesize = %d\n", thisfunc, gptl_private::tablesize);
      return 0;
    case GPTLsync_mpi:
#ifdef ENABLE_PMPI
      gptl_once::sync_mpi = (bool) val;
      if (verbose)
	printf ("%s: boolean sync_mpi = %d\n", thisfunc, val);
#else
      fprintf (stderr, "%s: option GPTLsync_mpi requires MPI\n", thisfunc);
#endif
      return 0;
    case GPTLmaxthreads:
      if (val < 1)
	return gptl_util::error ("%s: maxthreads must be positive. %d is invalid\n", thisfunc, val);

      gptl_thread::maxthreads = val;
      return 0;
    case GPTLonlyprint_rank0:
      gptl_once::onlypr_rank0 = (bool) val; 
      if (verbose)
	printf ("%s: onlypr_rank0 = %d\n", thisfunc, val);
      return 0;
    
    case GPTLmultiplex:
      /* Allow GPTLmultiplex to fall through because it will be handled by GPTL_PAPIsetoption() */
    default:
      break;
    }
#ifdef HAVE_PAPI
    if (gptl_papi::PAPIsetoption (option, val) == 0) {
      if (val)
	dousepapi = true;
      return 0;
    }
#endif
    return gptl_util::error ("%s: failure to enable option %d\n", thisfunc, option);
  }

  /*
  ** GPTLsetutr: set underlying timing routine.
  **
  ** Input arguments:
  **   option: index which sets function
  **
  ** Return value: 0 (success) or gptl_util::error (failure)
  */
  int GPTLsetutr (const int option)
  {
    int i;  // index over number of underlying timer
    static const char *thisfunc = "GPTLsetutr";

    if (gptl_once::initialized)
      return gptl_util::error ("%s: must be called BEFORE GPTLinitialize\n", thisfunc);

    for (i = 0; i < gptl_once::nfuncentries; i++) {
      if (option == (int) gptl_once::funclist[i].option) {
	if (gptl_once::verbose)
	  printf ("%s: underlying wallclock timer = %s\n", thisfunc, gptl_once::funclist[i].name);
	gptl_once::funcidx = i;

	// Return an error condition if the function is not available.
	// OK for the user code to ignore: GPTLinitialize() will reset to gettimeofday
	if ((*gptl_once::funclist[i].funcinit)() < 0)
	  return gptl_util::error ("%s: utr=%s not available or doesn't work\n",
				   thisfunc, gptl_once::funclist[i].name);
	else
	  return 0;
      }
    }
    return gptl_util::error ("%s: unknown option %d\n", thisfunc, option);
  }

  /*
  ** GPTLinitialize (): Initialization routine must be called from single-threaded
  **   region before any other timing routines may be called.  The need for this
  **   routine could be eliminated if not targetting timing library for threaded
  **   capability. 
  **
  ** return value: 0 (success) or gptl_util::error (failure)
  */
  int GPTLinitialize (void)
  {
    using gptl_thread::maxthreads;
    using gptl_util::allocate;
    using gptl_once::funclist;
    using gptl_once::funcidx;
    using namespace gptl_private;    // violate "using namespace" rule here
    int i;          // loop index
    int t;          // thread index
    double t1, t2;  // returned from underlying timer
    static const char *thisfunc = "GPTLinitialize";

    if (gptl_once::initialized)
      return gptl_util::error ("%s: has already been called\n", thisfunc);

    if (gptl_thread::threadinit () < 0)
      return gptl_util::error ("%s: bad return from threadinit\n", thisfunc);

    if ((gptl_once::ticks_per_sec = sysconf (_SC_CLK_TCK)) == -1)
      return gptl_util::error ("%s: failure from sysconf (_SC_CLK_TCK)\n", thisfunc);

    // Allocate space for global arrays
    callstack       = (Timer ***)    allocate (maxthreads * sizeof (Timer **), thisfunc);
    stackidx        = (Nofalse *)    allocate (maxthreads * sizeof (Nofalse), thisfunc);
    timers          = (Timer **)     allocate (maxthreads * sizeof (Timer *), thisfunc);
    last            = (Timer **)     allocate (maxthreads * sizeof (Timer *), thisfunc);
    hashtable       = (Hashentry **) allocate (maxthreads * sizeof (Hashentry *), thisfunc);

    // Initialize array values
    for (t = 0; t < maxthreads; t++) {
      callstack[t] = (Timer **) allocate (MAX_STACK * sizeof (Timer *), thisfunc);
      hashtable[t] = (Hashentry *) allocate (tablesize * sizeof (Hashentry), thisfunc);
      for (i = 0; i < tablesize; i++) {
	hashtable[t][i].nument = 0;
	hashtable[t][i].entries = 0;
      }

      // Make a timer "GPTL_ROOT" to ensure no orphans, and to simplify printing
      timers[t] = (Timer *) allocate (sizeof (Timer), thisfunc);
      memset (timers[t], 0, sizeof (Timer));
      strcpy (timers[t]->name, "GPTL_ROOT");
      timers[t]->onflg = true;
      last[t] = timers[t];

      stackidx[t].val = 0;
      callstack[t][0] = timers[t];
      for (i = 1; i < MAX_STACK; i++)
	callstack[t][i] = 0;
    }

#ifdef HAVE_PAPI
    if (gptl_papi::PAPIinitialize (maxthreads, verbose, &nevents, eventlist) < 0)
      return gptl_util::error ("%s: Failure from PAPIinitialize\n", thisfunc);
#endif

    // Call init routine for underlying timing routine
    if ((*funclist[funcidx].funcinit)() < 0) {
      fprintf (stderr, "%s: Failure initializing %s. Reverting underlying timer to %s\n", 
	       thisfunc, funclist[funcidx].name, gptl_once::funclist[0].name);
      funcidx = 0;
    }

    gptl_private::ptr2wtimefunc = funclist[funcidx].func;

    if (gptl_once::verbose) {
      t1 = (*ptr2wtimefunc) ();
      t2 = (*ptr2wtimefunc) ();
      if (t1 > t2)
	fprintf (stderr, "%s: negative delta-t=%g\n", thisfunc, t2-t1);
      printf ("Per call overhead est. t2-t1=%g should be near zero\n", t2-t1);
      printf ("Underlying wallclock timing routine is %s\n", funclist[funcidx].name);
    }

    imperfect_nest = false;
    gptl_once::initialized = true;
    return 0;
  }

  /*
  ** GPTLfinalize (): Finalization routine must be called from single-threaded
  **   region. Free all malloc'd space
  **
  ** return value: 0 (success) or error (failure)
  */
  int GPTLfinalize (void)
  {
    using namespace gptl_once;  // violate global "using" rulehere
    using namespace gptl_private;  // violate global "using" rulehere
    int t;                // thread index
    int n;                // array index
    Timer *ptr, *ptrnext; // linked list indices
    static const char *thisfunc = "GPTLfinalize";

    if ( ! initialized)
      return gptl_util::error ("%s: initialization was not completed\n", thisfunc);

    for (t = 0; t < gptl_thread::maxthreads; ++t) {
      for (n = 0; n < tablesize; ++n) {
	if (hashtable[t][n].nument > 0)
	  free (hashtable[t][n].entries);
      }
      free (hashtable[t]);
      hashtable[t] = NULL;
      free (callstack[t]);
      for (ptr = timers[t]; ptr; ptr = ptrnext) {
	ptrnext = ptr->next;
	if (ptr->nparent > 0) {
	  free (ptr->parent);
	  free (ptr->parent_count);
	}
	if (ptr->nchildren > 0)
	  free (ptr->children);
	delete ptr;
      }
    }

    free (callstack);
    free (stackidx);
    free (timers);
    free (last);
    free (hashtable);

    gptl_thread::threadfinalize ();
    (void) GPTLreset_errors ();

#ifdef HAVE_PAPI
    gptl_papi::PAPIfinalize (maxthreads);
#endif

    // Reset initial values
    timers = 0;
    last = 0;
    gptl_thread::nthreads = -1;
#ifdef THREADED_PTHREADS
    gptl_thread::maxthreads = MAX_THREADS;
#else
    gptl_thread::maxthreads = -1;
#endif
    depthlimit = 99999;
    disabled = false;
    initialized = false;
    dousepapi = false;
    verbose = false;
    percent = false;
    dopr_preamble = true;
    dopr_threadsort = true;
    dopr_multparent = true;
    dopr_collision = false;
    ref_gettimeofday = -1;
    ref_clock_gettime = -1;
#ifdef _AIX
    ref_read_real_time = -1;
#endif
    funcidx = 0;
#ifdef HAVE_NANOTIME
    cpumhz= 0;
    cyc2sec = -1;
#endif
    tablesize = DEFAULT_TABLE_SIZE;
    tablesizem1 = tablesize - 1;
    return 0;
  }
}

// Functions local to this file
static float get_clockfreq ()
{
  FILE *fd = 0;
  char buf[LEN];
  int is;
  float freq = -1.;             // clock frequency (MHz)
  static const char *thisfunc = "get_clockfreq";
  static const char *max_freq_fn = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
  static const char *cpuinfo_fn = "/proc/cpuinfo";

  // First look for max_freq, but that isn't guaranteed to exist
  if ((fd = fopen (max_freq_fn, "r"))) {
    if (fgets (buf, LEN, fd)) {
      freq = 0.001 * (float) atof (buf);  // Convert from KHz to MHz
      if (gptl_once::verbose)
	printf ("GPTL: %s: Using max clock freq = %f for timing\n", thisfunc, freq);
    }
    gptl_once::clock_source = (char *) max_freq_fn;
    (void) fclose (fd);
    return freq;
  }
  
  // Next try /proc/cpuinfo. That has the disadvantage that it may give wrong info
  // for processors that have either idle or turbo mode
#ifdef HAVE_SLASHPROC
  if (gptl_once::verbose && freq < 0.)
    printf ("GPTL: %s: CAUTION: Can't find max clock freq. Trying %s instead\n",
	    thisfunc, cpuinfo_fn);
  
  if ( (fd = fopen (cpuinfo_fn, "r"))) {
    while (fgets (buf, LEN, fd)) {
      if (strncmp (buf, "cpu MHz", 7) == 0) {
	for (is = 7; buf[is] != '\0' && !isdigit (buf[is]); is++);
	if (isdigit (buf[is])) {
	  freq = (float) atof (&buf[is]);
	  if (gptl_once::verbose)
	    printf ("GPTL: %s: Using clock freq from /proc/cpuinfo = %f for timing\n",
		    thisfunc, freq);
	  gptl_once::clock_source = (char *) cpuinfo_fn;
	  break;
	}
      }
    }
    (void) fclose (fd);
  }
#endif
  return freq;
}

#ifdef HAVE_LIBMPI
static int init_mpiwtime () {return 0;}
#endif

// Probably need to link with -lrt for this one to work 
#ifdef HAVE_LIBRT
static int init_clock_gettime ()
{
  static const char *thisfunc = "init_clock_gettime";
  struct timespec tp;
  (void) clock_gettime (CLOCK_REALTIME, &tp);
  gptl_once::ref_clock_gettime = tp.tv_sec;
  if (gptl_once::verbose)
    printf ("GPTL: %s: ref_clock_gettime=%ld\n", thisfunc, (long) gptl_once::ref_clock_gettime);
  return 0;
}
#endif

// High-res timer on AIX: read_real_time
#ifdef _AIX
static int init_read_real_time ()
{
  static const char *thisfunc = "init_read_real_time";
  timebasestruct_t ibmtime;
  (void) read_real_time (&ibmtime, TIMEBASE_SZ);
  (void) time_base_to_time (&ibmtime, TIMEBASE_SZ);
  gptl_once::ref_read_real_time = ibmtime.tb_high;
  if (gptl_once::verbose)
    printf ("GPTL: %s: ref_read_real_time=%ld\n", thisfunc, (long) gptl_once::ref_read_real_time);
  return 0;
}
#endif

// Default available most places: gettimeofday
#ifdef HAVE_GETTIMEOFDAY
static int init_gettimeofday ()
{
  static const char *thisfunc = "init_gettimeofday";
  struct timeval tp;
  (void) gettimeofday (&tp, 0);
  gptl_once::ref_gettimeofday = tp.tv_sec;
  if (gptl_once::verbose)
    printf ("GPTL: %s: ref_gettimeofday=%ld\n", thisfunc, (long) gptl_once::ref_gettimeofday);
  return 0;
}
#endif

#ifdef HAVE_NANOTIME
    int init_nanotime ()
    {
      static const char *thisfunc = "init_nanotime";
      if ((gptl_once::cpumhz = get_clockfreq ()) < 0)
	return gptl_util::error ("%s: Can't get clock freq\n", thisfunc);

      if (gptl_once::verbose)
	printf ("GPTL: %s: Clock rate = %f MHz\n", thisfunc, gptl_once::cpumhz);

      gptl_once::cyc2sec = 1./(gptl_once::cpumhz * 1.e6);
      return 0;
    }
#endif
// placebo: does nothing and returns zero always. Useful for estimating overhead costs
static int init_placebo () {return 0;}

// set_abort_on_error: Set abort_on_error flag
static void set_abort_on_error (bool val) {gptl_util::abort_on_error = val;}
