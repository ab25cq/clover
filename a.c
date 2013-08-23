#include <stdio.h>
#include <stdlib.h>

int main()
{
    char buf[128];
    char* p = buf;
    *p = 16;
    p++;

    *((int*)p) = 257;

    p = buf;

    printf("buf[0] %d\n", *p);
    p++;
    printf("(int)buf[1] %d\n", *(int*)p);

    exit(0);
}
