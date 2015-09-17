#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLShort);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_short_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = create_type_object_with_class_name("short");

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_short_object(unsigned short value)
{
    CLObject obj;

    obj = alloc_short_object();

    CLSHORT(obj)->mValue = value;

    return obj;
}

static CLObject create_short_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_short_object(0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_immediate_short(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_short_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gShortClass = klass;
        gShortTypeObject = create_type_object(gShortClass);
    }
}

BOOL short_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "short", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "short", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLSHORT(self)->mValue = CLSHORT(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL short_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "short", info)) {
        return FALSE;
    }

    new_obj = create_int_object(CLSHORT(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL short_toLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "short", info)) {
        return FALSE;
    }

    new_obj = create_long_object(CLSHORT(self)->mValue);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

