ParamInitializerTest a = new ParamInitializerTest();

a.method(123);

print("Param initializer test...");
if(a.field1 == 123 && a.field2 == "AAA" && a.field3 == 1024) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

a.method2(1, "BBB");

print("Param initializer test2...");
if(a.field1 == 1 && a.field2 == "BBB" && a.field3 == 12) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

