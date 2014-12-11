ParamInitializerTestA a = new ParamInitializerTestA();

print("ParamInitializerTestA test1...");
if(a.method(111) == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}
