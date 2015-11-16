// Eksempel på input med IJVM

.method main
.args 1
.locals 1
.define c = 1
.define object_ref = 5

        ldc_w object_ref   // c = in ()
	invokevirtual getchar
	istore c
	iload c            // if (c >= 96) {
	ldc_w 96
	isub
	iflt end_if
	iload c            //   c = c - 32
	ldc_w 32
	isub
	istore c
end_if:
	ldc_w object_ref   // out ( c )
	iload c
	invokevirtual putchar
	ireturn
