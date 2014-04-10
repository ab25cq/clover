
Clover.print("field test....");

FieldInitializarTest a = new FieldInitializarTest();

int value1 = a.get_value1();
int value2 = a.get_value2();
int value3 = a.get_value3();

if(value1 == 123 && value2 == 234 && value3 == 345) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.println("value1 -- > " + value1.to_s());
    Clover.println("value2 -- > " + value2.to_s());
    Clover.println("value3 -- > " + value3.to_s());
    Clover.exit(1);
}

Clover.print("class field test...");

if(FieldInitializarTest.array.length() == 3 && FieldInitializarTest.array[0] == "aaa") {
    Clover.println("OK");
}
else {
    "FieldInitializarTest.array.length() --> ".print(); FieldInitializarTest.array.length().to_s().println();
    "FieldInitializarTest.array[0].print() --> ".print(); FieldInitializarTest.array[0].println();

    Clover.println("FALSE");
    Clover.exit(2);
}
