class MyClass {
    private int field1;

    MyClass(int a) { 
        self.field1 = a;
    }

    static void method() {
        Clover.print("I'm a class method without parametor");
    }

    static void method(int a) {
        Clover.print("I'm a class method with parametor");
    }

    void method() {
        Clover.print("I'm a non class method without parametor");
    }
}

class A {
    A() {}
}

inherit class MyClass {
    inherit void method() {
        inherit();
        Clover.print("HELLO WORLD on non class method");
    }

    inherit static void method() {
        inherit(1);
        Clover.print("HELLO WORLD on class method");
    }
}
