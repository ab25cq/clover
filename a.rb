b = "size_t";

a = <<"EOF"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    size_t size = sizeof(#{b});

    if(size == 1) {
        puts("char");
    }
    else if(size == 2) {
        puts("short");
    }
    else if(size == 4) {
        puts("uint");
    }
    else if(size == 8) {
        puts("long");
    }

    exit(0);
}
EOF

home = ENV['HOME'];

b = open(home + "/.clover/tmpfiles/a.c", "w", 0600);
b.write(a);
b.close();

system("gcc -o ~/.clover/tmpfiles/a.out ~/.clover/tmpfiles/a.c");
system("~/.clover/tmpfiles/a.out");
