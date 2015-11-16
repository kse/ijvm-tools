.method main
.args   1
.define OBJREF = 44
.locals 1   // int p;
.define p = 1

     bipush 0    // p = 0;
     istore p

     iinc p, 1   // p += 1;
     iinc p, -3  // p += 1;

     iload p     // return p;
     ireturn
