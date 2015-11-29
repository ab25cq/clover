TestA a = new TestA(111, 222);

print("Field test1...");
Clover.assert(a.field1 == 111 && a.field3 == 222);
println("TRUE");

TestB b = new TestB(1, 2, 3, 4);

print("Field test2...");
Clover.assert(b.field1 == 1 && b.field2 == 2 && b.field3 == 3 && b.field4 == 4);
println("TRUE");
