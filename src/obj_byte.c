#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLByte);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_byte_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gByteTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_byte_object(unsigned char value)
{
    CLObject obj;

    obj = alloc_byte_object();

    CLBYTE(obj)->mValue = value;

    return obj;
}

static CLObject create_byte_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    
    self = create_byte_object(0);

    push_object(self, info);

    CLOBJECT_HEADER(self)->mType = type_object;

    pop_object(info);

    return self;
}

void initialize_hidden_class_method_of_immediate_byte(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_byte_object_for_new;
}

BOOL byte_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    unsigned char self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject ovalue1;

    vm_mutex_lock();

    ovalue1 = lvar->mObjectValue.mValue;

    self = CLBYTE(ovalue1)->mValue; // self

    len = snprintf(buf, 128, "%c", self);
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

BOOL byte_to_int(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    unsigned char self;
    CLObject ovalue1;

    ovalue1 = lvar->mObjectValue.mValue;
    self = CLBYTE(ovalue1)->mValue; // self

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self); // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL byte_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self, value;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    if(!check_type(value, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLBYTE(self)->mValue = CLBYTE(value)->mValue;

    new_obj = create_byte_object(CLBYTE(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL byte_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_byte_object(CLBYTE(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL byte_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;
    char buf[128];
    int len;
    wchar_t wstr[128];

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    len = snprintf(buf, 128, "%c", CLBYTE(self)->mValue);
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

BOOL byte_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self); // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL byte_downcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char c;
    char c2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    c = CLBYTE(self)->mValue;

    if(c >= 'A' && c <= 'Z') {
        c2 = c - 'A' + 'a';
    }
    else {
        c2 = c;
    }

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(c2);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL byte_upcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char c;
    char c2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    c = CLBYTE(self)->mValue;

    if(c >= 'a' && c <= 'z') {
        c2 = c - 'a' + 'A';
    }
    else {
        c2 = c;
    }

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(c2);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
