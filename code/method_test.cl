MethodClassTestA2 a = new MethodClassTestA2(111, 222);

a.field1 = 333;

int c = MethodClassTestA2->toClass().getMethodFromNameAndParametorTypes("method", { int, int }).invokeMethod(a, 111, 222);
int d = a.type().toClass().methods()[5].invokeMethod(a); // int method2()

print("Method Refrection Test1...");
if(c == 222 && d == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
