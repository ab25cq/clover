
class Test2 <T> {
    T field1;

    T method() {
        System.out.println("Hello I'm Test2::method");

        return field1;
    }
}

class Test {
    public static void main(String args[]) {
        Test2<String> test = new Test2<String>();
        System.out.println(test.method());

        int a = 1;

//        !1;
//        !a;
//

        boolean flg = true;
        boolean flg2 = !flg;
        //boolean flg3 = !1;

        /// for test ///
        for(int i=0; i<5; i++) {
            System.out.println("Hello");
        }

        for(int i=0; i<5; i++) {
            System.out.println("Hello");
        }

        int x = i;
    }
}
