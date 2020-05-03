#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(push) {
  if (id_dest->type == OP_TYPE_REG) {
    rtl_lr(&t0, id_dest->reg, 4);
  } else if (id_dest->type == OP_TYPE_MEM) {
    t0 = id_dest->addr;
    rtl_sext(&t0, &t0, id_dest->width);
  } else {
    t0 = id_dest->imm;
  }
    
  rtl_push(&t0);

  print_asm_template1(push);
}

make_EHelper(pop) {
  rtl_pop(&t0);
  operand_write(id_dest, &t0);

  print_asm_template1(pop);
}

make_EHelper(pusha) {
  TODO();

  print_asm("pusha");
}

make_EHelper(popa) {
  TODO();

  print_asm("popa");
}

make_EHelper(leave) {
  rtl_lr(&t0, R_EBP, 4);
  rtl_sr(R_ESP, 4, &t0);
  rtl_pop(&t0);
  if (decoding.is_operand_size_16)
    rtl_sr(R_BP, 2, &t0);
  else
    rtl_sr(R_EBP, 4, &t0);

  print_asm("leave");
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    rtl_lr(&t0, R_AX, 2);
    t1 = t0 < 0 ? 0xffff : 0;
    rtl_sr(R_DX, 2, &t1);
  }
  else {
    rtl_lr(&t0, R_EAX, 4);
    t1 = t0 < 0 ? 0xffffffff : 0;
    rtl_sr(R_EDX, 4, &t1);
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    TODO();
  }
  else {
    TODO();
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  rtl_sext(&t2, &id_src->val, id_src->width);
  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  operand_write(id_dest, &id_src->val);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}
