
String value = Clover.output_to_s() {
    void {
        Clover.println("HELLO BLOCK");
        Clover.println("I'm a block");
    }
}

print("block test1...");
if(value == "HELLO BLOCK\nI'm a block\n") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int a = int {
    111;
}

print("the result of block test2...");
if(a == 111) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
