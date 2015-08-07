#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main() {
    long a = 123;

    printf("long %d\n", sizeof(long));

    printf("sizeof(dev_t) %d\n", sizeof(dev_t));
    printf("sizeof(mode_t) %d\n", sizeof(mode_t));
    printf("sizeof(nlink_t) %d\n", sizeof(nlink_t));
    printf("sizeof(pid_t) %d\n", sizeof(pid_t));
    printf("sizeof(logn int) %d\n", sizeof(long int));
    printf("sizeof(int) %d\n", sizeof(int));

    dev_t x = ;
    unsigned long int y = x;

    printf("%ld %ld\n", x, y);

    exit(1);
}
