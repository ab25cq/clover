#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLMutex);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_mutex_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

static CLObject create_mutex_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    pthread_mutexattr_t attr;

    obj = alloc_mutex_object(type_object, info);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&CLMUTEX(obj)->mMutex, &attr);

    return obj;
}

static void show_mutex_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size_;
    int size;
    char* str;

    obj_size_ = object_size();
    VMLOG(info, "object size %d\n", obj_size_);
}

static void free_mutex_object(CLObject obj)
{
    pthread_mutex_destroy(&CLMUTEX(obj)->mMutex);
}

void initialize_hidden_class_method_of_mutex(sCLClass* klass)
{
    klass->mFreeFun = free_mutex_object;
    klass->mShowFun = show_mutex_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_mutex_object;
}

BOOL Mutex_Mutex(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Mutex_run(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject block;
    BOOL result_existance;

    self = lvar->mObjectValue.mValue;
    block = (lvar+1)->mObjectValue.mValue;

    vm_mutex_lock();

    pthread_mutex_lock(&CLMUTEX(self)->mMutex);

    result_existance = FALSE;
    
    if(!cl_excute_block(block, result_existance, FALSE, info, vm_type)) {
        pthread_mutex_unlock(&CLMUTEX(self)->mMutex);
        vm_mutex_unlock();
        return FALSE;
    }

    pthread_mutex_unlock(&CLMUTEX(self)->mMutex);

    vm_mutex_unlock();

    return TRUE;
}

BOOL Mutex_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    if(!check_type_with_class_name(self, "Mutex", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;
    if(!check_type_with_class_name(value, "Mutex", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLMUTEX(self)->mMutex = CLMUTEX(value)->mMutex;

    vm_mutex_unlock();

    return TRUE;
}
