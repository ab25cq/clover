

GenericsTestClass8<String, int> a = new GenericsTestClass8<String, int>(128, "AAA");

print("Generics test8...");
if(a.get_value1() == 128 && a.get_value2() == "AAA" && a.method(64, "BBB") == 64 && a.method("BBB", 64) == 64) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
