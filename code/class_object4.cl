Field field = ClassClassFieldTest4->toClass().getFieldFromName("field4");

print("Class.getFieldFromName test...");
Clover.assert(field.index() == 1 && field.name() == "field4" && field.fieldType() == int);
println("TRUE");

print("Class.superClasses test...");
Clover.assert(ClassClassFieldTest4->toClass().superClasses() == { Object, UserClass, ClassClassFieldTest3 });
println("TRUE");

print("Class.implementedInterfaces test...");
Clover.assert(ClassClassFieldTest4->toClass().implementedInterfaces() == { IHashKey, IInspectable, ICloneable, IComparable});
println("TRUE");
