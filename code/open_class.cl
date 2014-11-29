
String a = Clover.outputToString() {
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

String c = Clover.outputToString() {
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

String d = Clover.outputToString() {
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

String e = Clover.outputToString() {
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
