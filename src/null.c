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

BOOL null_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    vm_mutex_lock();

    len = snprintf(buf, 128, "null");
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL null_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    (*stack_ptr)->mObjectValue.mValue = create_int_object(0);
    (*stack_ptr)++;

    return TRUE;
}

BOOL null_to_bool(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(0);
    (*stack_ptr)++;

    return TRUE;
}

