Clover.load("Test::Object");
Clover.load("Test::void");
Clover.load("Test::int");
Clover.load("Test::float");
Clover.load("Test::String");
Clover.load("Test::Clover");
Clover.load("Test::Array");

Clover.show_classes();

Test::int a = 7;
Clover.println(a.to_s());

int b = a + 3;
Clover.println(b.to_s());

Test::Array<String> c = new Test::Array<String>();

Clover.println("test value is " + new Test::Array<String>().class_name());

Clover.println("class name of c is " + c.class_name());

c.println();

