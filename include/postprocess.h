#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "private.h"
#include <stdio.h>

// Everything in here is needed only by functions in postprocess.cc

typedef struct {
  int max_depth;
  int max_namelen;
  int max_chars2pr;
} Outputfmt;
    
namespace gptl_postprocess {
  extern GPTL_Method method;
  extern "C" {
    char *methodstr (GPTL_Method method);
  }
}
#endif
