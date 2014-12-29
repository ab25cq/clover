int a = 1;
int b = 2;

MethodBlockTest3 c = new MethodBlockTest3();

print("MethodBlockTest3 test1...");
if(c.method() int {|int a, int b|
    int result = 0;
    int d = 3;
    int e = 4;

    a = 5;
    b = 7;

    for(int i = 0; i<5; i++) {
        result += i;
    }

    return result;
} == 10)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("MethodBlockTest3 test2...");
if(a == 1 && b == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

