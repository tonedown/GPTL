#include "config.h" /* Must be first include. */
#include "once.h"
#include "thread.h"
#include "private.h"
#include "autoinst.h"
#include "getoverhead.h"

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>  // for free()

namespace gptl_overhead {
  // prototypes for functions in anonymous namespace
  namespace {
    using namespace gptl_private;
    int start_sim (char *, int);
    void misc_sim (int);
  }

  /*
  ** get_overhead: return current status info about a timer. If certain stats are not enabled, 
  ** they should just have zeros in them. If PAPI is not enabled, input counter info is ignored.
  ** 
  ** Input args:
  **   fp:          File descriptor to write to
  **
  ** Output args:
  **   self_ohd:    Estimate of GPTL-induced overhead in the timer itself (included in "Wallclock")
  **   parent_ohd:  Estimate of GPTL-induced overhead for the timer which appears in its parents
  */
  int get_overhead (FILE *fp, double *self_ohd, double *parent_ohd)
  {
    using namespace gptl_private;
    using namespace gptl_thread;
    using namespace gptl_autoinst;
    double t1, t2;             // Initial, final timer values
    double ftn_ohd;            // Fortran-callable layer
    double get_thread_num_ohd; // Getting my thread index
    double genhashidx_ohd;     // Generating hash index
    double getentry_ohd;       // Finding entry in hash table
    double utr_ohd;            // Underlying timing routine
    double papi_ohd;           // Reading PAPI counters
    double total_ohd;          // Sum of overheads
    double getentry_instr_ohd; // Finding entry in hash table for auto-instrumented calls
    double addr2name_ohd;      // libunwind or backtrace (called from __cyg_profile_func_enter)
    double misc_ohd;           // misc. calcs within start/stop
    int i, n;
    int ret;
    int mythread;              // which thread are we
    unsigned int hashidx;      // Hash index
    int randomvar;             // placeholder for taking the address of a variable
    Timer *entry;              // placeholder for return from "getentry()"
    static const char *thisfunc = "GPTLget_overhead";

    // Gather timings by running kernels 1000 times each. First: Fortran wrapper overhead
    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      // 9 is the number of characters in "timername"
      ret = start_sim ("timername", 9);
    }
    t2 = (ptr2wtimefunc)();
    ftn_ohd = 0.001 * (t2 - t1);

    // get_thread_num() overhead
    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      mythread = get_thread_num ();
    }
    t2 = (*ptr2wtimefunc)();
    get_thread_num_ohd = 0.001 * (t2 - t1);

    // genhashidx overhead
    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      hashidx = genhashidx ("timername");
    }
    t2 = (*ptr2wtimefunc)();
    genhashidx_ohd = 0.001 * (t2 - t1);

    // getentry overhead
    // Find the first hashtable entry with a valid name. Start at 1 because 0 is not a valid hash
    for (n = 1; n < tablesize; ++n) {
      if (hashtable[0][n].nument > 0 && strlen (hashtable[0][n].entries[0]->name) > 0) {
	hashidx = genhashidx (hashtable[0][n].entries[0]->name);
	t1 = (*ptr2wtimefunc)();
	for (i = 0; i < 1000; ++i)
	  entry = getentry (hashtable[0], hashtable[0][n].entries[0]->name, hashidx);
	t2 = (*ptr2wtimefunc)();
	fprintf (fp, "%s: using hash entry %d=%s for getentry estimate\n", 
		 thisfunc, n, hashtable[0][n].entries[0]->name);
	break;
      }
    }
    if (n == tablesize) {
      fprintf (fp, "%s: hash table empty: Using alternate means to find getentry time\n", thisfunc);
      t1 = (*ptr2wtimefunc)();
      for (i = 0; i < 1000; ++i)
	entry = getentry (hashtable[0], "timername", hashidx);
      t2 = (*ptr2wtimefunc)();
    }
    getentry_ohd = 0.001 * (t2 - t1);

    // utr overhead
    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      t2 = (*ptr2wtimefunc)();
    }
    utr_ohd = 0.001 * (t2 - t1);

    // PAPI overhead
#ifdef HAVE_PAPI
    if (gptl_private::dousepapi) {
      t1 = (*ptr2wtimefunc)();
      read_counters1000 ();
      t2 = (*ptr2wtimefunc)();
    } else {
      t1 = 0.;
      t2 = 0.;
    }
    papi_ohd = 0.001 * (t2 - t1);
#else
    papi_ohd = 0.;
#endif

#ifdef HAVE_LIBUNWIND
    // libunwind overhead
    unw_cursor_t cursor;
    unw_context_t context;
    unw_word_t offset, pc;
    char symbol[MAX_SYMBOL_NAME+1];

    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      // Initialize cursor to current frame for local unwinding.
      unw_getcontext (&context);
      unw_init_local (&cursor, &context);

      (void) unw_step (&cursor);
      unw_get_reg (&cursor, UNW_REG_IP, &pc);
      (void) unw_get_proc_name (&cursor, symbol, sizeof(symbol), &offset);
    }
    t2 = (*ptr2wtimefunc)();
    addr2name_ohd = 0.001 * (t2 - t1);
#endif

#ifdef HAVE_BACKTRACE
    void *buffer[2];
    int nptrs;
    char **strings;

    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      nptrs = backtrace (buffer, 2);
      strings = backtrace_symbols (buffer, nptrs);
      free (strings);
    }
    t2 = (*ptr2wtimefunc)();
    addr2name_ohd = 0.001 * (t2 - t1);
#endif

    // getentry_instr overhead
    t1 = (*ptr2wtimefunc)();
    for (i = 0; i < 1000; ++i) {
      entry = getentry_instr (hashtable[0], &randomvar, &hashidx);
    }
    t2 = (*ptr2wtimefunc)();
    getentry_instr_ohd = 0.001 * (t2 - t1);

    // misc start/stop overhead
    if (imperfect_nest) {
      fprintf (fp, "Imperfect nesting detected: setting misc_ohd=0\n");
      misc_ohd = 0.;
    } else {
      t1 = (*ptr2wtimefunc)();
      for (i = 0; i < 1000; ++i) {
	misc_sim (0);
      }
      t2 = (*ptr2wtimefunc)();
      misc_ohd = 0.001 * (t2 - t1);
    }

    total_ohd = ftn_ohd + get_thread_num_ohd + genhashidx_ohd + getentry_ohd + 
                utr_ohd + misc_ohd + papi_ohd;
    fprintf (fp, "Total overhead of 1 GPTL start or GPTLstop call=%g seconds\n", total_ohd);
    fprintf (fp, "Components are as follows:\n");
    fprintf (fp, "Fortran layer:             %7.1e = %5.1f%% of total\n", 
	     ftn_ohd, ftn_ohd / total_ohd * 100.);
    fprintf (fp, "Get thread number:         %7.1e = %5.1f%% of total\n", 
	     get_thread_num_ohd, get_thread_num_ohd / total_ohd * 100.);
    fprintf (fp, "Generate hash index:       %7.1e = %5.1f%% of total\n", 
	     genhashidx_ohd, genhashidx_ohd / total_ohd * 100.);
    fprintf (fp, "Find hashtable entry:      %7.1e = %5.1f%% of total\n", 
	     getentry_ohd, getentry_ohd / total_ohd * 100.);
    fprintf (fp, "Underlying timing routine: %7.1e = %5.1f%% of total\n", 
	     utr_ohd, utr_ohd / total_ohd * 100.);
    fprintf (fp, "Misc start/stop functions: %7.1e = %5.1f%% of total\n", 
	     misc_ohd, misc_ohd / total_ohd * 100.);
#ifdef HAVE_PAPI
    if (dousepapi) {
      fprintf (fp, "Read PAPI counters:        %7.1e = %5.1f%% of total\n", 
	       papi_ohd, papi_ohd / total_ohd * 100.);
    }
#endif
    fprintf (fp, "\n");
#ifdef HAVE_LIBUNWIND
    fprintf (fp, "Overhead of libunwind (invoked once per auto-instrumented start entry)=%g seconds\n", addr2name_ohd);
#elif defined HAVE_BACKTRACE
    fprintf (fp, "Overhead of backtrace (invoked once per auto-instrumented start entry)=%g seconds\n", addr2name_ohd);
#endif
    fprintf (fp, "NOTE: If GPTL is called from C not Fortran, the 'Fortran layer' overhead is zero\n");
    fprintf (fp, "NOTE: For calls to GPTLstart_handle()/GPTLstop_handle(), the 'Generate hash index' overhead is zero\n");
    fprintf (fp, "NOTE: For auto-instrumented calls, the cost of generating the hash index plus finding\n"
	     "      the hashtable entry is %7.1e not the %7.1e portion taken by GPTLstart\n", 
	     getentry_instr_ohd, genhashidx_ohd + getentry_ohd);
    fprintf (fp, "NOTE: Each hash collision roughly doubles the 'Find hashtable entry' cost of that timer\n");
    *self_ohd   = ftn_ohd + utr_ohd; /* In GPTLstop() ftn wrapper is called before utr */
    *parent_ohd = ftn_ohd + utr_ohd + misc_ohd +
                  2.*(get_thread_num_ohd + genhashidx_ohd + getentry_ohd + papi_ohd);
    return 0;
  }

  // Use anonymous namespace for functions private to the outer namespace
  namespace {
    // start_sim: Simulate the cost of Fortran wrapper layer "gptlstart()"
    int start_sim (char *name, int nc)
    {
      char cname[nc+1];

      strncpy (cname, name, nc);
      cname[nc] = '\0';
      return 0;
    }

    // misc_sim: Simulate the cost of miscellaneous computations in start/stop
    void misc_sim (int t)
    {
      using namespace gptl_once;
      using namespace gptl_private;
      int bidx;
      Timer *bptr;
      static Timer *ptr = 0;
      static const char *thisfunc = "misc_sim";
      
      if (gptl_once::disabled)
	printf ("GPTL: %s: should never print disabled\n", thisfunc);

      if (! gptl_once::initialized)
	printf ("GPTL: %s: should never print ! initialized\n", thisfunc);

      bidx = gptl_private::stackidx[t].val;
      bptr = gptl_private::callstack[t][bidx];
      if (ptr == bptr)
	printf ("GPTL: %s: should never print ptr=bptr\n", thisfunc);

      --stackidx[t].val;
      if (gptl_private::stackidx[t].val < -2)
	printf ("GPTL: %s: should never print stackidxt < -2\n", thisfunc);
      
      if (++gptl_private::stackidx[t].val > MAX_STACK-1)
	printf ("GPTL: %s: should never print stackidxt > MAX_STACK-1\n", thisfunc);
      
      return;
    }
  }
}
