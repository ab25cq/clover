#include "clover.h"
#include "common.h"

BOOL int_to_s(MVALUE** stack_ptr, MVALUE* lvar)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    self = lvar->mIntValue; // self

    len = snprintf(buf, 128, "%d", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(gExceptionType.mClass, "mbstowcs");
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

void initialize_hidden_class_method_of_immediate_value(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
}
