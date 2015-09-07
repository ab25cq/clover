
int value = 123;

print("printf test...");
Clover.assert(sprintf("value --> %d value2 --> %s value3 --> %ls", value, B"ABC", "DEF") == "value --> 123 value2 --> ABC value3 --> DEF");
println("TRUE");

print("printf test2...");
Clover.assert(sprintf("%02d", 2) == "02");
println("TRUE");

print("printf test3...");
Clover.assert(sprintf("%-2d", -2) == "-2");
println("TRUE");

print("printf test4...");
Clover.assert(sprintf("%-2d", +2) == "2 ");
println("TRUE");

print("printf test5...");
Clover.assert(sprintf("% 2d", +2) == " 2");
println("TRUE");
