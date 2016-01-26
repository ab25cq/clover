#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLNull);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_null_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gNullTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_null_object()
{
    CLObject obj;

    obj = alloc_null_object();

    CLNULL(obj)->mValue = 0;

    return obj;
}

static CLObject create_null_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_null_object();
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

void initialize_hidden_class_method_of_null(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_null_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gNullClass = klass;
        gNullTypeObject = create_type_object(gNullClass);
    }
}

