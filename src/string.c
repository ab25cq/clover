#include "clover.h"
#include "common.h"

CLObject alloc_string_object(unsigned int len)
{
    CLObject obj;
    unsigned int size;

    size = sizeof(wchar_t) * len;
    size += sizeof(MVALUE) * STRING_HEADER_NUM;

    obj = alloc_heap_mem(size);

    return obj;
}

unsigned int string_size(CLObject string)
{
    unsigned int size;

    size = sizeof(wchar_t) * CLSTRING_LEN(string);
    size += sizeof(MVALUE) * STRING_HEADER_NUM;

    return size;
}

CLObject create_string_object(wchar_t* str, unsigned int len)
{
    CLObject obj;
    wchar_t* data;
    int i;

    obj = alloc_string_object(len+1);

    CLCLASS(obj) = gStringType.mClass; 
    CLSTRING_LEN(obj) = len+1;

    data = CLSTRING_START(obj);

    for(i=0; i<len; i++) {
        data[i] = str[i];
    }

    data[i] = 0;

    return obj;
}
