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
    unsigned int hash1, hash2, hash3, hash4, hash5, hash6;

    hash1 = get_hash("int.to_str");

    printf("hash1 %d\n", hash1);

    exit(0);
}
