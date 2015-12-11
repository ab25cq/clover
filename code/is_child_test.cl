ObjectChildTestA a = new ObjectChildTestA();

print("Object.isChild test...");
Clover.assert(a.isChild(String));
println("TRUE");

ObjectChildTestC<int> b = new ObjectChildTestC<int>();

print("Object.isChild test2....");
Clover.assert(b.isChild(ObjectChildTestC$1));
println("TRUE");

print("Object.isChild test3....");
Clover.assert(b.isChild(ObjectChildTestB$2));
println("TRUE");


