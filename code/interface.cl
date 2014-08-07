
IBase a = new A();

print("interface test...");
if(a.get_field1(123) == 111 && a.get_field2() == 222 && a.get_field3(123,456) == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

