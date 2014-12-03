int a = 123;

print("Object class test1...");
if(a.instanceOf(int) && a.isChild(Object)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
