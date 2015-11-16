.method main
.args 3                    // ( int a, int b )
.define a = 1
.define b = 2

        bipush 88          // Push object reference.
        iload a
        iload b
        invokevirtual min
        ireturn            // return min ( a, b );


.method min
.args 3                    // ( int a, int b )
.define a = 1
.define b = 2
.locals 1                  // int r;
.define r = 3

        iload a            // if ( a >= b )
        iload b
        isub

// stack = a - b, ... ; a - b < 0 => a < b

        iflt else

        iload b            //   r = b;
        istore r
        goto end_if

else:
        iload a            //   r = a;
        istore r

end_if:
        iload r            // return r;
        ireturn
