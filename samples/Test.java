
class Test2 <T> {
    T field1;

    T method() {
        System.out.println("Hello I'm Test2::method");

        field1 = "ABC";

        return field1;
    }
}

class Test {
    public static void main(String args[]) {
        Test2<String> test = new Test2<String>();
        System.out.println(test.method());
    }
}
