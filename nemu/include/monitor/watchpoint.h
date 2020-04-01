#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  bool is_wp;
  char expr[64];
  uint32_t old_val;
  uint32_t new_val;

} WP;

int set_watchpoint(char *e);

bool delete_watchpoint(int NO);

void list_watchpoint();

bool scan_watchpoint();

int set_breakpoint(char *e);

bool trap_breakpoint(vaddr_t *eip);

#endif
