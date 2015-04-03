
GenericsTest21ClassA<String, int> a = new GenericsTest21ClassA<String, int>();

a.field.put(1, "AAA");

print("Generics21 test1...");
if(a.field.get(1) == "AAA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
