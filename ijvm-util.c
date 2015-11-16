#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "ijvm-spec.h"
#include "ijvm-util.h"

/* ijvm-util.c
 *
 * This file contains functions to disassemble and print IJVM
 * instructions as defined in the configuration file. */

static IJVMSpec *ijvm_spec;


IJVMImage *ijvm_image_new (uint16 main_index, 
			   uint8 *method_area, uint32 method_area_size,
			   int32 *cpool, uint32 cpool_size)
{
  IJVMImage *image;

  image = malloc (sizeof (IJVMImage));
  image->main_index = main_index;
  image->method_area = malloc (method_area_size);
  memcpy (image->method_area, method_area, method_area_size);
  image->method_area_size = method_area_size;
  image->cpool = malloc (cpool_size * sizeof (int32));
  memcpy (image->cpool, cpool, cpool_size * sizeof (int32));
  image->cpool_size = cpool_size;

  return image;
}

IJVMImage *
ijvm_image_load (FILE *file)
{
  IJVMImage *image;
  int j, fields;
  uint32 byte, method_area_size, cpool_size, main_index;
  int32 word;

  image = malloc (sizeof (IJVMImage));

  fields = fscanf (file, "main index: %d\n", &main_index);
  if (fields == 0) {
    printf ("Bytecode file not recognized\n");
    exit (-1);
  }
  image->main_index = main_index;
    
  fields = fscanf (file, "method area: %d bytes\n", &method_area_size);
  if (fields == 0) {
    printf ("Bytecode file not recognized\n");
    exit (-1);
  }

  image->method_area = malloc (method_area_size);
  image->method_area_size = method_area_size;
  for (j = 0; j < method_area_size; j++) {
    fscanf (file, "%x", &byte);
    image->method_area[j] = byte;
  }

  fields = fscanf (file, "\nconstant pool: %d words\n", &cpool_size);
  if (fields == 0) {
    printf ("Bytecode file not recognized\n");
    exit (-1);
  }

  image->cpool = malloc (cpool_size * sizeof(int32));
  image->cpool_size = cpool_size;
  for (j = 0; j < cpool_size; j++) {
    fscanf (file, "%x", &word);
    image->cpool[j] = word;
  }

  return image;
}

void
ijvm_image_write (FILE *file, IJVMImage *image)
{
  int i;

  fprintf (file, "main index: %d\n", image->main_index);
  fprintf (file, "method area: %d bytes\n", image->method_area_size);
  for (i = 0; i < image->method_area_size; i++) {
    fprintf (file, "%02x", image->method_area[i] & 255);
    if ((i & 15) == 15 && i < image->method_area_size - 1)
      fprintf (file, "\n");
    else
      fprintf (file, " ");
  }
  if ((i & 15) != 0)
    fprintf (file, "\n");
  fprintf (file, "constant pool: %d words\n", image->cpool_size);
  for (i = 0; i < image->cpool_size; i++)
    fprintf (file, "%08x\n", image->cpool[i]);
}

static void
fill (int length)
{
  int j;

  for (j = 0; j < length; j++)
    printf (" ");
}

/* Setup the terminal to use charater based I/O.  Pretty much derived
 * from an example in the GNU Lib C info pages.  Type:
 *
 *   info libc "low-level term"
 *
 * at the prompt to view them.  We silently ignore errors; either
 * stdin is redirected from a file and thus not a tty, which is ok,
 * or something else is wrong, which we just dont care about.
 */

static struct termios saved_term_attributes;

static void
reset_terminal (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_term_attributes);
}

void
ijvm_print_setup_terminal (void)
{
  struct termios attr;

  if (!isatty (STDIN_FILENO))
    return;
  tcgetattr (STDIN_FILENO, &saved_term_attributes);
  atexit (reset_terminal);

  tcgetattr (STDIN_FILENO, &attr);
  attr.c_lflag &= ~(ICANON | ECHO);
  attr.c_cc[VMIN] = 1;
  attr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &attr);
}

void
ijvm_print_init (int *argc, char *argv[])
{
  ijvm_spec = ijvm_spec_init (argc, argv);
  ijvm_print_setup_terminal ();
}

int
ijvm_get_opcode (char *mnemonic)
{
  IJVMInsnTemplate *tmpl;

  tmpl = ijvm_spec_lookup_template_by_mnemonic (ijvm_spec, mnemonic);
  if (tmpl == NULL)
    return -1;
  else
    return tmpl->opcode;
}

void
ijvm_print_stack (int32 *stack, int length, int indent)
{
  int i, *sp;

  if (indent)
    fill (32);

  printf ("stack = ");
  for (i = 0, sp = stack; i < length; i++, sp--)
    if (i == length - 1)
      printf ("%d", *sp);
    else
      printf ("%d, ", *sp);
  printf ("\n");
}

void
ijvm_print_opcodes (uint8 *opcodes, int length)
{
  int i;

  printf ("[");
  for (i = 0; i < length; i++)
    if (i == length - 1)
      printf ("%02x]  ", opcodes[i]);
    else
      printf ("%02x ", opcodes[i]);
  fill ((3 - length) * 3);
}

void
ijvm_print_snapshot (uint8 *opcodes)
{
  IJVMInsnTemplate *tmpl;
  uint8 opcode, byte;
  int8 sbyte;
  uint16 varnum, uword;
  int16 word;
  int j, length, index;


  opcode = opcodes[0];
  tmpl = ijvm_spec_lookup_template_by_opcode (ijvm_spec, opcode);
  if (tmpl == NULL) {
    printf ("unknown opcode: 0x%02x\n", opcode); 
    return;
  }

  length = printf ("%s ", tmpl->mnemonic);
  index = 1;

  for (j = 0; j < tmpl->noperands; j++) {
    if (j > 0)
      length += printf (", ");
    
    switch (tmpl->operands[j]) {
    case IJVM_OPERAND_BYTE:
      sbyte = opcodes[index];
      length += printf ("%d", sbyte);
      index += 1;
      break;

    case IJVM_OPERAND_LABEL:
      word = opcodes[index] * 256 + opcodes[index + 1];
      length += printf ("%d", word);
      index += 2;
      break;

    case IJVM_OPERAND_METHOD:
    case IJVM_OPERAND_CONSTANT:
      uword = opcodes[index] * 256 + opcodes[index + 1];
      length += printf ("%d", uword);
      index += 2;
      break;

    case IJVM_OPERAND_VARNUM:
      byte = opcodes[index];
      length += printf ("%d", byte);
      index += 1;
      break;

    case IJVM_OPERAND_VARNUM_WIDE:
      if (0) { /*FIXME*/
	varnum = opcodes[index] * 256 + opcodes[index + 1];
	length += printf ("%d", varnum);
	index += 2;
      }
      else {
	varnum = opcodes[index];
	length += printf ("%d", varnum);
	index += 1;
      }
      break;
    }
  }

  fill (20 - length);
  ijvm_print_opcodes (opcodes, index);
}
