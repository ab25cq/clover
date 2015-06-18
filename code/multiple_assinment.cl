
Tuple<int, int> x = 1,2;

print("Multiple Assignment test1...");
if(x.get1() == 1 && x.get2() == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int a = 0;
int b = 0;

Tuple<int,int> c = a,b = 1,2;

print("Multiple Assignment test2...");
if(a == 1 && b == 2 && c == (1,2)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int d, int e = 3,4;

print("Multiple Assignment test3...");
if(d == 3 && e == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

TestClassWithStoringTuple f = new TestClassWithStoringTuple();

f.value1, f.value2 = 1,2;

print("Multiple Assignment test4...");
if(f.value1 == 1 && f.value2 == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

TestClassWithStoringTuple.value3, f.value2 = 3,4;

print("Multiple Assignment test5...");
if(TestClassWithStoringTuple.value3 == 3 && f.value2 == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


