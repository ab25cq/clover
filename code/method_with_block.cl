
int x = 99;
int y = 88;

Integer a = new Integer(3);

a.upto(5) { |int n|
    int z = 77;

    print("method with block test...");
    if((n == 3 && x == 99) || (n != 3 && x == 106) && y == 88 && z == 77) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    Integer b = new Integer(5);

    b.upto(7) { |int m|
        x = 104;
        int z = 87;

        print("method with block test2...");
        if(x == 104 && y == 88 && z == 87) {
            println("OK");
        }
        else {
            println("FALSE");
            System.exit(2);
        }
    }

    print("method with block test3...");
    if(x == 104 && z == 77) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    b.upto(7) { |int l|
        x = 105;
        z = 76;
    }

    print("method with block test4...");
    if(x == 105 && z == 76) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    x = 106;

    int l = 999;
    int m = 1000;

    print("method with block var test5...");
    if(l == 999 && m == 1000) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}

int z = 999;

print("method with block test6...");
if(x == 106) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

print("method with block test7...");
if(z == 999) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

a.method() { 
    println("method is called"); 
}

