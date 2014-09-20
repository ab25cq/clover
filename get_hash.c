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

    hash1 = get_hash("String._constructor()");
    hash2 = get_hash("Array._constructor()");
    hash3 = get_hash("Thread._constructor()void{}");

    printf("hash1 %d\n", hash1);
    printf("hash2 %d\n", hash2);
    printf("hash3 %d\n", hash3);

    exit(0);
}
