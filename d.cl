
print("Directory test1...");
Clover.assert(Directory.entries(p"man") == { ".", "..", "man1" });
println("TRUE");

print("Directory test2...");
Clover.assert(Directory.glob(p"src", "xfun*") == { "xfunc.c", "xfunc.o" });
println("TRUE");

print("Directory test3...");
Clover.assert(p"man".entries() == { ".", "..", "man1" });
println("TRUE");

print("Directory test4...");
Clover.assert(p"src".glob("xfun*") == { "xfunc.c", "xfunc.o" });
println("TRUE");

Directory.chdir(p"man");

print("Directory test5...");
Clover.assert(p".".entries() == { ".", "..", "man1" });
println("TRUE");

p"..".chdir();


