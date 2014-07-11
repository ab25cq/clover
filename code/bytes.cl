
Bytes a = B"AAABBBCCC";

print("Bytes test...");
if(a[3].to_string() == "B") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

a[0] = 123.to_byte();

print("Bytes test2...");
if(a[0] == 123.to_byte()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
