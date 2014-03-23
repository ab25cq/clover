
Clover.load("Integer");

int x = 99;
int y = 88;

Integer a = new Integer(3);

a.upto(5) { |int n|
    int z = 77;

    Clover.println("*** head of upto(5)");
    Clover.println("n ---> " + n.to_s());
    Clover.println("x ---> " + x.to_s());
    Clover.println("z --> " + z.to_s());

    Integer b = new Integer(5);

    b.upto(7) { |int m|
        x = 104;
        Clover.println("upto in upto1 x --> " + x.to_s());
    }
    Clover.println("x ---> " + x.to_s());

    b.upto(7) { |int l|
        x = 105;
        Clover.println("upto in upto2 x --> " + x.to_s());
        z = 76;
    }

    Clover.println("x ---> " + x.to_s());
    Clover.println("z ---> " + z.to_s());
    x = 106;
    Clover.println("*** tail of upto(5)");
}

Clover.println("affter upto2 x --> " + x.to_s());


a.method() { 
    Clover.println("method is called"); 
}

