
Base a = new Extended(111, 222);

String b = Clover.output_to_s() {
    a.method();
}

Clover.print("Extend test...");
if(b == "I'm Base.method().\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String c = Clover.output_to_s() {
    a.method2();
}

Clover.print("Extend test2...");
if(c == "I'm Extended. method2().\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String d = Clover.output_to_s() {
    a.method3();
}

Clover.print("Extend test3...");
if(d == "I'm Base.method3().\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String e = Clover.output_to_s() {
    a.show();
}

Clover.print("Extend test4...");
if(e == "field1 111\nX field1 111\nfield2 222\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.print("Extend test5...");
if(a.class_name() == "Extended") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}
