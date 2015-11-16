/* Subliminal message to emacs: -*- mode:c -*- */

%{

#define YY_NO_UNPUT

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mic1-asm.h"
#include "mic1-parse.h"

static int lex_current_line = 1;

%}

ID     [a-zA-Z][a-zA-Z0-9_]*
CONST  (0x[0-9a-fA-F]+|[0-9]+)

%x str

%%

#.*             /* ignore */
;               return ';';
:               return ':';
=               return '=';
\+              return '+';
-               return '-';
"("             return '(';
")"             return ')';
"<<"            return T_LSHIFT;
">>"            return T_RSHIFT;
mar		return T_MAR;
mdr		return T_MDR;
pc		return T_PC;
mbr		return T_MBR;
mbru		return T_MBRU;
sp		return T_SP;
lv		return T_LV;
cpp		return T_CPP;
tos		return T_TOS;
opc		return T_OPC;
h		return T_H;
n		return T_N;
z		return T_Z;
if		return T_IF;
else		return T_ELSE;
goto		return T_GOTO;
wr		return T_WR;
rd		return T_RD;
fetch		return T_FETCH;
and		return T_AND;
or		return T_OR;
inv		return T_INV;
empty		return T_EMPTY;
halt		return T_HALT;
{ID}            yylval.id = masm_strdup (yytext); return T_LABEL;
{CONST}         yylval.integer = strtol (yytext, NULL, 0); return T_CONST;

\n              lex_current_line++; return T_NEWLINE;
[\t ]           /* ignore */;
.               masm_abort ("illegal character in input ('%c') line %d\n", yytext[0], lex_current_line);

%%

int yywrap(void)
{
  return 1;
}
