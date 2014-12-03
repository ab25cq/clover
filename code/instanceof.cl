
Object str = "String";

Clover.print("instanceof test...");
if(str.instanceOf(String)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

Clover.print("instanceOf test2...");
if("String2".instanceOf(String)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

Type type = Object;

Clover.print("is_child test...");
if(str.isChild(type)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

