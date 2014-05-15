
New a = new New();
a.set_field1("ABC");

print("New test...");
if(a.call_get_field1() == "ABC") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}


New b = NewOtherClass.get_new_object();
b.set_field1("DEF");

print("New test2...");
if(b.call_get_field1() == "DEF") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

