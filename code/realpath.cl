
print("realpath test...");
Clover.assert(System.realpath(p"/usr/include/.././.") == p"/usr");
println("TRUE");


