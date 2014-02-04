#include "clover.h"
#include "common.h"

CLObject alloc_string_object(unsigned int len)
{
    CLObject obj;
    unsigned int size;

    size = sizeof(sCLString) - sizeof(wchar_t) * DUMMY_ARRAY_SIZE;
    size += sizeof(wchar_t) * len;

    obj = alloc_heap_mem(size);

    return obj;
}

unsigned int string_size(CLObject string)
{
    unsigned int size;

    size = sizeof(sCLString) - sizeof(wchar_t) * DUMMY_ARRAY_SIZE;
    size += sizeof(wchar_t) * CLSTRING(string)->mLen;

    return size;
}

CLObject create_string_object(wchar_t* str, unsigned int len)
{
    CLObject obj;
    wchar_t* data;
    int i;

    obj = alloc_string_object(len+1);

    CLOBJECT_HEADER(obj)->mClass = gStringType.mClass; 
    CLSTRING(obj)->mLen = len+1;

    data = CLSTRING(obj)->mChars;

    for(i=0; i<len; i++) {
        data[i] = str[i];
    }

    data[i] = 0;

    return obj;
}
