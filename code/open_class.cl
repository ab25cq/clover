
String a = Clover.output_to_string() {
    Test2::OpenClass.main();
}

print("OpenClass Test1...");
if(a == "Hello OpenClass\n") {
    println("OK");
}
else {
    print("FALSE");
    System.exit(2);
}

Test::OpenClass b = new Test::OpenClass("HELLO");

String c = Clover.output_to_string() {
    b.method();
}

print("OpenClass Test2...");
if(c == "888\n") {
    println("OK");
}
else {
    print("FALSE");
    System.exit(2);
}

String d = Clover.output_to_string() {
    b.method2();
}

print("OpenClass Test3...");
if(d == "HELLO\n") {
    println("OK");
}
else {
    print("FALSE");
    System.exit(2);
}

String e = Clover.output_to_string() {
    b.method3();
}

print("OpenClass Test4...");
if(e == "Hello I'm Test::OpenClass.method3()\n") {
    println("OK");
}
else {
    print("FALSE");
    System.exit(2);
}
