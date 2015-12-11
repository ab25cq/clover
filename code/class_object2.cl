Object a = new GetFieldsTest();

print("Object test1...");
if(a.numFields() == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Object test2...");
if(a.fields(0) == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Object test3...");
if(a.fields(1) == "AAA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Object d = new GetFieldsTest();
Object e = d;

print("Object test4...");
if(d.ID() == e.ID()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

d.setField(0, 245);

print("Object test5...");
if(d.fields(0) == 245 && e.fields(0) == 245) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Class f = d.type().toClass();

print("Class Object test1...");
if(f.toString() == "GetFieldsTest") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Object test2...");
if(f.isNativeClass() == false && "AAA".type().toClass().isNativeClass() == true) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Object g = f.newInstance();

print("Class Object test3...");
if(g.type() == GetFieldsTest) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Type type = Array<int>;

anonymous array = type.toClass().newInstance();

array.add(1);
array.add(2);
array.add(3);

print("Class Object test4...");
if(array.length() == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

ClassObjectTest<int> class_object_test = new ClassObjectTest<int>();

print("Class Object test5...");
if(class_object_test.field == { 1,2,3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Object test6...");
if(class_object_test.createArray() == { 1,2,3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<Field> fields = GetFieldsTest->toClass().fields();

print("field test1...");
if(fields.length() == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

fields.each() {|Field field, int i|
    println((i+1).toString() + " --> " + field.toString());
}

print("field test2...");
if(fields[0].isStaticField() == false && fields[0].isPrivateField() == false && fields[0].isProtectedField() == false && fields[0].isPublicField() == true)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("field test3...");
if(fields[0].name() == "field1" && fields[0].fieldType() == int && fields[1].name() == "field2" && fields[1].fieldType() == String) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<Method> methods = GetFieldsTest->toClass().methods();

print("Method test1...");
if(methods.length() == 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Method test2...");
if(methods[4].parametors() == { String, float, int } && methods[4].blockParametors() == { int, float } && methods[4].blockResultType() == String && methods[4].resultType() == String && methods[4].exceptions() == { NullPointerException, Exception }) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

methods.each() {|Method method, int n|
    println(n.toString() + " --> " + method.toString());
}

print("Class test1...");
if(int->toClass() == int->toClass()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class test2...");
if(int->toClass().isNativeClass() && int->toClass().isStruct() && Method->toClass().isFinalClass() && IComparableMore->toClass().isInterface()) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class test3...");
if(NullPointerException->toClass().superClasses() == { Object, UserClass, Exception })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class test4...");

if(Object->toClass().implementedInterfaces() == { IHashKey, IInspectable, ICloneable })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Object->toClass().classDependences().toString().println();
