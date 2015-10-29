pointer a = B"ABCDEFG".toPointer();

a.forward(2);
a.backward();

print("pointer test...");
Clover.assert(a.getByte().toInt().toChar() == 'B');
println("TRUE");
