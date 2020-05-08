#include "trap.h"

int main() {
  int a = 0x80000000;
  nemu_assert(a >> 20 == 0xfffff800);
  uint32_t b = 0x80000000;
  nemu_assert(b >> 20 == 0x00000800);
  return 0;
}
