FreeOrderB a = new FreeOrderA(123);
FreeOrderD b = new FreeOrderC(234);
FreeOrderF c = new FreeOrderE(345);

print("free order on class definition test1...");
if(a.get_field1() == 123 && b.get_field1() == 234 && c.get_field1() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    println("a.field1 --> " + a.get_field1().to_string());
    println("b.field1 --> " + b.get_field1().to_string());
    System.exit(1);
}

print("free order on alias definition test1...");
if(alias_test() == 234) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}
