#include "clover.h"
#include "common.h"

BOOL WaitStatus_exited(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int status_value;
    CLObject status_value_object;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value_object = CLUSEROBJECT(self)->mFields[0].mObjectValue.mValue;

    if(!check_type_with_class_name(status_value_object, "int", info)) {
        return FALSE;
    }

    status_value = CLINT(status_value_object)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFEXITED(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_exitStatus(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int status_value;
    CLObject status_value_object;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value_object = CLUSEROBJECT(self)->mFields[0].mObjectValue.mValue;

    if(!check_type_with_class_name(status_value_object, "int", info)) {
        return FALSE;
    }

    status_value = CLINT(status_value_object)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WEXITSTATUS(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_signaled(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int status_value;
    CLObject status_value_object;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value_object = CLUSEROBJECT(self)->mFields[0].mObjectValue.mValue;

    if(!check_type_with_class_name(status_value_object, "int", info)) {
        return FALSE;
    }

    status_value = CLINT(status_value_object)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_bool_object(WIFSIGNALED(status_value));
    (*stack_ptr)++;

    return TRUE;
}

BOOL WaitStatus_signalNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int status_value;
    CLObject status_value_object;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "WaitStatus", info)) {
        return FALSE;
    }

    status_value_object = CLUSEROBJECT(self)->mFields[0].mObjectValue.mValue;

    if(!check_type_with_class_name(status_value_object, "int", info)) {
        return FALSE;
    }

    status_value = CLINT(status_value_object)->mValue;
    
    (*stack_ptr)->mObjectValue.mValue = create_int_object(WTERMSIG(status_value));
    (*stack_ptr)++;

    return TRUE;
}
