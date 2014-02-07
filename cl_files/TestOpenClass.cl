
namespace Test;

open class MyClass2 {
    private int field;

    MyClass2() {
        self.field = 777;
    }

    void method() {
        Clover.print(self.field.to_s());
    }
}

inherit class MyClass2 {
    private String field2;

    MyClass2(String message) {
        self.field = 888;
        self.field2 = message;
    }

    void method2() {
        Clover.print(self.field2);
    }
}

namespace Test2;

class MyClass2 {
    static void main() {
        Test::MyClass2 a = new Test::MyClass2("HELLO");

        a.method();
        a.method2();
        a.method3();

        Test::MyClass2 b = new Test::MyClass2();

        b.method();
        //b.method2();
        b.method3();
    }
}

namespace Test;

inherit class MyClass2 {
    void method3() {
        Clover.print("Hello I'm Test::MyClass.method3()");
    }
}

