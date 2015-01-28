
int a = 1;
int b = 2;

a = b;

a.setValue(123);

println("a --> " + a.toString());
println("b --> " + b.toString());

print("struct test1...");
if(a == 123 && b == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

StructFieldTest struct_field_test = new StructFieldTest();

int c = 1;

struct_field_test.field = c;
struct_field_test.field.setValue(100);

println("c --> " + c.toString());
println("struct_field_test.field --> " + struct_field_test.field.toString());

print("struct test2...");
if(c == 1 && struct_field_test.field == 100) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int d = 2;

struct_field_test.method(d);

print("struct test3...");
if(d == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int e = 1;
int* f = e;
f.setValue(2);

print("struct* test1...");
if(e == 2 && f == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

String g = "ABC";
String h = g;
h.setValue("DEF");

print("struct* test2...");
if(g == "DEF" && h == "DEF") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

String i = "ABC";
String* j = i;
j.setValue("DEF");

print("struct* test3...");
if(i == "ABC" && j == "DEF") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
