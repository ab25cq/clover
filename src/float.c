#include "clover.h"
#include "common.h"

BOOL float_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    float self;

    self = lvar->mFloatValue;

    (*stack_ptr)->mIntValue = (int)self;
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

    vm_mutex_lock();

    self = lvar->mFloatValue; // self

    len = snprintf(buf, 128, "%f", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "error mbstowcs on self");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

