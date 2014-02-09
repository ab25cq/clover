
class MyClass8 <T> {
    T field1;

    MyClass8() {
    }

    T get_field1() {
        return self.field1;
    }

    void set_field1(T a) {
        self.field1 = a;
    }

    T call_get_field1() {
        return self.get_field1();
    }

    void test() {
        T a = new T();
    }
}

class CreateMyClass8 {
    static MyClass8 <String> getGenericsObject() {
        return new MyClass8 <String>();
    }
}


