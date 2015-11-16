#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include "mic1-asm.h"

static int masm_warning_count = 0;

static void 
masm_log_ap (const char *fmt, va_list ap)
{
  vfprintf (stderr, fmt, ap);
}

void
masm_abort (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  masm_log_ap (fmt, ap);
  va_end (ap);
  exit (-1);
}

void
masm_assert (int cond, const char *fmt, ...)
{
  va_list ap;

  if (!cond) {
    va_start (ap, fmt);
    masm_log_ap (fmt, ap);
    va_end (ap);
    exit (-1);
  }
}

void
masm_warning (const char *fmt, ...)
{
  va_list ap;

  masm_warning_count++;
  va_start (ap, fmt);
  masm_log_ap (fmt, ap);
  va_end (ap);
}

void *masm_malloc (int size)
{
  void *p;

  p = malloc (size);
  masm_assert (p != NULL, "virtual memory exhausted\n");
  return p;
}

char *masm_strdup (char *s)
{
  char *new;

  new = masm_malloc (strlen (s) + 1);
  strcpy (new, s);
  return new;
}

int
main (int argc, char *argv[])
{
  MasmLine *lines, **store;
  FILE *f;
  extern int yydebug;
  int entry;

  if (argv[1] != NULL && strcmp (argv[1], "-v") == 0) {
    printf ("mic1-asm version " VERSION " compiled " 
	    COMPILE_DATE " on " COMPILE_HOST "\n");
    exit (0);
  }    

  if (getenv ("MASM_DEBUG") != NULL)
    yydebug = 1;

  if (argv[1] != NULL) {
    f = freopen (argv[1], "r", stdin);
    if (f == NULL)
      masm_abort ("Couldn't open assembler file `%s'\n", argv[1]);
  }

  if (argv[1] != NULL && argv[2] != NULL) {
    f = freopen (argv[2], "w", stdout);
    if (f == NULL)
      masm_abort ("Couldn't open `%s' for writing\n", argv[2]);
  }

  lines = masm_parse ();
  store = masm_layout_line (lines);
  masm_check_line (lines, NULL);
  entry = masm_lines_entry (lines);
  if (masm_warning_count > 0)
    masm_abort ("assembly aborted: %d error%s\n", masm_warning_count,
		masm_warning_count == 1 ? "" : "s");
  else
    masm_emit (store, entry);
  return 0;
}
