
Generics12_2TestB<Generics12_2TestC> a = new Generics12_2TestB<Generics12_2TestC>();

print("Generics12-2 test1...");

if(a.method() == 300 && a.method2() == 200) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
