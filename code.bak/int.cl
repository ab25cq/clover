
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
