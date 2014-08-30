#include "clover.h"
#include "common.h"

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
    new_obj = create_string_object(gStringClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL null_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    (*stack_ptr)->mIntValue = 0;
    (*stack_ptr)++;

    return TRUE;
}

BOOL null_to_bool(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    (*stack_ptr)->mIntValue = 0;
    (*stack_ptr)++;

    return TRUE;
}

