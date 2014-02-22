#include <stdio.h>
#include <stdlib.h>

int main()
{
    int i;

    i=0;
    while(i<5) {
        printf("i --> %d\n", i);

        if(i == 4) {
            i++;
            continue;
        }
        else {
            i++;
        }
    }

    for(i=0; i<5; i++) {
        printf("i --> %d\n", i);

        if(i == 4) {
            continue;
        }
    }

    i = 0;
    do {
        printf("i --> %d\n", i);
        if(i == 4) {
            i++;
            continue;
        }
        else {
            i++;
        }
    } while(i < 5);

    exit(0);
}
