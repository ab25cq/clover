ConstructorTest a = new ConstructorTest(123);
ConstructorTest b = new ConstructorTest(a);

print("ConstructorTest....");
Clover.assert(a.field == 123 && b.field == 123);
println("TRUE");

int c = new int(345);
int d = new int(c);

print("ConstructorTest....");
Clover.assert(c == 345 && d == 345);
println("TRUE");
