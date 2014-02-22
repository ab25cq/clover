
class Integer {
    int value;

    Integer(int value) {
        self.value = value;
    }

    void upto(int n) with void block {|int n|} {
        for(int i=self.value; i<n; i++) {
            block(i);
        }
    }
}
