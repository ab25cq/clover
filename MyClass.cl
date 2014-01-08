
class Base {
    Base() {}

    void method() {
        Clover.print("I'm Base.method().");
    }
    virtual void method2() {
        Clover.print("I'm Base.method2().");
    }

}

class Extended extends Base {
    Extended() {}

    override void method2() {
        Clover.print("I'm Extended. method2().");
    }
}

