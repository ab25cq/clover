
class MyClass {
    int field;
    static int field2;

    void method() {
        Clover.print("Hello, I'm method");
    }
}

class MyClass {
    private String field3;

    MyClass() {
        self.field3 = "hello world";
    }

    MyClass(String message) {
        self.field3 = message;
    }

    private void method2() {
        Clover.print(self.field3);
    }

    void function() {
        MyClass a = new MyClass();
        a.field3 = "aaa";
    }
}

class MyClass2 {
    static void main() {
        MyClass b = new MyClass();
        b.field3 = "aaa";
        b.method2();
    }
}

