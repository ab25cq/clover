
Generics12_3TestB<Generics12_3TestC> a = new Generics12_3TestB<Generics12_3TestC>();

print("Generics12-3 test1...");

if(a.method() == 300 && a.method2() == 200 && a.method3() == 999) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
