#ifndef __TYPES_H__
#define __TYPES_H__

#include <unistd.h>
#include <sys/types.h>

typedef unsigned long uintptr_t

struct _RegSet {
  uintptr_t esi, ebx, eax, eip, edx, error_code, eflags, ecx, cs, esp, edi, ebp;
  int       irq;
};

#endif
