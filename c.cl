File a = new File("a.txt", "w");
a.write(B"ABC");
a.close();

Command.cat("a.txt").toString().println();
