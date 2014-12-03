
Clover.print("Array<int> test1...");

Array<int> e = { 111, 222, 333 };

if(e.length() == 3 && e[0] == 111 && e[1] == 222 && e[2] == 333 && e[-1] == 333) {
    Clover.println("OK");
} else {
    Clover.println("FALSE");
    System.exit(2);
}

print("Array<int> type name test...");

if(e.type() == Array<int>) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

Clover.print("Array<int> test2...");

if(e == {111, 222, 333}) {
    Clover.println("OK");
} else {
    Clover.println("FALSE");
    System.exit(2);
}

