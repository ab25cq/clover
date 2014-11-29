
byte a = 111.toByte();
byte b = 128.toByte();
byte c = a + b;

print("byte operator+ test...");
if(c == 239.toByte()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
