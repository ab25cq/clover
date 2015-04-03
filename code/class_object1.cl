
Type a = Array<int>;

Array<int> b = a->classObject().newInstance();

b.add(123);
b.add(245);

print("Class Object test....");
if(b == { 123, 245 } ) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

