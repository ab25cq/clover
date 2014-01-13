
class Base {
    int field1;

    Base() {}

    void show() {
        Clover.print("field1 " + self.field1.to_s());
    }

    void method() {
        Clover.print("I'm Base.method().");
    }
    void method2() {
        Clover.print("I'm Base.method2().");
    }
}

class Extended extends Base {
    int field2;

    Extended(int a, int b) {
        self.field1 = a;
        self.field2 = b;
    }

    void show() {
        super();
        Clover.print("field2 " + self.field2.to_s());
    }

    void method2() {
        Clover.print("I'm Extended. method2().");
    }
}

class Base {
    void show() {
        inherit();
        Clover.print("X field1 " + self.field1.to_s());
    }
    void method3() {
        Clover.print("I'm Base.method3().");
    }
}

