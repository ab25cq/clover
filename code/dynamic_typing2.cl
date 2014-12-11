DynamicTypingTest a = new DynamicTypingTest2();

print("Dynamic Typing Test1...");
if(a.method() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

DynamicTypingTest b = new DynamicTypingTest3();

b.method5();

1 + 1;

DynamicTypingTest c = new DynamicTypingTest3();
print("Dynamic Typing Test2...");
try {
    c.method2() {|int k|  }
} catch(Exception e) {
    if(e.type() == MethodMissingException) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(1);
    }
}

DynamicTypingTest d = new DynamicTypingTest3();
print("Dynamic Typing Test3...");
if(d.method2() int {|int k| return k; } == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


2 + 2;
