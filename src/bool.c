#include "clover.h"
#include "common.h"

BOOL bool_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mIntValue; // self

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

    new_obj = create_string_object(gStringClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL bool_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    int result;

    self = lvar->mIntValue; // self

    if(self) {
        result = 1;
    }
    else {
        result = 0;
    }

    (*stack_ptr)->mIntValue = result;
    (*stack_ptr)++;

    return TRUE;
}
