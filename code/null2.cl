int a = null;

print("null test1...");
if(a == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("null test2...");
if(a.toNull().toInt() == 0 && a.toNull().toString() == "null" && a.toNull().toBool() == false && a.toNull() == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Null b = new Null();

print("null test3...");
if(b.toInt() == 0 && b == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int c = null;

print("null test4...");
if(c == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
