System.setenv("ABC", "1", 1);

print("setenv test....");
Clover.assert(System.getenv("ABC") == "1");
println("TRUE");

System.setenv("ABC", "2", 0);

print("setenv test2....");
Clover.assert(System.getenv("ABC") == "1");
println("TRUE");

System.setenv("ABC", "2", 1);

print("setenv test3....");
Clover.assert(System.getenv("ABC") == "2");
println("TRUE");
