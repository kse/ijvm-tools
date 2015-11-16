%{

#include <stdlib.h>
#include <stdio.h>
#include "ijvm-asm.h"

#define YYDEBUG 1

int yylex(void);
void yyerror(char *msg);

static JasmMethod *program;

%}

%union {
  struct { char *value; int line_num; } symbol;
  struct { unsigned int value; int line_num; } integer;
  JasmMethod *method;
  JasmOperand *operand;
  JasmDir *dir;
  JasmInsn *insn;
  struct { IJVMInsnTemplate *tmpl; int line_num; } mnemonic;
  JasmExpr *expr;
  int line_num;
};

%token T_BIPUSH T_DUP T_GOTO T_IADD T_IAND T_IFEQ
%token T_IFLT T_IF_ICMPEQ T_IINC T_ILOAD T_INVOKEVIRTUAL 
%token T_IOR T_IRETURN T_ISTORE T_ISUB T_LDC_W T_NOP
%token T_POP T_SWAP T_NEWLINE T_METHOD 

%token <line_num> T_LOCALS T_ARGS T_DEFINE
%token <symbol> T_SYMBOL T_LABEL
%token <integer> T_INTEGER
%token <mnemonic> T_MNEMONIC
%type <method> program methods method
%type <dir> directives directive
%type <insn> insns insn
%type <expr> expr
%type <operand> operands

%left '+' '-'
%left UNARY

%%

program : methods { program = $1; }

methods: 
   method methods { $1->next = $2; $$ = $1; }
 | method         { $$ = $1; }
 ;

method:
   T_METHOD T_SYMBOL directives insns 
                               { $$ = jasm_method_make ($2.value, $3, $4); }
 | T_METHOD T_SYMBOL insns     
                               { $$ = jasm_method_make ($2.value, NULL, $3); }
 ;

directives:
   directive directives { $1->next = $2; $$ = $1; }
 | directive            { $$ = $1; }
 ;

directive:
   T_ARGS expr                { $$ = jasm_dir_make_args ($2, $1); }
 | T_LOCALS expr              { $$ = jasm_dir_make_locals ($2, $1); }
 | T_DEFINE T_SYMBOL '=' expr { $$ = jasm_dir_make_define ($2.value, $4, $1); }
 ;

insns:
   insn insns { $1->next = $2; $$ = $1; }
 | insn       { $$ = $1; }
 ; 

insn:
   T_MNEMONIC operands
                { $$ = jasm_insn_make_generic ($1.tmpl, $2, $1.line_num); }
 | T_MNEMONIC   { $$ = jasm_insn_make_generic ($1.tmpl, NULL, $1.line_num); }
 | T_LABEL      { $$ = jasm_insn_make_label ($1.value, $1.line_num); }
 ;

operands:
   expr ',' operands { $$ = jasm_operand_make ($1, $3); }
 | expr              { $$ = jasm_operand_make ($1, NULL); }
 ;

expr: 
   T_INTEGER          
           { $$ = jasm_expr_make_integer ($1.value, $1.line_num); }
 | T_SYMBOL   
           { $$ = jasm_expr_make_symbol ($1.value, $1.line_num); }
 | expr '+' expr
           { $$ = jasm_expr_make_binop (JASM_EXPR_PLUS, $1, $3, $1->line); }
 | expr '-' expr
           { $$ = jasm_expr_make_binop (JASM_EXPR_MINUS, $1, $3, $1->line); }
 | '-' expr %prec UNARY
           { $$ = jasm_expr_make_neg ($2, $2->line); }
 | '(' expr ')'    
           { $$ = $2; }
 ;

%%

void yyerror (char *msg)
{
  extern char *yytext;

  jasm_abort ("in line %d: parse error before `%s'\n", 
	      jasm_lex_current_line (), yytext);
}

JasmMethod *
jasm_parse ()
{
  yyparse ();
  return program;
}
