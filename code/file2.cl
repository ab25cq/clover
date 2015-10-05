Command.touch("XXX");

time_t actime = new tm(2015@year, 9@month, 11@day_of_month, 23@hour, 0@minuts, 0@sec).to_time_t();

p"XXX".utime(actime, System.time()@modtime);

tm time2 = p"XXX".to_stat().atime();

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
Clover.assert(p"XXX".to_stat().S_ISBLK() == false);
println("TRUE");

print("File test6...");
Clover.assert(p"XXX".to_stat().S_ISREG() == true);
println("TRUE");

Command.rm("XXX");

Command.mkdir("YYY");

print("File test7...");
Clover.assert(p"YYY".to_stat().S_ISDIR() == true);
println("TRUE");

Command.rmdir("YYY");

Command.touch("AAA");
p"AAA".chmod(0755.toLong().to_mode_t());

print("File test8...");
Clover.assert(p"AAA".to_stat().permission() == 0755.toLong().to_mode_t());
println("TRUE");

p"AAA".chown();

print("File test9...");
Clover.assert(p"AAA".to_stat().uid() == System.getuid() && p"AAA".to_stat().gid() == System.getgid());
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


print("File test15...");
Clover.assert(p"a.c".fnmatch("*.c") && p"foobar".fnmatch("foo*"));
println("TRUE");

print("absolute path test1...");
Clover.assert(p"xyz".absolutePath() == System.getcwd() + "/xyz");
println("TRUE");

print("absolute path test2...");
Clover.assert(p"/home/ab25cq/./lib/./share/./".absolutePath() == p"/home/ab25cq/lib/share");
println("TRUE");

print("absolute path test3...");
Clover.assert(p"/aaa/../bcd".absolutePath() == p"/bcd");
println("TRUE");

print("absolute path test4...");
Clover.assert(p"/../../../bcd".absolutePath() == p"/bcd");
println("TRUE");

print("absolute path test5...");
Clover.assert(p"/etc".absolutePath() == p"/etc/");
println("TRUE");

print("absolute path test6...");
Clover.assert(p"/etc/passwd/".absolutePath() == p"/etc/passwd");
println("TRUE");

Command.touch("AAA");
p"AAA".link(p"BBB");

print("file test16...");
Clover.assert(p"BBB".to_stat().S_ISREG());
println("TRUE");

Command.rm("AAA");
Command.rm("BBB");

Command.touch("AAA");
p"AAA".symlink(p"BBB");

print("file test17...");
Clover.assert(p"BBB".to_lstat().S_ISLNK());
println("TRUE");

print("file test18...");
Clover.assert(p"BBB".readlink() == p"AAA");
println("TRUE");

p"BBB".rename(p"CCC");

print("file test19...");
Clover.assert(p"CCC".readlink() == p"AAA");
println("TRUE");

Command.rm("AAA");
Command.rm("CCC");

File a = File.open(p"AAA", "w");
a.write(B"ABCDEFG");
a.close();

p"AAA".truncate(3l.to_off_t());

print("file test20...");
Clover.assert(p"AAA".to_stat().st_size == 3l.to_off_t());
println("TRUE");

Command.rm("AAA");

File.umask(022l.to_mode_t());

print("file test21...");
Clover.assert(File.umask() == 022l.to_mode_t());
println("TRUE");

a = File.open(p"AAA", "w");
a.write(B"ABC");
a.flock(FileLockOperation.LOCK_EX);
a.close();

Command.rm(p"AAA");

p"EEE".write(B"KKK");

print("file test22...");
Clover.assert(p"EEE".readOut() == B"KKK");
println("TRUE");

Command.rm("EEE");

p"EEE".write(B"ABC\nDEF\nGHI");

print("file test23...");
Clover.assert(p"EEE".readOut().toString().lines() == { "ABC\n", "DEF\n", "GHI" });
println("TRUE");

Command.rm("EEE");

Command.ls().egrep("^man").write(p"ABC");

print("Command test1...");
Clover.assert(p"ABC".readOut().toString() == "man\n");
println("TRUE");

Command.rm("ABC");
