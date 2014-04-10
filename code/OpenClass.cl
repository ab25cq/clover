
String a = Clover.output_to_s() {
    Test2::OpenClass.main();
}

Clover.print("OpenClass Test1...");
if(a == "Hello OpenClass\n") {
    Clover.println("OK");
}
else {
    Clover.print("FALSE");
    Clover.exit(2);
}

Test::OpenClass b = new Test::OpenClass("HELLO");

String c = Clover.output_to_s() {
    b.method();
}

Clover.print("OpenClass Test2...");
if(c == "888\n") {
    Clover.println("OK");
}
else {
    Clover.print("FALSE");
    Clover.exit(2);
}

String d = Clover.output_to_s() {
    b.method2();
}

Clover.print("OpenClass Test3...");
if(d == "HELLO\n") {
    Clover.println("OK");
}
else {
    Clover.print("FALSE");
    Clover.exit(2);
}

String e = Clover.output_to_s() {
    b.method3();
}

Clover.print("OpenClass Test4...");
if(e == "Hello I'm Test::OpenClass.method3()\n") {
    Clover.println("OK");
}
else {
    Clover.print("FALSE");
    Clover.exit(2);
}
