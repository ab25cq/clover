#include "clover.h"
#include "common.h"
#include <stdio.h>
#include <wchar.h>

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

    ASSERT(gIntTypeObject != 0);

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

CLObject create_int_object_with_type(int value, CLObject type_object)
{
    CLObject obj;

    obj = alloc_integer_object();

    CLINT(obj)->mValue = value;

    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

CLObject create_int_object_with_type_name(int value, char* type_name, sVMInfo* info)
{
    CLObject obj;
    CLObject type_object;

    obj = alloc_integer_object();

    CLINT(obj)->mValue = value;

    push_object(obj, info);

    type_object = create_type_object_with_class_name(type_name);
    CLOBJECT_HEADER(obj)->mType = type_object;

    pop_object(info);

    return obj;
}

static CLObject create_int_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_int_object(0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_int(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_int_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gIntClass = klass;
        gIntTypeObject = create_type_object(gIntClass);
    }
}

BOOL int_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(value, gIntTypeObject, info)) {
        return FALSE;
    }

    CLINT(self)->mValue = CLINT(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    len = snprintf(buf, 128, "%d", CLINT(self)->mValue);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_byte_object((unsigned char)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_short_object((unsigned short)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_uint_object((unsigned int)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_long_object((unsigned long)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_float_object((float)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_double_object((double)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toChar(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_char_object((wchar_t)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    return TRUE;
}

