#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLFloat);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_float_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gFloatTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_float_object(float value)
{
    CLObject obj;

    obj = alloc_float_object();

    CLFLOAT(obj)->mValue = value;

    return obj;
}

BOOL float_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject ovalue1;
    float self;

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLFLOAT(ovalue1)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self);
    (*stack_ptr)++;

    return TRUE;
}

BOOL float_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    float self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject ovalue1;

    vm_mutex_lock();

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLFLOAT(ovalue1)->mValue;   // self

    len = snprintf(buf, 128, "%f", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on self");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

