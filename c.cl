time_t actime = new Time(2015@year, 9@month, 11@day_of_month, 23@hour, 0@minuts, 0@sec).to_time_t();

p"XXX".changeAccessAndModificationTimes(actime, System.time()@modtime);

Time time2 = p"XXX".toFileStatus().accessTime();

print("File test....");
Clover.assert(time2.month() == 9 && time2.dayOfMonth() == 11);
println("TRUE");

print("File test2....");
Clover.assert(p"clover/clover.cl".basename() == p"clover.cl");
println("TRUE");

print("File test3...");
Clover.assert(p"clover/clover.cl".basename(".cl") == p"clover");
println("TRUE");

print("File test4...");
Clover.assert(p"clover/clover.cl".basename(".*") == p"clover");
println("TRUE");

print("File test5...");
Clover.assert(p"XXX".toFileStatus().isBlockDevice() == false);
println("TRUE");

print("File test6...");
Clover.assert(p"XXX".toFileStatus().isRegularFile() == true);
println("TRUE");

Command.mkdir("YYY");

print("File test7...");
Clover.assert(p"YYY".toFileStatus().isDirectory() == true);
println("TRUE");

Command.rmdir("YYY");

Command.touch("AAA");
p"AAA".chmod(0755.toLong().to_mode_t());

print("File test8...");
Clover.assert(p"AAA".toFileStatus().permission() == 0755.toLong().to_mode_t());
println("TRUE");

p"AAA".chown();

print("File test9...");
Clover.assert(p"AAA".toFileStatus().uid() == System.getuid() && p"AAA".toFileStatus().gid() == System.getgid());
println("TRUE");

Command.rm("AAA");

Command.touch("BBB");

print("File test10...");
Clover.assert(p"BBB".access(AccessMode.F_OK) == 0);
println("TRUE");

p"BBB".unlink();

print("File test11...");
Clover.assert(p"BBB".access(AccessMode.F_OK) != 0);
println("TRUE");

print("File test12...");
Clover.assert(p"dir/file.ext".dirname() == p"dir" && p"file.ext".dirname() == p"." && p"foo/bar/".dirname() == p"foo" && p"foo//bar".dirname() == p"foo");
println("TRUE");

print("FIle test13...");
Clover.assert(p"foo/foo.txt".extname() == p".txt" && p"foo/foo.tar.gz".extname() == p".gz" && p"foo/bar".extname() == p"" && p"foo/.bar".extname() == p"" && p"foo.txt/bar".extname() == p"" && p".foo".extname() == p"");
println("TRUE");

print("File test14...");
Clover.assert(p"ABC".toString() == "ABC" && "ABC".toPath() == p"ABC");
println("TRUE");

