
class GenericsTest <T, T2> {
    T field1;

    GenericsTest() {
    }

    T get_field1() {
        return self.field1;
    }

    void set_field1(T a) {
        T b = self.field1;
        self.field1 = b;
    }

    T call_get_field1() {
        return self.get_field1();
    }
}

class OtherClass {
    static GenericsTest<String> getGenericsObject() {
        return new GenericsTest <String>();
    }
}

class RunGenericsTest {
    static void main() {
        GenericsTest<String> a = new GenericsTest<String>();
        a.set_field1("ABC");
        Clover.print(a.get_field1());

        GenericsTest<String> b = OtherClass.getGenericsObject();
    }
}
