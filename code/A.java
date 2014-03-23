
public class A {
    private int field1 = 111;
    private int field2 = 222;

    A (int a, int b) {
        this.field1 = a;
        this.field2 = b;
    }
}

class B {
    static A a = new A(1, 2);

    B() {
    }
}
