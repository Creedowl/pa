#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <setjmp.h>

static jmp_buf env;

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_REG,
  TK_HEX,
  TK_DEC,
  TK_NEG,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_NOT,
  TK_DEREF
};

static struct {
  int type;
  uint8_t pri;
} ops [] = {
  { TK_OR, 0 },
  { TK_AND, 1 },
  { TK_NEQ, 2 },
  { TK_EQ, 2 },
  { '+', 3 },
  { '-', 3 },
  { '*', 4 },
  { '/', 4 },
  { TK_NOT, 5 },
  { TK_NEG, 5 },
  { TK_DEREF, 5 },
  { 0xff, 0xff }
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"^\\$e(ax|cx|dx|bx|sp|bp|si|di|ip)", TK_REG},
  {"^0(x|X)[0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_DEC},  // posix regex doesn't support "\d"
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"!", TK_NOT},
  {"\\*", TK_DEREF}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_REG:
          case TK_HEX:
          case TK_DEC:
            if(substr_len > 31) {
              printf("token str too long\n");
              return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
          default: 
            tokens[nr_token++].type = rules[i].token_type;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  nr_token--;
  return true;
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') return false;
  int count = 0;
  bool valid = true;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') count++;
    if (tokens[i].type == ')') {
      count--;
      if (count == 0 && i != q) valid = false;
      if (count < 0) 
        // Bracket mismatch
        longjmp(env, 1);
    }
  }
  if (count != 0) longjmp(env, 1);  
  return valid;
}

bool check_operator(int type) {
  if (type != TK_HEX && type != TK_DEC && type != TK_REG && type != '('&& type != ')')
    return true;
  return false;
}

bool compare_operator(int op1, int op2) {
  int w1 = -1, w2 = -1;
  for (int i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
    if (op1 == ops[i].type) w1 = ops[i].pri;
    if (op2 == ops[i].type) w2 = ops[i].pri;
    if (w1 != -1 && w2 != -1) break;
  }
  return w1 >= w2;
}

int find_dominated_op(int p, int q) {
  int op = -1, count = 0, pre = 0xff;
  for (int i=p; i<=q; i++) {
    if (tokens[i].type == '(') {
      count++;
      while (1) {
        i++;
        if (i > nr_token) longjmp(env, 2);
        if (tokens[i].type == '(') count++;
        if (tokens[i].type == ')') {
          count--;
          if (count == 0) break;
        }
      }
      continue;
    }
    if (!check_operator(tokens[i].type)) continue;
    if (compare_operator(pre, tokens[i].type)) {
      pre = tokens[i].type;
      op = i;
    }
  }
  return op;
}

uint32_t eval(int p, int q) {
    if (p > q) {
      /* Bad expression */
      longjmp(env, 2);
    }
    else if (p == q) {
      /* Single token.
      * For now this token should be a number.
      * Return the value of the number.
      */
      uint32_t val;
      char *str = tokens[p].str;
      switch (tokens[p].type) {
        case TK_DEC:
          sscanf(str, "%d", &val);
          return val;
        case TK_HEX:
          sscanf(str, "%x", &val);
          return val;
        case TK_REG:
          if (strcmp(str + 1, "eax") == 0)
            return cpu.eax;
          else if (strcmp(str + 1, "ecx") == 0)
            return cpu.ecx;
          else if (strcmp(str + 1, "edx") == 0)
            return cpu.edx;
          else if (strcmp(str + 1, "ebx") == 0)
            return cpu.ebx;
          else if (strcmp(str + 1, "esp") == 0)
            return cpu.esp;
          else if (strcmp(str + 1, "ebp") == 0)
            return cpu.ebp;
          else if (strcmp(str + 1, "esi") == 0)
            return cpu.esi;
          else if (strcmp(str + 1, "edi") == 0)
            return cpu.edi;
          else if (strcmp(str + 1, "eip") == 0)
            return cpu.eip;
        
        default:
          break;
      }
    }
    else if (check_parentheses(p, q) == true) {
      /* The expression is surrounded by a matched pair of parentheses.
      * If that is the case, just throw away the parentheses.
      */
      return eval(p + 1, q - 1);
    }
    else {
      /* We should do more things here. */
      int op = find_dominated_op(p, q);
      int val2 = eval(op + 1, q);
      if (op == p) {
        switch (tokens[op].type) {
        case TK_NOT:
          return !val2;
        case TK_NEG:
          return -val2;
        case TK_DEREF:
          if  (val2 > 128 * 1024 * 1024) longjmp(env, 3);
          return vaddr_read(val2, 4);
        
        default:
          break;
        }
      }
      int val1 = eval(p, op - 1);
      switch (tokens[op].type) {
        case '+':
          return val1 + val2;
        case '-':
          return val1 - val2;
        case '*':
          return val1 * val2;
        case '/':
          return val1 / val2;
        case TK_EQ:
          return val1 == val2;
        case TK_NEQ:
          return val1 != val2;
        case TK_AND:
          return val1 && val2;
        case TK_OR:
          return val1 || val2;
        default:
          break;
      }
    }
    return 0;
}

bool check_pre(int p) {
  int type = tokens[p-1].type;
  if (type != TK_HEX && type != TK_DEC && type != ')') return true;
  return false;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  int bracket_count = 0;
  for (int i=0; i<=nr_token; i++) {
    switch (tokens[i].type) {
      case '*':
        if (i==0 || check_pre(i)) tokens[i].type = TK_DEREF;
        break;
      case '-':
        if (i==0 || check_pre(i)) tokens[i].type = TK_NEG;
        break;
      case '(':
        bracket_count++;
        break;
      case ')':
        bracket_count--;
        break;
      
      default:
        break;
    }
  }

  if (bracket_count != 0) {
    printf("Bracket mismatch\n");
    *success = false;
    return 0;
  }

  switch (setjmp(env)) {
    case 0:
      *success = true;
      return eval(0, nr_token);
    case 1:
      printf("Bracket mismatch\n");
      *success = false;
      break;
    case 2:
      printf("Bad expression\n");
      *success = false;
      break;
    case 3:
      printf("physical address is out of bound\n");
      *success = false;
      break;
    
    default:
      break;
  }
  return 0;
}
