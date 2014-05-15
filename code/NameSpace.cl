
NameSpaceA::ClassA a = new NameSpaceA::ClassA(1);

print("namespace test1...");
if(a.get_field1() == 1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

NameSpaceB::ClassB b = new NameSpaceB::ClassB(222);

print("namespace test2...");
if(b.get_field1() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

NameSpaceB::ClassC c = new NameSpaceB::ClassC(333);

print("namespace test3...");
if(c.method1() == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

