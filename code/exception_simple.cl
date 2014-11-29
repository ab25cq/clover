
int a = 111;
int b = 222;
int c = 333;

try {
    throw new Exception("AAA");
}
catch(Exception e) {
    int d = 444;

    print("Test...");
    if(e.instanceOf(Exception)) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    print("Test2...");
    if(a == 111 && b == 222 && c == 333 && d == 444) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}

a = 111 + 111;

try {
    throw new Exception("BBB");
}
catch(Exception e) {
    int d = 444;

    print("Test3...");
    if(e.instanceOf(Exception)) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    print("Test4...");
    if(a == 222 && b == 222 && c == 333 && d == 444) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}

a = 222 + 222;
