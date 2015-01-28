BlockTest a = new BlockTest();

int b = 234;

int result = a.methodWithBlock2() int {|int num|
    int c = 345;

    print("Block Test1...");
    if(b == 234 && c == 345) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }

    println("num --> " + num.toString());
    
    return 123;
}

print("Block Test2...");
if(result == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
