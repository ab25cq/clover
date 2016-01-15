print("Method block test...");

BlockTestClassB b = new BlockTestClassB();
int c = b.method() {|int n|
    Clover.assert(caller.type() == BlockTestClassA);
    return n;
}

Clover.assert(c == 123);
println("TRUE");

print("Method block test2...");

int d = BlockTestClassB.method2() {|int n|
    return n;
}

Clover.assert(d == 123);
println("TRUE");
