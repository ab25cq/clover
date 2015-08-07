int a = 
#clang
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("123");
    exit(0);
}
#endclang

print("preprocesser test1...");
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

print("preprocesser test2...");
Clover.assert(b == 333);
println("TRUE");
