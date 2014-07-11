
Bytes a = B"AAA";
byte b = 123;

a[0] = b;

print("Bytes test...");
if(a[0] == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
