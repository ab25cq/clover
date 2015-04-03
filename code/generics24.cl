
GenericsTest24ClassA<GenericsTest24ClassC, GenericsTest24ClassE> a = new GenericsTest24ClassA<GenericsTest24ClassC, GenericsTest24ClassE>();

print("Generics24 test...");
if(a.field.getValue() == 123 && a.field.getValue2() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics24 test2...");
if(a.field2[0].getValue() == 234 && a.field2[0].getValue2() == 567 && a.field2[1].getValue() == 345 && a.field2[1].getValue2() == 678) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics24 test3...");
if(a.field3[0].getNumber() == 13579 && a.field3[0].toString() == "13579") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics24 test4...");
if(a.field2 == { new GenericsTest24ClassC(234, 567), new GenericsTest24ClassC(345, 678) })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
