#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

char* xstrncpy(char* des, char* src, int size)
{
    des[size-1] = 0;
    return strncpy(des, src, size-1);
}

char* xstrncat(char* des, char* str, int size)
{
    des[size-1] = 0;
    return strncat(des, str, size-1);
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
