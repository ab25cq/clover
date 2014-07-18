String a = "ABC";
a[0] = a[-1] = 'E';

print("string test2...");
if(a == "EBE") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

String b = "DEF";

b[0] = b[-1] = 'あ';

print("string test3...");
if(b == "あEあ") {
    println("TRUE");
}
else {
    println("FALSE");
    println("b --> " + b);
    System.exit(2);
}

StringTest2.field2 = "ABC";

StringTest2.field2[0] = StringTest2.field2[-1] = 'い';

print("string test4...");
if(StringTest2.field2 == "いBい") {
    println("TRUE");
}
else {
    println("FALSE");
    println("StringTest2.field2 --> " + StringTest2.field2);
    System.exit(2);
}

StringTest2 c = new StringTest2();

c.field1 = "DEF";
c.field1[0] = c.field1[-1] = 'う';

print("string test5...");
if(c.field1 == "うEう") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

String d = "あいうえお";
Bytes e = d.to_bytes();

print("string to bytes test...");
if(e == B"あいうえお" && e.to_string() == "あいうえお") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

