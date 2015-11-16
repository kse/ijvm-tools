#ifndef JASM_H
#define JASM_H

#include <stdio.h>

#include "ijvm-spec.h"
#include "ijvm-util.h"
#include "types.h"

typedef struct JasmMethod JasmMethod;
typedef struct JasmDir JasmDir;

typedef struct JasmOperand JasmOperand;

typedef enum JasmInsnKind JasmInsnKind;
typedef struct JasmInsn JasmInsn;
typedef struct JasmExpr JasmExpr;
typedef enum JasmExprKind JasmExprKind;

typedef struct JasmDefine JasmDefine;
typedef struct JasmCPool JasmCPool;

struct  JasmMethod
{
  char *name;
  JasmExpr *args, *locals;
  int size, address, index;
  JasmDir *dirs;
  JasmDefine *defines, *labels;
  JasmInsn *insns;
  JasmMethod *next;
};

JasmMethod *jasm_method_make (char *name, JasmDir *dirs, JasmInsn *insns);

typedef enum {
  JASM_DIR_LOCALS,
  JASM_DIR_ARGS,
  JASM_DIR_DEFINE
} JasmDirKind;

struct JasmDir
{
  JasmDirKind kind;
  int line;
  union
  {
    JasmExpr *locals;
    JasmExpr *args;
    struct { char *symbol; JasmExpr *expr; } define;
  } u;
  JasmDir *next;
};

JasmDir *jasm_dir_make_locals (JasmExpr *expr, int line_number);
JasmDir *jasm_dir_make_args (JasmExpr *expr, int line_number);
JasmDir *jasm_dir_make_define (char *symbol, JasmExpr *expr, int line_number);

struct JasmOperand {
  JasmExpr *expr; 
  union {
    int value;
    char *label;
  } u;
  JasmOperand *next;
};

JasmOperand *jasm_operand_make (JasmExpr *expr, JasmOperand *next);

enum JasmInsnKind {
  JASM_INSN_GENERIC,
  JASM_INSN_LABEL
};

struct JasmInsn
{
  JasmInsnKind kind;
  union {
    struct {
      IJVMInsnTemplate *tmpl;
      JasmOperand *operands;
    } generic;
    char *label;
  } u;
  int pc, line, wide;
  JasmInsn *next;    
};

JasmInsn *jasm_insn_make_generic (IJVMInsnTemplate *tmpl, 
				  JasmOperand *operands,
				  int line_number);
JasmInsn *jasm_insn_make_label (char *label, int line_number);

enum JasmExprKind
{
  JASM_EXPR_INTEGER,
  JASM_EXPR_SYMBOL,
  JASM_EXPR_PLUS,
  JASM_EXPR_MINUS,
  JASM_EXPR_NEG
};

struct JasmExpr
{
  JasmExprKind kind;
  int line;
  int touch;
  union 
  {
    unsigned int integer;
    char *symbol;
    struct { JasmExpr *left, *right; } binop;
    JasmExpr *neg;
  } u;
};

JasmExpr *jasm_expr_make_integer (unsigned int value, int line_number);
JasmExpr *jasm_expr_make_symbol (char *value, int line_number);
JasmExpr *jasm_expr_make_binop (JasmExprKind kind, 
				JasmExpr *left, JasmExpr *right, 
				int line_number);
JasmExpr *jasm_expr_make_neg (JasmExpr *expr, int line_number);

struct JasmDefine
{
  char *symbol;
  union 
  {
    JasmExpr *expr;
    JasmInsn *label;
  } u;
  JasmDefine *next;
};

struct JasmCPool
{
  int *consts;
  int length, alloc;
};

JasmCPool *jasm_cpool_make (void);
int jasm_cpool_add (JasmCPool *cpool, int cnst);
void jasm_cpool_emit (JasmCPool *cpool);
int jasm_expr_eval (JasmExpr *expr, JasmMethod *method);

void jasm_abort (const char *fmt, ...);
void jasm_assert (int cond, const char *fmt, ...);
void jasm_warning (const char *fmt, ...);
char *jasm_strdup (const char *str);

int jasm_lex_current_line ();
int jasm_lex_parse_int (const char *token);
JasmMethod *jasm_parse ();
int jasm_method_check (JasmMethod *method, JasmCPool *cpool);
IJVMImage *jasm_emit (JasmMethod *methods, JasmCPool *cpool);


#endif
