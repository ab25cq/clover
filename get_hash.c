#include <stdio.h>
#include <stdlib.h>

static unsigned int get_hash(unsigned char* name)
{
    unsigned int hash = 0;
    unsigned char* p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

int main()
{
    unsigned int hash = get_hash("int.to_s");

    printf("hash %d\n", hash);

    exit(0);
}
