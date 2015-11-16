#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mic1-util.h"
#include "ijvm-util.h"

typedef struct Mic1 Mic1;
struct Mic1 {
  int32 mar, mdr, pc, sp, lv, cpp, tos, opc, h;
  union { int8 mbr; uint8 mbru; } u;

  Mic1Word control_store[512];
  uint8 *mir;
  uint32 mpc;

  bool doing_rd, doing_fetch;
  uint8 *byte_store;
  int32 *word_store;

  /* This is the first address on the stack */
  uint32 stack_base;
};

typedef struct Mic1Breakpoint Mic1Breakpoint;
struct Mic1Breakpoint {
  int opcode;
  Mic1Breakpoint *next;
};

Mic1Breakpoint *mic1_breakpoint_list;
bool mic1_default_microtrace = FALSE, mic1_microtrace;

bool
mic1_breakpoint_add (char *mnemonic)
{
  int opcode;
  Mic1Breakpoint *bp;

  if (strcmp (mnemonic, "all") == 0) {
    mic1_default_microtrace = TRUE;
    return TRUE;
  }
  opcode = ijvm_get_opcode (mnemonic);
  if (opcode == -1)
    return FALSE;
  bp = malloc (sizeof (Mic1Breakpoint));
  bp->opcode = opcode;
  bp->next = mic1_breakpoint_list;
  mic1_breakpoint_list = bp;
  return TRUE;
}

bool
mic1_is_breakpoint (int opcode)
{
  Mic1Breakpoint *bp;

  if (mic1_default_microtrace)
    return TRUE;

  for (bp = mic1_breakpoint_list; bp != NULL; bp = bp->next)
    if (bp->opcode == opcode)
      return TRUE;
  return FALSE;
}

void
mic1_print_stack (Mic1 *m, int indent)
{
  int length;

  if (0 <= m->sp && m->sp < IJVM_MEMORY_SIZE / 4) {
    length = MIN (m->sp - m->stack_base, 8);
    ijvm_print_stack (m->word_store + m->sp, length, indent);
  }
  else
    printf ("SP out of range (SP = %d)\n", m->sp);
}

void
mic1_print_registers (Mic1 *m)
{
  printf ("  MAR=%d MDR=%d PC=%d MBR=%d MBRU=%d SP=%d "
	  "LV=%d CPP=%d TOS=%d OPC=%d H=%d\n\n",
	  m->mar, m->mdr, m->pc, m->u.mbr, m->u.mbru, m->sp,
	  m->lv, m->cpp, m->tos, m->opc, m->h);
}

void
mic1_print_state (Mic1 *m)
{
  static bool first_line = TRUE;

  if (mic1_microtrace)
    mic1_print_registers (m);
  if (mic1_word_get_bit (m->control_store[m->mpc], MIC1_WORD_JMPC_BIT)) {
    if (mic1_microtrace || first_line)
      mic1_print_stack (m, TRUE);
    else
      mic1_print_stack (m, FALSE);
    first_line = FALSE;
  }
}

void
mic1_print_instruction (Mic1 *m)
{
  char buf[80];

  /* Note: m->mir isn't valid here, since we print this before the cycle */

  if (mic1_word_get_bit (m->control_store[m->mpc], MIC1_WORD_JMPC_BIT)) {
    ijvm_print_snapshot (m->byte_store + m->pc);
    if (mic1_is_breakpoint (m->byte_store[m->pc])) {
      mic1_microtrace = TRUE;
      printf ("\n\n");
      mic1_print_registers (m);
    }
    else
      mic1_microtrace = FALSE;
  }

  if (mic1_microtrace) {
    mic1_word_disassemble (m->control_store[m->mpc], buf);
    printf ("0x%03x:  %s\n\n", m->mpc, buf);
  }
}

int
mic1_active (Mic1 *m)
{
  int b_bus;

  /* Note: m->mir isn't valid here, since we test this before the cycle */

  b_bus = mic1_word_get_bits (m->control_store[m->mpc], 
			      MIC1_WORD_B_BUS_OFFSET,
			      MIC1_WORD_B_BUS_SIZE);

  if (b_bus == 15)
    return FALSE;
  else
    return TRUE;
}

int
mic1_read_b_bus (Mic1 *m)
{
  int b_bus;

  b_bus = mic1_word_get_bits (m->mir, MIC1_WORD_B_BUS_OFFSET,
			      MIC1_WORD_B_BUS_SIZE);
  switch (b_bus) {

  case 0:
    return m->mdr;

  case 1:
    return m->pc;

  case 2:
    return m->u.mbr;

  case 3:
    return m->u.mbru;

  case 4:
    return m->sp;

  case 5:
    return m->lv;

  case 6:
    return m->cpp;

  case 7:
    return m->tos;

  case 8:
    return m->opc;

  default:
    return random ();
  }
}

int 
mic1_alu (int alu_bits, int h, int b_bus)
{
  switch (alu_bits) {

  case MIC1_ALU_H:
    return h;

  case MIC1_ALU_B_BUS:
    return b_bus;

  case MIC1_ALU_INV_H:
    return ~h;
   
  case MIC1_ALU_INV_B_BUS:
    return ~b_bus;

  case MIC1_ALU_ADD_B_BUS_H:
    return h + b_bus;

  case MIC1_ALU_ADD_B_BUS_H_1:
    return h + b_bus + 1;

  case MIC1_ALU_ADD_H_1:
    return h + 1;

  case MIC1_ALU_ADD_B_BUS_1:
    return b_bus + 1;

  case MIC1_ALU_SUB_B_BUS_H:
    return b_bus - h;
    
  case MIC1_ALU_SUB_B_BUS_1:
    return b_bus - 1;
    
  case MIC1_ALU_NEG_H:
    return -h;
    
  case MIC1_ALU_H_AND_B_BUS:
    return h & b_bus;
    
  case MIC1_ALU_H_OR_B_BUS:
    return h | b_bus;
    
  case MIC1_ALU_0:
    return 0;
    
  case MIC1_ALU_1:
    return 1;

  case MIC1_ALU_MINUS_1:
    return -1;
    
  default:
    return random ();
  }
}

void
mic1_write_c_bus (Mic1 *m, int value)
{
  int i;

  for (i = MIC1_WORD_MAR_BIT; i <= MIC1_WORD_H_BIT; i++)
    if (mic1_word_get_bit (m->mir, i)) {
      switch (i) {

      case MIC1_WORD_MAR_BIT:
	m->mar = value;
	break;

      case MIC1_WORD_MDR_BIT:
	m->mdr = value;
	break;

      case MIC1_WORD_PC_BIT:
	m->pc = value;
	break;

      case MIC1_WORD_SP_BIT:
	m->sp = value;
	break;

      case MIC1_WORD_LV_BIT:
	m->lv = value;
	break;

      case MIC1_WORD_CPP_BIT:
	m->cpp = value;
	break;

      case MIC1_WORD_TOS_BIT:
	m->tos = value;
	break;

      case MIC1_WORD_OPC_BIT:
	m->opc = value;
	break;

      case MIC1_WORD_H_BIT:
	m->h = value;
	break;
      }
    }
}

void
mic1_cycle (Mic1 *m)
{
  int alu_bits, b_bus, address, res, h;
  bool z_bit, n_bit;

  /* Set up signals to drive data path (Subcycle 1). */

  m->mir = m->control_store[m->mpc];

  alu_bits = mic1_word_get_bits (m->mir, 
				 MIC1_WORD_ALU_OFFSET, 
				 MIC1_WORD_ALU_SIZE);

  /* Drive H and B bus (Subcycle 2). */

  h = m->h;
  b_bus = mic1_read_b_bus (m);

  /* B bus and H stable, next up is ALU and shifter (Subcycle 3). */

  res = mic1_alu (alu_bits, m->h, b_bus);
  
  if (mic1_word_get_bit (m->mir, MIC1_WORD_SRA1_BIT)) {
    if (res < 0)
      res = ~(~res >> 1);
    else
      res = res >> 1;
  }
  if (mic1_word_get_bit (m->mir, MIC1_WORD_SLL8_BIT))
    res = res << 8;

  /* Rising edge of clock: load registers from C bus and MBR/MDR from
   * memory if previous cycle initiated a fetch/rd. 
   */

  if (m->doing_rd) {
    if (0 <= m->mar && m->mar < IJVM_MEMORY_SIZE / 4)
      m->mdr = m->word_store[m->mar];
    else
      m->mdr = 0;
    m->doing_rd = FALSE;
  }
  if (m->doing_fetch) {
    if (0 <= m->mar && m->mar < IJVM_MEMORY_SIZE)
      m->u.mbru = m->byte_store[m->pc];
    else
      m->u.mbru = 0;
    m->doing_fetch = FALSE;
  }
  mic1_write_c_bus (m, res);

  n_bit = (res < 0);
  z_bit = (res == 0);

  /* Initiate memory operations, if any, now that MAR and PC has been
   * loaded. */

  if (mic1_word_get_bit (m->mir, MIC1_WORD_WRITE_BIT) && 
      0 <= m->mar && m->mar < IJVM_MEMORY_SIZE / 4)
    m->word_store[m->mar] = m->mdr;

  if (mic1_word_get_bit (m->mir, MIC1_WORD_READ_BIT))
    m->doing_rd = TRUE;

  if (mic1_word_get_bit (m->mir, MIC1_WORD_FETCH_BIT)) {
    m->doing_fetch = TRUE;
  }

  /* Calculate new address.  This finishes during the clock high, but
   * after MBR/MDR are available (since MPC might depend on MBR).  So
   * this is an important exception: when initiating a fetch in cycle
   * k data is available in MBR in cycle k+2, for normal instructions
   * (eg. H = MBR << 8), but for goto (MBR), it's available in cycle
   * k+1.
   */
  
  address = mic1_word_get_bits (m->mir, MIC1_WORD_ADDRESS_OFFSET,
				MIC1_WORD_ADDRESS_SIZE);
  if (mic1_word_get_bit (m->mir, MIC1_WORD_JAMZ_BIT) && z_bit)
    address = address | 0x100;
  if (mic1_word_get_bit (m->mir, MIC1_WORD_JAMN_BIT) && n_bit)
    address = address | 0x100;
  if (mic1_word_get_bit (m->mir, MIC1_WORD_JMPC_BIT))
    address = address | m->u.mbru;

  m->mpc = address;
}

/* Construct a Mic1 simulator from a Mic1 image and a IJVM image. */

Mic1 *
mic1_new (Mic1Image *mic1_image, IJVMImage *ijvm_image,
	  int argc, char *argv[])
{
  Mic1 *m;
  int i;
  char *end_ptr;

  m = malloc (sizeof (Mic1));
  memset (m, 0, sizeof (Mic1));
  m->byte_store = malloc (IJVM_MEMORY_SIZE);
  m->word_store = (int32 *) m->byte_store;
  memset (m->byte_store, 0, IJVM_MEMORY_SIZE);

  if (ijvm_image != NULL) {
    m->h = ijvm_image->main_index;
    m->cpp = (ijvm_image->method_area_size + 3) / 4;
    m->sp = m->cpp + ijvm_image->cpool_size - 1;

    m->stack_base = m->sp;

    memcpy (m->byte_store, ijvm_image->method_area, 
	    ijvm_image->method_area_size);
    memcpy (m->word_store + m->cpp, ijvm_image->cpool,
	    ijvm_image->cpool_size * sizeof (int32));

    m->sp++;
    m->word_store[m->sp] = 42; //IJVM_INITIAL_OBJ_REF;
    for (i = 0; i < argc; i++) {
      m->sp++;
      m->word_store[m->sp] = strtol (argv[i], &end_ptr, 0);
      if (argv[i] == end_ptr) {
	printf ("Invalid argument to main method: `%s'\n", argv[i]);
	exit (-1);
      }
    }      
  }

  m->mpc = mic1_image->entry;
  memcpy (m->control_store, mic1_image->control_store, 
	  sizeof (m->control_store));

  m->doing_rd = FALSE;
  m->doing_fetch = FALSE;

  return m;
}

int 
main (int argc, char *argv[])
{
  FILE *mic1_file, *ijvm_file;
  Mic1Image *mic1_image;
  IJVMImage *ijvm_image;
  Mic1 *m;
  bool verbose, step;
  char *time_string;
  time_t t;

  ijvm_print_init (&argc, argv);

  verbose = TRUE;
  step = FALSE;

  while (argc > 1) {

    if (strcmp (argv[1], "-s") == 0) {
      verbose = FALSE;
      argv = argv + 1;
      argc = argc - 1;
      continue;
    }

    if (strcmp (argv[1], "-t") == 0) {
      step = TRUE;
      argv = argv + 1;
      argc = argc - 1;
      continue;
    }

    if (strcmp (argv[1], "-b") == 0) {
      if (argc > 2) {
	if (!mic1_breakpoint_add (argv[2])) {
	  fprintf (stderr, "Unknown instruction: `%s'\n", argv[2]);
	  exit (-1);
	}
      }
      else {
	fprintf (stderr, "Option -b requires an argument\n");
	exit (-1);
      }
      argv = argv + 2;
      argc = argc - 2;
      continue;
    }

    if (strcmp (argv[1], "-f") == 0) {
      argv = argv + 2;
      argc = argc - 2;
      continue;
    }

    if (strcmp (argv[1], "-v") == 0) {
      printf ("mic1 version " VERSION " compiled " 
	      COMPILE_DATE " on " COMPILE_HOST "\n");
      printf ("Default specification file is " IJVM_DATADIR "/ijvm.spec\n");
      exit (0);
    }    
    break;
  }

  if (argc < 2) {
    fprintf (stderr, "Usage: mic1 [OPTION] MIC1-FILENAME [IJVM-FILENAME PARAMETERS ...]\n\n");
    fprintf (stderr, "Where OPTION is\n\n");
    fprintf (stderr, "  -s            Silent mode.  No snapshot is produced.\n");
    fprintf (stderr, "  -f SPEC-FILE  The IJVM specification file to use.\n");
    fprintf (stderr, "  -t            Singlestep through microtrace.\n");
    fprintf (stderr, "  -v            Display version and build info.\n");
    fprintf (stderr, "  -b INSN       Show microtrace for the IJVM instruction INSN.\n\n");
    fprintf (stderr, "If you pass `-' as the Mic1 filename, the simulator will read the bytecode\nfile from stdin.\n\n");
    fprintf (stderr, "You must specify as many arguments as your IJVM main method requires,\n");
    fprintf (stderr, "except one; the simulator will pass the initial object reference for you.\n");
    exit (-1);
  }
    
  if (strcmp (argv[1], "-") == 0)
    mic1_file = stdin;
  else {
    mic1_file = fopen (argv[1], "r");
    if (mic1_file == NULL) {
      printf ("Could not open Mic1 file `%s'\n", argv[1]);
      exit (-1);
    }
  }
  mic1_image = mic1_image_load (mic1_file);
  fclose (mic1_file);

  if (mic1_image == NULL) {
    printf ("Could not read Mic1 file `%s'\n", argv[1]);
    exit (-1);
  }

  if (argc > 2) {
    ijvm_file = fopen (argv[2], "r");
    if (ijvm_file == NULL) {
      printf ("Could not open IJVM file `%s'\n", argv[2]);
      exit (-1);
    }
    ijvm_image = ijvm_image_load (ijvm_file);
    fclose (ijvm_file);
    m = mic1_new (mic1_image, ijvm_image, argc - 3, argv + 3);
  }
  else {
    m = mic1_new (mic1_image, NULL, 0, NULL);
    mic1_default_microtrace = TRUE;
  }

  if (verbose) {
    t = time (NULL);
    time_string = ctime (&t);
    if (argv[2] != NULL)
      printf ("Mic1 Trace of %s with %s %s\n", argv[1], argv[2], time_string);
    else
      printf ("Mic1 Trace of %s %s\n", argv[1], time_string);
  }

  mic1_microtrace = mic1_default_microtrace;

  /* This is the interpreter main loop.  It essentially excecutes
   * mic1_cycle until the program terminates.  We print the
   * disassembled Mic1 instruction, and if we see goto (MBR) we
   * disassemble the bytes starting at PC as an IJVM instruction */

  if (verbose)
    mic1_print_state (m);
  while (mic1_active (m)) {
    if (verbose)
      mic1_print_instruction (m);
    if (step && mic1_microtrace)
      getchar ();
    mic1_cycle (m);
    if (verbose)
      mic1_print_state (m);
  }
  if (verbose) {
    mic1_print_instruction (m);
    mic1_print_stack (m, FALSE);
  }

  printf ("return value: %d\n", m->tos);
  return 0;
}
