Command.pwd().toString().split(//).join(" ").print();
Command.ls("-al").grep("src").toString().split(//).join(" ").toCommand().less();

Command.vim("a.txt");

Command.ls("-al").less();

Command.top();

