#include "mic1-asm.h"

/* Baisc idea: we do 3 passes over all the instructions -- in the
 * first pass we accommodate the absolute labels if possible, in the
 * second pass we lay out the targets for conditional jumps and in the
 * final pass we just throw in the remaining instructions, and resolve
 * label references.
 */

static MasmLine *mic_store[512];

typedef struct MasmLabelList MasmLabelList;
struct MasmLabelList {
  MasmLine *line;
  MasmLabelList *next;
};

static MasmLabelList *masm_labels = NULL;

void
masm_add_label (MasmLine *line)
{
  MasmLabelList *l;

  l = masm_malloc (sizeof (MasmLabelList));
  l->line = line;
  l->next = masm_labels;
  masm_labels = l;
}

MasmLine *
masm_lookup_label (char *label)
{
  MasmLabelList *l;

  for (l = masm_labels; l != NULL; l = l->next)
    if (strcasecmp (label, l->line->label->name) == 0)
      return l->line->label->line;
  return NULL;
}

void
masm_find_labels (MasmLine *lines, MasmLine *prev)
{
  MasmLine *line;

  if (lines != NULL) {
    if (lines->insns != NULL)
      line = lines;
    else
      line = prev;
    masm_find_labels (lines->next, line);
    if (lines->label != NULL) {
      lines->label->line = line;
      masm_add_label (lines);
    }
  }    
}

void
masm_line_layout (MasmLine *line, int address)
{
  line->laid_out = TRUE;
  line->address = address;
  mic_store[address] = line;
}

void
masm_layout_absolutes ()
{
  MasmLabelList *l;
  MasmLabel *label;

  for (l = masm_labels; l != NULL; l = l->next) {
    label = l->line->label;
    if (label->absolute) {
      if (label->address < 512) {
	if (mic_store[label->address] != NULL)
	  masm_warning ("in line %d: absolute label clash: address 0x%02x"
			" is claimed by %s and %s\n", 
			l->line->line_number,
			label->address, label->name, 
			mic_store[label->address]->label->name);
	if (!label->line->laid_out)
	  masm_line_layout (label->line, label->address);
      }
      else
	masm_warning ("in line %d: "
		      "absolute labels must be in the range 0-511\n",
		      l->line->line_number);
    }
  }
}    

MasmInsn *
masm_find_if (MasmLine *line)
{
  MasmInsn *i;

  for (i = line->insns; i != NULL; i = i->next)
    if (i->kind == MASM_INSN_IF)
      return i;
  return NULL;
}

int
masm_find_address (int first)
{
  int i;

  for (i = first; i < 512; i++)
    if (mic_store[i] == NULL)
      return i;
  if (i >= 512)
    masm_warning ("control store exhausted\n");
  return 0;
}

int
masm_find_address_pair (int first)
{
  int i;

  for (i = first; i < 256; i++)
    if (mic_store[i] == NULL && mic_store[i + 256] == NULL)
      return i;
  if (i == 256)
    masm_warning ("control store exhausted\n");
  return 0;
}

int
masm_layout_jumps (MasmLine *lines)
{
  int i, first;
  MasmInsn *insn;
  MasmLine *target1, *target2;

  if (lines == NULL) 
    return 0;
  else {
    first = masm_layout_jumps (lines->next);
    insn = masm_find_if (lines);
    if (insn != NULL) {
      target1 = masm_lookup_label (insn->u.insn_if.label1);
      if (target1 == NULL)
	masm_warning ("in line %d: label `%s' undefined\n", 
		      lines->line_number, insn->u.insn_if.label1);
      target2 = masm_lookup_label (insn->u.insn_if.label2);
      if (target2 == NULL)
	masm_warning ("in line %d: label `%s' undefined\n", 
		      lines->line_number, insn->u.insn_if.label2);
      if (target1 == NULL || target2 == NULL)
	return first;

      if (target1->laid_out && target2->laid_out) {
	if (target1->address != target2->address + 0x100)
	  masm_warning ("in line %d: invalid branch targets\n",
			lines->line_number);
      }
      else if (target1->laid_out) {
	if (target1->address < 0x100)
	  masm_warning ("in line %d: "
			"target for true branch must be placed above 0x100\n",
			lines->line_number, insn->u.insn_if.label1);
	else if (mic_store[target1->address - 0x100] != NULL) 
	  masm_warning ("in line %d: address of target for false branch "
			"is already occupied (0x%03x)\n", 
			lines->line_number, target1->address - 0x100);
	else
	  masm_line_layout (target2, target1->address - 0x100);
      }
      else if (target2->laid_out) {
	if (target2->address >= 0x100)
	  masm_warning ("in line %d: target for false branch must be placed "
			"below 0x100\n", lines->line_number);
	else if (mic_store[target2->address + 0x100] != NULL)
	  masm_warning ("in line %d: address of target for true branch is "
			"already occupied (0x%03x)\n", 
			lines->line_number, target2->address + 0x100);
	else
	  masm_line_layout (target1, target2->address + 0x100);
      }
      else {
	i = masm_find_address_pair (first);
	masm_line_layout (target1, i + 0x100);
	masm_line_layout (target2, i);
	return i + 1;
      }
    }
    return first;
  }
}

int
masm_layout_rest (MasmLine *lines)
{
  int i, first;
  
  if (lines == NULL) 
    return 0;
  else {
    first = masm_layout_rest (lines->next);
    if (lines->insns != NULL && !lines->laid_out) {
      i = masm_find_address (first);
      masm_line_layout (lines, i);
      return i + 1;
    }
    else
      return first;
  }
}

MasmLine **
masm_layout_line (MasmLine *lines)
{
  masm_find_labels (lines, NULL);
  masm_layout_absolutes (lines, NULL);
  masm_layout_jumps (lines);
  masm_layout_rest (lines);

  return mic_store;
}
