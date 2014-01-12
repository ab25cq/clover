class Test {
    private int field1;

    Test(int a) { 
        self.field1 = a;
    }

    void method() {
        Clover.print("HELLO");
        Clover.print(self.field1.to_s());
    }

    static void test() {
        Test a = new Test(111);
        a.method();
    }
}
