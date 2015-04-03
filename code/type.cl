Array<int> a = new Array<int>();

print("Type test1...");
if(a.type() == Array<int>) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test2...");
if(a.type().class() == Array-1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test3...");
if(a.type().genericsParam(0) == int && a.type()[0] == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test4...");
if(a.type().genericsParamNumber() == 1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test5...");

if(a.type().parentClass() == Object) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test6...");

if(a.type().parentClassNumber() == 1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type test7...");

if(a.type().parentClass().parentClass() == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

