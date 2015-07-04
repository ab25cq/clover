
print("string slice test1...");
if("ABC"[0..1] == "A") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test2...");
if("ABC"[0..-1] == "AB") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test3...");
if("ABC"[0..null] == "ABC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test4...");
if("ABC"[0..0] == "") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test5...");
if("ABC"[0..-2] == "A") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test6...");
if("ABC"[null..0] == "CBA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string slice test7...");
if("ABC"[2..0] == "BA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
