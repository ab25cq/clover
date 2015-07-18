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

CLObject create_range_object(CLObject type_object, CLObject head_object, CLObject tail_object)
{
    CLObject obj;

    obj = alloc_range_object(type_object);

    CLRANGE(obj)->mHead = head_object;
    CLRANGE(obj)->mTail = tail_object;

    return obj;
}

static CLObject create_range_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    CLObject head_object;
    CLObject tail_object;

    head_object = create_int_object(0);
    push_object(head_object, info);

    tail_object = create_int_object(0);
    push_object(tail_object, info);

    self = create_range_object(type_object, head_object, tail_object);
    CLOBJECT_HEADER(self)->mType = type_object;

    pop_object(info);
    pop_object(info);

    return self;
}

static void mark_range_object(CLObject object, unsigned char* mark_flg)
{
    CLObject object2;

    object2 = CLRANGE(object)->mHead;

    mark_object(object2, mark_flg);

    object2 = CLRANGE(object)->mTail;

    mark_object(object2, mark_flg);
}

void initialize_hidden_class_method_of_range(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_range_object;
    klass->mCreateFun = create_range_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gRangeClass = klass;
        gRangeTypeObject = create_type_object(gRangeClass);
    }
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

    (*stack_ptr)->mObjectValue.mValue = CLRANGE(self)->mHead;    // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_tail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;
    int tail_value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = CLRANGE(self)->mTail; // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setHead(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    CLObject head_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_nullable(value, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLRANGE(self)->mHead = value;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setTail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    CLObject tail_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type(self, gRangeTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_nullable(value, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLRANGE(self)->mTail = value;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Range_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    CLObject head_object;
    CLObject tail_object;
    int head_value;
    int tail_value;

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
