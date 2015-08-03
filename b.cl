int a = #clang
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

