
GenericsTest22ClassA<int, float> a = new GenericsTest22ClassA<int, float>();

int f = a.method(111, 222.222);

print("Generics Test22...");
if(f == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics Test22-2...");
if(a.getIntFromFloat(333.333) == 333 && a.getInt(555) == 555) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
