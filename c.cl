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
if(d == e) {
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

Class f = d.type().classObject();

print("Class Object test1...");
if(f.toString() == "GetFieldsTest") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Class Object test2...");
if(f.isSpecialClass() == false && "AAA".type().classObject().isSpecialClass() == true) {
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

anonymous array = type.classObject().newInstance();

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

