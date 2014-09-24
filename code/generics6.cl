
OperatorGenericsTest6<float, int, GenericsTest6<int>> a = new OperatorGenericsTest6<float, int, GenericsTest6<int>>(2.0, 3, new GenericsTest6<int>(123, 456));

print("generics6 test1...");
if(a.operator1(1.0, 2).to_string() == "2") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

print("generics6 test2...");
if(a.get_value2_in_value3().to_string() == "456") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("generics6 test3...");
if(a.get_value1_in_value3().to_string() == "123") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
