
print("Directory test1...");
Clover.assert(Directory.entries(p"man") == { "man1" });
println("TRUE");

print("Directory test2...");
Clover.assert(Directory.glob(p"src", "xfun*") == { "xfunc.c", "xfunc.o" });
println("TRUE");

print("Directory test3...");
Clover.assert(p"man".entries() == { "man1" });
println("TRUE");

print("Directory test4...");
Clover.assert(p"src".glob("xfun*") == { "xfunc.c", "xfunc.o" });
println("TRUE");

p"man".chdir();

print("Directory test5...");
Clover.assert(p".".entries() == { "man1" });
println("TRUE");

p"..".chdir();


p"TEST_DIR".mkdir();

print("Directory test6...");
Clover.assert(p"TEST_DIR".to_stat().S_ISDIR());
println("TRUE");

p"TEST_DIR".rmdir();

print("Directory test6...");
Clover.assert(!p"TEST_DIR".existance());
println("TRUE");

print("isatty test1...");
Clover.assert(System.isatty(0));
println("TRUE");

File a = new File(p"ABC", "w");

print("isatty test2...");
Clover.assert(!System.isatty(a.descriptor));
println("TRUE");

a.close();

Command.rm("ABC");
