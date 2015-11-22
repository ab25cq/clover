MethodClassTestA a = new MethodClassTestA(111, 222);

int b = a.type().classObject().fields()[0].get(a);

print("Field Refrection Test1...");
if(b == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

a.type().classObject().fields()[0].set(a, 333);

print("Field Refrection Test2...");
if(a.field1 == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int c = a.type().classObject().methods()[3].invokeMethod(a, 111, 222);
int d = a.type().classObject().methods()[4].invokeMethod(a);

print("Method Refrection Test1...");
if(c == 222 && d == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int e = a.type().classObject().methods()[5].invokeMethod(a, 1,2,3);

print("Method Refrection Test2...");
if(e == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test1...");
if(int->classObject().toType() == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test2...");
if(Array<int>->classObject().toType() == Array<int>) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test3...");
if(Type.createFromString("int") == int) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test4...");
if(Type.createFromString("Array<int>")== Array<int>) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test5...");
if(Type.createFromString("Tuple<Array<int>, String, float>")== Tuple<Array<int>, String, float>) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

NameSpaceTest1 ::RefrectionTestB aa = new NameSpaceTest1 ::RefrectionTestB(111, 222);

print("Class Refrection Test6...");
if(aa.type() == NameSpaceTest1 ::RefrectionTestB) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Type bb = Type.createFromString("NameSpaceTest1::RefrectionTestB");

print("Class Refrection Test7...");
if(bb == NameSpaceTest1::RefrectionTestB) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


print("Class Refrection Test8...");
if(Array<int>->class() == Array$1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type class Test1...");
if(int->substitutionPosibility(float) == false) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Type class Test2...");
if(Object->substitutionPosibility(float) == true) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test9...");
if(MethodClassTestA->classObject().constructors().length() == 1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test10...");
if(MethodClassTestA->classObject().getFieldFromName("field1").fieldType() == int) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

MethodClassTestA cc = new MethodClassTestA(1, 2);

print("Class Refrection Test11...");
if(MethodClassTestA->classObject().getMethodFromNameAndParametorTypes("method", { int, int }).invokeMethod(cc, 111, 222) == 222)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

GenericsRefrectionClassB grc1 = new GenericsRefrectionClassB();

print("Class Refrection Test12...");
if(GenericsRefrectionClassA<int, GenericsRefrectionClassB>->classObject().genericsParametorTypes()[0].extendsType() == int)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test13...");
if(GenericsRefrectionClassA<int, GenericsRefrectionClassB>->classObject().genericsParametorTypes()[1].implementedInterfaces() == { IHashKey, IInspectable })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


print("Class Refrection Test14...");
if(GenericsRefrectionClassA<int, GenericsRefrectionClassB>->classObject().genericsParametorTypes()[1].extendsType() == null)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Refrection Test15...");
if(GenericsRefrectionClassA<int, GenericsRefrectionClassB>->classObject().genericsParametorTypes()[0].implementedInterfaces() == {})
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
