
print("int test1...");
if(1.toBool() && !0.toBool()) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(1);
}

print("bool test1...");
if(true.toInt() == 1 && false.toInt() == 0) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(1);
}

int a = new int();
int b = new int(123);

print("new int test...");
if(a == 0 && b == 123) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(1);
}

