
GenericsTest17_1ClassA< GenericsTest17_1ClassB > a = new GenericsTest17_1ClassA< GenericsTest17_1ClassB >();

print("GenericsTest17_1 Test 1...");
if(a.method() == 300 && a.method2() == 100 && a.method3() == 111 && a.method4() == 999) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
