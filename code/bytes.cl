
Bytes a = B"AAABBBCCC";

print("Bytes test...");
if(a[3].toString() == "B") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

a[0] = 123.toByte();

print("Bytes test2...");
if(a[0] == 123.toByte()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
