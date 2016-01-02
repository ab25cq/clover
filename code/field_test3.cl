Field field3 = FieldClassTest1->toClass().getFieldFromName("field3");
Field field4 = FieldClassTest1->toClass().fields()[3];

field4.setClassFieldValue(123);

print("Field test...");
Clover.assert(FieldClassTest1.field4 == 123 && field3.classFieldValue() == 111);
println("TRUE");

Field field5 = FieldClassTest2->toClass().getFieldFromName("field5");

FieldClassTest2 object = new FieldClassTest2();

object.setField(field5.index(), 123);

print("Field test2...");
Clover.assert(object.field5 == 123);
println("TRUE");
