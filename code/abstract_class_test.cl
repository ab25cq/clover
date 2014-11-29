
AbstractClassTestA a = new AbstractClassTestB();

print("abstract class test1...");
if(a.method1() == 111 && a.method2() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    println("a.method1() --> " + a.method1().toString());
    println("a.method2() --> " + a.method2().toString());
    System.exit(2);
}

AbstractClassTestC b = new AbstractClassTestD();

print("abstract class test2...");
if(b.method1() == 111 && b.method2() == 222 && b.method3() == 333 && b.method4() == 444) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

AbstractClassTestC c = new AbstractClassTestE();

print("abstract class test3...");
if(c.method1() == 111 && c.method2() == 222 && c.method3() == 333 && c.method4() == 444) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

AbstractClassTestA d = new AbstractClassTestE();

print("abstract class test4...");
if(d.method1() == 111 && d.method2() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

AbstractClassTestA e = new AbstractClassTestD();

print("abstract class test5...");
if(e.method1() == 111 && e.method2() == 222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

AbstractClassTestF f = new AbstractClassTestG();

print("abstract class test6...");
if(f.method1() == 111 && f.method2() == 222 && f.method3() == 333 && f.method4() == 444) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

