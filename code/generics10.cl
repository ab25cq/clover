
Generics10Operator<String, GenericsTestInteger, GenericsTestInteger> operator = new Generics10Operator<String, GenericsTestInteger, GenericsTestInteger>();

GenericsTestInteger a = new GenericsTestInteger(111);
Float b = new Float(222.2);

print("Generics10 test1...");
if(a.value == 111 && b.value == 222.2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics10 test2...");
if(operator.call_plus_method(a, new GenericsTestInteger(222)).value == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

