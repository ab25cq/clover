FieldsTestB a = new FieldsTestB();

a.field = 123;
a.field2 = "ABC";

print("Object.fields test...");
Clover.assert(a.fields(0) == 123 && a.fields(1) == "ABC");
println("TRUE");

a.setField(0, 234);
a.setField(1, "DEF");

print("Object.fields test...");
Clover.assert(a.fields(0) == 234 && a.fields(1) == "DEF");
println("TRUE");

Field field = FieldsTestB->toClass().getFieldFromName("field2");

print("Object fields test2...");
Clover.assert(field.index() == 1);
println("TRUE");
