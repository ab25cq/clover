
Bytes a = B"AAA";

a[0] = 123;

print("Bytes test...");
if(a[0] == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
