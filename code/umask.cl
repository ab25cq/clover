
System.umask(FileAccess.S_IWGRP|FileAccess.S_IWOTH|FileAccess.S_IWUSR);

File a = new File(p"aaa", "w", 0666);

print("umask test...");
Clover.assert(p"aaa".to_stat().permission() == 0444.toLong().to_mode_t());
println("TRUE");

p"aaa".unlink();
