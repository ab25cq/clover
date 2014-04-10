#include "clover.h"
#include "common.h"

BOOL float_floor(MVALUE** stack_ptr, MVALUE* lvar)
{
puts("float_floor");

    return TRUE;
}

BOOL float_to_s(MVALUE** stack_ptr, MVALUE* lvar)
{
    float self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;

    self = lvar->mFloatValue; // self

    len = snprintf(buf, 128, "%f", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
puts("throw exception");
return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

