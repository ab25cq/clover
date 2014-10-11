
Generics10_6ClassC <Generics10_6ClassB<Generics10_6ClassY> > a = new Generics10_6ClassC <Generics10_6ClassB<Generics10_6ClassY>> (new Generics10_6ClassB<Generics10_6ClassY>(new Generics10_6ClassY(100, 200)));

print("Generics10-6 test1...");
if(a.call_field_method() == 100 && a.call_field_method2() == 200) {
    println("TRUE");
}
else {
    println("FALSE");
}
