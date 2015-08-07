bool a = bool { 123; true }

print("block test1...");
Clover.assert(a == true);
println("TRUE");

print("block test2...");
Clover.assert(bool { 1+1; true } == true);
println("TRUE");

print("block test3...");
Clover.assert(bool { 1+1; false } == false);
println("TRUE");

print("block test4...");
Clover.assert(int { 1+2; 1+1 } == 2);
println("TRUE");

print("block test5...");
Clover.assert(int { "AAA"; 1+2; 1+1+1 } == 3);
println("TRUE");

int c = 123;
int d = 100;

print("block test6...");
Clover.assert(bool { c++; c == 124 } == true);
println("TRUE");

print("block test7...");
Clover.assert(bool { d++; d == 101 });
println("TRUE");

c = 123;
d = 100;

print("block test8...");
Clover.assert(bool { c++; c == 124 } && bool { d++; d == 101 });
println("TRUE");

