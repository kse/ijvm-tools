%{

#include <stdlib.h>
#include <stdio.h>
#include "mic1-asm.h"

#define YYDEBUG 1

int yylex(void);
void yyerror(char *msg);

static MasmLine *top;
static int current_line = 1;

%}

%union {
  MasmLine *line;
  MasmLabel *label;
  MasmInsn *insn;
  MasmAssign *assign;
  MasmAlu *alu;
  char *id;
  int integer;
  int reg;
  int cond;
};

%token T_RD T_WR T_FETCH T_EMPTY T_HALT T_GOTO T_CONST T_IF T_ELSE T_N T_Z
%token T_INV T_AND T_OR T_RSHIFT T_LSHIFT T_NEWLINE
%token T_MAR T_MDR T_PC T_MBR T_MBRU T_SP T_LV T_CPP T_TOS T_OPC T_H
%token <integer> T_CONST
%token <id> T_LABEL

%type <line> lines line
%type <label> label
%type <insn> insn insns
%type <assign> assign
%type <alu> alu
%type <reg> reg vreg
%type <integer> const one
%type <cond> cond

%%

top:
   newlines_opt lines newlines_opt { top = $2; }
 ;

lines: 
   lines newlines line { $3->next = $1; $$ = $3; }
 | line                { $$ = $1; }
 ;

line:
   label insns { $$ = masm_line_make ($1, $2); }
 | label       { $$ = masm_line_make ($1, NULL); }
 | insns       { $$ = masm_line_make (NULL, $1); }
 ;

label:
   T_LABEL ':'             { $$ = masm_label_make ($1, 0, FALSE); }
 | T_LABEL '=' T_CONST ':' { $$ = masm_label_make ($1, $3, TRUE); }
 ;

insns:
   insns ';' insn { $3->next = $1; $$ = $3; }
 | insn           { $$ = $1; }
 ;

insn:
   T_RD                     	     { $$ = masm_insn_make_rd (); }
 | T_WR                     	     { $$ = masm_insn_make_wr (); }
 | T_FETCH                  	     { $$ = masm_insn_make_fetch (); } 
 | T_EMPTY                  	     { $$ = masm_insn_make_empty (); } 
 | T_HALT                  	     { $$ = masm_insn_make_halt (); } 
 | T_GOTO T_LABEL           	     { $$ = masm_insn_make_goto ($2); }
 | T_GOTO '(' T_MBR ')'     	     { $$ = masm_insn_make_igoto (0); }
 | T_GOTO '(' T_MBR T_OR T_CONST ')' { $$ = masm_insn_make_igoto ($5); }
 | T_IF '(' cond ')' T_GOTO T_LABEL ';' T_ELSE T_GOTO T_LABEL
                                     { $$ = masm_insn_make_if ($3, $6, $10); }
 | assign                            { $$ = masm_insn_make_assign ($1); }
 ;

cond:
   T_N { $$ = MASM_COND_N; }
 | T_Z { $$ = MASM_COND_Z; }
 ;

assign:
   vreg '=' assign               { $$ = masm_assign_make_chain ($1, $3); }
 | vreg '=' alu                  { $$ = masm_assign_make_basic ($1, $3); }
 | vreg '=' alu T_RSHIFT T_CONST { $$ = masm_assign_make_rshift1 ($1, $3); 
                                   if ($5 != 1)
                                     masm_warning ("in line %d: can only right shift 1 position\n", masm_parse_current_line ());
                                 }
 | vreg '=' alu T_LSHIFT T_CONST { $$ = masm_assign_make_lshift8 ($1, $3);
                                   if ($5 != 8)
                                     masm_warning ("in line %d: can only left shift 8 positions\n", masm_parse_current_line ());
                                 }

alu:
   reg                  { $$ = masm_alu_make_reg ($1); }
 | T_INV '(' reg ')'    { $$ = masm_alu_make_inv ($3); }
 | '-' reg              { $$ = masm_alu_make_neg ($2); }
 | reg '+' reg	        { $$ = masm_alu_make_add_rr ($1, $3); }
 | reg '+' reg '+' one  { $$ = masm_alu_make_add_rr1 ($1, $3); }
 | reg '+' one	        { $$ = masm_alu_make_add_r1 ($1); }
 | reg '-' reg	        { $$ = masm_alu_make_sub_rr ($1, $3); }
 | reg '-' one	        { $$ = masm_alu_make_sub_r1 ($1); }
 | reg T_AND reg        { $$ = masm_alu_make_and ($1, $3); }
 | reg T_OR reg	        { $$ = masm_alu_make_or ($1, $3); }
 | const                { $$ = masm_alu_make_const ($1); }
 | '-' const            { $$ = masm_alu_make_const (-$2); }
 ;

reg:
   T_MAR		{ $$ = MASM_REG_MAR; }
 | T_MBR		{ $$ = MASM_REG_MBR; }
 | T_MBRU               { $$ = MASM_REG_MBRU; }
 | T_MDR		{ $$ = MASM_REG_MDR; }
 | T_PC			{ $$ = MASM_REG_PC; }
 | T_SP			{ $$ = MASM_REG_SP; }
 | T_LV			{ $$ = MASM_REG_LV; }
 | T_CPP		{ $$ = MASM_REG_CPP; }
 | T_TOS		{ $$ = MASM_REG_TOS; }
 | T_OPC		{ $$ = MASM_REG_OPC; }
 | T_H		        { $$ = MASM_REG_H; }
 ;

vreg:
   reg                  { $$ = $1; }
 | T_N                  { $$ = MASM_REG_VIRTUAL; }
 | T_Z                  { $$ = MASM_REG_VIRTUAL; }
 ;

one: T_CONST { if ($$ != 1)
                 masm_warning ("illegal constant in arithmetic expression\n"); 
               $$ = $1;
             }
   ;

const: T_CONST { if ($1 != 1 && $1 != -1 && $1 != 0)
                   masm_warning ("illegal constant in arithmetic expression\n"); 
                 $$ = $1;
               }
 ;

newlines:
   newlines T_NEWLINE { current_line++; }
 | T_NEWLINE { current_line++; }
 ;

newlines_opt:
   newlines
 | 
 ;

%%

void yyerror (char *msg)
{
  extern char *yytext;

  masm_abort ("in line %d: parse error before `%s'\n", 
	      masm_parse_current_line (), yytext);
}

MasmLine *
masm_parse ()
{
  yyparse ();
  return top;
}

int
masm_parse_current_line ()
{
  return current_line;
}
