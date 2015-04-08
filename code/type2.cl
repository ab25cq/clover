int a = 1;

a.type().setValue(float);

print("Type test....");
if(a.type() == int && a == 1 && a * 2 == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

