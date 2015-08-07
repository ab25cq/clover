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

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    if(!check_type(value, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLINT(self)->mValue = CLINT(value)->mValue;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
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

BOOL int_toCharacter(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gIntTypeObject, info)) {
        return FALSE;
    }

    len = swprintf(wstr, 128, L"%lc", (wchar_t)CLINT(self)->mValue);
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL int_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
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

BOOL int_toShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_short_object((unsigned short)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_uint_object((unsigned int)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_long_object((unsigned long)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_float_object((float)CLINT(self)->mValue);        // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_downcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int c;
    int c2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    c = CLINT(self)->mValue;

    if(c >= 'A' && c <= 'Z') {
        c2 = c - 'A' + 'a';
    }
    else {
        c2 = c;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(c2);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL int_upcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int c;
    int c2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    c = CLINT(self)->mValue;

    if(c >= 'a' && c <= 'z') {
        c2 = c - 'a' + 'A';
    }
    else {
        c2 = c;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(c2);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
