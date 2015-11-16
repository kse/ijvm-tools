#include <stdlib.h>
#include "ijvm-asm.h"

JasmInsn *jasm_labels = NULL;

JasmMethod *
jasm_method_make (char *name, JasmDir *dirs, JasmInsn *insns)
{
  JasmMethod *method;

  method = malloc (sizeof (JasmMethod));
  method->name = name;
  method->args = NULL;
  method->locals = NULL;
  method->dirs = dirs;
  method->defines = NULL;
  method->labels = NULL;
  method->insns = insns;
  method->next = NULL;
    
  return method;
}

JasmDir *
jasm_dir_make_locals (JasmExpr *expr, int line_number)
{
  JasmDir *dir;

  dir = malloc (sizeof (JasmDir));
  dir->kind = JASM_DIR_LOCALS;
  dir->next = NULL;
  dir->u.locals = expr;
  dir->line = line_number;

  return dir;
}

JasmDir *
jasm_dir_make_args (JasmExpr *expr, int line_number)
{
  JasmDir *dir;

  dir = malloc (sizeof (JasmDir));
  dir->kind = JASM_DIR_ARGS;
  dir->next = NULL;
  dir->u.args = expr;
  dir->line = line_number;

  return dir;
}

JasmDir *
jasm_dir_make_define (char *symbol, JasmExpr *expr, int line_number)
{
  JasmDir *dir;

  dir = malloc (sizeof (JasmDir));
  dir->kind = JASM_DIR_DEFINE;
  dir->next = NULL;
  dir->u.define.symbol = symbol;
  dir->u.define.expr = expr;
  dir->line = line_number;

  return dir;
}

JasmInsn *
jasm_insn_make_generic (IJVMInsnTemplate *tmpl, JasmOperand *operands, 
			int line_number)
{
  JasmInsn *insn;

  insn = malloc (sizeof (JasmInsn));
  insn->kind = JASM_INSN_GENERIC;
  insn->u.generic.tmpl = tmpl;
  insn->u.generic.operands = operands;
  insn->wide = FALSE;
  insn->line = line_number;
  insn->next = NULL;

  return insn;
}

JasmInsn *
jasm_insn_make_label (char *label, int line_number)
{
  JasmInsn *insn;

  insn = malloc (sizeof (JasmInsn));
  insn->kind = JASM_INSN_LABEL;
  insn->u.label = label;
  insn->line = line_number;
  insn->next = NULL;

  return insn;
}

JasmOperand *
jasm_operand_make (JasmExpr *expr, JasmOperand *next)
{
  JasmOperand *operand;

  operand = malloc (sizeof (JasmOperand));
  operand->expr = expr;
  operand->next = next;

  return operand;
}

JasmExpr *
jasm_expr_make_integer (unsigned int value, int line_number)
{
  JasmExpr *expr;

  expr = malloc (sizeof (JasmExpr));
  expr->kind = JASM_EXPR_INTEGER;
  expr->touch = FALSE;
  expr->u.integer = value;
  expr->line = line_number;

  return expr;    
}

JasmExpr *
jasm_expr_make_symbol (char *value, int line_number)
{
  JasmExpr *expr;

  expr = malloc (sizeof (JasmExpr));
  expr->kind = JASM_EXPR_SYMBOL;
  expr->touch = FALSE;
  expr->u.symbol = value;
  expr->line = line_number;

  return expr;    
}

JasmExpr *
jasm_expr_make_binop (JasmExprKind kind, JasmExpr *left, JasmExpr *right, 
		      int line_number)
{
  JasmExpr *expr;

  expr = malloc (sizeof (JasmExpr));
  expr->kind = kind;
  expr->touch = FALSE;
  expr->u.binop.left = left;
  expr->u.binop.right = right;
  expr->line = line_number;

  return expr;    
}

JasmExpr *
jasm_expr_make_neg (JasmExpr *neg, int line_number)
{
  JasmExpr *expr;

  expr = malloc (sizeof (JasmExpr));
  expr->kind = JASM_EXPR_NEG;
  expr->touch = FALSE;
  expr->u.neg = neg;
  expr->line = line_number;

  return expr;    
}

