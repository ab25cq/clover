#include "clover.h"
#include "common.h"

CLObject alloc_string_object(uint len)
{
    CLObject obj;

    uint size = sizeof(wchar_t) * len;
    size += sizeof(MVALUE) * STRING_HEADER_NUM;

    obj = alloc_heap_mem(size);

    return obj;
}

uint string_size(CLObject string)
{
    uint size = sizeof(wchar_t) * CLSTRING_LEN(string);
    size += sizeof(MVALUE) * STRING_HEADER_NUM;

    return size;
}

CLObject create_string_object(wchar_t* str, uint len)
{
    CLObject obj;

    obj = alloc_string_object(len+1);

    CLCLASS(obj) = gStringClass; 
    CLSTRING_LEN(obj) = len+1;

    wchar_t* data = CLSTRING_START(obj);

    int i;
    for(i=0; i<len; i++) {
        data[i] = str[i];
    }

    data[i] = 0;

    return obj;
}
