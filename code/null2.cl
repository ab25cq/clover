int a = null;

print("null test1...");
if(a == null) {
    println("TRUE");
}
else {
    println("FALSE");
}

print("null test2...");
if(a.toNull().toInt() == 0 && a.toNull().toString() == "null" && a.toNull().toBool() == false && a.toNull() == null) {
    println("TRUE");
}
else {
    println("FALSE");
}

