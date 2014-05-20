StaticMethodBlock.method1() int { |int n|
    print("static method block test...");
    if(n == 123) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(1);
    }
    return 1111;
}

StaticMethodBlock a = new StaticMethodBlock(111, 222);

a.method2() {
    int b = 333;
    print("method block var test...");
    if(a.get_field1() == 111 && a.get_field2() == 222 && b == 333) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}
