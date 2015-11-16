#include "mic1-asm.h"
#include "mic1-util.h"

void masm_check_insn (MasmLine *l, MasmLine *prev);
void masm_check_assign (MasmAssign *assign, MasmLine *l);
char *masm_reg_name (enum MasmRegKind reg);
int masm_reg_c_bit (enum MasmRegKind reg);
int masm_reg_b_bus (enum MasmRegKind reg, int line_number);
char *masm_b_bus_name (int val);
void masm_check_alu (MasmAlu *alu, MasmLine *l);

void
masm_check_line (MasmLine *lines, MasmLine *prev)
{
  MasmLine *line;

  if (lines != NULL) {
    if (lines->insns != NULL)
      line = lines;
    else
      line = prev;
    masm_check_line (lines->next, line);
    masm_check_insn (lines, prev);
  }
}

void
masm_check_insn (MasmLine *l, MasmLine *prev)
{
  MasmInsn *insn;
  MasmLine *target;
  int saw_goto, saw_assign;

  saw_goto = FALSE;
  saw_assign = FALSE;
  mic1_word_clear (l->word);

  for (insn = l->insns; insn != NULL; insn = insn->next) {
    
    switch (insn->kind) {

    case MASM_INSN_RD:
      if (mic1_word_get_bit (l->word, MIC1_WORD_WRITE_BIT))
	masm_warning ("in line %d: only one of RD and WR allowed pr. line\n",
		      l->line_number);
      if (mic1_word_get_bit (l->word, MIC1_WORD_READ_BIT))
	masm_warning ("in line %d: duplicate RD on line\n",
		      l->line_number);
      mic1_word_set_bit (l->word, MIC1_WORD_READ_BIT);
      break;

    case MASM_INSN_WR:
      if (mic1_word_get_bit (l->word, MIC1_WORD_READ_BIT))
	masm_warning ("in line %d: only one of RD and WR allowed pr. line\n",
		      l->line_number);
      if (mic1_word_get_bit (l->word, MIC1_WORD_WRITE_BIT))
	masm_warning ("in line %d: duplicate WR on line\n",
		      l->line_number);
      mic1_word_set_bit (l->word, MIC1_WORD_WRITE_BIT);
      break;

    case MASM_INSN_FETCH:
      if (mic1_word_get_bit (l->word, MIC1_WORD_FETCH_BIT))
	masm_warning ("in line %d: duplicate FETCH on line\n",
		      l->line_number);
      mic1_word_set_bit (l->word, MIC1_WORD_FETCH_BIT);
      break;

    case MASM_INSN_GOTO:
      if (saw_goto)
	masm_warning ("in line %d: only one goto allowed pr. line\n",
		      l->line_number);
      saw_goto = TRUE;
      target = masm_lookup_label (insn->u.insn_goto.label);
      if (target != NULL)
	mic1_word_set_bits (l->word, target->address, 
			    MIC1_WORD_ADDRESS_OFFSET);
      else
	masm_warning ("in line %d: label `%s' undefined\n",
		      l->line_number, insn->u.insn_goto.label);
      break;

    case MASM_INSN_IGOTO:
      if (saw_goto)
	masm_warning ("in line %d: only one goto allowed pr. line\n",
		      l->line_number);
      saw_goto = TRUE;
      mic1_word_set_bits (l->word, insn->u.insn_igoto.next_address,
			  MIC1_WORD_ADDRESS_OFFSET);
      mic1_word_set_bit (l->word, MIC1_WORD_JMPC_BIT);
      break;

    case MASM_INSN_IF:
      if (saw_goto)
	masm_warning ("in line %d: only one goto allowed pr. line\n",
		      l->line_number);
      saw_goto = TRUE;
      target = masm_lookup_label (insn->u.insn_if.label2);
      if (target != NULL) {
	mic1_word_set_bits (l->word, target->address, 
			    MIC1_WORD_ADDRESS_OFFSET);
      }
      if (insn->u.insn_if.cond == MASM_COND_Z)
	mic1_word_set_bit (l->word, MIC1_WORD_JAMZ_BIT);
      else
	mic1_word_set_bit (l->word, MIC1_WORD_JAMN_BIT);
      break;

    case MASM_INSN_ASSIGN:
      if (saw_assign)
	masm_warning ("in line %d: only one alu operation allowed pr. line\n",
		      l->line_number);
      saw_assign = TRUE;
      masm_check_assign (insn->u.insn_assign, l);
      break;

    case MASM_INSN_EMPTY:
      if (insn != l->insns || insn->next != NULL)
	masm_warning ("in line %d: empty is only allowed on a line by itself\n",
		      l->line_number);
      break;

    case MASM_INSN_HALT:
      if (insn != l->insns || insn->next != NULL)
	masm_warning ("in line %d: halt is only allowed on a line by itself\n",
		      l->line_number);
      mic1_word_set_bits (l->word, 15, MIC1_WORD_B_BUS_OFFSET);
      saw_goto = TRUE;
    break;
    }

  }

  if (l->insns != NULL && !saw_goto) {
    if (prev == NULL) 
      masm_warning ("in line %d: last line should terminate "
		    "with an explicit goto or halt\n",
		    l->line_number);
    else
      mic1_word_set_bits (l->word, prev->address,
			  MIC1_WORD_ADDRESS_OFFSET);
  }
}

void
masm_check_assign_target (int target, MasmLine *l)
{
  switch (target) {
  case MASM_REG_MBR:
  case MASM_REG_MBRU:
    masm_warning ("in line %d: MBR and MBRU can not be assigned to\n",
		  l->line_number);
    break;
  case MASM_REG_VIRTUAL:
    break;
  default:
    if (mic1_word_get_bit (l->word, masm_reg_c_bit (target)))
      masm_warning ("in line %d: register %s assigned more than once\n", 
		    l->line_number, masm_reg_name (target));
    else
      mic1_word_set_bit (l->word, masm_reg_c_bit (target));
  }
}

void
masm_check_assign (MasmAssign *assign, MasmLine *l)
{
  switch (assign->kind) {
  case MASM_ASSIGN_BASIC:
    masm_check_assign_target (assign->u.assign_basic.reg, l);
    masm_check_alu (assign->u.assign_basic.alu, l);
    break;
  case MASM_ASSIGN_CHAIN:
    masm_check_assign_target (assign->u.assign_chain.reg, l);
    masm_check_assign (assign->u.assign_chain.assign, l);
    break;
  case MASM_ASSIGN_RSHIFT1:
    masm_check_assign_target (assign->u.assign_rshift1.reg, l);
    masm_check_alu (assign->u.assign_rshift1.alu, l);
    mic1_word_set_bit (l->word, MIC1_WORD_SRA1_BIT);
    break;
  case MASM_ASSIGN_LSHIFT8:
    masm_check_assign_target (assign->u.assign_lshift8.reg, l);
    masm_check_alu (assign->u.assign_lshift8.alu, l);
    mic1_word_set_bit (l->word, MIC1_WORD_SLL8_BIT);
    break;
  }
}

void
masm_check_alu (MasmAlu *alu, MasmLine *l)
{
  int reg;

  switch (alu->kind) {
  case MASM_ALU_REG:
    if (alu->u.alu_reg.reg == MASM_REG_H)
      mic1_word_set_bits (l->word, MIC1_ALU_H, MIC1_WORD_ALU_OFFSET);
    else {
      mic1_word_set_bits (l->word, MIC1_ALU_B_BUS, MIC1_WORD_ALU_OFFSET);
      mic1_word_set_bits (l->word, masm_reg_b_bus (alu->u.alu_reg.reg, 
						   l->line_number),
			  MIC1_WORD_B_BUS_OFFSET);
    }
    break;

  case MASM_ALU_INV:
    if (alu->u.alu_inv.reg == MASM_REG_H)
      mic1_word_set_bits (l->word, MIC1_ALU_INV_H, MIC1_WORD_ALU_OFFSET);
    else {
      mic1_word_set_bits (l->word, MIC1_ALU_INV_B_BUS, 
			  MIC1_WORD_ALU_OFFSET);
      mic1_word_set_bits (l->word, masm_reg_b_bus (alu->u.alu_inv.reg, 
						   l->line_number),
			  MIC1_WORD_B_BUS_OFFSET);
    }
    break;

  case MASM_ALU_ADD_RR1:
    if (alu->u.alu_add_rr1.reg1 == MASM_REG_H &&
	alu->u.alu_add_rr1.reg2 != MASM_REG_H)
      reg = alu->u.alu_add_rr1.reg2;
    else if (alu->u.alu_add_rr1.reg1 != MASM_REG_H &&
	     alu->u.alu_add_rr1.reg2 == MASM_REG_H)
      reg = alu->u.alu_add_rr1.reg1;
    else {
      masm_warning ("in line %d: one of the operand registers must be H\n",
		    l->line_number);
      break;
    }
    mic1_word_set_bits (l->word, MIC1_ALU_ADD_B_BUS_H_1, 
			MIC1_WORD_ALU_OFFSET);
    mic1_word_set_bits (l->word, masm_reg_b_bus (reg, l->line_number),
			MIC1_WORD_B_BUS_OFFSET);
    break;

  case MASM_ALU_ADD_RR:
    if (alu->u.alu_add_rr.reg1 == MASM_REG_H &&
	alu->u.alu_add_rr.reg2 != MASM_REG_H)
      reg = alu->u.alu_add_rr.reg2;
    else if (alu->u.alu_add_rr.reg1 != MASM_REG_H &&
	     alu->u.alu_add_rr.reg2 == MASM_REG_H) 
      reg = alu->u.alu_add_rr.reg1; 
    else {
      masm_warning ("in line %d: one of the operand registers must be H\n",
		    l->line_number);
      break;
    }
    mic1_word_set_bits (l->word, MIC1_ALU_ADD_B_BUS_H, 
			MIC1_WORD_ALU_OFFSET);
    mic1_word_set_bits (l->word, masm_reg_b_bus (reg, l->line_number),
			MIC1_WORD_B_BUS_OFFSET);
    break;

  case MASM_ALU_ADD_R1:
    if (alu->u.alu_add_r1.reg == MASM_REG_H)
      mic1_word_set_bits (l->word, MIC1_ALU_ADD_H_1, MIC1_WORD_ALU_OFFSET);
    else {
      mic1_word_set_bits (l->word, MIC1_ALU_ADD_B_BUS_1, 
			  MIC1_WORD_ALU_OFFSET);
      mic1_word_set_bits (l->word, masm_reg_b_bus (alu->u.alu_reg.reg, 
						   l->line_number),
			  MIC1_WORD_B_BUS_OFFSET);
    }
    break;

  case MASM_ALU_SUB_RR:
    if (alu->u.alu_sub_rr.reg1 != MASM_REG_H &&
	alu->u.alu_sub_rr.reg2 == MASM_REG_H) {
      mic1_word_set_bits (l->word, MIC1_ALU_SUB_B_BUS_H, 
			  MIC1_WORD_ALU_OFFSET);
      mic1_word_set_bits (l->word, masm_reg_b_bus (alu->u.alu_sub_r1.reg, 
						   l->line_number),
			  MIC1_WORD_B_BUS_OFFSET);
    }
    else
      masm_warning ("in line %d: subtractions must be either `reg - H' or `reg - 1'\n",
		    l->line_number);
    break;

  case MASM_ALU_SUB_R1:
    if (alu->u.alu_sub_r1.reg != MASM_REG_H) {
      mic1_word_set_bits (l->word, MIC1_ALU_SUB_B_BUS_1, 
			  MIC1_WORD_ALU_OFFSET);
      mic1_word_set_bits (l->word, masm_reg_b_bus (alu->u.alu_sub_r1.reg, 
						   l->line_number),
			  MIC1_WORD_B_BUS_OFFSET);
    }
    else
      masm_warning ("in line %d: subtractions must be either `reg - H' or `reg - 1'\n",
		    l->line_number);

    break;

  case MASM_ALU_NEG:
    if (alu->u.alu_neg.reg == MASM_REG_H)
      mic1_word_set_bits (l->word, MIC1_ALU_NEG_H, MIC1_WORD_ALU_OFFSET);
    else
      masm_warning ("in line %d: only H can be negated\n",
		    l->line_number);
    break;

  case MASM_ALU_AND:
    if (alu->u.alu_and.reg1 == MASM_REG_H &&
	alu->u.alu_and.reg2 != MASM_REG_H)
      reg = alu->u.alu_and.reg2;
    else if (alu->u.alu_and.reg1 != MASM_REG_H &&
	     alu->u.alu_and.reg2 == MASM_REG_H)
      reg = alu->u.alu_and.reg1;
    else {
      masm_warning ("in line %d: one of the operand registers must be H\n",
		    l->line_number);
      break;
    }
    mic1_word_set_bits (l->word, MIC1_ALU_H_AND_B_BUS, 
			MIC1_WORD_ALU_OFFSET);
    mic1_word_set_bits (l->word, masm_reg_b_bus (reg, l->line_number), 
			MIC1_WORD_B_BUS_OFFSET);
    break;

  case MASM_ALU_OR:
    if (alu->u.alu_or.reg1 == MASM_REG_H &&
	alu->u.alu_or.reg2 != MASM_REG_H)
      reg = alu->u.alu_or.reg2;
    else if (alu->u.alu_or.reg1 != MASM_REG_H &&
	     alu->u.alu_or.reg2 == MASM_REG_H)
      reg = alu->u.alu_or.reg1;
    else {
      masm_warning ("in line %d: one of the operand registers must be H\n",
		    l->line_number);
      break;
    }
    mic1_word_set_bits (l->word, MIC1_ALU_H_OR_B_BUS, MIC1_WORD_ALU_OFFSET);
    mic1_word_set_bits (l->word, masm_reg_b_bus (reg, l->line_number), 
			MIC1_WORD_B_BUS_OFFSET);
    break;

  case MASM_ALU_CONST:
    switch (alu->u.alu_const.val) {
      case 0:
	mic1_word_set_bits (l->word, MIC1_ALU_0, MIC1_WORD_ALU_OFFSET);
	break;
    
      case 1:
	mic1_word_set_bits (l->word, MIC1_ALU_1, MIC1_WORD_ALU_OFFSET);
	break;
    
      case -1:
	mic1_word_set_bits (l->word, MIC1_ALU_MINUS_1, 
			    MIC1_WORD_ALU_OFFSET);
	break;
    }    
    break;

  }
}

char *masm_reg_name (enum MasmRegKind reg)
{
  char *names[] = {
    "MAR", "MDR", "PC", "MBR", "MBRU", "SP", "LV", 
    "CPP", "TOS", "OPC", "H", "VIRTUAL"
  };

  return names[reg];
}

int masm_reg_c_bit (enum MasmRegKind reg)
{
  int bits[] = {
    MIC1_WORD_MAR_BIT,
    MIC1_WORD_MDR_BIT,
    MIC1_WORD_PC_BIT,
    0,
    0,
    MIC1_WORD_SP_BIT,
    MIC1_WORD_LV_BIT,
    MIC1_WORD_CPP_BIT,
    MIC1_WORD_TOS_BIT,
    MIC1_WORD_OPC_BIT,
    MIC1_WORD_H_BIT,
  };

  masm_assert (reg != MASM_REG_MBR,
	       "internal error: loading MBR from the C bus\n");
  masm_assert (reg != MASM_REG_MBRU,
	       "internal error: loading MBRU from the C bus\n");
  return bits[reg];
}

int masm_reg_b_bus (enum MasmRegKind reg, int line_number)
{
  int bus_index[] = {
    -1,  /* MAR cannot be enabled onto the B bus */
    0,   /* MDR */
    1,   /* PC */
    2,	 /* MBR */
    3,	 /* MBRU */
    4,	 /* SP */
    5,	 /* LV */
    6,	 /* CPP */
    7,	 /* TOS */
    8,   /* OPC */
  };

  if (reg == MASM_REG_MAR)
    masm_warning ("in line %d: can't enable MAR on B bus\n", line_number);

  masm_assert (reg != MASM_REG_H, 
	       "internal error: enabling H on the B bus\n");
  return bus_index[reg];
}

int
masm_lines_entry (MasmLine *lines)
{
  MasmLine *l, *entry;

  entry = NULL;
  for (l = lines; l != NULL; l = l->next)
    if (l->insns != NULL)
      entry = l;
  if (entry == NULL) {
    fprintf (stderr, "couldn't find entry point, something is wrong...\n");
    exit (-1);
  }

  return entry->address;
}

void 
masm_emit (MasmLine **store, int entry)
{
  Mic1Image image;
  int i;

  image.entry = entry;
  for (i = 0; i < 512; i++) {
    if (store[i] != NULL)
      memcpy (image.control_store[i], 
	      store[i]->word, sizeof (Mic1Word));
    else
      memset (image.control_store[i], 0, sizeof (Mic1Word));
  }

  mic1_image_write (stdout, &image);
}
