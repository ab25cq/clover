int a = 123;
int b = 123;

print("Object.ID() test...");
Clover.assert(a.ID() == b.ID() == false);
println("TRUE");

b = a;

print("Object.ID() test2...");
Clover.assert(a.ID() == b.ID() == false);
println("TRUE");

String c = "ABC";
String d = "DEF";

print("Object.ID() test3...");
Clover.assert(c.ID() == d.ID() == false);
println("TRUE");

c = d;

print("Object.ID() test4...");
Clover.assert(c.ID() == d.ID());
println("TRUE");

int e = 123;
int* f = e;

print("Object.ID() test5...");
Clover.assert(e.ID() == f.ID());
println("TRUE");

