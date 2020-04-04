#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  // wp may be a watchpoint or breakpoint
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

void list_breakpoint();

bool trap_breakpoint(vaddr_t *eip);

#endif
