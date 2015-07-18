
TestModuleClassA a = new TestModuleClassA();

print("including module test 1...");
if(a.method() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

TestModuleClassB b = new TestModuleClassB();

print("including module test 2...");
if(b.method() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("including module test 3..");
if(TestModuleClassA.method2() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("including module test 4...");
if(TestModuleClassB.method2() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("including module test 5...");
if(TestModuleClassA.createObject().type() == TestModuleClassA) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("including module test 6...");
if(TestModuleClassB.createObject().type() == TestModuleClassB) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
