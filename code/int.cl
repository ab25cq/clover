
print("int test1...");
if(1.to_bool() && !0.to_bool()) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(1);
}

print("bool test1...");
if(true.to_int() == 1 && false.to_int() == 0) {
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

