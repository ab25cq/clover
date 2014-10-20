
Generics12_4TestB<Generics12_4TestC> a = new Generics12_4TestB<Generics12_4TestC>();

print("Generics12-4 test1...");

if(a.method() == 300 && a.method2() == 200 && a.method3() == 999 && a.getFieldX() == 999) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
