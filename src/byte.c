#include "clover.h"
#include "common.h"

BOOL byte_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    unsigned char self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mByteValue; // self

    len = snprintf(buf, 128, "%c", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL byte_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    unsigned char self;

    self = lvar->mByteValue; // self

    (*stack_ptr)->mIntValue = (int)self; // push result
    (*stack_ptr)++;

    return TRUE;
}
