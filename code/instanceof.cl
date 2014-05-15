
Object str = "String";

Clover.print("instanceof test...");
if(str.instanceof(String)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

Clover.print("instanceof test2...");
if("String2".instanceof(String)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

ClassName class_name = Object;

Clover.print("is_child test...");
if(str.is_child(class_name)) {
    println("TRUE");
} else {
    println("FALSE");
    System.exit(2);
}

