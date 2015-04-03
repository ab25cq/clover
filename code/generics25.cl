GenericsTest25ClassA<String, int> a = new GenericsTest25ClassA<String, int>("AAA", 123);

print("GenericsTest25 test1...");
if(a.getField() == 123 && a.getField2() == "AAA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
