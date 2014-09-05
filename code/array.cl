Array<String> a = new Array<String>();

a.add("AAA");
a.add("BBB");
a.add("CCC");

Clover.print("Array.length() test...");
if(a.length() == 3) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    System.exit(1);
}

Clover.print("Array.items() test...");
if(a.items(0) == "AAA" && a.items(1) == "BBB" && a.items(2) == "CCC") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    System.exit(1);
}

Clover.print("Array.[] test...");
if(a[0] == "AAA" && a[1] == "BBB" && a[2] == "CCC" && a[-1] == "CCC") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    System.exit(1);
}

Clover.print("Array<String> test...");

Array<String> b = { "aaa", "bbb", "ccc" };

if(b.length() == 3 && b[0] == "aaa" && b[1] == "bbb" && b[2] == "ccc" && b[-1] == "ccc") {
    Clover.println("OK");
} else {
    Clover.println("FALSE");
    System.exit(2);
}

Array<Array<String>> c = new Array<Array<String>>();
c.add({"aaa", "bbb", "ccc"});
c.add({"ddd", "eee", "fff"});

Array<String> c2 = c[0];

Clover.print("Array<Array<String>> test...");
//if(c.length() == 2 && c[0][0] == "aaa" && c[1][0] == "ddd" && c[0] == {"aaa", "bbb", "ccc"}) {
if(c.length() == 2 && c[0][0] == "aaa" && c[1][0] == "ddd") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    System.exit(2);
}

Array<Array<Array<String>>> d = new Array<Array<Array<String>>>();
d.add({{"aaa", "bbb", "ccc"}, {"ddd", "eee", "fff"}});
d.add({{"ggg", "hhh", "iii"}, {"jjj", "kkk", "lll"}});

Clover.print("Array<Array<Array<String>>> test...");
if(d.length() == 2) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    System.exit(2);
}

//c.add({1, 2, 3});
//Array<int> c = { "aaa", "bbb", "ccc" };
