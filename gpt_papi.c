#include <papi.h>

#if ( defined THREADED_OMP )
#include <omp.h>
#elif ( defined THREADED_PTHREADS )
#include <pthread.h>
#endif

#include "private.h"

typedef struct {
  int counter;
  char *prstr;
  char *str;
} Papientry;

/*
** Mapping of PAPI counters to short and long printed strings
*/

static Papientry papitable [] = {
  {PAPI_L1_DCM, "L1 Dcache miss  ", "Level 1 data cache misses"},
  {PAPI_L1_ICM, "L1 Icache miss  ", "Level 1 instruction cache misses"},
  {PAPI_L2_DCM, "L2 Dcache miss  ", "Level 2 data cache misses"},             
  {PAPI_L2_ICM, "L2 Icache miss  ", "Level 2 instruction cache misses"},
  {PAPI_L3_DCM, "L3 Dcache miss  ", "Level 3 data cache misses"},             
  {PAPI_L3_ICM, "L3 Icache miss  ", "Level 3 instruction cache misses"},       
  {PAPI_L1_TCM, "L1 cache miss   ", "Level 1 total cache misses"},             
  {PAPI_L2_TCM, "L2 cache miss   ", "Level 2 total cache misses"},
  {PAPI_L3_TCM, "L3 cache miss   ", "Level 3 total cache misses"},
  {PAPI_CA_SNP, "Snoops          ", "Snoops          "},
  {PAPI_CA_SHR, "PAPI_CA_SHR     ", "Request for shared cache line (SMP)"},
  {PAPI_CA_CLN, "PAPI_CA_CLN     ", "Request for clean cache line (SMP)"},
  {PAPI_CA_INV, "PAPI_CA_INV     ", "Request for cache line Invalidation (SMP)"},
  {PAPI_CA_ITV, "PAPI_CA_ITV     ", "Request for cache line Intervention (SMP)"},
  {PAPI_L3_LDM, "L3 load misses  ", "Level 3 load misses"},
  {PAPI_L3_STM, "L3 store misses ", "Level 3 store misses"},
  {PAPI_BRU_IDL,"PAPI_BRU_IDL    ", "Cycles branch units are idle"},
  {PAPI_FXU_IDL,"PAPI_FXU_IDL    ", "Cycles integer units are idle"},
  {PAPI_FPU_IDL,"PAPI_FPU_IDL    ", "Cycles floating point units are idle"},
  {PAPI_LSU_IDL,"PAPI_LSU_IDL    ", "Cycles load/store units are idle"},
  {PAPI_TLB_DM, "Data TLB misses ", "Data translation lookaside buffer misses"},
  {PAPI_TLB_IM, "Inst TLB misses ", "Instr translation lookaside buffer misses"},
  {PAPI_TLB_TL, "Tot TLB misses  ", "Total translation lookaside buffer misses"},
  {PAPI_L1_LDM, "L1 load misses  ", "Level 1 load misses"},
  {PAPI_L1_STM, "L1 store misses ", "Level 1 store misses"},
  {PAPI_L2_LDM, "L2 load misses  ", "Level 2 load misses"},
  {PAPI_L2_STM, "L2 store misses ", "Level 2 store misses"},
  {PAPI_BTAC_M, "BTAC miss       ", "BTAC miss"},
  {PAPI_PRF_DM, "PAPI_PRF_DM     ", "Prefetch data instruction caused a miss"},
  {PAPI_L3_DCH, "L3 DCache Hit   ", "Level 3 Data Cache Hit"},
  {PAPI_TLB_SD, "PAPI_TLB_SD     ", "Xlation lookaside buffer shootdowns (SMP)"},
  {PAPI_CSR_FAL,"PAPI_CSR_FAL    ", "Failed store conditional instructions"},
  {PAPI_CSR_SUC,"PAPI_CSR_SUC    ", "Successful store conditional instructions"},
  {PAPI_CSR_TOT,"PAPI_CSR_TOT    ", "Total store conditional instructions"},
  {PAPI_MEM_SCY,"Cyc Stalled Mem ", "Cycles Stalled Waiting for Memory Access"},
  {PAPI_MEM_RCY,"Cyc Stalled MemR", "Cycles Stalled Waiting for Memory Read"},
  {PAPI_MEM_WCY,"Cyc Stalled MemW", "Cycles Stalled Waiting for Memory Write"},
  {PAPI_STL_ICY,"Cyc no InstrIss ", "Cycles with No Instruction Issue"},
  {PAPI_FUL_ICY,"Cyc Max InstrIss", "Cycles with Maximum Instruction Issue"},
  {PAPI_STL_CCY,"Cyc No InstrComp", "Cycles with No Instruction Completion"},
  {PAPI_FUL_CCY,"Cyc Max InstComp", "Cycles with Maximum Instruction Completion"},
  {PAPI_HW_INT, "HW interrupts   ", "Hardware interrupts"},
  {PAPI_BR_UCN, "Uncond br instr ", "Unconditional branch instructions executed"},
  {PAPI_BR_CN,  "Cond br instr ex", "Conditional branch instructions executed"},
  {PAPI_BR_TKN, "Cond br instr tk", "Conditional branch instructions taken"},
  {PAPI_BR_NTK, "Cond br instrNtk", "Conditional branch instructions not taken"},
  {PAPI_BR_MSP, "Cond br instrMPR", "Conditional branch instructions mispred"},
  {PAPI_BR_PRC, "Cond br instrCPR", "Conditional branch instructions corr. pred"},
  {PAPI_FMA_INS,"FMA instr comp  ", "FMA instructions completed"},
  {PAPI_TOT_IIS,"Total instr iss ", "Total instructions issued"},
  {PAPI_TOT_INS,"Total instr ex  ", "Total instructions executed"},
  {PAPI_INT_INS,"Int instr ex    ", "Integer instructions executed"},
  {PAPI_FP_INS, "FP instr ex     ", "Floating point instructions executed"},
  {PAPI_LD_INS, "Load instr ex   ", "Load instructions executed"},
  {PAPI_SR_INS, "Store instr ex  ", "Store instructions executed"},
  {PAPI_BR_INS, "br instr ex     ", "Total branch instructions executed"},
  {PAPI_VEC_INS,"Vec/SIMD instrEx", "Vector/SIMD instructions executed"},
  {PAPI_RES_STL,"Cyc proc stalled", "Cycles processor is stalled on resource"},
  {PAPI_FP_STAL,"Cyc any FP stall", "Cycles any FP units are stalled"},
  {PAPI_TOT_CYC,"Total cycles    ", "Total cycles"},
  {PAPI_LST_INS,"Tot L/S inst ex ", "Total load/store inst. executed"},
  {PAPI_SYC_INS,"Sync. inst. ex  ", "Sync. inst. executed"},
  {PAPI_L1_DCH, "L1 D Cache Hit  ", "L1 D Cache Hit"},
  {PAPI_L2_DCH, "L2 D Cache Hit  ", "L2 D Cache Hit"},
  {PAPI_L1_DCA, "L1 D Cache Acc  ", "L1 D Cache Access"},
  {PAPI_L2_DCA, "L2 D Cache Acc  ", "L2 D Cache Access"},
  {PAPI_L3_DCA, "L3 D Cache Acc  ", "L3 D Cache Access"},
  {PAPI_L1_DCR, "L1 D Cache Read ", "L1 D Cache Read"},
  {PAPI_L2_DCR, "L2 D Cache Read ", "L2 D Cache Read"},
  {PAPI_L3_DCR, "L3 D Cache Read ", "L3 D Cache Read"},
  {PAPI_L1_DCW, "L1 D Cache Write", "L1 D Cache Write"},
  {PAPI_L2_DCW, "L2 D Cache Write", "L2 D Cache Write"},
  {PAPI_L3_DCW, "L3 D Cache Write", "L3 D Cache Write"},
  {PAPI_L1_ICH, "L1 I cache hits ", "L1 instruction cache hits"},
  {PAPI_L2_ICH, "L2 I cache hits ", "L2 instruction cache hits"},
  {PAPI_L3_ICH, "L3 I cache hits ", "L3 instruction cache hits"},
  {PAPI_L1_ICA, "L1 I cache acc  ", "L1 instruction cache accesses"},
  {PAPI_L2_ICA, "L2 I cache acc  ", "L2 instruction cache accesses"},
  {PAPI_L3_ICA, "L3 I cache acc  ", "L3 instruction cache accesses"},
  {PAPI_L1_ICR, "L1 I cache reads", "L1 instruction cache reads"},
  {PAPI_L2_ICR, "L2 I cache reads", "L2 instruction cache reads"},
  {PAPI_L3_ICR, "L3 I cache reads", "L3 instruction cache reads"},
  {PAPI_L1_ICW, "L1 I cache wr   ", "L1 instruction cache writes"},
  {PAPI_L2_ICW, "L2 I cache wr   ", "L2 instruction cache writes"},
  {PAPI_L3_ICW, "L3 I cache wr   ", "L3 instruction cache writes"},
  {PAPI_L1_TCH, "L1 cache hits   ", "L1 total cache hits"},
  {PAPI_L2_TCH, "L2 cache hits   ", "L2 total cache hits"},
  {PAPI_L3_TCH, "L3 cache hits   ", "L3 total cache hits"},
  {PAPI_L1_TCA, "L1 cache acc    ", "L1 total cache accesses"},
  {PAPI_L2_TCA, "L2 cache acc    ", "L2 total cache accesses"},
  {PAPI_L3_TCA, "L3 cache acc    ", "L3 total cache accesses"},
  {PAPI_L1_TCR, "L1 cache read   ", "L1 total cache reads"},
  {PAPI_L2_TCR, "L2 cache read   ", "L2 total cache reads"},
  {PAPI_L3_TCR, "L3 cache read   ", "L3 total cache reads"},
  {PAPI_L1_TCW, "L1 cache wr     ", "L1 total cache writes"},
  {PAPI_L2_TCW, "L2 cache wr     ", "L2 total cache writes"},
  {PAPI_L3_TCW, "L3 cache wr     ", "L3 total cache writes"},
  {PAPI_FML_INS,"FM ins          ", "FM ins"},
  {PAPI_FAD_INS,"FA ins          ", "FA ins"},
  {PAPI_FDV_INS,"FD ins          ", "FD ins"},
  {PAPI_FSQ_INS,"FSq ins         ", "FSq ins"},
  {PAPI_FNV_INS,"Finv ins        ", "Finv ins"},
  {PAPI_FP_OPS, "FP ops executed ", "Floating point operations executed"}};

static const int nentries = sizeof (papitable) / sizeof (Papientry);
static Papientry eventlist[MAX_AUX];    /* list of PAPI events to be counted */
static int nevents = 0;                 /* number of events: initialize to 0 */ 
static int *EventSet;                   /* list of events to be counted by PAPI */
static long_long **papicounters;        /* counters return from PAPI */
static bool *started;                   /* flag indicates EventSet has been started */
static char papiname[PAPI_MAX_STR_LEN]; /* returned from PAPI_event_code_to_name */
static const int BADCOUNT = -999999;    /* Set counters to this when they are bad */
static bool overhead = false;           /* ability to overhead computations */
static int overheadindx = -999999;      /* init to bad index value */
static long_long *lastoverhead;         /* needed because aux not available for overhead */

/*
** Function prototypes
*/

static int chkstarted (const int mythread);

int GPT_PAPIsetoption (const int counter,
		       const int val)
{
  int n;
  
  /* Just return if the flag says disable an option, because default is off */

  if ( ! val)
    return 0;

  /*
  ** Loop through table looking for counter. If found, ensure it can be
  ** enabled on this architecture.
  */

  for (n = 0; n < nentries; n++)
    if (counter == papitable[n].counter) {
      if (PAPI_query_event (counter) != PAPI_OK) {
	(void) PAPI_event_code_to_name (counter, papiname);
	printf ("GPT_PAPIsetoption: event %s not available on this arch\n", papiname);
      } else {
	if (nevents+1 > MAX_AUX) {
	  (void) PAPI_event_code_to_name (counter, papiname);
	  printf ("GPT_PAPIsetoption: Event %s is too many\n", papiname);
	} else {
	  if (counter == PAPI_TOT_CYC) {
	    overhead = true;
	    overheadindx = nevents;
	  }
	  eventlist[nevents].counter = counter;
	  eventlist[nevents].prstr   = papitable[n].prstr;
	  eventlist[nevents].str     = papitable[n].str;
	  printf ("GPT_PAPIsetoption: event %s enabled\n", eventlist[nevents].str);
	  ++nevents;
	}
      }
      return 0;
    }

  return GPTerror ("GPT_PAPIsetoption: counter %d does not exist\n", counter);
}

int GPT_PAPIinitialize (const int maxthreads)
{
  int ret;
  int n;

  if ((ret = PAPI_library_init (PAPI_VER_CURRENT)) != PAPI_VER_CURRENT)
    return GPTerror ("GPT_PAPIinitialize: PAPI_library_init failure:%s\n",
		     PAPI_strerror (ret));

#if ( defined THREADED_OMP )
  if (PAPI_thread_init ((unsigned long (*)(void)) (omp_get_thread_num)) != PAPI_OK)
    return GPTerror ("GPT_PAPIinitialize: PAPI_thread_init failure\n");
#elif ( defined THREADED_PTHREADS )
  if (PAPI_thread_init ((unsigned long (*)(void)) (pthread_self)) != PAPI_OK)
    return GPTerror ("GPT_PAPIinitialize: PAPI_thread_init failure\n");
#endif

  started      = (bool *)       GPTallocate (maxthreads * sizeof (bool));
  EventSet     = (int *)        GPTallocate (maxthreads * sizeof (int));
  papicounters = (long_long **) GPTallocate (maxthreads * sizeof (long_long *));
  lastoverhead = (long_long *)  GPTallocate (maxthreads * sizeof (long_long));

  for (n = 0; n < maxthreads; n++) {
    started[n] = false;
    EventSet[n] = PAPI_NULL;
    papicounters[n] = (long_long *) GPTallocate (MAX_AUX * sizeof (long_long));
  }

  return 0;
}

int GPT_PAPIstart (const int mythread, 
		   Papistats *aux)
{
  int ret;
  int n;
  
  /* If no events are to be counted just return */

  if (nevents == 0)
    return 0;

  if (chkstarted (mythread) < 0)
    return -1;

  if ((ret = PAPI_read (EventSet[mythread], papicounters[mythread])) != PAPI_OK)
    return GPTerror ("GPT_PAPIstart: %s\n", PAPI_strerror (ret));

  for (n = 0; n < nevents; n++)
    aux->last[n] = papicounters[mythread][n];
  
  return 0;
}

static int chkstarted (const int mythread)
{
  int ret;
  int n;

  if ( ! started[mythread]) {
    if ((ret = PAPI_create_eventset (&EventSet[mythread])) != PAPI_OK)
      return GPTerror ("GPT_PAPIstart: failure creating eventset: %s\n", 
		       PAPI_strerror (ret));

    for (n = 0; n < nevents; n++) {
      if ((ret = PAPI_add_event (EventSet[mythread], eventlist[n].counter)) != PAPI_OK) {
	printf ("%s\n", PAPI_strerror (ret));
	return GPTerror ("GPT_PAPIstart: failure adding event: %s\n", 
			 eventlist[n].str);
      }
    }
    if ((ret = PAPI_start (EventSet[mythread])) != PAPI_OK)
      printf ("%s\n", PAPI_strerror (ret));

    started[mythread] = true;
  }
  return 0;
}

int GPT_PAPIstop (const int mythread, 
		  Papistats *aux)
{
  int ret;
  int n;
  long_long delta;

  /* If no events are to be counted just return */

  if (nevents == 0)
    return 0;

  if ((ret = PAPI_read (EventSet[mythread], papicounters[mythread])) != PAPI_OK)
    return GPTerror ("GPT_PAPIstop: %s\n", PAPI_strerror (ret));
  
  for (n = 0; n < nevents; n++) {
    delta = papicounters[mythread][n] - aux->last[n];
    if (delta < 0)
      aux->accum[n] = BADCOUNT;
    else if (aux->accum[n] != BADCOUNT)
      aux->accum[n] += delta;
  }
  return 0;
}

/* Have to set the static variable lastoverhead because a pointer to the correct
** aux timer is not yet available
*/

int GPT_PAPIoverheadstart (const int mythread)
{
  int ret;

  if (overhead) {
    if (chkstarted (mythread) < 0)
      return -1;

    if ((ret = PAPI_read (EventSet[mythread], papicounters[mythread])) != PAPI_OK)
      return GPTerror ("GPT_PAPIoverheadstart: %s\n", PAPI_strerror (ret));

    lastoverhead[mythread] = papicounters[mythread][overheadindx];
  }
  return 0;
}
    
int GPT_PAPIoverheadstop (const int mythread,
			  Papistats *aux)
{
  int ret;
  long_long diff;

  if (overhead) {
    if ((ret = PAPI_read (EventSet[mythread], papicounters[mythread])) != PAPI_OK)
      return GPTerror ("GPT_PAPIoverheadstart: %s\n", PAPI_strerror (ret));
    diff = papicounters[mythread][overheadindx] - lastoverhead[mythread];
    if (diff < 0)
      aux->accum_cycles = BADCOUNT;
    else
      aux->accum_cycles += diff;
  }
  return 0;
}

void GPT_PAPIprstr (FILE *fp)
{
  int n;
  
  for (n = 0; n < nevents; n++)
    fprintf (fp, "%16s ", eventlist[n].prstr);

  if (overhead)
    fprintf (fp, "Overhead (cycles)");
}

void GPT_PAPIpr (FILE *fp,
		const Papistats *aux)
{
  int n;
  
  for (n = 0; n < nevents; n++) {
    if (aux->accum[n] < 1000000)
      fprintf (fp, "%16ld ", (long) aux->accum[n]);
    else
      fprintf (fp, "%16.10e ", (double) aux->accum[n]);
  }
  if (overhead)
    if (aux->accum_cycles < 1000000)
      fprintf (fp, "%16ld ", (long) aux->accum_cycles);
    else
      fprintf (fp, "%16.10e ", (double) aux->accum_cycles);
}

void GPTPAPIprinttable ()
{
  int n;

  for (n = 0; n < nentries; n++)
    if (PAPI_query_event (papitable[n].counter) == PAPI_OK)
      printf ("%d %s\n", papitable[n].counter, papitable[n].str);
}

void GPT_PAPIadd (Papistats *auxout,
		  const Papistats *auxin)
{
  int n;
  
  for (n = 0; n < nevents; n++)
    if (auxin->accum[n] == BADCOUNT || auxout->accum[n] == BADCOUNT)
      auxout->accum[n] = BADCOUNT;
    else
      auxout->accum[n] += auxin->accum[n];

  if (overhead)
    if (auxin->accum_cycles == BADCOUNT || auxout->accum_cycles == BADCOUNT)
      auxout->accum_cycles += auxin->accum_cycles;
    else
      auxout->accum_cycles += auxin->accum_cycles;
}
