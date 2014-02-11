#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("a %d\n", 1%0);
    printf("%d\n", (float)4.0 % (float)2.0);

    exit(0);
}
