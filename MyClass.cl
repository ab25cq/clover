
class MyClass {
    int myField;
    static int myField2;

    void myMethod() {
        Clover.print("Hello, I'm myMethod");
    }
}

class MyClass {
    String myField3;

    MyClass() {
        this.myField3 = "hello world";
    }

    void myMethod2() {
        Clover.print(this.myField3);
    }
}
