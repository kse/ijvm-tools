#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ijvm-asm.h"
#include "ijvm-util.h"

static void 
jasm_log_ap (const char *fmt, va_list ap)
{
  vfprintf (stderr, fmt, ap);
}

void
jasm_abort (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  jasm_log_ap (fmt, ap);
  va_end (ap);
  exit (-1);
}

void
jasm_assert (int cond, const char *fmt, ...)
{
  va_list ap;

  if (!cond) {
    va_start (ap, fmt);
    jasm_log_ap (fmt, ap);
    va_end (ap);
    exit (-1);
  }
}

void
jasm_warning (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  jasm_log_ap (fmt, ap);
  va_end (ap);
}

char *
jasm_strdup (const char *str)
{
  int len;
  char *new;

  len = strlen (str);
  new = malloc (len + 1);
  strcpy (new, str);
  return new;
}

IJVMSpec *ijvm_spec;

int
main (int argc, char *argv[])
{
  IJVMImage *image;
  JasmMethod *methods;
  JasmCPool *cpool;
  char *spec_file;
  FILE *f;
  extern int yydebug;
  int size;

  if (argv[1] != NULL && strcmp (argv[1], "-v") == 0) {
    printf ("ijvm-asm version " VERSION " compiled " 
	    COMPILE_DATE " on " COMPILE_HOST "\n");
    printf ("Default specification file is " IJVM_DATADIR "/ijvm.spec\n");
    exit (0);
  }    

  if (getenv ("IJVM_ASM_DEBUG") != NULL)
    yydebug = 1;

  ijvm_spec = ijvm_spec_init (&argc, argv);

  if (argv[1] != NULL) {
    f = freopen (argv[1], "r", stdin);
    if (f == NULL)
      jasm_abort ("Couldn't open assembler file `%s'.\n", argv[1]);
  }

  if (argv[1] != NULL && argv[2] != NULL) {
    f = freopen (argv[2], "w", stdout);
    if (f == NULL)
      jasm_abort ("Couldn't open `%s' for writing.\n", argv[2]);
  }

  methods = jasm_parse ();
  cpool = jasm_cpool_make ();
  size = jasm_method_check (methods, cpool);

  image = jasm_emit (methods, cpool);
  ijvm_image_write (stdout, image);

  return 0;
}
