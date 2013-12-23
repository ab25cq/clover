
class MyClass {
    private int field;

    MyClass() {
        self.field = 777;
    }

    void method() {
        Clover.print(self.field.to_s());
    }
}

class MyClass {
    private String field2;

    MyClass(String message) {
        self.field = 888;
        self.field2 = message;
    }

    void method2() {
        Clover.print(self.field2);
    }
}

