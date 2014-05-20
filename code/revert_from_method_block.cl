
RevertTest test = new RevertTest(111);

int value = test.method() int { |int n|
    print("method block parametor test...");
    if(n == 111) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
    return 222;
}

print("method block value test...");
if(value == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int a = test.method3();

Clover.print("the value which test.method3() returns test...");
if(a == 999) {
    Clover.println("TRUE");
}
else {
    Clover.println("False");
    System.exit(1);
}

