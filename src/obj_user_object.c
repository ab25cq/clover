#include "clover.h"
#include "common.h"

static unsigned int object_size(sCLClass* klass)
{
    unsigned int size;
    int fields_num;

    fields_num = get_field_num_including_super_classes_without_class_field(klass);
    size = sizeof(sCLUserObject) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE;
    size += (unsigned int)sizeof(MVALUE) * fields_num;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(CLObject type_object, CLObject* obj, CLObject vm_type, sVMInfo* info)
{
    unsigned int size;
    sCLClass* klass;

    klass = CLTYPEOBJECT(type_object)->mClass;

    ASSERT(klass != NULL);

    size = object_size(klass);

    *obj = alloc_heap_mem(size, type_object);

    push_object(*obj, info);

    /// run initializers of fields ///
    if(!run_fields_initializer(*obj, klass, vm_type)) {
        pop_object(info);
        return FALSE;
    }

    pop_object(info);

    return TRUE;
}

static void mark_user_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject type_object;
    sCLClass* klass;

    type_object = CLOBJECT_HEADER(object)->mType;
    klass = CLTYPEOBJECT(type_object)->mClass;

    for(i=0; i<get_field_num_including_super_classes(klass); i++) {
        CLObject obj2;

        obj2 = CLUSEROBJECT(object)->mFields[i].mObjectValue.mValue;

        mark_object(obj2, mark_flg);
    }
}

static void show_user_object(sVMInfo* info, CLObject obj)
{
    int j;
    sCLClass* klass;
    CLObject type_object;

    type_object = CLOBJECT_HEADER(obj)->mType;

    klass = CLTYPEOBJECT(type_object)->mClass;

    VMLOG(info, " class name (%s)\n", REAL_CLASS_NAME(klass));

    for(j=0; j<get_field_num_including_super_classes(klass); j++) {
        VMLOG(info, " field#%d %d\n", j, CLUSEROBJECT(obj)->mFields[j].mObjectValue);
    }
}

void initialize_hidden_class_method_of_user_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_user_object;
    klass->mMarkFun = mark_user_object;
    klass->mCreateFun = NULL;
}

BOOL Object_type(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    type_object = CLOBJECT_HEADER(self)->mType;

    new_obj = create_type_object_from_other_type_object(type_object, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Object_setType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    type_object = (lvar+1)->mObjectValue.mValue;

    if(!check_type(type_object, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLOBJECT_HEADER(self)->mType = type_object;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Object_ID(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Object_isUninitialized(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(is_valid_object(self)) {
        (*stack_ptr)->mObjectValue.mValue = create_bool_object(TRUE);
        (*stack_ptr)++;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_bool_object(FALSE);
        (*stack_ptr)++;
    }

    vm_mutex_unlock();

    return TRUE;
}
