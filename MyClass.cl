
class MyClass {
    int myField;
    int myField2;
}

class MyClass {
    String myField3;

    MyClass() {
        this.myField3 = "hello world";
    }

/*
    MyClass(String str) {
        this.myField3 = str;
    }
*/
    void myMethod() {
        Clover.print(this.myField3);
    }
}
