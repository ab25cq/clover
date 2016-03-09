#include "clover.h"
#include "common.h"

int get_int_value_from_stack(int stack_num, sVMInfo* info)
{
    int result;
    CLObject object;

    object = (info->stack_ptr-stack_num)->mObjectValue.mValue;

    result = CLINT(object)->mValue;

    return result;
}

