int a = new int(521);

println("a.className() --> " + a.className().toString());

print("int test 1...");
if(a == 521 && a.toString() == "521" && a.className() == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("int test2...");
if(a.instanceOf(int)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

MyInt b = new MyInt(123);

print("MyInt test1...");

if(b.toString() == "123" && b.isChild(int)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int c = new MyInt(1000);

print("MyInt test2...");
if(c == 1000) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int d = b + c + 123;

print("MyInt test3...");
if(d == 1246) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

