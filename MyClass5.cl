
final class Base {
    int field1;

    Base() {}

    virtual void show() {
        Clover.print("field1 " + self.field1.to_s());
    }

    void method() {
        Clover.print("I'm Base.method().");
    }
    virtual void method2() {
        Clover.print("I'm Base.method2().");
    }
}

class Extended extends Base {
    int field2;

    Extended(int a, int b) {
        self.field1 = a;
        self.field2 = b;
    }

    override void show() {
        super();
        Clover.print("field2 " + self.field2.to_s());
    }

    override void method2() {
        Clover.print("I'm Extended. method2().");
    }
}

