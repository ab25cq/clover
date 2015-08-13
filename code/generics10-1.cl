
Generics10_1Operator<String, GenericsTestInteger, GenericsTestInteger> operator = new Generics10_1Operator<String, GenericsTestInteger, GenericsTestInteger>();

GenericsTestInteger a = new GenericsTestInteger(111);
GenericsTestFloat b = new GenericsTestFloat(222.2f);
GenericsTestInteger c = new GenericsTestInteger(222);

print("Generics10-1 test1...");
if(a.value == 111 && b.value == 222.2f) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics10-1 test2...");
if(operator.call_plus_method(a, new GenericsTestInteger(222)).value == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Generics10-1 test3...");
if(operator.call_test_method(a, c)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
