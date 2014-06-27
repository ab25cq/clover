#include "clover.h"
#include "common.h"

static pthread_mutex_t gVMMutex;
static pthread_cond_t gStartVMCond = PTHREAD_COND_INITIALIZER;

void thread_init()
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&gVMMutex, &attr);
}

void thread_final()
{
    pthread_mutex_destroy(&gVMMutex);
}

void vm_mutex_lock()
{
    pthread_mutex_lock(&gVMMutex);
}

void vm_mutex_unlock()
{
    pthread_mutex_unlock(&gVMMutex);
}

void start_vm_mutex_wait()
{
    struct timespec timeout;

    timeout.tv_sec = time(NULL) + 1;
    timeout.tv_nsec = 0;
    pthread_cond_timedwait(&gStartVMCond, &gVMMutex, &timeout);
}

void start_vm_mutex_signal()
{
    pthread_cond_signal(&gStartVMCond);
}

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLThread);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_thread_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, klass);

    return obj;
}

CLObject create_thread_object(sCLClass* klass)
{
    CLObject obj;
    int i;

    obj = alloc_thread_object(klass);

    return obj;
}

static void show_thread_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size_;
    int size;
    char* str;

    obj_size_ = object_size();

    VMLOG(info, "object size %d\n", obj_size_);
}

struct sThreadFuncArg {
    CLObject mBlock;
    BOOL mResultExistance;
    sVMInfo* mNewVMInfo;
};

void* thread_func(void* param)
{
    MVALUE result;
    struct sThreadFuncArg* arg;

    vm_mutex_lock();

    arg = param;

    if(!cl_excute_block_with_new_stack(&result, arg->mBlock, NULL, arg->mResultExistance, arg->mNewVMInfo)) // 1 --> block
    {
        FREE(arg->mNewVMInfo);
        FREE(arg);
        return FALSE;
    }

    FREE(arg->mNewVMInfo);
    FREE(arg);

    return NULL;
}

BOOL Thread_Thread(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject block;
    BOOL result_existance;
    struct sThreadFuncArg* arg;
    sVMInfo* new_info;
    MVALUE* lvar2;
    pthread_t thread_id;
    int real_param_num;

    result_existance = FALSE;

    self = lvar->mObjectValue;
    block = (lvar+1)->mObjectValue;

    /// make new stack to the sub thread on the main thread because of GC ///
    vm_mutex_lock();

    /// create new stack ///
    new_info = CALLOC(1, sizeof(sVMInfo));

    new_info->stack = CALLOC(1, sizeof(MVALUE)*CL_STACK_SIZE);
    new_info->stack_size = CL_STACK_SIZE;
    new_info->stack_ptr = new_info->stack;

    real_param_num = 1;             // this param is block
    lvar2 = new_info->stack;

    /// copy params to current local vars ///
    memmove(lvar2 + CLBLOCK(block)->mNumParentVar, info->stack_ptr-real_param_num, sizeof(MVALUE)*real_param_num);

    /// copy caller local vars to current local vars ///
    memmove(lvar2, CLBLOCK(block)->mParentLocalVar, sizeof(MVALUE)*CLBLOCK(block)->mNumParentVar);

    new_info->stack_ptr += CLBLOCK(block)->mNumParentVar + real_param_num;

    if(CLBLOCK(block)->mNumLocals - real_param_num > 0) {
        new_info->stack_ptr += (CLBLOCK(block)->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
    }

    push_vminfo(new_info);
    vm_mutex_unlock();

    /// make arg for thread ///
    arg = CALLOC(1, sizeof(struct sThreadFuncArg));

    arg->mBlock = block;
    arg->mResultExistance = result_existance;
    arg->mNewVMInfo = MANAGED new_info;

    if(pthread_create(&thread_id, NULL, thread_func, MANAGED arg) != 0) {
        pthread_detach(thread_id);
        entry_exception_object(info, gExceptionType.mClass, "pthread_create exception1", info);
        return FALSE;
    }

    if(thread_id == 0) {
        entry_exception_object(info, gExceptionType.mClass, "pthread_create exception2", info);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue = self;
    (*stack_ptr)++;

    vm_mutex_lock();
    CLTHREAD(self)->mThread = thread_id;
    vm_mutex_unlock();

    return TRUE;
}

BOOL Thread_join(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    pthread_t thread;

    self = lvar->mObjectValue;

    vm_mutex_lock();
    thread = CLTHREAD(self)->mThread;
    vm_mutex_unlock();

    pthread_join(thread, NULL);

    return TRUE;
}

void initialize_hidden_class_method_of_thread(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_thread_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_thread_object;
}

