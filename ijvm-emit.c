#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "ijvm-asm.h"
#include "ijvm-util.h"

JasmCPool *
jasm_cpool_make (void)
{
  JasmCPool *cpool;

  cpool = malloc (sizeof (JasmCPool));
  cpool->consts = NULL;
  cpool->length = 0;
  cpool->alloc = 0;

  return cpool;
}

int
jasm_cpool_append (JasmCPool *cpool, int value)
{
  if (cpool->length == cpool->alloc) {
    cpool->alloc = MAX (cpool->alloc * 2, 16);
    cpool->consts = realloc (cpool->consts, cpool->alloc * sizeof (int));
  }
  
  cpool->consts[cpool->length] = value;

  return cpool->length++;
}

int
jasm_cpool_add (JasmCPool *cpool, int value)
{
  int i;

  for (i = 0; i < cpool->length; i++)
    if (cpool->consts[i] == value)
      return i;

  return jasm_cpool_append (cpool, value);
}
    
void
jasm_cpool_emit (JasmCPool *cpool)
{
  int i;

  for (i = 0; i < cpool->length; i++)
    printf ("%08x\n", cpool->consts[i]);

#if 0
  image->cpool = malloc (sizeof (int32) * cpool->length);
  image->cpool_size = cpool->length;
  for (i = 0; i < cpool->length; i++)
    image->cpool[i] = cpool->consts[i];
#endif

}

/* IJVM Builtin methods. 
 *
 * The IJVM has been extended with input/ouput capabilites by adding
 * two builtin methods: `getchar' and `putchar'.  To read a keypress
 * use `getchar'.  The simulator read a character from the terminal
 * and return the ASCII code.  Likewise, `putchar' take the ASCII code
 * of a character as parameter and prints it on the terminal.
 *
 * The functions are implemented by reserving the upper half of the
 * indices in the constant pool.  The instruction `invokevirtual
 * putchar' is assembled into B6 80 00, and the simulator knows that
 * indices above 0x8000 are special and dispatches to a C function,
 * implementing the actual I/O */

char *ijvm_builtins[] = { "getchar", "putchar", NULL };

int
jasm_builtin_lookup (char *name)
{
  int i;

  for (i = 0; ijvm_builtins[i] != NULL; i++)
    if (strcasecmp (name, ijvm_builtins[i]) == 0)
      return i + 0x8000;
  return -1;
}
   
JasmExpr *
jasm_method_lookup_define (JasmMethod *method, char *symbol)
{
  JasmDefine *define;

  for (define = method->defines; define != NULL; define = define->next)
    if (strcasecmp (define->symbol, symbol) == 0)
      return define->u.expr;
  return NULL;
}

int
jasm_method_add_define (JasmMethod *method, char *symbol, JasmExpr *expr)
{
  JasmDefine *define;

  if (jasm_method_lookup_define (method, symbol))
    return TRUE;

  define = malloc (sizeof (JasmDefine));
  define->symbol = symbol;
  define->u.expr = expr;
  define->next = method->defines;
  method->defines = define;
  return FALSE;
}

JasmInsn *
jasm_method_lookup_label (JasmMethod *method, char *symbol)
{
  JasmDefine *define;

  for (define = method->labels; define != NULL; define = define->next)
    if (strcasecmp (define->symbol, symbol) == 0)
      return define->u.label;
  return NULL;
}

int
jasm_method_add_label (JasmMethod *method, char *symbol, JasmInsn *label)
{
  JasmDefine *define;

  if (jasm_method_lookup_label (method, symbol))
    return TRUE;

  define = malloc (sizeof (JasmDefine));
  define->symbol = symbol;
  define->u.label = label;
  define->next = method->labels;
  method->labels = define;
  return FALSE;
}

static void
jasm_method_check_directives (JasmMethod *method)
{
  JasmDir *dir;

  for (dir = method->dirs; dir != NULL; dir = dir->next)
    switch (dir->kind) {
    case JASM_DIR_LOCALS:
      if (method->locals != NULL)
	jasm_warning ("in method `%s' line %d: .locals already specified\n", 
		      method->name, dir->line);
      else
	method->locals = dir->u.locals;
      break;

    case JASM_DIR_ARGS:
      if (method->args != NULL)
	jasm_warning ("in method `%s' line %d: .args already specified\n", 
		      method->name, dir->line);
      else
	method->args = dir->u.args;
      break;

    case JASM_DIR_DEFINE:
      if (jasm_method_add_define (method, dir->u.define.symbol,
				  dir->u.define.expr))
	jasm_warning ("in method `%s' line %d: symbol `%s' previously defined\n",
		      method->name, dir->line, dir->u.define.symbol);
      break;
    }
}

/* This evaluation method checks for overflow in every step, so if
 * some sub-result is out of range, it is an error.  The negation is a
 * special case: integer constants are parsed as unsigned ints, so we
 * can represent values in the range [0 ; 2^32-1].  However the
 * constant we actually allow in the program are [-2^31 ; 2^31-1], and
 * to achieve this we only allow 2^31 to appear as an argument to a
 * negation and anything above 2^31 is an overflow.  So negative
 * integer constants are parsed as the negation of a positive
 * constant.
 */

int
jasm_expr_eval (JasmExpr *expr, JasmMethod *method)
{
  int left, right, result;
  JasmExpr *bound_expr;

  if (expr->touch)
    jasm_abort ("in method `%s' line %d: circular reference in defines\n",
		method->name, expr->line);
  expr->touch = TRUE;
  switch (expr->kind) {
  case JASM_EXPR_INTEGER:
    if (expr->u.integer > INT_MAX)
      jasm_abort ("in line %d: constant out of range (%u)\n", 
		  expr->line, expr->u.integer);
    result = expr->u.integer;
    break;
  case JASM_EXPR_SYMBOL:
    bound_expr = jasm_method_lookup_define (method, expr->u.symbol);
    if (bound_expr == NULL)
      jasm_abort ("in method `%s' line %d: undefined symbol `%s'\n",
		  method->name, expr->line, expr->u.symbol);
    result = jasm_expr_eval (bound_expr, method);
    break;
  case JASM_EXPR_PLUS:
    left = jasm_expr_eval (expr->u.binop.left, method);
    right = jasm_expr_eval (expr->u.binop.right, method);
    result = left + right;
    if ((left ^ right) >= 0 && (left ^ result) < 0)
      jasm_abort ("in method `%s' line %d: overflow in expression\n", 
		  method->name, expr->line);
    break;
  case JASM_EXPR_MINUS:
    left = jasm_expr_eval (expr->u.binop.left, method);
    right = jasm_expr_eval (expr->u.binop.right, method);
    result = left - right;
    if ((left ^ right) < 0 && (left ^ result) < 0)
      jasm_abort ("in method `%s' line %d: overflow in expression\n", 
		  method->name, expr->line);
    break;
  case JASM_EXPR_NEG:
    if (expr->u.neg->kind == JASM_EXPR_INTEGER) {
      if (expr->u.neg->u.integer > (unsigned int) INT_MIN)
	jasm_abort ("in line %d: constant out of range (-%u)\n", 
		    expr->line, expr->u.neg->u.integer);
      else
	result = -expr->u.neg->u.integer;
    }
    else {
      result = -jasm_expr_eval (expr->u.neg, method);
      if (result == INT_MIN)
	jasm_abort ("in method `%s' line %d: overflow in expression\n", 
		    method->name, expr->line);
    }
    break;
  default:
    jasm_assert (FALSE, "unexpected variant in jasm_expr_eval_loop!\n");
    result = 0;
  }
  expr->touch = FALSE;
  return result;
}

/* Evaluate operands for the given insn and check for over-/underflow
 * considering the operand type.  Check that the number of arguments
 * is correct. Returns the total size of the operands. */

int
jasm_insn_check_operands (JasmInsn *insn, JasmMethod *method, JasmCPool *cpool)
{
  JasmOperand *op;
  IJVMInsnTemplate *tmpl;
  int size, i;
  int value, kind;

  size = 0;
  op = insn->u.generic.operands;
  tmpl = insn->u.generic.tmpl;
  for (i = 0; i < tmpl->noperands && op != NULL; i++, op = op->next) {
    kind = tmpl->operands[i];
    switch (kind) {
    case IJVM_OPERAND_BYTE:
      op->u.value = jasm_expr_eval (op->expr, method);
      if (op->u.value < -128 || op->u.value > 127)
	jasm_abort ("in method `%s' line %d: value out of range (%d)\n",
		    method->name, insn->line, op->u.value);
      size += 1;
      break;

    case IJVM_OPERAND_METHOD:
    case IJVM_OPERAND_LABEL:
      if (op->expr->kind == JASM_EXPR_SYMBOL)
	op->u.label = op->expr->u.symbol;
      else
	jasm_abort ("in method `%s' line %d: %s expected\n",
		    method->name, insn->line, op->u.value,
		    kind == IJVM_OPERAND_LABEL ? "label" : "method name");
      size += 2;
      break;

    case IJVM_OPERAND_VARNUM:
      op->u.value = jasm_expr_eval (op->expr, method);
      if (op->u.value < 0 || op->u.value > 255)
	jasm_abort ("in method `%s' line %d: value out of range (%d)\n",
		    method->name, insn->line, op->u.value);
      size += 1;
      break;

    case IJVM_OPERAND_VARNUM_WIDE:
      op->u.value = jasm_expr_eval (op->expr, method);
      if (op->u.value < 0 || 65535 < op->u.value)
	jasm_abort ("in method `%s' line %d: varnum out of range (%d)\n",
		    method->name, insn->line, op->u.value);
      if (op->u.value > 255) {
	insn->wide = TRUE;
	size += 2;
      }
      else
	size += 1;
      break;

    case IJVM_OPERAND_CONSTANT:
      value = jasm_expr_eval (op->expr, method);
      op->u.value = jasm_cpool_add (cpool, value);
      size += 2;
    }
  }

  if (op != NULL || i != tmpl->noperands)
    jasm_abort ("in method `%s' line %d: wrong number of operands\n",
		method->name, insn->line);
      
  return size;
}

int
jasm_method_check_insns (JasmMethod *method, int initial_pc, JasmCPool *cpool)
{
  JasmInsn *insn;
  int pc, size;

  pc = initial_pc;
  for (insn = method->insns; insn != NULL; insn = insn->next) {
    insn->pc = pc;
    switch (insn->kind) {
    case JASM_INSN_GENERIC:
      size = jasm_insn_check_operands (insn, method, cpool);
      pc += 1 + size + (insn->wide ? 1 : 0);
      break;

    case JASM_INSN_LABEL:
      if (jasm_method_add_label (method, insn->u.label, insn))
	jasm_warning ("in method `%s' line %d: symbol `%s' previously defined\n",
		      method->name, insn->line, insn->u.label);
      break;
    }
  }
  return pc;
}

typedef struct ByteStream ByteStream;
struct ByteStream {
  int length, alloc;
  uint8 *bytes;
};

ByteStream *
byte_stream_new (void)
{
  ByteStream *bs;

  bs = malloc (sizeof (ByteStream));
  bs->length = 0;
  bs->alloc = 0;
  bs->bytes = NULL;

  return bs;
}

int
byte_stream_append (ByteStream *bs, uint8 byte)
{
  if (bs->length == bs->alloc) {
    bs->alloc = MAX (bs->alloc * 2, 16);
    bs->bytes = realloc (bs->bytes, bs->alloc);
  }
  
  bs->bytes[bs->length] = byte;

  return bs->length++;
}

void
jasm_emit_byte (int byte, ByteStream *bs)
{
  byte_stream_append (bs, byte);
}

void
jasm_emit_int16 (int word, ByteStream *bs)
{
  jasm_emit_byte (word >> 8, bs);
  jasm_emit_byte (word, bs);
}

JasmMethod *
jasm_method_lookup (char *name, JasmMethod *methods)
{
  if (methods == NULL)
    return NULL;
  else if (strcasecmp (name, methods->name) == 0)
    return methods;
  else 
    return jasm_method_lookup (name, methods->next);
}

void
jasm_insn_emit_operands (JasmInsn *insn, JasmMethod *method, JasmMethod *all,
			 ByteStream *bs)
{
  JasmMethod *target;
  JasmOperand *op;
  JasmInsn *label;
  int index, i, offset;

  op = insn->u.generic.operands;
  for (i = 0; i < insn->u.generic.tmpl->noperands; i++, op = op->next)
    switch (insn->u.generic.tmpl->operands[i]) {
    case IJVM_OPERAND_BYTE:
    case IJVM_OPERAND_VARNUM:
      jasm_emit_byte (op->u.value, bs);
      break;
      
    case IJVM_OPERAND_LABEL:
      label = jasm_method_lookup_label (method, op->u.label);
      if (label == NULL)
	jasm_abort ("in method `%s' line %d: label `%s' not defined\n",
		    method->name, insn->line, op->u.label);
      offset = label->pc - insn->pc;
      jasm_emit_int16 (offset, bs);
      break;

    case IJVM_OPERAND_METHOD:
      index = jasm_builtin_lookup (op->u.label);
      if (index >= 0)
	jasm_emit_int16 (index, bs);
      else {
	target = jasm_method_lookup (op->u.label, all);
	if (target == NULL)
	  jasm_abort ("in method %s line %d: method %s not defined\n", 
		      method->name, insn->line, op->u.label);
	jasm_emit_int16 (target->index, bs);
      }
      break;

    case IJVM_OPERAND_VARNUM_WIDE:
      if (op->u.value > 255)
	jasm_emit_int16 (op->u.value, bs);
      else
	jasm_emit_byte (op->u.value, bs);
      break;

    case IJVM_OPERAND_CONSTANT:
      jasm_emit_int16 (op->u.value, bs);
      break;
    }
}

void
jasm_method_emit_insns (JasmMethod *method, JasmMethod *all, ByteStream *bs)
{
  JasmInsn *insn;

  for (insn = method->insns; insn != NULL; insn = insn->next) {
    if (insn->kind == JASM_INSN_GENERIC) {
      if (insn->wide)
	jasm_emit_byte (IJVM_OPCODE_WIDE, bs);
      jasm_emit_byte (insn->u.generic.tmpl->opcode, bs);
      jasm_insn_emit_operands (insn, method, all, bs);
    }
  }
}  

int
jasm_method_check (JasmMethod *method, JasmCPool *cpool)
{
  JasmMethod *m;
  int pc;

  pc = 0;
  for (m = method; m != NULL; m = m->next) {
    m->address = pc;
    m->index = jasm_cpool_add (cpool, pc);

    jasm_method_check_directives (m);
    pc = jasm_method_check_insns (m, pc + 4, cpool);
  }

  return pc;
}

void
jasm_method_emit (JasmMethod *methods, JasmCPool *cpool, ByteStream *bs)
{
  JasmMethod *m;

  for (m = methods; m != NULL; m = m->next) {
    if (m->args == NULL)
      jasm_emit_int16 (1, bs);
    else
      jasm_emit_int16 (jasm_expr_eval (m->args, m), bs);

    if (m->locals == NULL)
      jasm_emit_int16 (0, bs);
    else
      jasm_emit_int16 (jasm_expr_eval (m->locals, m), bs);

    jasm_method_emit_insns (m, methods, bs);
  }
}

IJVMImage *
jasm_emit (JasmMethod *methods, JasmCPool *cpool)
{
  JasmMethod *main_method;
  ByteStream *bs;

  bs = byte_stream_new ();
  jasm_method_emit (methods, cpool, bs);
  
  main_method = jasm_method_lookup ("main", methods);
  if (main_method == NULL)
    jasm_abort ("Method `main' not found\n");

  return ijvm_image_new (main_method->index,
			  bs->bytes, bs->length,
			  cpool->consts, cpool->length);
}
