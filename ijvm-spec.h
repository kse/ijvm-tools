#ifndef SPEC_H
#define SPEC_H

typedef struct IJVMSpec IJVMSpec;
typedef enum IJVMOperandKind IJVMOperandKind;
typedef struct IJVMInsnTemplate IJVMInsnTemplate;

struct IJVMSpec {
  IJVMInsnTemplate **templates;
  int ntemplates, allocation;
};

enum IJVMOperandKind
{
  IJVM_OPERAND_BYTE,        /* 8 bit signed */
  IJVM_OPERAND_LABEL,       /* A label within current method */
  IJVM_OPERAND_METHOD,      /* The name of a method */
  IJVM_OPERAND_VARNUM,      /* 8 bit unsigned */
  IJVM_OPERAND_VARNUM_WIDE, /* 8 or 16 bit unsigned, emits wide */
  IJVM_OPERAND_CONSTANT     /* 32 bit signed, emits cpool index */
};

struct IJVMInsnTemplate
{
  int opcode;
  char *mnemonic;
  IJVMOperandKind *operands;  /* array of operand types this insn expects */
  int noperands, nalloc;      /* number of operands, allocated size of array */
};

IJVMInsnTemplate *ijvm_spec_lookup_template_by_mnemonic (IJVMSpec *spec, char *mnemonic);

IJVMInsnTemplate *ijvm_spec_lookup_template_by_opcode (IJVMSpec *spec, int opcode);
void ijvm_spec_add_template (IJVMSpec *spec, IJVMInsnTemplate *tmpl);
IJVMSpec *ijvm_spec_new ();
IJVMSpec *ijvm_spec_parse (FILE *f);

IJVMSpec *ijvm_spec_init (int *argc, char *argv[]);

#endif
