
Generics10_3ClassA <Generics10_3ClassB<int>> a = new Generics10_3ClassA< Generics10_3ClassB<int> >(new Generics10_3ClassC<int>(100, 200));

print("generics 10-3 test1...");
if(a.call_field_method() == 100 && a.call_field_method2() == 200) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

