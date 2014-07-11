#include <stdio.h>
#include <stdlib.h>

int main()
{
    char buf[128];
    int len;

    len = snprintf(buf, 128, "111");
    printf("%d\n", len);

    exit(0);
}
