
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

