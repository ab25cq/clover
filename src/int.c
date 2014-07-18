#include "clover.h"
#include "common.h"

BOOL int_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mIntValue; // self

    len = snprintf(buf, 128, "%d", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "failed to mbstowcs on self");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_to_byte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;

    vm_mutex_lock();

    self = lvar->mIntValue; // self

/*
    if(self < 0 || self > 0xff) {
        entry_exception_object(info, gExOverflowType.mClass, "overflow");
        vm_mutex_unlock();
        return FALSE;
    }
*/

    (*stack_ptr)->mByteValue = (unsigned char)self;        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_to_bool(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    BOOL result;

    vm_mutex_lock();

    self = lvar->mIntValue;

    if(self) {
        result = 1;
    }
    else {
        result = 0;
    }

    (*stack_ptr)->mIntValue = result;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

void initialize_hidden_class_method_of_immediate_value(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}
