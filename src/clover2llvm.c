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

void llentry_exception_object(sVMInfo* info, char* class_name, char* msg)
{
    entry_exception_object_with_class_name(info, class_name, msg);
}

int llcreate_type_object_from_string(char* class_name)
{
    return create_type_object_with_class_name(class_name);
}

void llpush_object(int object, sVMInfo* info)
{
    push_object(object, info);
}

void llpop_object(sVMInfo* info)
{
    pop_object(info);
}

void llpop_object_except_top(sVMInfo* info)
{
    pop_object_except_top(info);
}

int llsolve_generics_types(int type_object, int vm_type, sVMInfo* info)
{
    CLObject result;

    if(!solve_generics_types_of_type_object(type_object, ALLOC &result, vm_type, info))
    {
        return -1;
    }

    return result;
}
