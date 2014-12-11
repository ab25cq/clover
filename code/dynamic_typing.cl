
DTInterface a = new DTClassA();

a.setField(120);

print("Dynamic Typing test1...");
if(a.getField() == 120) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Dynamic Typing test2...");
try {
    a.getField().test() { |int a|
    }
} catch(Exception e) {
    if(e.type() == MethodMissingException) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}
