#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    mode_t mode = umask(0);

    printf("umask %d\n", mode);

    umask(mode);

    exit(0);
}
