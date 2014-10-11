
Generics10_4ClassA <Generics10_4ClassB<int>> a = new Generics10_4ClassA< Generics10_4ClassB<int> >(new Generics10_4ClassC(100, 200));

print("generics 10-4 test1...");
if(a.call_field_method() == 100 && a.call_field_method2() == 200) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

