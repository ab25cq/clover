
Base a = new Extended(111, 222, 333);

String b = Clover.output_to_string() {
    a.method();
}

print("Extend test...");
if(b == "I'm Base.method().\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String c = Clover.output_to_string() {
    a.method2();
}

print("Extend test2...");
if(c == "I'm Extended. method2().\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String d = Clover.output_to_string() {
    a.method3();
}

print("Extend test3...");
if(d == "I'm Base.method3().\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String e = Clover.output_to_string() {
    a.show();
};

print("Extend test4...");
if(e == "field1 111\nfield2 222\nfield3 333\n") {
    println("OK");
}
else {
    println(e);
    println("FALSE");
    System.exit(2);
}

print("Extend test5...");
if(a.class_name() == "Extended") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

