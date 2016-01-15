Command.touch("ABC");

p"ABC".to_stat().st_mode.toString().println();

Command.rm("ABC");


Command.touch("ABC");

p"ABC".to_stat().to_permission().toString().println();

Command.rm("ABC");

Command.touch("ABC");
p"ABC".to_stat().to_uid_t().toString().println();
Command.rm("ABC");

Command.touch("ABC");
p"ABC".to_stat().to_gid_t().toString().println();
Command.rm("ABC");

Command.touch("ABC");
p"ABC".to_stat().size().type().toString().println();
Command.rm("ABC");

Command.touch("ABC");
p"ABC".to_stat().st_ctime.to_tm().toString().println();
Command.rm("ABC");


tm time = new tm(System.time());
time.toString().println();

Command.mkdir("ABCDEFG");
print("S_ISDIR test...");
Clover.assert(p"ABCDEFG".to_stat().S_ISDIR());
println("TRUE");
Command.rmdir("ABCDEFG");

Command.touch("ABC");
print("S_ISREG test...");
Clover.assert(p"ABC".to_stat().S_ISREG());
println("TRUE");

Command.touch("ABC");
p"ABC".to_stat().to_uid_t().toString().println();
Command.rm("ABC");

Command.touch("ABC");
p"ABC".to_stat().to_permission().toString().println();
Command.rm("ABC");

print("Path toFile test...");
Command.echo("12345").write(p"ABC");
p"ABC".toFile() {|File file|
    Bytes data = new Bytes();
    file.read(data, 5);
    Clover.assert(data == B"12345");
}
Command.rm("ABC");
println("TRUE");

Clover.print("Path write test...");
p"ABC".write(B"12345");
Clover.assert(p"ABC".readOut() == B"12345");
println("TRUE");
Command.rm("ABC");

Command.echo("12345").write(p"ABC");
print("Path.readOut test...");
Clover.assert(p"ABC".readOut() == B"12345\n");
println("TRUE");

Command.rm("ABC");

print("Path.basename test...");
Clover.assert(p"/etc/aaa.txt".basename() == p"aaa.txt");
println("TRUE");

print("Path basename test2...");
Clover.assert(p"/etc/aaa.txt".basename(".txt") == p"aaa");
println("TRUE");

print("Path basename test3...");
Clover.assert(p"/etc/aaa.txt".basename(".*") == p"aaa");
println("TRUE");

print("Path.dirname test....");
Clover.assert(p"/etc/aaa.txt".dirname() == p"/etc");
println("TRUE");

print("Path.extname test...");
Clover.assert(p"/etc/aaa.txt".extname() == ".txt");
println("TRUE");

print("fnmatch test...");
Clover.assert(p"ABC.txt".fnmatch("*.txt") && !p"ABC.txt".fnmatch("*.c"));
println("TRUE");

print("Path.isIdentical test...");
Clover.assert(p"/etc/../bin".isIdentical(p"/./bin"));
println("TRUE");

print("Path.realpath test...");
Clover.assert(p"/./././usr/bin/..".realpath() == p"/usr");
println("TRUE");

p"/".entries().each() {|Path path|
    path.toString().println();
}

p"/etc/".glob("*.conf").each() {|Path path|
    path.toString().println();
}

File file = new File(p"aaa.txt", "w", 0644);
file.write(B"ABC");
file.close();
Command.rm("aaa.txt");

Command.echo("12345").write(p"ABC");
Bytes data3 = new Bytes();

File file2 = new File(p"ABC", "r");
file2.read(data3, 5);
file2.close();

print("File read test...");
Clover.assert(data3 == B"12345");
println("TRUE");

Command.rm("ABC");

Command.echo("12345").write(p"ABC");
print("Command test...");
Clover.assert(p"ABC".readOut() == B"12345\n");
println("TRUE");
Command.rm("ABC");
