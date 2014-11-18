#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLInt);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_integer_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gIntTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_int_object(int value)
{
    CLObject obj;

    obj = alloc_integer_object();

    CLINT(obj)->mValue = value;

    return obj;
}

static CLObject create_int_object_for_new(struct sCLClassStruct* klass)
{
    return create_int_object(0);
}

BOOL int_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self, ovalue2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    ovalue2 = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    if(!check_type(ovalue2, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLINT(self)->mValue = CLINT(ovalue2)->mValue;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    len = snprintf(buf, 128, "%d", CLINT(self)->mValue);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs on self");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_byte_object((unsigned char)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toBool(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    BOOL result;
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(self)->mValue) {
        result = 1;
    }
    else {
        result = 0;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

void initialize_hidden_class_method_of_immediate_value(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_int_object_for_new;
}

