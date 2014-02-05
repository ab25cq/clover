#include <stdio.h>
#include <stdlib.h>

static unsigned int get_hash(char* name)
{
    unsigned int hash = 0;
    char* p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

int main()
{
    unsigned int hash, hash2, hash3;

    hash = get_hash("Array.Array");
    hash2 = get_hash("Array.get");
    hash3 = get_hash("Array.add");

    printf("hash %d\n", hash);
    printf("hash2 %d\n", hash2);
    printf("hash3 %d\n", hash3);

    exit(0);
}
