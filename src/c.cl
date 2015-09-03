
#def clang
bash <<EOS
gcc -o ~/.clover/tmpfiles/a.out $HEREDOCUMENT
~/.clover/tmpfiles/a.out $PARAM0 $PARAM1
rm ~/.clover/tmpfiles/a.out
EOS
#enddef

int a = #call clang(1, 2) <<EOS
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv) {
    printf("%d;", atoi(argv[0]), atoi(argv[1]));
    exit(0);
}
EOS
#endcall

print("preprocessor test2...");
Clover.assert(a == 3);
println("TRUE");
