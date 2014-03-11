
class Integer {
    int value;

    Integer(int value) {
        self.value = value;
    }

    void upto(int n) with void block {|int k|} {
        for(int i=self.value; i<n; i++) {
            block(i);
        }
    }

    void method() with void block {} {
        block();
    }
}
