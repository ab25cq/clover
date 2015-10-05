#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLLong);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_long_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = create_type_object_with_class_name("long");

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_long_object(unsigned long value)
{
    CLObject obj;

    obj = alloc_long_object();

    CLLONG(obj)->mValue = value;

    return obj;
}

CLObject create_long_object_with_type(unsigned long value, CLObject type_object)
{
    CLObject obj;

    obj = alloc_long_object();

    CLLONG(obj)->mValue = value;
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

static CLObject create_long_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_long_object(0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_long(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_long_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gLongClass = klass;
        gLongTypeObject = create_type_object(gLongClass);
    }
}

BOOL long_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "long", info)) {
        return FALSE;
    }

    CLLONG(self)->mValue = CLLONG(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_int_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_byte_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_short_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_uint_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_float_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_double_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toChar(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "long", info)) {
        return FALSE;
    }

    new_obj = create_char_object(CLLONG(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL long_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gLongTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    len = snprintf(buf, 128, "%lu", CLLONG(self)->mValue);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
