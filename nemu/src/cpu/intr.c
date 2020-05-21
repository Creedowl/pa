#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  rtl_push(&cpu.EFLAGS);
  t0 = cpu.cs;
  rtl_push(&t0);
  rtl_push(&ret_addr);
  uint32_t addr = cpu.idtr.base;
  uint32_t offset_low = vaddr_read(addr + NO * 8, 2);
  uint32_t offset_high = vaddr_read(addr + NO * 8 + 6, 2);
  decoding.jmp_eip = (offset_high << 16) | offset_low;
  Log("%x", decoding.jmp_eip);
  decoding.is_jmp = true;
}

void dev_raise_intr() {
}
