#include "clover.h"
#include "common.h"

CLObject alloc_array_object(unsigned int array_size)
{
    CLObject obj;
    unsigned int size = sizeof(MVALUE) * array_size;
    size += sizeof(MVALUE) * ARRAY_HEADER_NUM;

    obj = alloc_heap_mem(size);

    return obj;
}

unsigned int array_size(CLObject array)
{
    unsigned int size = sizeof(MVALUE) * CLARRAY_SIZE(array);
    size += sizeof(MVALUE) * ARRAY_HEADER_NUM;

    return size;
}

CLObject create_array_object(MVALUE elements[], unsigned int elements_len)
{
    CLObject obj;
    void* data;

    const unsigned int array_size = (elements_len + 1) * 2;

    obj = alloc_array_object(array_size);

    CLCLASS(obj) = gArrayType.mClass; 
    CLARRAY_SIZE(obj) = array_size;
    CLARRAY_LEN(obj) = elements_len;

    data = CLARRAY_START(obj);

    memcpy(data, elements, sizeof(MVALUE)*elements_len);

    return obj;
}

void mark_array_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    for(i=0; i<CLARRAY_LEN(object); i++) {
        CLObject object2 = CLARRAY_ITEM(object, i).mObjectValue;

        mark_object(object2, mark_flg);
    }
}

