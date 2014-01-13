class Test {
    private int field1;

    Test(int a) { 
        self.field1 = a;
    }

    static void method() {
        Clover.print("I'm a class method without parametor");
    }

    void method() {
        Clover.print("I'm a non class method without parametor");
    }
}

class A {
    A() {}
}

class Test {
    void method() {
        inherit();
        Clover.print("HELLO WORLD");
    }
}
