#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLFloat);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_float_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gFloatTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_float_object(float value)
{
    CLObject obj;

    obj = alloc_float_object();

    CLFLOAT(obj)->mValue = value;

    return obj;
}

static CLObject create_float_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    
    self = create_float_object(0.0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_float(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_float_object_for_new;
}

BOOL float_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject ovalue1;
    float self;

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLFLOAT(ovalue1)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self);
    (*stack_ptr)++;

    return TRUE;
}

BOOL float_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    float self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject ovalue1;

    vm_mutex_lock();

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLFLOAT(ovalue1)->mValue;   // self

    len = snprintf(buf, 128, "%f", self);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on self");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL float_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    if(!check_type(value, gFloatTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLFLOAT(self)->mValue = CLFLOAT(value)->mValue;

    new_obj = create_float_object(CLFLOAT(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL float_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_float_object(CLFLOAT(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}






