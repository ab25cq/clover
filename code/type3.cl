
print("Type test...");
Clover.assert(Type.createFromString("Type") == Type);
println("TRUE");

print("Type test2...");
Clover.assert(Type.createFromString("Array<int>") == Array<int>);
println("TRUE");

print("Type test3...");
Clover.assert(Type.createFromString("Array$1") == Array$1);
println("TRUE");

print("Type test4...");
Clover.assert(Array<int>->class() == Array$1);
println("TRUE");

print("Type test5...");
Clover.assert(Array<int>->genericsParam(0) == int);
println("TRUE");

print("Type test6...");
Clover.assert(Hash<String, int>->genericsParamNumber() == 2);
println("TRUE");

Array<Type> array = new Array<Type>();

Type it = int;
while(it != null) {
    array.add(it);
    it = it.parentClass();
}

print("Type test7...");
Clover.assert(array == { int, NativeClass, Object });
println("TRUE");

print("Type test8...");
Clover.assert(int->parentClassNumber() == 2);
println("TRUE");

print("Type test9...");
Clover.assert(int->substitutionPosibility(NativeClass) == false);
println("TRUE");

print("Type test10...");
Clover.assert(NativeClass->substitutionPosibility(int));
println("TRUE");

print("Type test11...");
Clover.assert(String->substitutionPosibility(int) == false);
println("TRUE");

print("Type test12...");
Clover.assert(int->isInstance(123));
println("TRUE");
