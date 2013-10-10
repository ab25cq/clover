
class MyClass {
    int myField;
    String myField2;

    MyClass() {
        this.myField2 = "hello world";

        return this;
    }

    void myMethod() {
        Clover.print(this.myField2);
    }

    static void myStaticMethod() {
        Clover.print("Hello World");
    }
}

