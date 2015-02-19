VariableArgumentsTest a = new VariableArgumentsTest(123);

print("VariableArgumentsTest1...");
if(Clover.outputToString() { a.method("Format", "A", 1, "B"); } == "Format\nA\n1\nB\n") 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("VariableArgumentsTest2...");
if(Clover.outputToString() { a.method2("Format", 1, "A", "B", "C") int { return 123; }} == "Format\n1\nA\nB\nC\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("VariableArgumentsTest3...");
if(Clover.outputToString() { VariableArgumentsTest.method3("Format", 1, "A", "B", "C") int { return 123; }} == "Format\n1\nA\nB\nC\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

