#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

// single step
static int cmd_si(char *args) {
  int n = 1;
  if (args) {
    // if n in args isn't a number, n = 1
    sscanf(args, "%d", &n);
    // n must greater than 0
    n = n >= -1 ? n : 1;
  }
  cpu_exec(n);
  return 0;
}

// show infomation of registers or watchpoints
static int cmd_info(char *args){
  char *arg = strtok(NULL, " ");
  // no arg or arg isn't a char
  if (arg == NULL || strlen(arg)>1) {
    printf("Usage: info r|w\n");
    return 1;
  }
  if(*arg == 'r') {
    printf("%-4s\t%-10s\t%s\n", "REG", "HEX", "DEC");
    for (int i = R_EAX; i <= R_EDI; i++) {
      printf("%s:\t0x%08x\t%d\n", regsl[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
    }
  }else if(*arg == 'w') {
    /* TODO */
  }else {
    printf("Usage: info r|w\n");
    return 1;
  }
  return 0;
}

// scan memory
static int cmd_x(char *args) {
  char *_count = strtok(NULL, " ");
  char *exp = args + strlen(_count) + 1;
  // count and expression are both required
  if(_count == NULL) {
    printf("Usage: x [count] [expression]\n");
    return 1;
  }
  int count = 0;
  paddr_t address = 0x0;
  if (!sscanf(_count, "%d", &count)) {
    printf("Usage: x [count] [expression]\n");
    return 1;
  }
  bool success;
  // evaluate
  address = expr(exp, &success);
  if (!success) {
    printf("\033[31mError: execute expression failed\033[0m\n");
    return 1;
  }
  if (address < 0 || address >= 128 * 1024 * 1024) {
    printf("physical address %08x is out of bound\n", address);
    return 1;
  }
  printf("%-10s\t%-10s\t%s\n", "Address", "Dword", "Byte");
  for (int i = 0; i < count; i++) {
    printf("0x%08x\t", address + i * 4);
    // read 4 bytes memory, the value is little endian
    printf("0x%08x\t", vaddr_read(address + i * 4, 4));
    // read 1 byte four times
    for (int j = 0; j < 4; j++) {
      printf("%02x ", vaddr_read(address + i * 4 + j, 1));
    }
    printf("\n");
  }
  return 0;
}

// print value of expression
static int cmd_p(char *args) {
  // cmd p must have an expression
  if (args == NULL) {
    printf("Usage: p [EXP]\n");
    return 1;
  }
  bool success;
  uint32_t res;
  // expression evaluation
  res = expr(args, &success);
  if (!success) {
    printf("\033[31mError: execute expression failed\033[0m\n");
    return 1;
  }
  printf("DEC: %d\nHEX: 0x%x\n", res, res);
  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w [EXP]\n");
    return 1;
  }
  int no = set_watchpoint(args);
  return no == -1 ? 1 : 0;
}

static int cmd_d(char *args) {
  int NO = 0;
  if (args == NULL || !sscanf(args, "%d", &NO)) {
    printf("Usage: w [EXP]\n");
    return 1;
  }
  if (!delete_watchpoint(NO)) {
    printf("No such watchpoint\n");
    return 1;
  }
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Run single step, si [steps]", cmd_si },
  { "info", "Show information of registers or watchpoints, info r|w", cmd_info },
  { "x", "Scan memory, x [count] [expression]", cmd_x },
  { "p", "Print value of expression, p EXP", cmd_p },
  { "w", "Set a watchpoint for an expression, w EXP", cmd_w },
  { "d", "Delete a watchpoint, w NO", cmd_d }
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
