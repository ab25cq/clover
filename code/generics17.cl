
GenericsTest17ClassA<GenericsTest17ClassB> a = new GenericsTest17ClassA<GenericsTest17ClassB>();


print("Generics Test 17 Test 1...");
if(a.method() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics Test 17 Test 2...");
if(a.method2() == 789) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics Test 17 Test 3...");
if(a.method3() == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
