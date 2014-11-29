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

static CLObject alloc_mutex_object(sCLClass* klass, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    size = object_size();

    type_object = create_type_object(klass);
    push_object(type_object, info);

    obj = alloc_heap_mem(size, type_object);

    pop_object(info);

    return obj;
}

CLObject create_mutex_object(sCLClass* klass, sVMInfo* info)
{
    CLObject obj;
    pthread_mutexattr_t attr;

    obj = alloc_mutex_object(klass, info);

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

BOOL Mutex_Mutex(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Mutex_run(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject block;
    BOOL result_existance;

    self = lvar->mObjectValue.mValue;
    block = (lvar+1)->mObjectValue.mValue;

    vm_mutex_lock();

    pthread_mutex_lock(&CLMUTEX(self)->mMutex);

    result_existance = FALSE;
    
    if(!cl_excute_block(block, result_existance, FALSE, info, 0)) {
        pthread_mutex_unlock(&CLMUTEX(self)->mMutex);
        vm_mutex_unlock();
        return FALSE;
    }

    pthread_mutex_unlock(&CLMUTEX(self)->mMutex);

    vm_mutex_unlock();

    return TRUE;
}
