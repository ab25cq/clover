
ParametorTestClass a = new ParametorTestClass();

print("method parametor test1...");
if(a.method(new Array<int>()) == 1 && a.method(new Array<String>()) == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
