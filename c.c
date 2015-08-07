#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    int n;
    int n2;
    unsigned long l;
    unsigned long l2;

    l = 0xFFFFFFFFFFF;

    memcpy(&n, &l, sizeof(int));
    memcpy(&n2, (char*)&l + sizeof(int), sizeof(int));
    
    memcpy(&l2, &n, sizeof(int));
    memcpy((char*)&l2 + sizeof(int), &n2, sizeof(int));

    printf("l %ld l2 %ld\n", l, l2);

    exit(0);
}
