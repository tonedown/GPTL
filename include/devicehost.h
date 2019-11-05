/*
** $Id$
**
** Author: Jim Rosinski
**
** Contains definitions shared between CPU and GPU
*/

/* longest timer name allowed (probably safe to just change) */
#ifndef GPTL_DH
#define GPTL_DH

#define SUCCESS 0
#define FAILURE -1
#define MAX_CHARS 127

#ifdef HAVE_CUDA
// Warpsize will be verified by the library
#define WARPSIZE 32
// Pascal: 56 SMs 64 cuda cores each = 3584 cores
#define DEFAULT_MAXWARPS_GPU 1792
#define DEFAULT_MAXTIMERS_GPU 30
#ifdef __cplusplus
extern "C" {
#endif
int GPTLinitialize_gpu (const int, const int, const int, const double);
void GPTLfinalize_gpu_host (void);
void GPTLreset_gpu_host (void);
#ifdef __cplusplus
}
#endif
#endif
#endif
