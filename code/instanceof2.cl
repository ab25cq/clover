
String a = new String();

print("instanceOf test1...");
Clover.assert(a.instanceOf(String));
println("TRUE");

InstanceOfTestA<String, int> b = new InstanceOfTestA<String, int>();

print("instanceOf test2...");
Clover.assert(b.instanceOf(InstanceOfTestA<String, int>));
println("TRUE");

print("instanceOf test3...");
Clover.assert(b.instanceOf(InstanceOfTestA<String, String>) == false);
println("TRUE");

