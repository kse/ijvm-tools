// Program to test the simulator
// Tests each instruction with a representative from each input class,
// ie. positiv and negative numbers, 0 and max and min values.

.method main
.args 1

back:
	bipush 12
	bipush 0
	bipush -7
	bipush 127
	bipush -128
	dup
	goto back
	goto forward
label_goto:
	goto label_goto
	iadd          
	iand          
	ifeq back
	ifeq forward
label_ifeq:
	ifeq label_ifeq
	iflt back
	iflt forward
label_iflt:
	iflt label_iflt
	if_icmpeq back
	if_icmpeq forward
label_icmpeq:
	if_icmpeq label_icmpeq
	iinc 1, 2
	iinc 0, -2
	iinc 1, -128
	iinc 1, 127
	iload 0
	iload 12
	iload 1234
	invokevirtual main
	ior
	ireturn
	istore 0
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
	swap
forward:
