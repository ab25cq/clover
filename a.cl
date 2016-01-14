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
