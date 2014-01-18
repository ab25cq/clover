reffer "Test.cl";

class MyClass {
    static void main() {
        Test.field = 999;
        Clover.print(Test.field.to_s());
    }
}
