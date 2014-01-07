
class Base {
    Base() {}

    void method() {
        Clover.print("I'm Base.method().\n");
    }
    void method2() {
        Clover.print("I'm Base.method2().\n");
    }

}

class Extended extends Base {
    Extended() {}

    void method2() {
        Clover.print("I'm Extended. method2().\n");
    }
}

