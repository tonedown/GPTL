// Convenience routines moved from gptl.cpp to here so that gptl.cpp can be understood
// by nvcc as host-only code.

__host__ GPTLfinalize_gpu_host (void);
__host__ GPTLreset_gpu_host (void);

void GPTLfinalize_gpu_host ()
{
  GPTLfinalize_gpu<<<1,1>>>();
}
void GPTLreset_gpu_host ()
{
  GPTLreset_gpu<<<1,1>>>();
}
