#include <stdlib.h> 	/* for malloc and atoi */
#include <stdio.h>      /* for FILE, fgetc, fputc, stdin, stdout, 
                         * fprintf, printf, fopen and fscanf */
#include <time.h>   	/* for time_t, time and ctime */
#include "ijvm-util.h"

typedef struct IJVM IJVM;
struct IJVM 
{
  uint32 sp, lv, pc, wide;
  int32 *stack;
  int32 *cpp;
  uint8 *method;

  uint32 initial_sp;
};

int8   ijvm_fetch_int8 (IJVM *i);
uint8  ijvm_fetch_uint8 (IJVM *i);
int16  ijvm_fetch_int16 (IJVM *i);
uint16 ijvm_fetch_uint16 (IJVM *i);
void   ijvm_push (IJVM *i, int32 word);
int32  ijvm_pop (IJVM *i);
void   ijvm_invoke_virtual (IJVM *i, uint16 index);
void   ijvm_ireturn (IJVM *i);
void   ijvm_execute_opcode (IJVM *i);
int    ijvm_active (IJVM *i);
IJVM  *ijvm_new (IJVMImage *image, int argc, char *argv[]);

int8
ijvm_fetch_int8 (IJVM *i)
{
  int8 byte;

  byte = i->method[i->pc];
  i->pc = i->pc + 1;
  return byte;
}

uint8
ijvm_fetch_uint8 (IJVM *i)
{
  uint8 byte;

  byte = i->method[i->pc];
  i->pc = i->pc + 1;
  return byte;
}

int16
ijvm_fetch_int16 (IJVM *i)
{
  int16 word;

  word = i->method[i->pc] * 256 + i->method[i->pc + 1];
  i->pc = i->pc + 2;
  return word;
}

uint16
ijvm_fetch_uint16 (IJVM *i)
{
  uint16 word;

  word = i->method[i->pc] * 256 + i->method[i->pc + 1];
  i->pc = i->pc + 2;
  return word;
}

void
ijvm_push (IJVM *i, int32 word)
{
  i->sp = i->sp + 1;
  i->stack[i->sp] = word;
}

int32
ijvm_pop (IJVM *i)
{
  int32 result;

  result = i->stack[i->sp];
  i->sp = i->sp - 1;

  return result;
}

void ijvm_invoke_builtin (IJVM *i, uint16 index)
{
  int c;

  switch (index) {
  case 0:
    ijvm_pop (i);  /* Remove object ref. from stack. */
    c = fgetc (stdin);
    if (c == EOF)
      ijvm_push (i, -1); /* Return -1 as end of file */
    else
      ijvm_push (i, c);  /* Place return value on stack */
    break;
  case 1:
    c = fputc (ijvm_pop (i), stdout);
    ijvm_pop (i);  /* Remove object ref. from stack. */
    ijvm_push (i, c);  /* Place return value on stack */
  }
}

void
ijvm_invoke_virtual (IJVM *i, uint16 index)
{
  uint32 address;
  uint16 nargs, nlocals;

  if (index >= 0x8000) {
    ijvm_invoke_builtin (i, index - 0x8000);
    return;
  }

  address = i->cpp[index];
  nargs = i->method[address] * 256 + i->method[address + 1];
  nlocals  = i->method[address + 2] * 256 + i->method[address + 3];

  i->sp += nlocals;
  ijvm_push (i, i->pc);
  ijvm_push (i, i->lv);
  i->lv = i->sp - nargs - nlocals - 1;
  i->stack[i->lv] = i->sp - 1;
  i->pc = address + 4;
}

void
ijvm_ireturn (IJVM *i)
{
  int linkptr;

  linkptr = i->stack[i->lv];
  i->stack[i->lv] = i->stack[i->sp]; /* Leave result on top of stack */
  i->sp = i->lv;
  i->pc = i->stack[linkptr];
  i->lv = i->stack[linkptr + 1];
}

void
ijvm_execute_opcode (IJVM *i)
{
  uint8 opcode;
  uint16 index, varnum;
  int16 offset;
  int32 a, b;
  uint32 opc;

  opc = i->pc;
  opcode = ijvm_fetch_uint8 (i);

  switch (opcode) {
  case IJVM_OPCODE_BIPUSH:
    /* The next byte is fetched as a signed 8 bit value and then
     * sign extended to 32 bits */
    ijvm_push (i, ijvm_fetch_int8 (i));
    break;

  case IJVM_OPCODE_DUP:
    ijvm_push (i, i->stack[i->sp]);
    break;

  case IJVM_OPCODE_GOTO:
    /* Fetch the next 2 bytes interpreted as a signed 16 bit offset
     * and add this to pc. */
    offset = ijvm_fetch_int16 (i); 
    i->pc = opc + offset;
    break;

  case IJVM_OPCODE_IADD:
    a = ijvm_pop (i);
    b = ijvm_pop (i);
    ijvm_push (i, a + b);
    break;

  case IJVM_OPCODE_IAND:
    a = ijvm_pop (i);
    b = ijvm_pop (i);
    ijvm_push (i, a & b);
    break;

  case IJVM_OPCODE_IFEQ:
    offset = ijvm_fetch_int16 (i);
    a = ijvm_pop (i);
    if (a == 0)
      i->pc = opc + offset;
    break;

  case IJVM_OPCODE_IFLT:
    offset = ijvm_fetch_int16 (i);
    a = ijvm_pop (i);
    if (a < 0)
      i->pc = opc + offset;
    break;

  case IJVM_OPCODE_IF_ICMPEQ:
    offset = ijvm_fetch_int16 (i);
    a = ijvm_pop (i);
    b = ijvm_pop (i);
    if (a == b)
      i->pc = opc + offset;
    break;

  case IJVM_OPCODE_IINC:
    varnum = ijvm_fetch_uint8 (i);
    a = ijvm_fetch_int8 (i);
    i->stack[i->lv + varnum] += a;
    break;

  case IJVM_OPCODE_ILOAD:
    if (i->wide)
      varnum = ijvm_fetch_uint16 (i);
    else
      varnum = ijvm_fetch_uint8 (i);
    ijvm_push (i, i->stack[i->lv + varnum]);
    break;

  case IJVM_OPCODE_INVOKEVIRTUAL:
    index = ijvm_fetch_uint16 (i);
    ijvm_invoke_virtual (i, index);
    break; 
    
  case IJVM_OPCODE_IOR:
    a = ijvm_pop (i);
    b = ijvm_pop (i);
    ijvm_push (i, a | b);
    break;
    
  case IJVM_OPCODE_IRETURN:
    ijvm_ireturn (i);
    break; 

  case IJVM_OPCODE_ISTORE:
    if (i->wide)
      varnum = ijvm_fetch_uint16 (i);
    else
      varnum = ijvm_fetch_uint8 (i);
    i->stack[i->lv + varnum] = ijvm_pop (i);
    break;

  case IJVM_OPCODE_ISUB:
    a = ijvm_pop (i);
    b = ijvm_pop (i);
    ijvm_push (i, b - a);
    break;

  case IJVM_OPCODE_LDC_W:
    index = ijvm_fetch_uint16 (i);
    ijvm_push (i, i->cpp[index]);
    break;

  case IJVM_OPCODE_NOP:
    break;

  case IJVM_OPCODE_POP:
    ijvm_pop (i);
    break;

  case IJVM_OPCODE_SWAP:
    a = i->stack[i->sp];
    i->stack[i->sp] = i->stack[i->sp - 1];
    i->stack[i->sp - 1] = a;
    break;

  case IJVM_OPCODE_WIDE:
    i->wide = TRUE;
    break;
  }
  
  if (opcode != IJVM_OPCODE_WIDE)
    i->wide = FALSE;
}

/* The IJVM is active as long as PC is different from INITIAL_PC. PC
 * only becomes INITIAL_PC when an `ireturn' from (the initial
 * invocation of) main is executed, and this terminates the
 * interpreter. */

int
ijvm_active (IJVM *i)
{
  return i->pc != IJVM_INITIAL_PC;
}

void
ijvm_print_result (IJVM *i)
{
  printf ("return value: %d\n", i->stack[i->sp]);
}

/* Initialize a new IJVM interpreter given a bytecode image.  The
 * entry point for the java bytecode program is the method main.  The
 * index in the constant pool of the address of main is specified in
 * the bytecode file in the first line; eg. `main index: 38'.  The
 * arguments given on the command line are converted to integers and
 * passed to main. */

IJVM *
ijvm_new (IJVMImage *image, int argc, char *argv[])
{
  IJVM *i;
  int main_offset, nargs, j;
  char *end_ptr;

  i = malloc (sizeof (IJVM));
  i->method = malloc (IJVM_MEMORY_SIZE);
  i->cpp = (int32 *) i->method + (image->method_area_size + 3) / 4;
  i->stack = (int32 *) i->method;
  memset (i->method, 0, IJVM_MEMORY_SIZE);

  i->sp = i->cpp + image->cpool_size - i->stack - 1;
  i->initial_sp = i->sp;
  i->lv = 0;
  i->pc = IJVM_INITIAL_PC;
  i->wide = FALSE;

  memcpy (i->method, image->method_area, image->method_area_size);
  memcpy (i->cpp, image->cpool, image->cpool_size * sizeof (int32));

  main_offset = i->cpp[image->main_index];
  /* Number of arguments to main */  
  nargs = i->method[main_offset] * 256 + i->method[main_offset + 1];

  /* Dont count argv[0], argv[1] or obj. ref. */
  if (argc - 2 != nargs - 1) {
    printf ("Incorrect number of arguments\n");
    exit (-1);
  }

  ijvm_push (i, IJVM_INITIAL_OBJ_REF);
  for (j = 0; j < nargs - 1; j++) {
    ijvm_push (i, strtol (argv[j + 2], &end_ptr, 0));
    if (argv[j + 2] == end_ptr) {
      printf ("Invalid argument to main method: `%s'\n", argv[j + 2]);
      exit (-1);
    }
  }      

  /* Initialize the IJVM by simulating a call to main */
  ijvm_invoke_virtual (i, image->main_index);

  return i;
}

int 
main (int argc, char *argv[])
{
  FILE *file;
  IJVMImage *image;
  IJVM *i;
  int verbose;
  char *time_string;
  time_t t;

  ijvm_print_init (&argc, argv);

  if (argc < 2) {
    fprintf (stderr, "Usage: ijvm [OPTION] FILENAME [PARAMETERS ...]\n\n");
    fprintf (stderr, "Where OPTION is\n\n");
    fprintf (stderr, "  -s            Silent mode.  No snapshot is produced.\n");
    fprintf (stderr, "  -f SPEC-FILE  The IJVM specification file to use.\n\n");
    fprintf (stderr, "If you pass `-' as the filename the simulator will read the bytecode\nfile from stdin.\n\n");
    fprintf (stderr, "You must specify as many arguments as your main method requires, except\n");
    fprintf (stderr, "one; the simulator will pass the initial object reference for you.\n");
    exit (-1);
  }

  verbose = TRUE;
  if (strcmp (argv[1], "-s") == 0) {
    verbose = FALSE;
    argv = argv + 1;
    argc = argc - 1;
  }

  if (strcmp (argv[1], "-") == 0)
    file = stdin;
  else
    file = fopen (argv[1], "r");
  if (file == NULL) {
    printf ("Could not open bytecode file `%s'\n", argv[1]);
    exit (-1);
  }
  image = ijvm_image_load (file);
  fclose (file);
  i = ijvm_new (image, argc, argv);

  if (verbose) {
    t = time (NULL);
    time_string = ctime (&t);
    printf ("IJVM Trace of %s %s\n", argv[1], time_string);
  }

  /* This is the interpreter main loop.  It essentially excecutes
   * ijvm_execute_opcode until the program terminates (which is when
   * an ireturn from main is encountered).  In each step, the
   * instruction, its arguments and the top 8 elements on the stack
   * are printed. */

  if (verbose)
    ijvm_print_stack (i->stack + i->sp, MIN (i->sp - i->initial_sp, 8), TRUE);
  while (ijvm_active (i)) {
    if (verbose)
      ijvm_print_snapshot (i->method + i->pc);
    ijvm_execute_opcode (i);
    if (verbose)
      ijvm_print_stack (i->stack + i->sp, MIN (i->sp - i->initial_sp, 8), FALSE);
  }

  ijvm_print_result (i);
  return 0;
}
