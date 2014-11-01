
Generics18ClassA<int> a = new Generics18ClassA<int>(123);

print("Generics18 Test1...");
if(a.method() == 246 && a.method2() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
