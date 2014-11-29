
DTInterface a = new DTClassA();

a.setField(120);

print("Dynamic Typing test1...");
if(a.getField().toInt() == 120) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
