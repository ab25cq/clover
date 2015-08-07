
FieldInitializer a = new FieldInitializer();

print("field initializer test2...");
if(a.get_field() == 123 && a.get_field2() == "AAA" && a.field3 == null && a.field4 == null && a.get_field5() == "ABC") 
{
    println("OK");
}
else {
    println("FALSE");
    System.exit(1);
}
