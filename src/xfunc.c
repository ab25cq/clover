#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

char* xstrncpy(char* des, char* src, int size)
{
    char* result;

    result = strncpy(des, src, size-1);
    des[size-1] = 0;

    return result;
}

char* xstrncat(char* des, char* str, int size)
{
    char* result;

    result = strncat(des, str, size-1);
    des[size-1] = 0;

    return result;
}

int xgetmaxx()
{
    struct winsize ws;

    ioctl(1, TIOCGWINSZ, &ws);

    return ws.ws_col;
}

int xgetmaxy()
{
    struct winsize ws;

    ioctl(1, TIOCGWINSZ, &ws);

    return ws.ws_row;
}

void* xxrealloc(void* old_data, size_t old_data_size, size_t new_size)
{
/*
    void* result;

    if(new_size <= old_data_size) {
        fprintf(stderr, "invalid new_size on xxrealloc\n");
        exit(2);
    }

    result = CALLOC(1, new_size);

    memcpy(result, old_data, old_data_size);

    return result;
*/

    return REALLOC(old_data, new_size);
}

