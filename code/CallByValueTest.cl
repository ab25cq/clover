StructParamTest a = new StructParamTest();
int b = 1;
int c = 2;

a.methodX(b, c);

print("Struct Param Test1...");
if(a.field == 1 && b == 1 && c == 777) {
    println("TRUE");
}
else {
    println("b --> " + b.toString());
    println("c --> " + c.toString());
    println("FALSE");
    System.exit(2);
}


CallByValueTestClass d = new CallByValueTestClass(0);
CallByValueTestClass e = new CallByValueTestClass(0);

a.methodY(d, e);

print("Struct Param Test2...");
if(d.field == 777 && e.field == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
