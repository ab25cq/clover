
print("realpath test...");
Clover.assert(System.realpath(p"/usr/bin/.././.") == p"/usr");
println("TRUE");


