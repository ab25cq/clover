#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBool);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_bool_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gBoolTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_bool_object(BOOL value)
{
    CLObject obj;

    obj = alloc_bool_object();

    CLBOOL(obj)->mValue = value;

    return obj;
}

static CLObject create_bool_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_bool_object(0);
    push_object(self, info);

    CLOBJECT_HEADER(self)->mType = type_object;

    pop_object(info);

    return self;
}

void initialize_hidden_class_method_of_bool(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_bool_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gBoolClass = klass;
        gBoolTypeObject = create_type_object(gBoolClass);
    }
}

BOOL bool_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBoolTypeObject, info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;
    if(!check_type(value, gBoolTypeObject, info)) {
        return FALSE;
    }

    CLBOOL(self)->mValue = CLBOOL(value)->mValue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

