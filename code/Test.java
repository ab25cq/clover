
class Test2 <T> {
    T field1;

    T method() {
        System.out.printlnln("Hello I'm Test2::method");

        return field1;
    }
}

class Test {
    public static void main(String args[]) {
        Test2<String> test = new Test2<String>();
        System.out.printlnln(test.method());

        int a = 1;

//        !1;
//        !a;
//

        boolean flg = true;
        boolean flg2 = !flg;
        //boolean flg3 = !1;

        /// for test ///
        for(int i=0; i<5; i++) {
            System.out.printlnln("Hello");
        }

        for(int i=0; i<5; i++) {
            System.out.printlnln("Hello");
        }

        int x = i;
    }
}
