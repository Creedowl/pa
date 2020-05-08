#include "trap.h"

int main() {
  int a = 0x80000000;
  nemu_assert(a >> 20 == 0xfffff800);
  return 0;
}
