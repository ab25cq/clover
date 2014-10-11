
GInteger a = new GInteger(100);
GInteger b = new GInteger(200);

GOperator<GInteger> c = new GOperator<GInteger>();

print("generics11.cl test1...");
if(c.call_plus_method(a, b).value == 300) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}
