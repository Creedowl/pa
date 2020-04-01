#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "cpu/reg.h"
#include "memory/memory.h"

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
    if (!wp->is_wp) vaddr_write(wp->new_val, 1, wp->old_val);
    pre->next = wp->next;
    wp->next = free_;
    free_ = wp;
  }
  return true;
}

int set_watchpoint(char *e) {
  bool success;
  uint32_t res;
  // expression evaluation
  res = expr(e, &success);
  if (!success) {
    printf("\033[31mError: invalid expression\033[0m\n");
    return -1;
  }
  WP *wp = new_wp();
  if (wp == NULL) {
    printf("\033[31mError: wp_pool is full\033[0m\n");
    return -1;
  }
  wp->old_val = res;
  strcpy(wp->expr, e);
  wp->expr[strlen(e)] = '\0';
  wp->is_wp = true;
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
    if (wp->is_wp)
      printf("%2d %-20s 0x%08x\n", wp->NO, wp->expr, wp->old_val);
    wp = wp->next;
  }
}

bool scan_watchpoint() {
  if (head == NULL) return false;
  WP *wp = head;
  bool success, pause = false;
  while (wp != NULL) {
    if (wp->is_wp) {
      // expression evaluation
      wp->new_val = expr(wp->expr, &success);
      if (!success) panic("bad expression");
      if (wp->new_val != wp->old_val) {
        pause = true;
        printf("\033[34mHit watchpoint %d at address 0x%08x\033[0m\n",
              wp->NO, cpu.eip);
        printf("expr      = %s\n", wp->expr);
        printf("old value = 0x%08x\n", wp->old_val);
        printf("new value = 0x%08x\n", wp->new_val);
        wp->old_val = wp->new_val;
      }
    }
    wp = wp->next;
  }
  if (pause) printf("program paused\n");
  return pause;
}

int set_breakpoint(char *e) {
  bool success;
  vaddr_t address;
  address = expr(e, &success);
  if (!success) {
    printf("\033[31mError: invalid expression\033[0m\n");
    return -1;
  }
  if (address < 0 || address >= 128 * 1024 * 1024) {
    printf("\033[31mError: address %08x is out of bound\033[0m\n", address);
    return -1;
  }
  WP *wp = new_wp();
  if (wp == NULL) {
    printf("\033[31mError: wp_pool is full\033[0m\n");
    return -1;
  }
  // address
  wp->new_val = address;
  strcpy(wp->expr, e);
  wp->expr[strlen(e)] = '\0';
  wp->is_wp = false;
  wp->old_val = vaddr_read(address, 1);
  vaddr_write(address, 1, 0xcc);
  printf("Set breakpoint #%d\n", wp->NO);
  printf("expr    = %s\n", e);
  printf("Address = 0x%08x\n", address);
  printf("Opcode  = %x\n", wp->old_val);
  return wp->NO;
}

void list_breakpoint() {
  if (head == NULL) {
    printf("No breakpoints\n");
    return;
  }
  printf("NO %-20s Address\n", "Expr");
  WP *wp = head;
  while (wp != NULL) {
    if (!wp->is_wp)
      printf("%2d %-20s 0x%08x\n", wp->NO, wp->expr, wp->new_val);
    wp = wp->next;
  }
}

bool trap_breakpoint(vaddr_t *eip) {
  if (head == NULL) panic("no breakpoints set\n");
  *eip -= 1;
  WP *wp = head;
  while (wp != NULL) {
    if (wp->is_wp) continue;
    if (wp->new_val == *eip) {
      printf("\033[34mHit breakpoint %d at address 0x%08x\033[0m\n",
        wp->NO, *eip);
      vaddr_write(*eip, 1, wp->old_val);
      break;
    }
    wp = wp->next;
  }
  if (wp == NULL) panic("no breakpoints set at 0x%08x\n", *eip);
  return true;
}
