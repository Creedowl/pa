#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if (free_ == NULL) return NULL;
  if (head == NULL) {
    head = free_;
    free_ = free_->next;
    head->next = NULL;
    return head;
  }
  WP *pre = head;
  while (pre->next != NULL) pre = pre->next;
  pre->next = free_;
  free_ = free_->next;
  pre->next->next = NULL;
  return pre->next;
}

bool free_wp(WP *wp) {
  if (head == wp) {
    head = wp->next;
    wp->next = free_;
    free_ = wp;
  } else {
    WP *pre = head;
    while (pre->next != wp) {
      pre = pre->next;
      if (pre == NULL) return false;
    }
    pre->next = wp->next;
    wp->next = free_;
    free_ = wp;
  }
  return true;
}

int set_watchpoint(char *e) {
  WP *wp = new_wp();
  if (wp == NULL) {
    printf("\033[31mError: wp_pool is full\033[0m\n");
    return -1;
  }
  bool success;
  uint32_t res;
  // expression evaluation
  res = expr(e, &success);
  if (!success) {
    printf("\033[31mError: invalid expression\033[0m\n");
    return -1;
  }
  wp->old_val = res;
  strcpy(wp->expr, e);
  wp->expr[strlen(e)] = '\0';
  printf("Set watchpoint #%d\n", wp->NO);
  printf("expr      = %s\n", e);
  printf("old value = 0x%08x\n", res);
  return wp->NO;
}

bool delete_watchpoint(int NO) {
  if (NO >= NR_WP || NO < 0) return false;
  return free_wp(&wp_pool[NO]);
}

void list_watchpoint() {
  if (head == NULL) {
    printf("No watchpoints\n");
    return;
  }
  printf("NO %-20s Old Value\n", "Expr");
  WP *wp = head;
  while (wp != NULL) {
    printf("%2d %-20s 0x%08x\n", wp->NO, wp->expr, wp->old_val);
    wp = wp->next;
  }
}
