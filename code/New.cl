
New a = new New();
a.set_field1("ABC");

Clover.print("New test...");
if(a.call_get_field1() == "ABC") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}


New b = NewOtherClass.getNewObject();
b.set_field1("DEF");

Clover.print("New test2...");
if(b.call_get_field1() == "DEF") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

