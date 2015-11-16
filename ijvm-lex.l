/* -*- mode: c -*- */

%{

#define YY_NO_UNPUT

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ijvm-asm.h"
#include "ijvm-parse.h"

int jasm_lex_lookup_id (char *token);
int jasm_lex_label (char *token);
int jasm_lex_parse_int (const char *token);

static int current_line = 1;

%}

ID  [a-zA-Z][a-zA-Z0-9_]*
NUM (0x)?[0-9a-fA-F]+

%%

[-():=+,]       return *yytext;
\.method        return T_METHOD;
\.locals        yylval.line_num = current_line; return T_LOCALS;
\.args          yylval.line_num = current_line; return T_ARGS;
\.define        yylval.line_num = current_line; return T_DEFINE;
{ID}            return jasm_lex_lookup_id (yytext);
{ID}:           return jasm_lex_label (yytext);
{NUM}           return jasm_lex_parse_int (yytext);
\n              current_line++;
[\t ]           /* ignore */;
"//".*          /* ignore */;
.               jasm_abort ("illegal character in input (`%c') line %d\n", yytext[0], current_line);

%%

int yywrap(void)
{
  return 1;
}

int
jasm_lex_current_line ()
{
  return current_line;
}

int
jasm_lex_lookup_id (char *token)
{
  IJVMInsnTemplate *tmpl;
  extern IJVMSpec *ijvm_spec;

  tmpl = ijvm_spec_lookup_template_by_mnemonic (ijvm_spec, token);
  if (tmpl == NULL) {
    yylval.symbol.value = strdup (yytext);
    yylval.symbol.line_num = current_line;
    return T_SYMBOL;
  }
  else {
    yylval.mnemonic.tmpl = tmpl;
    yylval.mnemonic.line_num = current_line;
    return T_MNEMONIC;
  }
}
      
int
jasm_lex_label (char *token)
{
  IJVMInsnTemplate *tmpl;
  extern IJVMSpec *ijvm_spec;
  int len;

  len = strlen (yytext);
  yylval.symbol.value = strdup (yytext);
  yylval.symbol.value[len - 1] = 0;
  yylval.symbol.line_num = current_line;

  tmpl = ijvm_spec_lookup_template_by_mnemonic (ijvm_spec, 
						yylval.symbol.value);
  if (tmpl != NULL)
    jasm_abort ("in line %d: attempt to use `%s' as label name\n", 
		jasm_lex_current_line (), yylval.symbol);
  
  return T_LABEL;
}
      

int
jasm_lex_parse_int (const char *token) 
{
  errno = 0;
  yylval.integer.value = strtoul (yytext, NULL, 0); 
  yylval.integer.line_num = current_line;

  if (errno == ERANGE) {
    jasm_abort ("in line %d: constant out of range (%s)\n", 
		jasm_lex_current_line (), token);     
    return 0; /* to shut up gcc */
  }
  else
    return T_INTEGER;
}
