#include <stdlib.h>
#include "mic1-asm.h"
#include "mic1-util.h"

void
mic1_word_clear (Mic1Word word)
{
  int i;

  for (i = 0; i < 5; i++)
    word[i] = 0;
}

void
mic1_word_set_bits (Mic1Word word, unsigned int bits, int pos)
{
  int index;

  index = pos / 8;
  bits <<= pos & 7;
  while (bits != 0) {
    word[index] |= bits;
    bits >>= 8;
    index++;
  }
}

void
mic1_word_set_bit (Mic1Word word, int pos)
{
  mic1_word_set_bits (word, 1, pos);
}

unsigned int
mic1_word_get_bits (Mic1Word word, int pos, int size)
{
  int index, offset;
  unsigned result;

  index = pos / 8;
  offset = pos & 7;
  result = word[index] >> offset;
  index++;
  offset = 8 - offset;
  while (offset + 8 < pos + size) {
    result |= word[index] << offset;
    index++;
    offset += 8;
  }
  return result & ((1 << size) - 1);
}

int
mic1_word_get_bit (Mic1Word word, int pos)
{
  return mic1_word_get_bits (word, pos, 1);
}

void
mic1_word_read (Mic1Word word, char *buf)
{
  int i, c;
  char *p;

  for (p = buf, i = 4; i >= 0; i--, p += 2) {
    sscanf (p, "%02x", &c);
    word[i] = c;
  }
}

void
mic1_word_write (Mic1Word word, char *buf)
{
  int i;
  char *p;

  for (i = 4, p = buf; i >= 0; i--, p += 2)
    sprintf (p, "%02x", word[i]);
} 


void
mic1_word_disassemble (Mic1Word word, char *buf)
{
  int i, l, alu, b_bus, c_bus, address, need_alu;
  char *c_bit_names[] = {
    "MAR", "MDR", "PC", "SP", "LV", "CPP", "TOS", "OPC", "H"
  };
  char *b_bus_names[] = {
    "MDR", "PC", "MBR", "MBRU", "SP", "LV", "CPP", "TOS", "OPC"
  };

  l = 0;
  c_bus = FALSE;
  need_alu = FALSE;

  alu = mic1_word_get_bits (word, MIC1_WORD_INC_BIT, 6);
  b_bus = mic1_word_get_bits (word, MIC1_WORD_B_BUS_OFFSET,
			      MIC1_WORD_B_BUS_SIZE);
    
  if (b_bus == 15) {
    l += sprintf (buf + l, "halt");
    return;
  }

  for (i = MIC1_WORD_MAR_BIT; i <= MIC1_WORD_H_BIT; i++)
    if (mic1_word_get_bit (word, i)) {
      l += sprintf (buf + l, "%s = ", c_bit_names[i - MIC1_WORD_MAR_BIT]);
      c_bus = TRUE;
    }

  if (!c_bus && mic1_word_get_bit (word, MIC1_WORD_JAMZ_BIT)) {
    l += sprintf (buf + l, "Z = ");
    need_alu = TRUE;
  }
  else if (!c_bus && mic1_word_get_bit (word, MIC1_WORD_JAMN_BIT)) {
    l += sprintf (buf + l, "N = ");
    need_alu = TRUE;
  }

  if (c_bus || need_alu) {
    if (mic1_word_get_bit (word, MIC1_WORD_SRA1_BIT) ||
	mic1_word_get_bit (word, MIC1_WORD_SLL8_BIT))
      l += sprintf (buf + l, "(");

    switch (alu) {

    case MIC1_ALU_H:
      l += sprintf (buf + l, "H");
      break;

    case MIC1_ALU_B_BUS:
      l += sprintf (buf + l, "%s", b_bus_names[b_bus]);
      break;

    case MIC1_ALU_INV_H:
      l += sprintf (buf + l, "inv (H)");
      break;
   
    case MIC1_ALU_INV_B_BUS:
      l += sprintf (buf + l, "inv (%s)", b_bus_names[b_bus]);
      break;

    case MIC1_ALU_ADD_B_BUS_H:
      l += sprintf (buf + l, "H + %s", b_bus_names[b_bus]);
      break;

    case MIC1_ALU_ADD_B_BUS_H_1:
      l += sprintf (buf + l, "H + %s + 1", b_bus_names[b_bus]);
      break;

    case MIC1_ALU_ADD_H_1:
      l += sprintf (buf + l, "H + 1");
      break;

    case MIC1_ALU_ADD_B_BUS_1:
      l += sprintf (buf + l, "%s + 1", b_bus_names[b_bus]);
      break;

    case MIC1_ALU_SUB_B_BUS_H:
      l += sprintf (buf + l, "%s - H", b_bus_names[b_bus]);
      break;
    
    case MIC1_ALU_SUB_B_BUS_1:
      l += sprintf (buf + l, "%s - 1", b_bus_names[b_bus]);
      break;
    
    case MIC1_ALU_NEG_H:
      l += sprintf (buf + l, "-H");
      break;
    
    case MIC1_ALU_H_AND_B_BUS:
      l += sprintf (buf + l, "H and %s", b_bus_names[b_bus]);
      break;
    
    case MIC1_ALU_H_OR_B_BUS:
      l += sprintf (buf + l, "H or %s", b_bus_names[b_bus]);
      break;
    
    case MIC1_ALU_0:
      l += sprintf (buf + l, "0");
      break;
    
    case MIC1_ALU_1:
      l += sprintf (buf + l, "1");
      break;

    case MIC1_ALU_MINUS_1:
      l += sprintf (buf + l, "-1");
      break;
    
    default:
      l += sprintf (buf + l, "<unknown alu operation: 0x%02x>", alu);
    }

    if (mic1_word_get_bit (word, MIC1_WORD_SRA1_BIT))
      l += sprintf (buf + l, ") >> 1");
    if (mic1_word_get_bit (word, MIC1_WORD_SLL8_BIT))
      l += sprintf (buf + l, ") << 8");
    l += sprintf (buf + l, "; ");
  }

  if (mic1_word_get_bit (word, MIC1_WORD_WRITE_BIT))
    l += sprintf (buf + l, "wr; ");
  if (mic1_word_get_bit (word, MIC1_WORD_READ_BIT))
    l += sprintf (buf + l, "rd; ");
  if (mic1_word_get_bit (word, MIC1_WORD_FETCH_BIT))
    l += sprintf (buf + l, "fetch; ");

  address = mic1_word_get_bits (word, MIC1_WORD_ADDRESS_OFFSET,
				MIC1_WORD_ADDRESS_SIZE);
  if (mic1_word_get_bit (word, MIC1_WORD_JAMZ_BIT))
    l += sprintf (buf + l, "if (Z) goto 0x%03x; else goto 0x%03x;",
	    address | 0x100, address);
  else if (mic1_word_get_bit (word, MIC1_WORD_JAMN_BIT))
    l += sprintf (buf + l, "if (N) goto 0x%03x; else goto 0x%03x;",
	    address | 0x100, address);
  else if (mic1_word_get_bit (word, MIC1_WORD_JMPC_BIT)) {
    if (address == 0)
      l += sprintf (buf + l, "goto (MBR);");
    else
      l += sprintf (buf + l, "goto (MBR or 0x%03x);", address);
  }
  else
    l += sprintf (buf + l, "goto 0x%03x;", address);
}

/* The micro assembler uses mic1_store_write to write the contents of
 * the control store.  The file consists of 512 lines like this one:
 *
 *   0e7:  0123456789  <disassembled microinstruction>
 *
 * First entry is the address in the Mic1 control store, second entry
 * is the microinstruction, consisting of 10 hex digits, the first of
 * which is always 0.  The rest of the line is the microinstruction
 * disassembled.  Only the 10 hex digits are significant, the rest is
 * just provided for convenience.  */

void 
mic1_image_write (FILE *file, Mic1Image *image)
{
  char disassembled[256], word[64];
  int i;

  fprintf (file, "entry: %03x\n", image->entry);
  for (i = 0; i < 512; i++) {
    mic1_word_write (image->control_store[i], word);
    mic1_word_disassemble (image->control_store[i], disassembled);
    fprintf (file, "%03x:  %s  %s\n", i, word, disassembled);
  }
}
  
#define LINE_BUF_SIZE 256

Mic1Image *
mic1_image_load (FILE *file)
{
  char line[LINE_BUF_SIZE];
  Mic1Image *image;
  int i, fields, entry;

  fgets (line, LINE_BUF_SIZE, file);
  fields = sscanf (line, "entry: %03x", &entry);
  if (fields != 1)
    return NULL;

  image = malloc (sizeof (Mic1Image));
  image->entry = entry;
  for (i = 0; i < 512; i++) {
    fgets (line, LINE_BUF_SIZE, file);
    mic1_word_read (image->control_store[i], line + 6);
  }
  return image;
}
  
