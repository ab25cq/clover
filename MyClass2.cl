
namespace Test;

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

namespace Test2;

class MyClass {
    static void main() {
        Test::MyClass a = new Test::MyClass();

        a.method();
        a.method2();
        a.method3();
    }
}

namespace Test;

class MyClass {
    void method3() {
        Clover.print("Hello I'm Test::MyClass.method3()\n");
    }
}


