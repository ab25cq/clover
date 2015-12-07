Array<String> argv = Clover.getCloverArgv();

print("Clover.getCloverArgv test....");
Clover.assert(argv == { "clover", "code/argv_test.cl" });
println("TRUE");

print("Clover.getCloverArgv test2....");
Clover.assert(Clover.argv == { "clover", "code/argv_test.cl" });
println("TRUE");
