#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBool);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_bool_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gBoolTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_bool_object(BOOL value)
{
    CLObject obj;

    obj = alloc_bool_object();

    CLBOOL(obj)->mValue = value;

    return obj;
}

static CLObject create_bool_object_for_new(struct sCLClassStruct* klass, sVMInfo* info)
{
    CLObject self, type_object;

    self = create_bool_object(0);
    push_object(self, info);

    type_object = create_type_object(klass);
    CLOBJECT_HEADER(self)->mType = type_object;

    pop_object(info);

    return self;
}

void initialize_hidden_class_method_of_immediate_bool(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_bool_object_for_new;
}

BOOL bool_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject ovalue1;

    vm_mutex_lock();

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLBOOL(ovalue1)->mValue; // self

    if(self) {
        len = snprintf(buf, 128, "true");
    }
    else {
        len = snprintf(buf, 128, "false");
    }

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL bool_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    int self;
    int result;
    CLObject ovalue1;

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLBOOL(ovalue1)->mValue;             // self

    if(self) {
        result = 1;
    }
    else {
        result = 0;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL bool_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self, value;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gBoolTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    if(!check_type(value, gBoolTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLBOOL(self)->mValue = CLBOOL(value)->mValue;

    new_obj = create_bool_object(CLBOOL(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL bool_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBoolTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_bool_object(CLBOOL(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}



