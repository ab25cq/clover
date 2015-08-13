double a = 1.1;

print("double test1...");
Clover.assert(a == 1.1);
println("TRUE");

print("double test2...");
Clover.assert(1.1 * 2.0 == 2.2);
println("TRUE");

/*
print("double test3...");
Clover.assert(2.0 - 1.1 == 0.9);
println("TRUE");
*/

print("double test4...");
Clover.assert(2.0 + 1.1 == 3.1);
println("TRUE");

print("double test5...");
Clover.assert(6.0 / 3.0 == 2.0);
println("TRUE");

print("double test6...");
Clover.assert(6.0 > 3.0);
println("TRUE");

print("double test7...");
Clover.assert(1.0 < 3.0);
println("TRUE");

print("double test8...");
Clover.assert(1.0 <= 3.0);
println("TRUE");

print("double test9...");
Clover.assert(6.0 >= 3.0);
println("TRUE");

print("double test9...");
Clover.assert(2.0 != 1.0);
println("TRUE");

print("double test10...");
Clover.assert(!(2.0 == 1.0));
println("TRUE");
