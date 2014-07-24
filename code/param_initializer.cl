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


a.method3(2, "CCC");

print("Param initializer test2...");
if(a.field1 == 2 && a.field2 == "CCC" && a.field3 == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    println("a.field1 --> " + a.field1.to_string());
    println("a.field2 --> " + a.field2);
    println("a.field3 --> " + a.field3.to_string());
    System.exit(2);
}

