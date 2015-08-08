int a = 
#clang 
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("123");
    exit(0);
}
#endclang

print("preprocessor test1...");
Clover.assert(a == 123);
println("TRUE");

int b = 
#clang 111 222
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("%d", atoi(argv[1]) + atoi(argv[2]));
    exit(0);
}
#endclang

print("preprocessor test2...");
Clover.assert(b == 333);
println("TRUE");

String c =
#preprocessor echo "\"abc\"" #endpreprocessor

print("preprocessor test3...");
Clover.assert(c == "abc");
println("TRUE");

String d = 
#preprocessor #endpreprocessor
"aaaa";

print("preprocessor test4...");
Clover.assert(d == "aaaa");
println("TRUE");

int e = 
#preprocessor /usr/bin/python <<EOS
print "123";
EOS
#endpreprocessor

print("preprocessor test5...");
Clover.assert(e == 123);
println("TRUE");

Array<String> f = 
#preprocessor bash <<EOS
echo "{"
echo "\"AAA\", \"BBB\""
echo "};"
EOS
#endpreprocessor

print("preprocessor test6...");
Clover.assert(f == { "AAA", "BBB" });
println("TRUE");

int g = 
#preprocessor bash<<EOS
echo -e "111\n222\n333" | grep 111
EOS
#endpreprocessor

print("preprocessor test7...");
Clover.assert(g == 111);
println("TRUE");

int h = 
#clang 
#endclang
123;

print("preprocessor test8...");
Clover.assert(h == 123);
println("TRUE");
