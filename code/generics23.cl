Array<int> a = Array<int>->toClass().newInstance();

a.add(1);
a.add(2);
a.add(3);

print("class object test1...");
if(a == { 1, 2, 3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

GenericsTest23ClassA<int> b = new GenericsTest23ClassA<int>();

print("class object test2...");
if(b.field == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("class object test3...");
if(b.field2 == { 123, 456 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
