
termios a = new termios();
System.tcgetattr(0, a);

a.c_iflag.toString().println();
a.c_oflag.toString().println();
a.c_cflag.toString().println();
a.c_lflag.toString().println();
a.c_cc.toString().println();

a.toString().println();

a.c_lflag &= (~tcflag_t.ECHO).toLong().to_tcflag_t();

System.tcsetattr(0, TCSetAttrAction.TCSANOW, a);

a = new termios();
System.tcgetattr(0, a);

a.toString().println();
