
Array<int> a = new Array<int>();

print("Array Test5 test1...");
if(a == {}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Array Test5 test2...");
if({}[0..-2] == {}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
