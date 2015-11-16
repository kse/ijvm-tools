// Factorials.

// fak(1)=1, fak(n) = n*fak(n-1), n > 1.


.method fak                     // int fak
.args   2                       // ( int n ){
.define n = 1                   
.locals 1                       // int r;
.define r = 2
.define OBJREF = 43

        bipush 1                // if 
        iload n
        if_icmpeq then          //    ( n == 1 )
        goto else
then:   bipush 1
        istore r                //    r = 1;
        goto end_if
else:                           // else
        bipush OBJREF
        iload n
// stack = n,OBJREF
        bipush OBJREF
        iload n
        bipush 1
        isub
        invokevirtual fak
// stack = fak(n-1),n,OBJREF
        invokevirtual imul
        istore r                //    r = n*fak(n-1);
end_if:
        iload r
        ireturn                 // return r;
                                // }

// Integer multiplication.

.method imul                    // int imul
.args   3                       // ( int x, int y )
.define x = 1
.define y = 2 
                                // {                   
.locals 1                       // int p;
.define p = 3

        bipush 0
        istore p                // p = 0;

while:                          // while        
        iload x
        ifeq  end_while         //    ( x > 0 )
                                // {
        iload x
        bipush 1
        isub
        istore x                //    x = x - 1;
        iload p
        iload y
        iadd
        istore p                //    p = p + y;
        goto while              // }
end_while:
        iload  p
        ireturn                 // return p;
                                //}


.method main                    // int main(){
.args   1
.define OBJREF = 44
  
        bipush OBJREF
        bipush 2
        invokevirtual fak       
        ireturn                 // return fak(2);
                                //}
