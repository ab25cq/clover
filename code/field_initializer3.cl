FieldTestClassA a = new FieldTestClassA();

print("Field Test1...");
if(a.field == null && a.field2 == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Field Test2...");
if(FieldTestClassA.staticField1 == null && FieldTestClassA.staticField2 == null)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
