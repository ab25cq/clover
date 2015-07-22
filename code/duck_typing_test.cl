DynamicTypingTest a = new DynamicTypingTest();

print("Duck typing test1...");
if(a.method(1,2) {|int a, int b|
    return b;
} == 2)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
