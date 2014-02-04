#include "clover.h"
#include "common.h"

unsigned int object_size(sCLClass* klass)
{
    unsigned int size;

    size = sizeof(sCLObject) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE;
    size += (unsigned int)sizeof(MVALUE)*get_field_num_including_super_classes(klass);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject create_object(sCLClass* klass)
{
    CLObject obj = alloc_heap_mem(object_size(klass));
    CLOBJECT_HEADER(obj)->mClass = klass;

    return obj;
}

void mark_user_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    for(i=0; i<get_field_num_including_super_classes(CLOBJECT_HEADER(object)->mClass); i++) {
        CLObject obj2;

        obj2 = CLOBJECT(object)->mFields[i].mObjectValue;

        mark_object(obj2, mark_flg);
    }
}
