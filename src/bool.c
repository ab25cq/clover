#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBool);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_bool_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gBoolTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_bool_object(BOOL value)
{
    CLObject obj;

    obj = alloc_bool_object();

    CLBOOL(obj)->mValue = value;

    return obj;
}


BOOL bool_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject ovalue1;

    vm_mutex_lock();

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLBOOL(ovalue1)->mValue; // self

    if(self) {
        len = snprintf(buf, 128, "true");
    }
    else {
        len = snprintf(buf, 128, "false");
    }

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_string_object(wstr, len);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL bool_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    int result;
    CLObject ovalue1;

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLBOOL(ovalue1)->mValue;             // self

    if(self) {
        result = 1;
    }
    else {
        result = 0;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}
