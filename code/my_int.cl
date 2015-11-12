int a = new int(521);

print("int test 1...");
if(a == 521 && a.toString() == "521" && a.type() == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("int test2...");
if(a.instanceOf(int) && a.type() == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

MyInt b = new MyInt(123);

print("MyInt test1...");

if(b.toString() == "123" && b.isChild(int) && b.type() == MyInt) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int c = new MyInt(1000);

print("MyInt test2...");
if(c == 1000 && c.type() == MyInt) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int d = b.toInt() + c.toInt() + 123 + new MyInt(100).toInt();

print("MyInt test3...");
if(d == 1346 && d.type() == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int e = new MyInt(1) + new MyInt(2);

print("MyInt test4...");
if(e == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

