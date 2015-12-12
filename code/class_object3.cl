
print("Class.toType() test...");
Clover.assert(int->toClass().toType() == int);
println("TRUE");

print("Class.toType() test2...");
Clover.assert(Array<int>->toClass().toType() == Array<int>);
println("TRUE");


Class class = Array<String>->toClass();
Array<String> a = class.newInstance();

a.add("AAA");
a.add("BBB");
a.add("CCC");

print("Class.newInstance() test...");
Clover.assert(a.type() == Array<String> && a == {"AAA", "BBB", "CCC"});
println("TRUE");

Array<Field> b = ClassClassFieldTest2->toClass().fields();

print("Class.fields() test...");
Clover.assert(b.length() == 2 && b[0].name() == "field3" && b[1].name() == "field4");
println("TRUE");

Array<Method> c = ClassClassMethodTest2->toClass().methods();

print("Class.methods() test...");
Clover.assert(c.length() == 6 && c[4].name() == "method3" && c[5].name() == "method4");
println("TRUE");

Array<Method> d = ClassClassMethodTest2->toClass().constructors();

print("Class.constructors() test...");
Clover.assert(d.length() == 4);

