
GenericsTest2<int, float, String> a = new GenericsTest2<int, float, String>(12, 2.0, "AAA");

a.set_class_a(new ClassA<int, float>(24, 5.5));

print("generics7 test1...");
if(a.get_value2_in_value4() == 5.5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
