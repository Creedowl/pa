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

void free_wp(WP *wp) {
  if (head == wp) {
    head->next = free_;
    free_ = head;
    head = NULL;
    return;
  }
  WP *pre = head;
  while (pre->next != wp) pre = pre->next;
  pre->next = wp->next;
  wp->next = free_;
  free_ = wp;
}
