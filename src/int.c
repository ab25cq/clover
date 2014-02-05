#include "clover.h"
#include "common.h"

BOOL int_to_s(MVALUE* stack_ptr, MVALUE* lvar)
{
    MVALUE* self;
    char buf[128];
    int len;
    wchar_t wstr[len+1];
    CLObject new_obj;

    self = lvar; // self
    len = snprintf(buf, 128, "%d", self->mIntValue);

    mbstowcs(wstr, buf, len+1);

    new_obj = create_string_object(wstr, len);

    gCLStackPtr->mObjectValue = new_obj;  // push result
    gCLStackPtr++;

    return TRUE;
}
