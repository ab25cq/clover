#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLUInt);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_uint_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = create_type_object_with_class_name("uint");

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_uint_object(unsigned int value)
{
    CLObject obj;

    obj = alloc_uint_object();

    CLUINT(obj)->mValue = value;

    return obj;
}

static CLObject create_uint_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_uint_object(0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_uint(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_uint_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gUIntClass = klass;
        gUIntTypeObject = create_type_object(gUIntClass);
    }
}

BOOL uint_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "uint", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "uint", info)) {
        return FALSE;
    }

    CLUINT(self)->mValue = CLUINT(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL uint_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "uint", info)) {
        return FALSE;
    }

    new_obj = create_int_object((int)CLUINT(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL uint_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gUIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    len = snprintf(buf, 128, "%u", CLUINT(self)->mValue);
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
