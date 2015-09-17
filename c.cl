Time time = File.atime(p"XXX");

print("File test....");
Clover.assert(time.month() == 9 && time.dayOfMonth() == 11);
println("TRUE");

print("File test2....");
Clover.assert(File.basename(p"clover/clover.cl") == p"clover.cl");
println("TRUE");

print("File test3...");
Clover.assert(File.basename(p"clover/clover.cl", ".cl") == p"clover");
println("TRUE");

print("File test4...");
Clover.assert(File.basename(p"clover/clover.cl", ".*") == p"clover");
println("TRUE");

print("File test5...");
Clover.assert(File.isBlockDevice(p"XXX") == false);
println("TRUE");

print("File test6...");
Clover.assert(File.isRegularFile(p"XXX") == true);
println("TRUE");

Command.mkdir("YYY");

print("File test7...");
Clover.assert(File.isDirectory(p"YYY") == true);
println("TRUE");

Command.rmdir("YYY");

Command.touch("AAA");
File.chmod(p"AAA", 0755.toLong().to_mode_t());

print("File test8...");
Clover.assert(File.permission(p"AAA") == 0755.toLong().to_mode_t());
println("TRUE");

File.chown(p"AAA");

print("File test9...");
Clover.assert(File.uid(p"AAA") == System.getuid() && File.gid(p"AAA") == System.getgid());
println("TRUE");

Command.rm("AAA");

Command.touch("BBB");

print("File test10...");
Clover.assert(File.access(p"BBB", AccessMode.F_OK) == 0);
println("TRUE");

File.unlink(p"BBB");

print("File test11...");
Clover.assert(File.access(p"BBB", AccessMode.F_OK) != 0);
println("TRUE");

print("File test12...");
Clover.assert(File.dirname(p"dir/file.ext") == p"dir" && File.dirname(p"file.ext") == p"." && File.dirname(p"foo/bar/") == p"foo" && File.dirname(p"foo//bar") == p"foo");
println("TRUE");

print("FIle test13...");
Clover.assert(File.extname(p"foo/foo.txt") == p".txt" && File.extname(p"foo/foo.tar.gz") == p".gz" && File.extname(p"foo/bar") == p"" && File.extname(p"foo/.bar") == p"" && File.extname(p"foo.txt/bar") == p"" && File.extname(p".foo") == p"");
println("TRUE");


print("File test14...");
Clover.assert(p"ABC".toString() == "ABC" && "ABC".toPath() == p"ABC");
println("TRUE");

