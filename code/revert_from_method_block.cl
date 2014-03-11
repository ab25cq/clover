class RevertTest {
    private int field1;

    RevertTest(int value) {
        self.field1 = value;
    }

    void method() with int block {|int n|} {
        int result = block(111);
        Clover.print("block of RevertTest::method() returns " + result.to_s());
    }

    void method2() with void block {} {
        block();
    }

    int method3() {
        Clover.print("method3 starts");
        RevertTest a = new RevertTest(123);
        for(int i=0; i<100; i++) {
            a.method2() {
                if(true) {
                    a.method2() {
                        while(true) {
                            a.method2() {
                                return 999;
                            }
                        }
                    }
                }
            }
        }
        Clover.print("method3 finished");
    }
}
