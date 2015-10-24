pointer a = new pointer();

print("pointer test...");
Clover.assert(a.toString() == "(nil)" || a.toString() == "0x0");
println("TRUE");

Bytes b = B"ABC";

pointer c = b.toPointer();

print("pointer test2...");
Clover.assert(c.toString() != "(nil)" && c.toString() != "0x0");
println("TRUE");

print("pointer test3...");
Clover.assert(c.getByte().toInt().toChar() == 'A');
println("TRUE");

c.forward();

Clover.gc();

print("pointer test4...");
Clover.assert(c.getByte().toInt().toChar() == 'B');
println("TRUE");

c.forward();

Clover.gc();

print("pointer test5...");
Clover.assert(c.getByte().toInt().toChar() == 'C');
println("TRUE");
