// Convenience routines moved from gptl.cpp to here so that gptl.cpp can be understood
// by nvcc as host-only code.

#include "config.h" /* Must be first include. */

#include <cuda.h>
#include <helper_cuda.h>
#include "../include/private.h"
#include "../../include/devicehost.h"
#include "../include/gptl_cuda.h"

extern "C" {
  
void GPTLfinalize_gpu_host ()
{
  GPTLfinalize_gpu<<<1,1>>>();
}

void GPTLreset_gpu_host ()
{
  GPTLreset_gpu<<<1,1>>>();
}

// Return useful GPU properties. Use arg list for SMcount, cores_per_sm, and cores_per_gpu even 
// though they're globals, because this is a user-callable routine
int GPTLget_gpu_props (int *khz, int *warpsize, int *devnum, int *SMcount,
		       int *cores_per_sm, int *cores_per_gpu)
{
  cudaDeviceProp prop;
  size_t size;
  cudaError_t err;
  static const size_t onemb = 1024 * 1024;
  static const char *thisfunc = "GPTLget_gpu_props";

  if ((err = cudaGetDeviceProperties (&prop, 0)) != cudaSuccess) {
    printf ("%s: error:%s", thisfunc, cudaGetErrorString (err));
    return -1;
  }

  *khz           = prop.clockRate;
  *warpsize      = prop.warpSize;
  *SMcount       = prop.multiProcessorCount;
#ifdef HAVE_HELPER_CUDA_H
  *cores_per_sm  = _ConvertSMVer2Cores (prop.major, prop.minor);
  *cores_per_gpu = *cores_per_sm * (*SMcount);
#else
  *cores_per_sm  = -1;
  *cores_per_gpu = -1;
#endif  
  printf ("%s: major.minor=%d.%d\n", thisfunc, prop.major, prop.minor);
  printf ("%s: SM count=%d\n",      thisfunc, *SMcount);
  printf ("%s: cores per sm=%d\n",  thisfunc, *cores_per_sm);
  printf ("%s: cores per GPU=%d\n", thisfunc, *cores_per_gpu);

  err = cudaGetDevice (devnum);  // device number
  err = cudaDeviceGetLimit (&size, cudaLimitMallocHeapSize);
  printf ("%s: default cudaLimitMallocHeapSize=%d MB\n", thisfunc, (int) (size / onemb));
  return 0;
}

int GPTLcudadevsync (void)
{
  cudaDeviceSynchronize ();
  return 0;
}
}
