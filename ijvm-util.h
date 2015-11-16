#ifndef IJVM_UTIL_H
#define IJVM_UTIL_H

#include "types.h"
#include "ijvm-spec.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define IJVM_MEMORY_SIZE (640 << 10)
#define IJVM_INITIAL_OBJ_REF 42
#define IJVM_INITIAL_PC 1

#define IJVM_OPCODE_BIPUSH        0x10
#define IJVM_OPCODE_DUP           0x59
#define IJVM_OPCODE_GOTO          0xA7
#define IJVM_OPCODE_IADD          0x60
#define IJVM_OPCODE_IAND          0x7E
#define IJVM_OPCODE_IFEQ    	  0x99
#define IJVM_OPCODE_IFLT    	  0x9B
#define IJVM_OPCODE_IF_ICMPEQ     0x9F
#define IJVM_OPCODE_IINC          0x84
#define IJVM_OPCODE_ILOAD         0x15
#define IJVM_OPCODE_INVOKEVIRTUAL 0xB6
#define IJVM_OPCODE_IOR           0x80
#define IJVM_OPCODE_IRETURN       0xAC
#define IJVM_OPCODE_ISTORE        0x36
#define IJVM_OPCODE_ISUB          0x64
#define IJVM_OPCODE_LDC_W         0x13
#define IJVM_OPCODE_NOP           0x00
#define IJVM_OPCODE_POP           0x57
#define IJVM_OPCODE_SWAP          0x5F
#define IJVM_OPCODE_WIDE          0xC4

typedef struct IJVMImage IJVMImage;
struct IJVMImage {
  uint16 main_index;
  uint8 *method_area;
  uint32 method_area_size;
  int32 *cpool;
  uint32 cpool_size;
};

/* extern IJVMSpec *ijvm_spec; */

IJVMImage *ijvm_image_new (uint16 main_index, 
			   uint8 *method_area, uint32 method_area_size,
			   int32 *cpool, uint32 cpool_size);
IJVMImage *ijvm_image_load (FILE *file);
void ijvm_image_write (FILE *file, IJVMImage *image);
int ijvm_get_opcode (char *mnemonic);

void ijvm_print_init (int *argc, char *argv[]);
void ijvm_print_stack (int32 *stack, int length, int indent);
void ijvm_print_opcodes (uint8 *opcodes, int length);
void ijvm_print_snapshot (uint8 *opcodes);

#endif
