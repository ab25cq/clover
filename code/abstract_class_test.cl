
AbstractClassTestA a = new AbstractClassTestB();

print("abstract class test1...");
if(a.method1() == 111 && a.method2() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    println("a.method1() --> " + a.method1().to_string());
    println("a.method2() --> " + a.method2().to_string());
    System.exit(2);
}
