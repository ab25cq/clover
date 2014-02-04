#include "clover.h"
#include "common.h"

CLObject alloc_array_object(unsigned int array_size)
{
    CLObject obj;

    obj = alloc_heap_mem(sizeof(sCLArray));

    CLARRAY(obj)->mItems = alloc_heap_mem(sizeof(MVALUE)*array_size);

    return obj;
}

unsigned int array_size(CLObject array)
{
    unsigned int size;

    size = sizeof(sCLArray);

    return size;
}

CLObject create_array_object(MVALUE elements[], unsigned int elements_len)
{
    CLObject obj;
    MVALUE* data;

    const unsigned int array_size = (elements_len + 1) * 2;

    obj = alloc_array_object(array_size);

    CLOBJECT_HEADER(obj)->mExistence = 0;
    CLOBJECT_HEADER(obj)->mClass = gArrayType.mClass;
    CLARRAY(obj)->mSize = array_size;
    CLARRAY(obj)->mLen = elements_len;

    data = object_to_ptr(CLARRAY(obj)->mItems);

    memcpy(data, elements, sizeof(MVALUE)*elements_len);

    return obj;
}

void mark_array_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    CLObject object2 = CLARRAY(object)->mItems;
    mark_object(object2, mark_flg);

    for(i=0; i<CLARRAY(object)->mLen; i++) {
        CLObject object3 = CLARRAY_ITEMS(object, i).mObjectValue;

        mark_object(object3, mark_flg);
    }
}

