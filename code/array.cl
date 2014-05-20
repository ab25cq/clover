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

//Array<int> c = { "aaa", "bbb", "ccc" };
