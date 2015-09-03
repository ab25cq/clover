#def clang
mv $BLOCK_FILE ~/.clover/tmpfiles/a.c
gcc -o ~/.clover/tmpfiles/a.out ~/.clover/tmpfiles/a.c
if test -x ~/.clover/tmpfiles/a.out
then
    ~/.clover/tmpfiles/a.out $PARAM0 $PARAM1 $PARAM2 $PARAM3 $PARAM4 $PARAM5 $PARAM6 $PARAM7 $PARAM8 $PARAM9
    rm -f ~/.clover/tmpfiles/a.out
    rm -f ~/.clover/tmpfiles/a.c
fi
#enddef

int a =
#call clang 1 2
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("%d;", atoi(argv[1]) +  atoi(argv[2]));
    exit(0);
}
#endcall

print("preprosessor test1...");
Clover.assert(a == 3);
println("TRUE");
