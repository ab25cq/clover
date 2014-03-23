Clover.load("MyClass");
Clover.load("A");

MyClass a = new MyClass(123);
a.method();

MyClass b = new MyClass(555);
b.method();

MyClass c = new MyClass(666);
c.method();

a.method();
b.method();
c.method();

