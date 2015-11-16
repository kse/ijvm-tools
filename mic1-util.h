#ifndef MIC1_WORD_H
#define MIC1_WORD_H

#include "types.h"

#define MIC1_WORD_B_BUS_OFFSET 0
#define MIC1_WORD_B_BUS_SIZE 4

#define MIC1_WORD_FETCH_BIT 4
#define MIC1_WORD_READ_BIT 5
#define MIC1_WORD_WRITE_BIT 6

#define MIC1_WORD_C_BUS_OFFSET 7
#define MIC1_WORD_MAR_BIT 7
#define MIC1_WORD_MDR_BIT 8
#define MIC1_WORD_PC_BIT 9
#define MIC1_WORD_SP_BIT 10    
#define MIC1_WORD_LV_BIT 11
#define MIC1_WORD_CPP_BIT 12
#define MIC1_WORD_TOS_BIT 13
#define MIC1_WORD_OPC_BIT 14
#define MIC1_WORD_H_BIT 15   

#define MIC1_WORD_ALU_OFFSET 16
#define MIC1_WORD_ALU_SIZE 6
#define MIC1_WORD_INC_BIT 16
#define MIC1_WORD_INVA_BIT 17
#define MIC1_WORD_ENB_BIT 18
#define MIC1_WORD_ENA_BIT 19
#define MIC1_WORD_F1_BIT 20
#define MIC1_WORD_F0_BIT 21
#define MIC1_WORD_SRA1_BIT 22
#define MIC1_WORD_SLL8_BIT 23

#define MIC1_WORD_JAMZ_BIT 24
#define MIC1_WORD_JAMN_BIT 25
#define MIC1_WORD_JMPC_BIT 26

#define MIC1_WORD_ADDRESS_OFFSET 27
#define MIC1_WORD_ADDRESS_SIZE 9

#define F0    (1 << 5) 
#define F1    (1 << 4)
#define ENA   (1 << 3)
#define ENB   (1 << 2)
#define INVA  (1 << 1)
#define INC   (1 << 0)

#define MIC1_ALU_H             (F1 | ENA)
#define MIC1_ALU_B_BUS         (F1 | ENB)
#define MIC1_ALU_INV_H         (F1 | ENA | INVA)
#define MIC1_ALU_INV_B_BUS     (F0 | ENA | ENB)
#define MIC1_ALU_ADD_B_BUS_H   (F0 | F1 | ENA | ENB)
#define MIC1_ALU_ADD_B_BUS_H_1 (F0 | F1 | ENA | ENB | INC)
#define MIC1_ALU_ADD_H_1       (F0 | F1 | ENA | INC)
#define MIC1_ALU_ADD_B_BUS_1   (F0 | F1 | ENB | INC)
#define MIC1_ALU_SUB_B_BUS_H   (F0 | F1 | ENA | ENB | INVA | INC)
#define MIC1_ALU_SUB_B_BUS_1   (F0 | F1 | ENB | INVA | INC)
#define MIC1_ALU_NEG_H         (F0 | F1 | ENA | INVA | INC)
#define MIC1_ALU_H_AND_B_BUS   (ENA | ENB)
#define MIC1_ALU_H_OR_B_BUS    (F1 | ENA | ENB)
#define MIC1_ALU_0             (F1)
#define MIC1_ALU_1             (F1 | INC)
#define MIC1_ALU_MINUS_1       (F1 | INVA)

typedef uint8 Mic1Word[5];
typedef struct Mic1Image Mic1Image;
struct Mic1Image {
  int entry;
  Mic1Word control_store[512];
};

void mic1_word_clear (Mic1Word word);
void mic1_word_set_bits (Mic1Word word, unsigned int bits, int pos);
void mic1_word_set_bit (Mic1Word word, int pos);
unsigned int mic1_word_get_bits (Mic1Word word, int pos, int size);
int mic1_word_get_bit (Mic1Word word, int pos);
void mic1_word_write (Mic1Word word, char *buf);
void mic1_word_read (Mic1Word word, char *line);
void mic1_word_disassemble (Mic1Word word, char *buf);

void mic1_image_write (FILE *file, Mic1Image *image);
Mic1Image *mic1_image_load (FILE *file);

#endif
