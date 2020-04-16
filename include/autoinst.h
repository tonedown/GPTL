#ifndef AUTOINST_H
#define AUTOINST_H

#include "private.h"

namespace gptl_autoinst {
  extern "C" {
    inline gptl_private::Timer *getentry_instr (const int, void *, unsigned int *);
  }
};
#endif
