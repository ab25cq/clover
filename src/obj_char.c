#include "clover.h"
#include "common.h"
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLChar);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_char_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = create_type_object_with_class_name("char");

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_char_object(wchar_t value)
{
    CLObject obj;

    obj = alloc_char_object();

    CLCHAR(obj)->mValue = value;

    return obj;
}

static CLObject create_char_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_char_object((wchar_t)0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_char(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_char_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gCharClass = klass;
        gCharTypeObject = create_type_object(gCharClass);
    }
}

BOOL char_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "char", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "char", info)) {
        return FALSE;
    }

    CLCHAR(self)->mValue = CLCHAR(value)->mValue;

    return TRUE;
}

BOOL char_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "char", info)) {
        return FALSE;
    }

    new_obj = create_int_object(CLCHAR(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL char_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;
    wint_t wint_value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gCharTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    wint_value = CLCHAR(self)->mValue;
    len = snprintf(buf, 128, "%lc", wint_value);
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
