reffer "my_class2.cl"

class MyClass {
    int myField;
    int myField2; 
    int myField3;

    static int myMethod3(int x, int y) {
        return x + y;
    }

    static void myMethod2() {
        Clover.print("myMethod2");
    }

    static void myMethod() {
        int a;
        int b;

        a = b = MyClass.myMethod3("aaa", 1);
    }

    String myField3;
}

