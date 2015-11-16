// Eksempel på output med IJVM

.method main
.args 1
.locals 1
.define i = 1
.define object_ref = 5

	ldc_w 32             // i = 32
	istore i

while:
	ldc_w 128            // while ( i != 128 ) {
	iload i
	isub
	ifeq end_while
	ldc_w object_ref     //   out ( i );
	iload i
	invokevirtual putchar
	pop
	iload i              //   i = i + 1;
	ldc_w 1
	iadd
	istore i
	goto while           // }

end_while:
	ireturn
