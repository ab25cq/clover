#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLHash);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_hash_object(sCLClass* klass, unsigned int hash_size)
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    size = object_size();

    type_object = create_type_object(klass);

    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_hash_object(sCLClass* klass, MVALUE keys[], MVALUE elements[], int elements_len)
{
/*
    CLObject obj;
    MVALUE* data;

    const unsigned int hash_size = (elements_len + 1) * 2;

    obj = alloc_hash_object(hash_size, info);

    CLOBJECT_HEADER(obj)->mExistence = 0;
    CLOBJECT_HEADER(obj)->mClass = gArrayType.mClass;
    CLARRAY(obj)->mSize = hash_size;
    CLARRAY(obj)->mLen = elements_len;

    data = object_to_ptr(CLARRAY(obj)->mItems);

    memcpy(data, elements, sizeof(MVALUE)*elements_len);

    return obj;
*/

    return 0;
}

static void mark_hash_object(CLObject object, unsigned char* mark_flg)
{
}

static void show_hash_object(sVMInfo* info, CLObject obj)
{
    VMLOG(info, "\n");
}

void initialize_hidden_class_method_of_hash(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_hash_object;
    klass->mMarkFun = mark_hash_object;
    klass->mCreateFun = NULL;
}
