
Accessor a = new Accessor(111, 222);

Clover.print("field test...");
if(a.get_field1() == 111 && a.get_field2() == 222) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

a.set_field1(888);
a.set_field2(999);

Clover.print("field test2...");
if(a.get_field1() == 888 && a.get_field2() == 999) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}
