
Generics12_1TestB<Generics12_1TestA> a = new Generics12_1TestB<Generics12_1TestA>();

print("Generics12-1 test1...");

if(a.method() == 300) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
