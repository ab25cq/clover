#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLRange);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_range_object(CLObject type_object)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_range_object(CLObject type_object, int head, int tail)
{
    CLObject obj;

    obj = alloc_range_object(type_object);

    CLRANGE(obj)->mHead = head;
    CLRANGE(obj)->mTail = tail;

    return obj;
}

static CLObject create_range_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_range_object(type_object, 0, 0);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_range(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_range_object_for_new;
}

BOOL Range_head(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_int_object(CLRANGE(self)->mHead);

    (*stack_ptr)->mObjectValue.mValue = new_obj;    // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_tail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_int_object(CLRANGE(self)->mTail);

    (*stack_ptr)->mObjectValue.mValue = new_obj;    // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setHead(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLRANGE(self)->mHead = CLINT(value)->mValue;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setTail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLRANGE(self)->mTail = CLINT(value)->mValue;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLRANGE(self)->mHead = CLRANGE(value)->mHead;
    CLRANGE(value)->mTail = CLRANGE(value)->mTail;

    vm_mutex_unlock();

    return TRUE;
}
