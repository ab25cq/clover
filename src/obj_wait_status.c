#include "clover.h"
#include "common.h"
#include <sys/wait.h>
#include <sys/types.h>

BOOL WaitStatus_WIFEXITED(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFEXITED(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WEXITSTATUS(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WEXITSTATUS(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WIFSIGNALED(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFSIGNALED(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WTERMSIG(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WTERMSIG(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WCOREDUMP(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WCOREDUMP(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WIFSTOPPED(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFSTOPPED(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WSTOPSIG(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WSTOPSIG(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_WIFCONTINUED(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    int status_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value = CLINT(self)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFCONTINUED(status_value));
    (*stack_ptr)++;

    return TRUE;
}
