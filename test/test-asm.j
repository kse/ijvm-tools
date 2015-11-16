// Program to test the assembler
// Tests each instruction with a representative from each input class,
// ie. positiv and negative numbers, 0 and max and min values.

.method main
.args 1
.locals 2

	bipush 12
	bipush 0
	bipush -7
	bipush 127
	bipush -128
	dup
	pop
	pop
	iadd
	swap
	pop
	iand
	pop
	iinc 1, 2
	iinc 2, -2
	iinc 1, -128
	iinc 1, 127
	iload 0
	iload 12
	iload 1234
	pop
	pop            // Note: the value read from LV (local 0) is
                       // stored back into LV.
	istore 0

	bipush 13
	bipush 56
	bipush 82
	ior
	bipush 35

back:
	istore 12
	istore 1234
	isub
	ldc_w 0
	ldc_w 1234
	ldc_w -1234
	ldc_w -0x80000000
	ldc_w 0x7fffffff
	nop
	pop
	pop
	pop
	pop

	goto next1
	nop
next1:
	bipush 0
	ifeq next2
	ior
next2:
	bipush 5
	ifeq loop
	bipush 27
	iflt loop
	bipush 0
	iflt loop
	bipush -5
	iflt next3
	istore 0
next3:
	bipush 5
	bipush 7
	if_icmpeq loop
	bipush -8
	bipush -8
	if_icmpeq next4
	goto next3
next4:
	ireturn

loop:
	goto loop

// The following jumps check that negative, positive and zero offsets
// are emitted correctly.  They aren't meant to be excecuted.

	goto back
	goto forward
goto_label:
	goto goto_label

	ifeq back
	ifeq forward
ifeq_label:
	ifeq ifeq_label
	
	iflt back
	iflt forward
iflt_label:
	iflt iflt_label
	
	if_icmpeq back
	if_icmpeq forward
if_icmpeq_label:
	if_icmpeq if_icmpeq_label

forward:
