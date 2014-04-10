
int x = 99;
int y = 88;

Integer a = new Integer(3);

a.upto(5) { |int n|
    int z = 77;

    Clover.print("method with block test...");
    if((n == 3 && x == 99) || (n != 3 && x == 106) && y == 88 && z == 77) {
        Clover.println("OK");
    }
    else {
        Clover.println("FALSE");
        Clover.exit(2);
    }

    Integer b = new Integer(5);

    b.upto(7) { |int m|
        x = 104;
        int z = 87;

        Clover.print("method with block test2...");
        if(x == 104 && y == 88 && z == 87) {
            Clover.println("OK");
        }
        else {
            Clover.println("FALSE");
            Clover.exit(2);
        }
    }

    Clover.print("method with block test3...");
    if(x == 104 && z == 77) {
        Clover.println("OK");
    }
    else {
        Clover.println("FALSE");
        Clover.exit(2);
    }

    b.upto(7) { |int l|
        x = 105;
        z = 76;
    }

    Clover.print("method with block test4...");
    if(x == 105 && z == 76) {
        Clover.println("OK");
    }
    else {
        Clover.println("FALSE");
        Clover.exit(2);
    }

    x = 106;
}

Clover.print("method with block test5...");
if(x == 106) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

a.method() { 
    Clover.println("method is called"); 
}
