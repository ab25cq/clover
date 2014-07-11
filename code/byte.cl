
byte a = 111.to_byte();
byte b = 128.to_byte();
byte c = a + b;

print("byte operator+ test...");
if(c == 239.to_byte()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
