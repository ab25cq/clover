
Generics10_5ClassA <Generics10_5ClassC> a = new Generics10_5ClassA < Generics10_5ClassC >(new Generics10_5ClassC(new Generics10_5ClassY(), new Generics10_5ClassY()));

print("generics 10-5 test1...");
if(a.call_field_method() == 0 && a.call_field_method2() == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
