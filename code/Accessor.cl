
Accessor a = new Accessor(111, 222);

print("field test...");
if(a.get_field1() == 111 && a.get_field2() == 222) {
    println("OK");
}
else {
    println("FALSE");
    Clover.exit(2);
}

a.set_field1(888);
a.set_field2(999);

print("field test2...");
if(a.get_field1() == 888 && a.get_field2() == 999) {
    println("OK");
}
else {
    println("FALSE");
    Clover.exit(2);
}
