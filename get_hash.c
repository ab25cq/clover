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
    unsigned int hash1, hash2, hash3, hash4, hash5, hash6, hash7, hash8, hash9, hash10;

    hash2 = get_hash("Clover.print(String)");

    printf("hash2 %d\n", hash2);

    exit(0);
}
