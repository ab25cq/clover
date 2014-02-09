
class MyClass7 {
    String field1;

    MyClass7() {
    }

    String get_field1() {
        return self.field1;
    }

    void set_field1(String a) {
        self.field1 = a;
    }

    String call_get_field1() {
        return self.get_field1();
    }
}

class OtherClass2 {
    static MyClass7 getMyClass7Object() {
        return new MyClass7 ();
    }
}


