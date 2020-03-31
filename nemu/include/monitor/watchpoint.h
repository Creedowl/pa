#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[64];
  uint32_t old_val;
  uint32_t new_val;

} WP;

int set_watchpoint(char *e);

#endif
