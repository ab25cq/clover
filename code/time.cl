File a = new File(p"a.txt", "w");
a.write(B"ABC");
a.close();

stat stat_buf = new stat();

System.stat(p"a.txt", stat_buf);

println(stat_buf.st_ctime.toString());

time_t time_value = System.time();

println(time_value.toString());

tm time = new tm(time_value);

println("dayOfYear --> " + time.dayOfYear().toString());
println("dayOfWeek --> " + time.dayOfWeek().toString());
println("year --> " + time.year().toString());
println("month --> " + time.month().toString());
println("dayOfMonth --> " + time.dayOfMonth().toString());
println("hour -->" + time.hour().toString());
println("minuts -->" + time.minuts().toString());
println("second -->" + time.second().toString());
println("isDaylightSavingTime -->" + time.isDaylightSavingTime().toString());

println("time --> " + time.toString());

Command.cat("a.txt").toString().println();
Command.rm("a.txt");
