#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("%f\n", 1.1 + 1.2);

    if((1.1 + 1.2) == 2.300000) {
        printf("OK\n");
    }
    else {
        printf("FALSE\n");
    }

    return 0;
}

