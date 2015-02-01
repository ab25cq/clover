NameSpace1::ModuleTestClass a = new NameSpace1::ModuleTestClass(111, 222);

print("module test1...");
if(a.method() == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

ModuleTestClass2 b = new ModuleTestClass2(333, 444);

print("module test2...");
if(b.method() == 777) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

ModuleTestClass3 c = new ModuleTestClass3(555, 333);

print("module test3...");
if(c.method() == 888) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
