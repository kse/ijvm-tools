#ifndef JASM_H
#define JASM_H

#include <stdio.h>
#include "mic1-util.h"

typedef struct MasmLine MasmLine;
typedef struct MasmLabel MasmLabel;
typedef struct MasmInsn MasmInsn;
typedef struct MasmAssign MasmAssign;
typedef struct MasmAlu MasmAlu;


/* This corresponds to a nonempty line in the micro program.  If
 * `label' is not NULL there was a label at the line.  If `insns' is
 * not NULL there were some instructions in the line, and in this case
 * if `laid_out' is true, they have been laid out at the address
 * denoted by `address'.  
 * 
 * The list of lines is linked backwards, i.e. the first element in
 * the list is the last line of the input, and the `next' pointer
 * points to the preceeding line. */

struct MasmLine {
  MasmLabel *label;
  MasmInsn *insns;
  int laid_out, address;
  int line_number;
  Mic1Word word;
  MasmLine *next;
};

MasmLine *masm_line_make (MasmLabel *label, MasmInsn *insns);


/* If the label is an absolute label, `absolute' will be true and
 * address will hold the address requested.  The line pointed to by
 * `line' is the first following line that has instructions in it.
 * This isn't necessarily the line where the label was defined. */

struct MasmLabel {
  char *name;
  int absolute;
  int address;
  MasmLine *line;
};

MasmLabel *masm_label_make (char *name, int address, int absolute);


enum MasmInsnKind {
  MASM_INSN_RD,
  MASM_INSN_WR,
  MASM_INSN_FETCH,
  MASM_INSN_EMPTY,
  MASM_INSN_HALT,
  MASM_INSN_GOTO,
  MASM_INSN_IGOTO,
  MASM_INSN_IF,
  MASM_INSN_ASSIGN
};

enum MasmCondKind {
  MASM_COND_Z,
  MASM_COND_N
};

struct MasmInsn 
{
  enum MasmInsnKind kind;
  union {
    struct { char *label; } insn_goto;
    struct { int next_address; } insn_igoto;
    struct { enum MasmCondKind cond; char *label1, *label2; } insn_if;
    MasmAssign *insn_assign;
  } u;
    
  MasmInsn *next;
};

MasmInsn *masm_insn_make_rd ();
MasmInsn *masm_insn_make_wr ();
MasmInsn *masm_insn_make_fetch ();
MasmInsn *masm_insn_make_empty ();
MasmInsn *masm_insn_make_halt ();
MasmInsn *masm_insn_make_goto (char *label);
MasmInsn *masm_insn_make_igoto (int next_address);
MasmInsn *masm_insn_make_if (enum MasmCondKind cond, 
			     char *label1, char *label2);
MasmInsn *masm_insn_make_assign (MasmAssign *assign);

enum MasmAssignKind {
  MASM_ASSIGN_BASIC,
  MASM_ASSIGN_CHAIN,
  MASM_ASSIGN_RSHIFT1,
  MASM_ASSIGN_LSHIFT8
};

enum MasmRegKind {
  MASM_REG_MAR,
  MASM_REG_MDR,
  MASM_REG_PC,
  MASM_REG_MBR,
  MASM_REG_MBRU,
  MASM_REG_SP,
  MASM_REG_LV,
  MASM_REG_CPP,
  MASM_REG_TOS,
  MASM_REG_OPC,
  MASM_REG_H,
  MASM_REG_VIRTUAL
};

struct MasmAssign {
  enum MasmAssignKind kind;
  union {
    struct { enum MasmRegKind reg; MasmAlu *alu; } assign_basic;
    struct { enum MasmRegKind reg; MasmAssign *assign; } assign_chain;
    struct { enum MasmRegKind reg; MasmAlu *alu; } assign_rshift1;
    struct { enum MasmRegKind reg; MasmAlu *alu; } assign_lshift8;
  } u;
};

MasmAssign *masm_assign_make_basic (enum MasmRegKind reg, MasmAlu *alu);
MasmAssign *masm_assign_make_chain (enum MasmRegKind reg, MasmAssign *src);
MasmAssign *masm_assign_make_rshift1 (enum MasmRegKind reg, MasmAlu *alu);
MasmAssign *masm_assign_make_lshift8 (enum MasmRegKind reg, MasmAlu *alu);

enum MasmAluKind {
  MASM_ALU_REG,
  MASM_ALU_INV,
  MASM_ALU_ADD_RR1,
  MASM_ALU_ADD_RR,
  MASM_ALU_ADD_R1,
  MASM_ALU_SUB_RR,
  MASM_ALU_SUB_R1,
  MASM_ALU_NEG,
  MASM_ALU_AND,
  MASM_ALU_OR,
  MASM_ALU_CONST
};

struct MasmAlu {
  enum MasmAluKind kind;
  union {
    struct { enum MasmRegKind reg; } alu_reg;
    struct { enum MasmRegKind reg; } alu_inv;
    struct { enum MasmRegKind reg1, reg2; } alu_add_rr1;
    struct { enum MasmRegKind reg1, reg2; } alu_add_rr;
    struct { enum MasmRegKind reg; } alu_add_r1;
    struct { enum MasmRegKind reg1, reg2; } alu_sub_rr;
    struct { enum MasmRegKind reg; } alu_sub_r1;
    struct { enum MasmRegKind reg; } alu_neg;
    struct { enum MasmRegKind reg1, reg2; } alu_and;
    struct { enum MasmRegKind reg1, reg2; } alu_or;
    struct { int val; } alu_const;
  } u;    
};

MasmAlu *masm_alu_make_reg (enum MasmRegKind reg);
MasmAlu *masm_alu_make_inv (enum MasmRegKind reg);
MasmAlu *masm_alu_make_add_rr1 (enum MasmRegKind reg1, enum MasmRegKind reg2);
MasmAlu *masm_alu_make_add_rr (enum MasmRegKind reg1, enum MasmRegKind reg2);
MasmAlu *masm_alu_make_add_r1 (enum MasmRegKind reg);
MasmAlu *masm_alu_make_sub_rr (enum MasmRegKind reg1, enum MasmRegKind reg2);
MasmAlu *masm_alu_make_sub_r1 (enum MasmRegKind reg);
MasmAlu *masm_alu_make_neg (enum MasmRegKind reg);
MasmAlu *masm_alu_make_and (enum MasmRegKind reg1, enum MasmRegKind reg2);
MasmAlu *masm_alu_make_or (enum MasmRegKind reg1, enum MasmRegKind reg2);
MasmAlu *masm_alu_make_const (int val);

#if 0
int masm_check_line (MasmLine *lines);
int masm_check_insn (MasmInsn *insn, int flags, int prev_flags);
void masm_check_assign_target (int target, int regs, int prev_flags);
void masm_check_assign (MasmAssign *assign, int regs, int prev_flags);
void masm_check_alu (MasmAlu *alu);
char *masm_reg_name (enum MasmRegKind reg);
#endif

MasmLine **masm_layout_line (MasmLine *lines);
void masm_emit_line (MasmLine *lines);
MasmLine *masm_lookup_label (char *label);
void masm_check_line (MasmLine *lines, MasmLine *prev);
void masm_emit (MasmLine **store, int entry);
int masm_lines_entry (MasmLine *lines);

MasmLine *masm_parse ();
int masm_parse_current_line ();

void *masm_malloc (int size);
void masm_abort (const char *fmt, ...);
void masm_assert (int cond, const char *fmt, ...);
void masm_warning (const char *fmt, ...);
void *masm_malloc (int size);
char *masm_strdup (char *s);

#endif

