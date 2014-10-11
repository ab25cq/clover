
GenericsTest10_2ClassA<GenericsTest10_2ClassB> a = new GenericsTest10_2ClassA<GenericsTest10_2ClassB>(new GenericsTest10_2ClassC(100, 200));

print("generics 10-2 test1...");
if(a.call_field_method() == 100 && a.call_field_method2() == 200) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

