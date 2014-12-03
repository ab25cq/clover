MyThread a = new MyThread();

print("MyThread test1...");
if(a.method() == 999 && a.type() == MyThread) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

/*
MyType b = new MyType();
print("MyType test1...");
if(b.method() == 1000 && b.type() == MyType) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
*/

MyString c = new MyString("aaa");
print("MyString test1...");
if(c.type() == MyString && c == "aaa" && c.method() == "aaa" && c + "a" == "aaaa") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

MyArray<String> d = new MyArray<String>({"a", "b", "c"});
print("MyArray test1...");
if(d.type() == MyArray<String> && d[0] == "a" && d.method()[1] == "b" && d == {"a", "b", "c"}) 
{
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

