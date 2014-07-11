
Inherit a = new Inherit(123);
String b = Clover.output_to_string() {
    a.method();
}

Clover.print("Inherit test1...");
if(b == "I'm a non class method without parametor\nHELLO WORLD on non class method\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String c = Clover.output_to_string() {
    Inherit.method();
}

Clover.print("Inherit test2...");
if(c == "I'm a class method with parametor\nHELLO WORLD on class method\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}
