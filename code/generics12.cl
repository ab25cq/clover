Generics12TestB<Generics12TestA> a = new Generics12TestB<Generics12TestA>();

print("Generics12 test1...");
if(a.method() == 200) {
    println("TRUE");
}
else {
    println("FALSE"); 
    System.exit(2);
}
