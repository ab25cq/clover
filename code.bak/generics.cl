
MyClass8<String> a = new MyClass8<String>();
a.set_field1("ABC");
Clover.println(a.get_field1());

MyClass8<String> b = CreateMyClass8.getGenericsObject();
b.set_field1("DDD");
Clover.println(b.get_field1());

//MyClass8<String, String> c = new MyClass8<String, String>();
