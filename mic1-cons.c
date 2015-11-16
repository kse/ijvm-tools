#include <stdlib.h>
#include <stdarg.h>
#include "mic1-asm.h"

MasmLine *
masm_line_make (MasmLabel *label, MasmInsn *insns)
{
  MasmLine *line;

  line = masm_malloc (sizeof (MasmLine));
  line->label = label;
  line->insns = insns;
  line->laid_out = FALSE;
  line->line_number = masm_parse_current_line ();
  line->next = NULL;
  return line;
}

MasmLabel *
masm_label_make (char *name, int address, int absolute)
{
  MasmLabel *label;

  label = masm_malloc (sizeof (MasmLabel));
  label->name = name;
  label->address = address;
  label->absolute = absolute;
  return label;
}

MasmInsn *
masm_insn_make_rd ()
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn)); 
  insn->kind = MASM_INSN_RD;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_wr ()
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_WR;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_fetch ()
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_FETCH;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_empty ()
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_EMPTY;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_halt ()
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_HALT;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_goto (char *label)
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_GOTO;
  insn->u.insn_goto.label = label;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_igoto (int next_address)
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_IGOTO;
  insn->u.insn_igoto.next_address = next_address;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_if (enum MasmCondKind cond, char *label1, char *label2)
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_IF;
  insn->u.insn_if.cond = cond;
  insn->u.insn_if.label1 = label1;
  insn->u.insn_if.label2 = label2;
  insn->next = NULL;
  return insn;
}

MasmInsn *
masm_insn_make_assign (MasmAssign *assign)
{
  MasmInsn *insn;

  insn = masm_malloc (sizeof (MasmInsn));
  insn->kind = MASM_INSN_ASSIGN;
  insn->u.insn_assign = assign;
  insn->next = NULL;
  return insn;
}

MasmAssign *
masm_assign_make_basic (enum MasmRegKind reg, MasmAlu *alu)
{
  MasmAssign *assign;

  assign = masm_malloc (sizeof (MasmAssign));
  assign->kind = MASM_ASSIGN_BASIC;
  assign->u.assign_basic.reg = reg;
  assign->u.assign_basic.alu = alu;
  return assign;  
}

MasmAssign *
masm_assign_make_chain (enum MasmRegKind reg, MasmAssign *src)
{
  MasmAssign *assign;

  assign = masm_malloc (sizeof (MasmAssign));
  assign->kind = MASM_ASSIGN_CHAIN;
  assign->u.assign_chain.reg = reg;
  assign->u.assign_chain.assign = src;
  return assign;  
}

MasmAssign *
masm_assign_make_rshift1 (enum MasmRegKind reg, MasmAlu *alu)
{
  MasmAssign *assign;

  assign = masm_malloc (sizeof (MasmAssign));
  assign->kind = MASM_ASSIGN_RSHIFT1;
  assign->u.assign_rshift1.reg = reg;
  assign->u.assign_rshift1.alu = alu;
  return assign;  
}

MasmAssign *
masm_assign_make_lshift8 (enum MasmRegKind reg, MasmAlu *alu)
{
  MasmAssign *assign;

  assign = masm_malloc (sizeof (MasmAssign));
  assign->kind = MASM_ASSIGN_LSHIFT8;
  assign->u.assign_lshift8.reg = reg;
  assign->u.assign_lshift8.alu = alu;
  return assign;  
}

MasmAlu *
masm_alu_make_reg (enum MasmRegKind reg)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_REG;
  alu->u.alu_reg.reg = reg;
  return alu;
}

MasmAlu *
masm_alu_make_inv (enum MasmRegKind reg)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_INV;
  alu->u.alu_inv.reg = reg;
  return alu;
}

MasmAlu *
masm_alu_make_add_rr1 (enum MasmRegKind reg1, enum MasmRegKind reg2)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_ADD_RR1;
  alu->u.alu_add_rr1.reg1 = reg1;
  alu->u.alu_add_rr1.reg2 = reg2;
  return alu;
}

MasmAlu *
masm_alu_make_add_rr (enum MasmRegKind reg1, enum MasmRegKind reg2)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_ADD_RR;
  alu->u.alu_add_rr.reg1 = reg1;
  alu->u.alu_add_rr.reg2 = reg2;
  return alu;
}

MasmAlu *
masm_alu_make_add_r1 (enum MasmRegKind reg)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_ADD_R1;
  alu->u.alu_add_r1.reg = reg;
  return alu;
}

MasmAlu *
masm_alu_make_sub_rr (enum MasmRegKind reg1, enum MasmRegKind reg2)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_SUB_RR;
  alu->u.alu_sub_rr.reg1 = reg1;
  alu->u.alu_sub_rr.reg2 = reg2;
  return alu;
}

MasmAlu *
masm_alu_make_sub_r1 (enum MasmRegKind reg)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_SUB_R1;
  alu->u.alu_sub_r1.reg = reg;
  return alu;
}

MasmAlu *
masm_alu_make_neg (enum MasmRegKind reg)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_NEG;
  alu->u.alu_neg.reg = reg;
  return alu;
}

MasmAlu *
masm_alu_make_and (enum MasmRegKind reg1, enum MasmRegKind reg2)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_AND;
  alu->u.alu_and.reg1 = reg1;
  alu->u.alu_and.reg2 = reg2;
  return alu;
}

MasmAlu *
masm_alu_make_or (enum MasmRegKind reg1, enum MasmRegKind reg2)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_OR;
  alu->u.alu_or.reg1 = reg1;
  alu->u.alu_or.reg2 = reg2;
  return alu;
}

MasmAlu *
masm_alu_make_const (int val)
{
  MasmAlu *alu;

  alu = masm_malloc (sizeof (MasmAlu));
  alu->kind = MASM_ALU_CONST;
  alu->u.alu_const.val = val;
  return alu;
}
