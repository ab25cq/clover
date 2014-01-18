#include "clover.h"
#include "common.h"

CLObject alloc_array_object(uint array_size)
{
    CLObject obj;

    uint size = sizeof(MVALUE) * array_size;
    size += sizeof(MVALUE) * ARRAY_HEADER_NUM;

    obj = alloc_heap_mem(size);

    return obj;
}

uint array_size(CLObject array)
{
    uint size = sizeof(MVALUE) * CLARRAY_SIZE(array);
    size += sizeof(MVALUE) * ARRAY_HEADER_NUM;

    return size;
}

CLObject create_array_object(MVALUE elements[], uint elements_len)
{
    CLObject obj;

    const uint array_size = (elements_len + 1) * 2;

    obj = alloc_array_object(array_size);

    CLCLASS(obj) = gArrayClass; 
    CLARRAY_SIZE(obj) = array_size;
    CLARRAY_LEN(obj) = elements_len;

    void* data = CLARRAY_START(obj);

    memcpy(data, elements, sizeof(MVALUE)*elements_len);

    return obj;
}

void mark_array_object(CLObject object, uchar* mark_flg)
{
    int i;
    for(i=0; i<CLARRAY_LEN(object); i++) {
        CLObject object2 = CLARRAY_ITEM(object, i).mObjectValue;

        mark_object(object2, mark_flg);
    }
}

