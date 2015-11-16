#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "ijvm-spec.h"

#define MAX_LINESIZE 80
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

IJVMSpec *
ijvm_spec_new ()
{
  IJVMSpec *spec;

  spec = malloc (sizeof (IJVMSpec));
  spec->templates = NULL;
  spec->ntemplates = 0;
  spec->allocation = 0;

  return spec;
}    

void
ijvm_spec_add_template (IJVMSpec *spec, IJVMInsnTemplate *tmpl)
{
  if (spec->ntemplates == spec->allocation) {
    spec->allocation = MAX (16, spec->allocation * 2);
    spec->templates = realloc (spec->templates, 
			       spec->allocation * sizeof (IJVMInsnTemplate *));
  }
  spec->templates[spec->ntemplates] = tmpl;
  spec->ntemplates++;
}

IJVMInsnTemplate *
ijvm_spec_lookup_template_by_mnemonic (IJVMSpec *spec, char *mnemonic)
{
  int i;

  for (i = 0; i < spec->ntemplates; i++)
    if (strcasecmp (mnemonic, spec->templates[i]->mnemonic) == 0)
      return spec->templates[i];
  return NULL;
}

IJVMInsnTemplate *
ijvm_spec_lookup_template_by_opcode (IJVMSpec *spec, int opcode)
{
  int i;

  for (i = 0; i < spec->ntemplates; i++)
    if (opcode == spec->templates[i]->opcode)
      return spec->templates[i];
  return NULL;
}

static struct { char *name; IJVMOperandKind kind; } operand_kinds[] =
{
  { "byte",        IJVM_OPERAND_BYTE },
  { "label",       IJVM_OPERAND_LABEL },
  { "method",      IJVM_OPERAND_METHOD },
  { "varnum",      IJVM_OPERAND_VARNUM },
  { "varnum-wide", IJVM_OPERAND_VARNUM_WIDE },
  { "constant",    IJVM_OPERAND_CONSTANT },
  { NULL, 0 }
};

static IJVMOperandKind
operand_name_to_kind (char *name)
{
  int i;

  for (i = 0; operand_kinds[i].name != NULL; i++)
    if (strcasecmp (name, operand_kinds[i].name) == 0)
      return operand_kinds[i].kind;

  fprintf (stderr, "Unknown operand type: `%s'\n", name);
  exit (-1);
  return 0;
}

static char *
operand_kind_to_name (IJVMOperandKind kind)
{
  return operand_kinds[kind].name;
}

static int line_number = 1;

/* Parse identifier from the string str, and strdup it into *id.
 * Return pointer to next non-whitespace char following the
 * identifier.
 */
static char *get_identifier (char *str, char **id)
{
  char *p;
  int len;

  p = str;
  while (isalnum ((int) *p) || *p == '-' || *p == '_')
    p++;
  if (p == str || !isalpha ((int) *str)) {
    fprintf (stderr, "Specification file parse error in line %d\n", 
	     line_number);
    exit (-1);
  }

  if (id != NULL) {
    len = p - str;
    *id = malloc (len + 1);
    strncpy (*id, str, len);
    (*id)[len] = 0;
  }

  while (*p && isspace ((int) *p))
    p++;

  return p;
}

/* Parse integer from the string str, and assign it to *i.  Return
 * pointer to next non-whitespace char following the integer.
 */
static char *get_int (char *str, int *i)
{
  char *p; 

  *i = strtol (str, &p, 0);
  if (p == str) {
    fprintf (stderr, "Specification file parse error in line %d\n", 
	     line_number);
    exit (-1);
  }

  while (*p && isspace ((int) *p))
    p++;

  return p;
}

static char *check_char (char *str, int c)
{
  char *p;

  if (*str == 0)
    return str;

  /* If we see a '#', then skip to end of line */
  if (*str == '#')
    return str + strlen (str);

  if (*str != c) {
    fprintf (stderr, "Specification file parse error in line %d\n", 
	     line_number);
    exit (-1);
  }

  p = str + 1;
  while (*p && isspace ((int) *p))
    p++;

  return p;
}

static int is_blank_line (char *str)
{
  int i;

  for (i = 0; str[i]; i++) {
    if (str[i] == '#')
      return 1;
    if (!isspace ((int) str[i]))
      return 0;
  }
  return 1;
}

IJVMInsnTemplate *
ijvm_insn_template_new (void)
{
  IJVMInsnTemplate *tmpl;

  tmpl = malloc (sizeof (IJVMInsnTemplate));
  tmpl->opcode = 0;
  tmpl->mnemonic = NULL;
  tmpl->operands = NULL;
  tmpl->noperands = 0;
  tmpl->nalloc = 0;

  return tmpl;  
}

void
ijvm_insn_template_add_operand (IJVMInsnTemplate *tmpl, IJVMOperandKind kind)
{
  if (tmpl->noperands == tmpl->nalloc) {
    tmpl->nalloc = MAX (4, tmpl->nalloc * 2);
    tmpl->operands = realloc (tmpl->operands, 
			      tmpl->nalloc * sizeof (IJVMOperandKind));
  }
  tmpl->operands[tmpl->noperands] = kind;
  tmpl->noperands++;
}

void
ijvm_spec_print (IJVMSpec *spec)
{
  int i, j;

  for (i = 0; i < spec->ntemplates; i++) {
    printf ("0x%2x %-13s ", 
	    spec->templates[i]->opcode, 
	    spec->templates[i]->mnemonic);
    for (j = 0; j < spec->templates[i]->noperands; j++)
      printf ("%s%s", j ? ", " : "", 
	      operand_kind_to_name (spec->templates[i]->operands[j]));
    printf ("\n");
  }
}

IJVMSpec *
ijvm_spec_parse (FILE *f)
{
  IJVMSpec *spec;
  IJVMInsnTemplate *tmpl;
  char buffer[MAX_LINESIZE], *p, *kind_name;
  IJVMOperandKind kind;

  spec = ijvm_spec_new ();
  while (fgets (buffer, MAX_LINESIZE, f)) {
    if (is_blank_line (buffer))
      continue;
    tmpl = ijvm_insn_template_new ();
    p = get_int (buffer, &tmpl->opcode);
    p = get_identifier (p, &tmpl->mnemonic);
    
    while (*p) {
      p = get_identifier (p, &kind_name);
      kind = operand_name_to_kind (kind_name);
      ijvm_insn_template_add_operand (tmpl, kind);
      p = check_char (p, ',');
    }
    ijvm_spec_add_template (spec, tmpl);
    line_number++;
  }

  return spec;
}

/* Search command line for `-f' option, then look in environment
 * variable IJVM_SPEC_FILE and eventually fall back on compiled in
 * default in order to determine name of spec file.  The parse the
 * file and return the specification.
 */

IJVMSpec *
ijvm_spec_init (int *argc, char *argv[])
{
  FILE *f;
  IJVMSpec *spec;
  char *spec_file;
  int i;

  spec_file = NULL;
  for (i = 1; i < *argc; i++) {
    if (strcmp (argv[i], "-f") == 0) {
      if (i + 1 < *argc) {
	spec_file = argv[i + 1];
	while (i + 2 < *argc) {
	  argv[i] = argv[i + 2];
	  i++;
	}
	*argc -= 2;
	argv[i] = NULL;
      }
      else {
	fprintf (stderr, "Option -f requires an argument\n");
	exit (-1);
      }
      break;
    }
  }

  if (spec_file == NULL) {
    if (getenv ("IJVM_SPEC_FILE") != NULL)
      spec_file = getenv ("IJVM_SPEC_FILE");
    else
      spec_file = IJVM_DATADIR "/ijvm.spec";
  }

  f = fopen (spec_file, "r");
  if (f == NULL) {
    fprintf (stderr, "Couldn't read specification file `%s'.\n", spec_file);
    exit (-1);
  }

  spec = ijvm_spec_parse (f);
  fclose (f);

  return spec;
}
