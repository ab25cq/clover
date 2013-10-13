
class MyClass {
    int myField;
    int myField2;
    String myField3;

    MyClass() {
        this.myField3 = "hello world";

        return this;
    }
}

class MyClass {
    void myMethod() {
        Clover.print(this.myField3);
    }

    static void myStaticMethod() {
        Clover.print("Hello World");
    }
}
