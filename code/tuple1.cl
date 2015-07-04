
Tuple<int, String> a = new Tuple<int, String>();

a.set1(1);
a.set2("AAA");

print("Tuple test1...");
if(a.get1() == 1 && a.get2() == "AAA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


Tuple<int, String> b = 2, "BBB" ;

print("Tuple test2...");
if(b.get1() == 2 && b.get2() == "BBB") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Tuple<int> c = new Tuple<int>();

c.set1(3);

print("Tuple test3...");
if(c.get1() == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

