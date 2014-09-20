
ITestInterface2 a = new TestInterface2(111, 222);
TestInterface2 b = new TestInterface2Child(111, 222);
ITestInterface2 c = new TestInterface2Child(111, 222);

print("Test interface2...");
if(a.get_field1() == 555 && a.get_field2() == 222 && b.get_field1() == 555 && b.get_field2() == 222 && c.get_field1() == 555 && c.get_field2() == 222) 
{
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
