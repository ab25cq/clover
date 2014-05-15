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
    unsigned int hash1, hash2, hash3;

    hash1 = get_hash("System.sleep");
    hash2 = get_hash("System.getenv");
    hash3 = get_hash("System.exit");

    printf("hash1 %d\n", hash1);
    printf("hash2 %d\n", hash2);
    printf("hash3 %d\n", hash3);

    exit(0);
}
