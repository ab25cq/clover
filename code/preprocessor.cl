int a = 
#preprocessor /bin/bash <<EOS
/bin/cat <<EOF > $HOME/.clover/tmpfiles/a.c
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("123");
    exit(0);
}
EOF
/usr/bin/cc -o $HOME/.clover/tmpfiles/a.out $HOME/.clover/tmpfiles/a.c;
$HOME/.clover/tmpfiles/a.out;
/bin/rm $HOME/.clover/tmpfiles/a.out;
EOS
#endpreprocessor

print("preprocessor test1...");
Clover.assert(a == 123);
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
#preprocessor /bin/bash <<EOS
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
#preprocessor 
#endpreprocessor
123;

print("preprocessor test8...");
Clover.assert(h == 123);
println("TRUE");

#preprocessor ruby<<EOS
a = 1;
if a == 1
    print "String str = \"bash\";"
else 
    print "String str = \"zsh\";"
end
EOS
#endpreprocessor

print("preprocessor test9...");
Clover.assert(str == "bash");
println("TRUE");

String source = 
#preprocessor bash<<EOS
echo \"$SOURCE\"
EOS
#endpreprocessor

print("preprocessor test10...");
Clover.assert(source == "code/preprocessor.cl");
println("TRUE");
