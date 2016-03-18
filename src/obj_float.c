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

CLObject create_float_object_with_type(float value, CLObject type_object)
{
    CLObject obj;

    obj = alloc_float_object();

    CLFLOAT(obj)->mValue = value;
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

static CLObject create_float_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    
    self = create_float_object(0.0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_float(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_float_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gFloatClass = klass;
        gFloatTypeObject = create_type_object(gFloatClass);
    }
}

BOOL float_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    float value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        return FALSE;
    }

    value = CLFLOAT(self)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL float_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    double value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        return FALSE;
    }

    value = (double)CLFLOAT(self)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_double_object(value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL float_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    float value;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        return FALSE;
    }

    value = CLFLOAT(self)->mValue;   // value

    len = snprintf(buf, 128, "%f", value);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL float_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gFloatTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(value, gFloatTypeObject, info)) {
        return FALSE;
    }

    CLFLOAT(self)->mValue = CLFLOAT(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

