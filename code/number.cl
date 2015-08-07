byte a = 123y;
byte b = 100y;

print("byte test1...");
Clover.assert(a + b == 223y && a - b == 23y && a * b == 12300y && a / b == 1y);
println("TRUE");

print("byte test1-2...");
Clover.assert(bool { a++; a == 124y } && bool { b++; b == 101y });
println("TRUE");

print("byte test1-3...");
Clover.assert(bool { a--; a == 123y } && bool { b--; b == 100y });
println("TRUE");

byte c = 0xFFy;

print("byte test2...");
Clover.assert(c == 255y);
println("TRUE");

byte d = 020.toByte();

print("byte test3...");
Clover.assert(d == 16.toByte());
println("TRUE");

int e = -2;

print("int test1...");
Clover.assert(e.toString() == "-2" && e == -2);
println("TRUE");

byte f = -1y;

print("byte test4...");
Clover.assert(f.toString() == "255" && f == 255y);
println("TRUE");

short g = -1s;

print("short test1...");
Clover.assert(g.toString() == "65535" && g == 65535s);
println("TRUE");

uint h = -1u;

print("uint test1...");
Clover.assert(h.toString() == "4294967295" && h == 4294967295u);
println("TRUE");

long i = -1l;

print("long test1...");
Clover.assert(i.toString() == "18446744073709551615" && i == 18446744073709551615l);
println("TRUE");

