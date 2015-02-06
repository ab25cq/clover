GenericsTest20TestClass2<String, int> a = new GenericsTest20TestClass2<String, int>("ABC", 123);

print("Generics20 test...");
if(a.method() == "ABC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
