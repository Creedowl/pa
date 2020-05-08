#include "cpu/exec.h"

make_EHelper(test) {
  rtl_and(&t0, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t0, id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_sext(&t0, &id_src->val, id_src->width);
  rtl_and(&t1, &id_dest->val, &t0);
  operand_write(id_dest, &t1);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t1, id_dest->width);

  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);

  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);

  print_asm_template2(or);
}

make_EHelper(sar) {
  Log("%d %08x %d %x", id_dest->width, id_dest->val, id_src->val, *eip);
  rtl_sar(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);
  Log("%08x", t2);
  rtl_update_ZFSF(&id_dest->val,id_dest->width);
  // rtl_sext(&t1, &id_dest->val, id_dest->width);
  // rtl_sar(&t0, &t1, &id_src->val);
  // t1 = 0x1 << (id_dest->width * 8);
  // rtl_or(&t2, &id_dest->val, &t1);
  // rtl_and(&t3, &t2, &t0);
  // operand_write(id_dest, &t3);
  // Log("%08x", t3);
  // rtl_update_ZFSF(&t3, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(shl) {
  rtl_shl(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  rtl_update_ZFSF(&t0, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_shr(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  rtl_update_ZFSF(&t0, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_not(&id_dest->val);
  operand_write(id_dest, &id_dest->val);

  print_asm_template1(not);
}

make_EHelper(rol) {
  rtl_shri(&t2, &id_dest->val, id_dest->width * 8 - id_src->val);
	rtl_shl(&t3, &id_dest->val, &id_src->val);
	rtl_or(&t1, &t2, &t3);
	operand_write(id_dest, &t1);
  
  print_asm_template2(rol);
}
