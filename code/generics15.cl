
GenericsTest15_A<int> a= new GenericsTest15_A<int>(123);

print("GenericsTest15 test1...");
if(a.getField1() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

GenericsTest15_A<GenericsTest15_B> b = new GenericsTest15_A<GenericsTest15_B>(new GenericsTest15_B(345));

print("GenericsTest15 test2...");
if(b.getField1().getField() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

