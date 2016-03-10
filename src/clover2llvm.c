#include "clover.h"
#include "common.h"

int llget_int_value_from_stack(int stack_num, sVMInfo* info)
{
    CLObject object = (info->stack_ptr-stack_num)->mObjectValue.mValue;
    return CLINT(object)->mValue;
}

char llget_byte_value_from_stack(int stack_num, sVMInfo* info)
{
    CLObject object = (info->stack_ptr-stack_num)->mObjectValue.mValue;
    return CLBYTE(object)->mValue;
}

int llget_type_object_from_stack(int stack_num, sVMInfo* info)
{
    CLObject object = (info->stack_ptr-stack_num)->mObjectValue.mValue;
    int type_object = (int)CLOBJECT_HEADER(object)->mType;
    return type_object;
}

void llinc_stack_pointer(int stack_num, sVMInfo* info)
{
    info->stack_ptr += stack_num;
}

void llcreate_int_object(int value, int type_object, sVMInfo* info)
{
    info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(value, type_object);
    info->stack_ptr++;
}
