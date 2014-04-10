#include "clover.h"
#include "common.h"

BOOL bool_to_s(MVALUE** stack_ptr, MVALUE* lvar)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    self = lvar->mIntValue; // self

    if(self) {
        len = snprintf(buf, 128, "true");
    }
    else {
        len = snprintf(buf, 128, "false");
    }
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
puts("throw eception");
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}
