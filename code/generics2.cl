
TestGenerics<GenericsTest2ClassB> a = new TestGenerics<GenericsTest2ClassB>();

print("Generics Test2-1....");
if(a.call_method1() == "111" && a.call_method2() == "111") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

TestGenerics3<int> c = new TestGenerics3<int>();

print("Generics Test2-2...");
if(c.call_new_operator() == "123" && c.call_new_operator2() == "123") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

