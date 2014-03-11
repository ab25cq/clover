class Base1 {
    private int value1;
    private int value2;

    Base1(int value1, int value2) {
        self.value1 = value1;
        self.value2 = value2;
    }

    void show() with void block {||} {
        Clover.print("Base::show");

        Clover.print("self.value1 --> " + self.value1.to_s());
        Clover.print("self.value2 --> " + self.value2.to_s());

        block();
    }

    void show2() with int block {||} {
        Clover.print("Base::show2");

        Clover.print("self.value1 --> " + self.value1.to_s());
        Clover.print("self.value2 --> " + self.value2.to_s());

        int a = block();

        Clover.print("block returns " + a.to_s());
    }

    static void show() with void block {||} {
        Clover.print("Base::show() static");
        block();
    }
}

class Extended1 extends Base1 {
    Extended1(int value1, int value2) {
        super(value1, value2);
    }

    void show() with void block {||} {
        Clover.print("Extended1::show");

        super();
    }

    void show2() {
        Clover.print("Extended1::show2");

        super() (int) { 
            Clover.print("calling super with block");
            revert 1;
        }
    }

    static void show() with void block {||} {
        Clover.print("Extended1::show() static");

        Base1.show() {
            Clover.print("block caled");
        }
    }
}

