
Field field = FieldTest4ClassA->toClass().getFieldFromName("field2");

print("Field test...");
Clover.assert(field.class() == FieldTest4ClassA && field.index() == 1);
println("TRUE");
