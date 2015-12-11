#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

sVMInfo* gHeadVMInfo;

void atexit_fun()
{
    sVMInfo* it;
    
#ifdef VM_DEBUG
    it = gHeadVMInfo;
    while(it) {
        fclose(it->debug_log);
        it = it->next_info;
    }
#endif
}

void push_vminfo(sVMInfo* info)
{
    info->next_info = gHeadVMInfo;
    gHeadVMInfo = info;
}

static void pop_vminfo(sVMInfo* info)
{
    sVMInfo* it;
    sVMInfo* it_before;

    it = gHeadVMInfo;
    it_before = NULL;
    while(it) {
        if(it == info) {
            if(it_before == NULL) {
                gHeadVMInfo = info->next_info;
                break;
            }
            else {
                it_before->next_info = it->next_info;
                break;
            }
        }
        it_before = it;
        it = it->next_info;
    }
}

static void set_env_vars()
{
    setenv("CLOVER_VERSION", "0.9.9", 1);
    setenv("CLOVER_DATAROOTDIR", DATAROOTDIR, 1);
}

static void set_signal()
{
    struct sigaction sa;
    sigset_t signal_set;

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGTTOU);

    sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

BOOL cl_init(int heap_size, int handle_size, int argc, char** argv)
{
    set_signal();
    set_env_vars();
    thread_init();

    heap_init(heap_size, handle_size);

    class_init();
    atexit(atexit_fun);

    gArgc = argc;
    gArgv = argv;

    return TRUE;
}

void cl_final()
{
    free_cl_types();
    heap_final();
    class_final();
    thread_final();
}

void push_object(CLObject object, sVMInfo* info)
{
    info->stack_ptr->mObjectValue.mValue = object;
    info->stack_ptr++;
}

// remove the object from stack
CLObject pop_object(sVMInfo* info)
{
    info->stack_ptr--;
    return info->stack_ptr->mObjectValue.mValue;
}

void pop_object_except_top(sVMInfo* info)
{
    CLObject top_object;

    top_object = (info->stack_ptr-1)->mObjectValue.mValue;
    info->stack_ptr-=2;

    info->stack_ptr->mObjectValue.mValue = top_object;
    info->stack_ptr++;
}

void pop_object_n(sVMInfo* info, int n)
{
    info->stack_ptr-=n;
}

void remove_object2(CLObject obj, sVMInfo* info)
{
    MVALUE* stack_ptr;

    stack_ptr = info->stack_ptr;
    while(stack_ptr >= info->stack) {
        if(stack_ptr->mObjectValue.mValue == obj) {
            memcpy(stack_ptr, stack_ptr + 1, sizeof(MVALUE)*(info->stack_ptr - info->stack));
            info->stack_ptr--;
            break;
        }

        stack_ptr--;
    }
}

void remove_object(sVMInfo* info, int number)
{
    info->stack_ptr-=number;
}

void vm_error(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}

void entry_exception_object(sVMInfo* info, sCLClass* klass, char* msg, ...)
{
    CLObject ovalue, ovalue2;
    wchar_t* wcs;
    int size;
    char msg2[1024];
    CLObject type1;

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    vm_mutex_lock();

    type1 = create_type_object(klass);

    (void)create_user_object(type1, &ovalue, 0, NULL, 0, info);

    info->stack_ptr->mObjectValue.mValue = ovalue;
    info->stack_ptr++;

    wcs = MALLOC(sizeof(wchar_t)*(strlen(msg2)+1));
    (void)mbstowcs(wcs, msg2, strlen(msg2)+1);
    size = wcslen(wcs);

    ovalue2 = create_string_object(wcs, size, gStringTypeObject, info);

    CLUSEROBJECT(ovalue)->mFields[0].mObjectValue.mValue = ovalue2;

    FREE(wcs);

    info->mRunningClassOnException = info->mRunningClass;
    info->mRunningMethodOnException = info->mRunningMethod;

    vm_mutex_unlock();
}

void entry_exception_object_with_class_name(sVMInfo* info, char* class_name, char* msg, ...)
{
    CLObject ovalue, ovalue2;
    wchar_t* wcs;
    int size;
    char msg2[1024];
    CLObject type1;
    sCLClass* klass;

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    vm_mutex_lock();

    klass = cl_get_class(class_name);

    if(klass == NULL) {
        fprintf(stderr, "unexpected error. abort on entry_exception_object_with_class_name. The class name is %s.\n", class_name);
        exit(2);
    }

    type1 = create_type_object(klass);

    (void)create_user_object(type1, &ovalue, 0, NULL, 0, info);

    info->stack_ptr->mObjectValue.mValue = ovalue;
    info->stack_ptr++;

    wcs = MALLOC(sizeof(wchar_t)*(strlen(msg2)+1));
    (void)mbstowcs(wcs, msg2, strlen(msg2)+1);
    size = wcslen(wcs);

    ovalue2 = create_string_object(wcs, size, gStringTypeObject, info);

    CLUSEROBJECT(ovalue)->mFields[0].mObjectValue.mValue = ovalue2;

    FREE(wcs);

    info->mRunningClassOnException = info->mRunningClass;
    info->mRunningMethodOnException = info->mRunningMethod;

    vm_mutex_unlock();
}

void output_exception_message(CLObject exception_object)
{
    CLObject message;
    char* mbs;
    CLObject type_object;

    if(!check_type_without_info(exception_object, "Exception")) {
        fprintf(stderr, "invalid exception object\n");
        return;
    }

    message = CLUSEROBJECT(exception_object)->mFields[0].mObjectValue.mValue;

    if(!check_type_without_info(message, "String")) {
        fprintf(stderr, "invalid exception message object\n");
        return;
    }

    mbs = CALLOC(1, MB_LEN_MAX*(CLSTRING(message)->mLen + 1));

    (void)wcstombs(mbs, CLSTRING_DATA(message)->mChars, CLSTRING(message)->mLen + 1);

    type_object = CLOBJECT_HEADER(exception_object)->mType;

    fprintf(stderr, "%s: %s\n", CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), mbs);
    FREE(mbs);
}

static void output_exception_message_with_info(sVMInfo* info)
{
    CLObject exception;
    CLObject message;
    char* mbs;
    CLObject type_object;

    exception = (info->stack_ptr-1)->mObjectValue.mValue;

    if(!check_type_with_class_name(exception, "Exception", info)) {
        fprintf(stderr, "invalid exception object\n");
        return;
    }

    message = CLUSEROBJECT(exception)->mFields[0].mObjectValue.mValue;
    if(!check_type(message, gStringTypeObject, info)) {
        fprintf(stderr, "invalid exception message object\n");
        return;
    }

    mbs = CALLOC(1, MB_LEN_MAX*(CLSTRING(message)->mLen + 1));

    (void)wcstombs(mbs, CLSTRING_DATA(message)->mChars, CLSTRING(message)->mLen + 1);

    type_object = CLOBJECT_HEADER(exception)->mType;

    if(info->mRunningClassOnException && info->mRunningMethodOnException) {
        fprintf(stderr, "%s: %s at %s.%s\n", CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), mbs, CLASS_NAME(info->mRunningClassOnException), METHOD_NAME2(info->mRunningClassOnException, info->mRunningMethodOnException));
    }
    else {
        fprintf(stderr, "%s: %s\n", CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), mbs);
    }

    FREE(mbs);
}

static unsigned char visible_control_character(unsigned char c)
{
    if(c < ' ') {
        return '^';
    }
    else {
        return c;
    }
}

#ifdef VM_DEBUG

static int num_vm;

void start_vm_log(sVMInfo* info)
{
    char file_name[PATH_MAX];

    info->log_number = num_vm;
    snprintf(file_name, PATH_MAX, "vm%d.log", num_vm++);
    info->debug_log = fopen(file_name, "w");
}

void vm_log(sVMInfo* info, char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    if(info->debug_log) {
        fprintf(info->debug_log, "%s", msg2);
        fflush(info->debug_log);
    }
}

void vm_log_with_log_number(int number, char* msg, ...)
{
    char msg2[1024];
    sVMInfo* info;
    int n;

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    info = gHeadVMInfo;

    while(info) {
        if(info->log_number == number) {
            break;
        }
        info = info->next_info;
    }

    if(info == NULL) {
        fprintf(stderr, "invalid log number\n");
        exit(2);
    }

    if(info->debug_log) {
        fprintf(info->debug_log, "%s", msg2);
        fflush(info->debug_log);
    }
}

static void show_type_object2(CLObject type_object, sVMInfo* info)
{
    int i;

    if(type_object == 0) return;

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, "%s<", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }
    else {
        vm_log(info, "%s", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject type_object2;

        type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
        show_type_object2(type_object2, info);

        if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
            vm_log(info, ",");
        }
    }

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, ">");
    }
}

void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_mutex_lock();

    if(top_of_stack && var ) {
        vm_log(info, "stack_ptr %d top_of_stack %d var %d\n", (int)(info->stack_ptr - info->stack), (int)(top_of_stack - info->stack), (int)(var - info->stack));

        if(info->stack_ptr < info->stack) 
        {
            fprintf(stderr, "invalid stack ptr on show_stack\n");
            exit(2);
        }

        if(info->stack_ptr > info->stack) {
            for(i=0; i<50; i++) {
                if(info->stack + i == info->stack_ptr) {
                    break;
                }
                if(info->stack + i == var) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->v-- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  v-- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == top_of_stack) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->--- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  --- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == info->stack_ptr) {
                    vm_log(info, "->    stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else {
                    vm_log(info, "      stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
            }
        }
        else {
            vm_log(info, "stack is empty\n");
        }
    }
    else {
        for(i=0; i<50; i++) {
            if(info->stack + i == info->stack_ptr) {
                vm_log(info, "->    stack[%d] ", i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
            else {
                vm_log(info, "      stack[%d] ",i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
        }
    }

    vm_mutex_unlock();
}

void show_object_value(sVMInfo* info, CLObject obj)
{
    CLObject type_object;

    if(obj == 0) {
        vm_log(info, "value %d\n", obj);
    }
    else {
        type_object = CLOBJECT_HEADER(obj)->mType;

        if(type_object) {
            if(CLTYPEOBJECT(type_object)->mClass == gNullClass) {
                vm_log(info, "obj %d data null");
            }
            else if(substitution_posibility_of_type_object_without_generics(gArrayTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLARRAY(obj)->mLen);

                for(j=0; j<10 && j<CLARRAY(obj)->mLen; j++) {
                    vm_log(info, "[%d] %d ", j, CLARRAY_ITEMS2(obj, j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gHashTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLHASH(obj)->mLen);

                for(j=0; j<CLHASH(obj)->mSize; j++) {
                    vm_log(info, "[%d] %d %d", j, CLHASH_KEY(obj, j), CLHASH_ITEM(obj,j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gStringTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d (%ls) size", obj, CLSTRING(obj)->mData, CLSTRING_DATA(obj)->mChars);
            }
            else if(substitution_posibility_of_type_object_without_generics(gIntTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLINT(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gByteTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLBYTE(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gBoolTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data %d", obj, CLBOOL(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gFloatTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %f", obj, CLFLOAT(obj)->mValue);
            }
            else {
                vm_log(info, "obj %d", obj);
            }

            vm_log(info, " ");

            vm_log(info, "type (obj %d) size %d ", type_object, CLOBJECT_HEADER(obj)->mHeapMemSize);

            show_type_object2(type_object, info);
            vm_log(info, "\n");
        }
        else {
            vm_log(info, "value %d ", obj);
            vm_log(info, "type (obj %d) \n", type_object);
        }
    }
}

#elif VM_DEBUG2

void vm_log(sVMInfo* info, char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    printf("%s", msg2);
    fflush(stdout);
}

static void show_type_object2(CLObject type_object, sVMInfo* info)
{
    int i;

    if(type_object == 0) return;

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, "%s<", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }
    else {
        vm_log(info, "%s", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject type_object2;

        type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
        show_type_object2(type_object2, info);

        if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
            vm_log(info, ",");
        }
    }

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, ">");
    }
}

void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_mutex_lock();

    if(top_of_stack && var ) {
        vm_log(info, "stack_ptr %d top_of_stack %d var %d\n", (int)(info->stack_ptr - info->stack), (int)(top_of_stack - info->stack), (int)(var - info->stack));

        if(info->stack_ptr < info->stack) 
        {
            fprintf(stderr, "invalid stack ptr on show_stack\n");
            exit(2);
        }

        if(info->stack_ptr > info->stack) {
            for(i=0; i<50; i++) {
                if(info->stack + i == info->stack_ptr) {
                    break;
                }
                if(info->stack + i == var) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->v-- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  v-- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == top_of_stack) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->--- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  --- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == info->stack_ptr) {
                    vm_log(info, "->    stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else {
                    vm_log(info, "      stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
            }
        }
        else {
            vm_log(info, "stack is empty\n");
        }
    }
    else {
        for(i=0; i<50; i++) {
            if(info->stack + i == info->stack_ptr) {
                vm_log(info, "->    stack[%d] ", i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
            else {
                vm_log(info, "      stack[%d] ",i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
        }
    }

    vm_mutex_unlock();
}

void show_object_value(sVMInfo* info, CLObject obj)
{
    CLObject type_object;

    if(obj == 0) {
        vm_log(info, "value %d\n", obj);
    }
    else {
        type_object = CLOBJECT_HEADER(obj)->mType;

        if(type_object) {
            if(CLTYPEOBJECT(type_object)->mClass == gNullClass) {
                vm_log(info, "obj %d data null");
            }
            else if(substitution_posibility_of_type_object_without_generics(gArrayTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLARRAY(obj)->mLen);

                for(j=0; j<10 && j<CLARRAY(obj)->mLen; j++) {
                    vm_log(info, "[%d] %d ", j, CLARRAY_ITEMS2(obj, j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gHashTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLHASH(obj)->mLen);

                for(j=0; j<CLHASH(obj)->mSize; j++) {
                    vm_log(info, "[%d] %d %d", j, CLHASH_KEY(obj, j), CLHASH_ITEM(obj,j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gStringTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d (%ls) size", obj, CLSTRING(obj)->mData, CLSTRING_DATA(obj)->mChars);
            }
            else if(substitution_posibility_of_type_object_without_generics(gIntTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLINT(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gByteTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLBYTE(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gBoolTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data %d", obj, CLBOOL(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gFloatTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %f", obj, CLFLOAT(obj)->mValue);
            }
            else {
                vm_log(info, "obj %d", obj);
            }

            vm_log(info, " ");

            vm_log(info, "type (obj %d) size %d ", type_object, CLOBJECT_HEADER(obj)->mHeapMemSize);

            show_type_object2(type_object, info);
            vm_log(info, "\n");
        }
        else {
            vm_log(info, "value %d ", obj);
            vm_log(info, "type (obj %d) \n", type_object);
        }
    }
}

#else 

void vm_log(sVMInfo* info, char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    printf("%s", msg2);
    fflush(stdout);
}

static void show_type_object2(CLObject type_object, sVMInfo* info)
{
    int i;

    if(type_object == 0) return;

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, "%s<", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }
    else {
        vm_log(info, "%s", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject type_object2;

        type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
        show_type_object2(type_object2, info);

        if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
            vm_log(info, ",");
        }
    }

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, ">");
    }
}

void show_stack(sVMInfo* info)
{
    int i;

    vm_mutex_lock();

    for(i=0; i<50; i++) {
        if(info->stack + i == info->stack_ptr) {
            vm_log(info, "      stack[%d] ",i);
            show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            //break;
        }
        else {
            vm_log(info, "      stack[%d] ",i);
            show_object_value(info, (info->stack + i)->mObjectValue.mValue);
        }
    }

    vm_mutex_unlock();
}

void show_object_value(sVMInfo* info, CLObject obj)
{
    CLObject type_object;

    if(obj == 0) {
        vm_log(info, "value %d\n", obj);
    }
    else {
        type_object = CLOBJECT_HEADER(obj)->mType;

        if(type_object) {
            if(CLTYPEOBJECT(type_object)->mClass == gNullClass) {
                vm_log(info, "obj %d data null");
            }
            else if(substitution_posibility_of_type_object_without_generics(gArrayTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLARRAY(obj)->mLen);

                for(j=0; j<10 && j<CLARRAY(obj)->mLen; j++) {
                    vm_log(info, "[%d] %d ", j, CLARRAY_ITEMS2(obj, j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gHashTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLHASH(obj)->mLen);

                for(j=0; j<CLHASH(obj)->mSize; j++) {
                    vm_log(info, "[%d] %d %d", j, CLHASH_KEY(obj, j), CLHASH_ITEM(obj,j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gStringTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d (%ls) size", obj, CLSTRING(obj)->mData, CLSTRING_DATA(obj)->mChars);
            }
            else if(substitution_posibility_of_type_object_without_generics(gIntTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLINT(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gByteTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %d", obj, CLBYTE(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gBoolTypeObject, type_object, FALSE))
            {
                int j;
                vm_log(info, "obj %d data %d", obj, CLBOOL(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gFloatTypeObject, type_object, FALSE))
            {
                vm_log(info, "obj %d data %f", obj, CLFLOAT(obj)->mValue);
            }
            else {
                vm_log(info, "obj %d", obj);
            }

            vm_log(info, " ");

            vm_log(info, "type (obj %d) size %d ", type_object, CLOBJECT_HEADER(obj)->mHeapMemSize);

            show_type_object2(type_object, info);
            vm_log(info, "\n");
        }
        else {
            vm_log(info, "value %d ", obj);
            vm_log(info, "type (obj %d) \n", type_object);
        }
    }
}

#endif

static void get_class_name_from_bytecodes(int** pc, sConst* constant, char** type)
{
    int ivalue1;

    ivalue1 = **pc;
    (*pc)++;

    *type = CONS_str(constant, ivalue1);
}

static BOOL get_class_info_from_bytecode(sCLClass** result, int** pc, sConst* constant, sVMInfo* info)
{
    char* real_class_name;

    get_class_name_from_bytecodes(pc, constant, &real_class_name);

    *result = cl_get_class(real_class_name);

    if(*result == NULL) {
        entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get class named %s", real_class_name);
        return FALSE;
    }

    return TRUE;
}

static BOOL excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type);
static BOOL excute_method(sCLMethod* method, sCLClass* klass, sCLClass* class_of_class_method_call, sConst* constant, int result_type, sVMInfo* info, CLObject vm_type);
static BOOL param_initializer(sConst* constant, sByteCode* code, int lv_num, int max_stack, sVMInfo* info, CLObject vm_type);

CLObject get_type_from_mvalue(MVALUE* mvalue, sVMInfo* info)
{
    CLObject result;

    if(mvalue->mObjectValue.mValue == 0) {
        result = 0;
    }
    else {
        result = CLOBJECT_HEADER(mvalue->mObjectValue.mValue)->mType;
    }

    return result;
}

static BOOL get_two_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(1)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_float_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(2)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gFloatTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gFloatTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_double_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(2)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gDoubleTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gDoubleTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_byte_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(3)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gByteTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gByteTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_short_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gShortTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gShortTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_uint_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gUIntTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gUIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_long_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gLongTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gLongTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_string_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(4)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gStringTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gStringTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_bytes_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(5)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBytesTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gBytesTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_bool_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(6)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBoolTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gBoolTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}


static BOOL get_string_and_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(ovalue1 == 0 || ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(7)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gStringTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_bytes_and_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(ovalue1 == 0 || ovalue2 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(8)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBytesTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_bool_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(9)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBoolTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_int_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(10)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_byte_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(11)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gByteTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_short_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(11)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gShortTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_uint_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(11)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gUIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_long_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception(11)");
        return FALSE;
    }

    if(!check_type(*ovalue1, gLongTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

// result_type 0: nothing 1: result exists 2: dynamic typing class
static BOOL call_method_missing_method(sCLClass* klass,  BOOL* method_missing_found, sVMInfo* info, CLObject vm_type, char* method_name, BOOL class_method, int num_params, BOOL existance_of_block, int result_type)
{
    if(class_method && klass->mMethodMissingMethodIndexOfClassMethod != -1) {
        sCLMethod* method_missing_method;
        int result_type2;
        CLObject params[CL_METHOD_PARAM_MAX];
        CLObject block;
        CLObject method_name_object;
        CLObject array;
        CLObject method_missing_result_object;
        int i;
        MVALUE* lvar;
        int real_param_num;

        vm_mutex_lock();

        method_missing_method = klass->mMethods + klass->mMethodMissingMethodIndexOfClassMethod;

        /// make parametors ///
        if(!create_string_object_from_ascii_string(&method_name_object, method_name, gStringTypeObject, info))
        {
            vm_mutex_unlock();
            return FALSE;
        }

        push_object(method_name_object, info);

        array = create_array_object_with_element_class_name("anonymous", NULL, 0, info);

        push_object(array, info);

        for(i=0; i<num_params; i++) {
            params[i] = (info->stack_ptr - num_params + i - (existance_of_block ? 1: 0) -2)->mObjectValue.mValue;

            add_to_array(array, params[i], info);
        }

        if(existance_of_block) {
            block = (info->stack_ptr -3)->mObjectValue.mValue;
        }
        else {
            block = create_null_object();
        }

        pop_object_n(info, 2);

        info->stack_ptr->mObjectValue.mValue = method_name_object;
        info->stack_ptr++;

        info->stack_ptr->mObjectValue.mValue = array;
        info->stack_ptr++;

        info->stack_ptr->mObjectValue.mValue = block;
        info->stack_ptr++;

        /// invoke ///
        vm_mutex_unlock();
        result_type2 = 1;

        if(!excute_method(method_missing_method, klass, klass, &klass->mConstPool, result_type2, info, vm_type))
        {
            return FALSE;
        }

        method_missing_result_object = (info->stack_ptr-1)->mObjectValue.mValue;
        info->stack_ptr--;

        /// restore stack pointer ///
        real_param_num = num_params + (class_method ? 0:1) + (existance_of_block ? 1:0) ;
        lvar = info->stack_ptr - real_param_num;

        info->stack_ptr = lvar;

        if(result_type != 0) {
            info->stack_ptr->mObjectValue.mValue = method_missing_result_object;
            info->stack_ptr++;
        }

        *method_missing_found = TRUE;
    }
    else if(!class_method && klass->mMethodMissingMethodIndex != -1) {
        sCLMethod* method_missing_method;
        int result_type2;
        CLObject self;
        CLObject params[CL_METHOD_PARAM_MAX];
        CLObject block;
        CLObject method_name_object;
        CLObject array;
        CLObject method_missing_result_object;
        int i;
        MVALUE* lvar;
        int real_param_num;

        vm_mutex_lock();

        method_missing_method = klass->mMethods + klass->mMethodMissingMethodIndex;

        /// make parametors ///
        if(!create_string_object_from_ascii_string(&method_name_object, method_name, gStringTypeObject, info))
        {
            vm_mutex_unlock();
            return FALSE;
        }

        push_object(method_name_object, info);

        self = (info->stack_ptr - num_params - (existance_of_block ? 1:0) -1 -1)->mObjectValue.mValue;

        push_object(self, info);

        array = create_array_object_with_element_class_name("anonymous", NULL, 0, info);

        push_object(array, info);

        for(i=0; i<num_params; i++) {
            params[i] = (info->stack_ptr - num_params + i - (existance_of_block ? 1: 0) -3)->mObjectValue.mValue;

            add_to_array(array, params[i], info);
        }

        if(existance_of_block) {
            block = (info->stack_ptr -4)->mObjectValue.mValue;
        }
        else {
            block = create_null_object();
        }

        pop_object_n(info, 3);

        info->stack_ptr->mObjectValue.mValue = self;
        info->stack_ptr++;

        info->stack_ptr->mObjectValue.mValue = method_name_object;
        info->stack_ptr++;

        info->stack_ptr->mObjectValue.mValue = array;
        info->stack_ptr++;

        info->stack_ptr->mObjectValue.mValue = block;
        info->stack_ptr++;

        /// invoke ///
        vm_mutex_unlock();
        result_type2 = 1;

        if(!excute_method(method_missing_method, klass, klass, &klass->mConstPool, result_type2, info, vm_type))
        {
            return FALSE;
        }

        method_missing_result_object = (info->stack_ptr-1)->mObjectValue.mValue;
        info->stack_ptr--;

        /// restore stack pointer ///
        real_param_num = num_params + (class_method ? 0:1) + (existance_of_block ? 1:0) ;
        lvar = info->stack_ptr - real_param_num;

        info->stack_ptr = lvar;

        if(result_type != 0) {
            if(strcmp(method_name, "_constructor") == 0) {
                info->stack_ptr->mObjectValue.mValue = self;
                info->stack_ptr++;
            }
            else {
                info->stack_ptr->mObjectValue.mValue = method_missing_result_object;
                info->stack_ptr++;
            }
        }

        *method_missing_found = TRUE;
    }
    else {
        *method_missing_found = FALSE;
    }

    return TRUE;
}

static BOOL call_clone_method(sCLClass* klass, sVMInfo* info, CLObject vm_type)
{
    sCLMethod* method;

    if(klass->mCloneMethodIndex == -1) {
        BOOL method_missing_found;

        method_missing_found = FALSE;

        if(!call_method_missing_method(klass, &method_missing_found, info, vm_type,  "clone", FALSE, 0, FALSE, 1))
        {
            return FALSE;
        }

        if(method_missing_found) {
            return TRUE;
        }
        else {
            entry_exception_object_with_class_name(info, "NullPointerException", "can't get \"clone\" method of %s", REAL_CLASS_NAME(klass));
            return FALSE;
        }
    }

    method = klass->mMethods + klass->mCloneMethodIndex;

    return excute_method(method, klass, klass, &klass->mConstPool, TRUE, info, vm_type);
}

static BOOL null_check_for_neq(sVMInfo* info)
{
    CLObject ovalue1, ovalue2;

    ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(check_type_without_exception(ovalue1, gNullTypeObject, info) || check_type_without_exception(ovalue2, gNullTypeObject, info))
    {
        BOOL not_null_and_null;
        
        not_null_and_null = !(check_type_without_exception(ovalue1, gNullTypeObject, info) && check_type_without_exception(ovalue2, gNullTypeObject, info));

        info->stack_ptr-=2;

        info->stack_ptr->mObjectValue.mValue = create_bool_object(not_null_and_null);  // push result
        info->stack_ptr++;

        return TRUE;
    }

    return FALSE;
}

static BOOL null_check_for_eq(sVMInfo* info)
{
    CLObject ovalue1, ovalue2;

    ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(check_type_without_exception(ovalue1, gNullTypeObject, info) || check_type_without_exception(ovalue2, gNullTypeObject, info))
    {
        BOOL null_and_null;
        
        null_and_null = check_type_without_exception(ovalue1, gNullTypeObject, info) && check_type_without_exception(ovalue2, gNullTypeObject, info);

        info->stack_ptr-=2;

        info->stack_ptr->mObjectValue.mValue = create_bool_object(null_and_null);  // push result
        info->stack_ptr++;

        return TRUE;
    }

    return FALSE;
}

static BOOL output_result_for_interpreter(sVMInfo* info)
{
    CLObject object;
    sCLMethod* method;
    sCLClass* klass;
    CLObject type_object;
    sCLClass* founded_class;
    CLObject result_value;

    object = (info->stack_ptr-1)->mObjectValue.mValue;

    type_object = CLOBJECT_HEADER(object)->mType;

    founded_class = NULL;

    method = get_virtual_method_with_params(type_object, "outputValueForInterpreter", NULL, 0, &founded_class, FALSE, 0, 0, NULL, 0, info);

    if(!cl_excute_method(method, founded_class, FALSE, info, &result_value))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var, sVMInfo* info, CLObject vm_type)
{
    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11, ivalue12, ivalue13, ivalue14;
    char cvalue1;
    unsigned char bvalue1, bvalue2, bvalue3, bvalue4, bvalue5;
    float fvalue1, fvalue2, fvalue3;
    double dvalue1, dvalue2;
    unsigned short shvalue1, shvalue2, shvalue3;
    unsigned int uivalue1, uivalue2, uivalu3;
    unsigned long lovalue1, lovalue2, lovalue3;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr2;
    CLObject catch_blocks[CL_CATCH_BLOCK_NUMBER_MAX];
    CLObject catch_block_type[CL_CATCH_BLOCK_NUMBER_MAX];

    sCLClass *klass1, *klass2, *klass3, *klass4;
    sCLMethod* method;
    sCLField* field;
    wchar_t* str;
    char* str2;
    char* real_class_name;
    CLObject params[CL_METHOD_PARAM_MAX];
    CLObject params2[CL_METHOD_PARAM_MAX];
    char* string_type1;
    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];
    MVALUE keys[CL_ARRAY_ELEMENTS_MAX];
    int num_vars;
    int i, j;
    MVALUE* top_of_stack;
    int* pc;
    CLObject type1, type2, type3;
    fCreateFun create_fun;

    int field_index;

    BOOL result;
    int return_count;

    pc = code->mCode;
    top_of_stack = info->stack_ptr;
    num_vars = info->stack_ptr - var;

VMLOG(info, "VM starts\n");
//SHOW_STACK(info, top_of_stack, var);
//SHOW_HEAP(info);

    while(pc - code->mCode < code->mLen) {
VMLOG(info, "pc - code->mCode %d\n", pc - code->mCode);
        switch(*pc) {
            case OP_IADD:
VMLOG(info, "OP_IADD\n");
                vm_mutex_lock();
                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue + CLINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BADD:
VMLOG(info, "OP_BADD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue + CLBYTE(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHADD:
VMLOG(info, "OP_SHADD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue + CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIADD:
VMLOG(info, "OP_UIADD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue + CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOADD:
VMLOG(info, "OP_LOADD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue + CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FADD:
VMLOG(info, "OP_FADD\n");
                vm_mutex_lock();
                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue + CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object_with_type(fvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DADD:
VMLOG(info, "OP_DADD\n");
                vm_mutex_lock();
                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                dvalue1 = CLDOUBLE(ovalue1)->mValue + CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_double_object_with_type(dvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;


            case OP_SADD:
VMLOG(info, "OP_SADD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSTRING(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLSTRING(ovalue2)->mLen;  // string length of ovalue2

                str = MALLOC(sizeof(wchar_t)*(ivalue1 + ivalue2 + 1));

                wcscpy(str, CLSTRING_DATA(ovalue1)->mChars);
                wcscat(str, CLSTRING_DATA(ovalue2)->mChars);
VMLOG(info, "%ls + %ls\n", CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars);

                ovalue3 = create_string_object(str, ivalue1 + ivalue2, CLOBJECT_HEADER(ovalue1)->mType, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;

                FREE(str);
                vm_mutex_unlock();
                break;

            case OP_BSADD:
VMLOG(info, "OP_BSADD");
                vm_mutex_lock();
                pc++;

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTES(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLBYTES(ovalue2)->mLen;  // string length of ovalue2

                str2 = CALLOC(1, sizeof(char)*(ivalue1 + ivalue2 + 1));

                xstrncpy(str2, CLBYTES(ovalue1)->mChars, ivalue1 + ivalue2 + 1);

                xstrncat(str2, CLBYTES(ovalue2)->mChars, ivalue1 + ivalue2 + 1);

                ovalue3 = create_bytes_object(str2, ivalue1 + ivalue2, CLOBJECT_HEADER(ovalue1)->mType, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;

                FREE(str2);
                vm_mutex_unlock();
                break;

            case OP_ISUB:
VMLOG(info, "OP_ISUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
    
                ivalue1 = CLINT(ovalue1)->mValue - CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BSUB:
VMLOG(info, "OP_BSUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue - CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHSUB:
VMLOG(info, "OP_SHSUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue - CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UISUB:
VMLOG(info, "OP_UISUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue - CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOSUB:
VMLOG(info, "OP_LOSUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue - CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FSUB:
VMLOG(info, "OP_FSUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue - CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object_with_type(fvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DSUB:
VMLOG(info, "OP_DSUB\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                dvalue1 = CLDOUBLE(ovalue1)->mValue - CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_double_object_with_type(dvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IMULT:
VMLOG(info, "OP_IMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
    
                ivalue1 = CLINT(ovalue1)->mValue * CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BMULT:
VMLOG(info, "OP_BMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue * CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHMULT:
VMLOG(info, "OP_SHMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue * CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIMULT:
VMLOG(info, "OP_UIMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue * CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOMULT:
VMLOG(info, "OP_LOMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue * CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FMULT:
VMLOG(info, "OP_FMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue * CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object_with_type(fvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DMULT:
VMLOG(info, "OP_DMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                dvalue1 = CLDOUBLE(ovalue1)->mValue * CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_double_object_with_type(dvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BSMULT:
VMLOG(info, "OP_BSMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_bytes_and_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue2)->mValue;
                ovalue3 = create_bytes_object_by_multiply_with_type(ovalue1, ivalue1, info, CLOBJECT_HEADER(ovalue1)->mType);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_SMULT:
VMLOG(info, "OP_SMULT\n");
                vm_mutex_lock();

                pc++;

                if(!get_string_and_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue2)->mValue;
                ovalue3 = create_string_object_by_multiply_with_type(ovalue1, ivalue1, info, CLOBJECT_HEADER(ovalue1)->mType);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IDIV:
VMLOG(info, "OP_IDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue2 = CLINT(ovalue2)->mValue;

                if(ivalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = ivalue1 / ivalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BDIV:
VMLOG(info, "OP_BDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue2 = CLBYTE(ovalue2)->mValue;

                if(bvalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = bvalue1 / bvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHDIV:
VMLOG(info, "OP_SHDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue;
                shvalue2 = CLSHORT(ovalue2)->mValue;

                if(shvalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = shvalue1 / shvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIDIV:
VMLOG(info, "OP_UIDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue;
                uivalue2 = CLUINT(ovalue2)->mValue;

                if(uivalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = uivalue1 / uivalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LODIV:
VMLOG(info, "OP_LODIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue;
                lovalue2 = CLLONG(ovalue2)->mValue;

                if(lovalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = lovalue1 / lovalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FDIV:
VMLOG(info, "OP_FDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue;
                fvalue2 = CLFLOAT(ovalue2)->mValue;

                if(fvalue2 == 0.0f) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = fvalue1 / fvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object_with_type(fvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DDIV:
VMLOG(info, "OP_DDIV\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                dvalue1 = CLDOUBLE(ovalue1)->mValue;
                dvalue2 = CLDOUBLE(ovalue2)->mValue;

                if(dvalue2 == 0.0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                dvalue1 = dvalue1 / dvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_double_object_with_type(dvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IMOD:
VMLOG(info, "OP_IMOD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue2 = CLINT(ovalue2)->mValue;

                if(ivalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = ivalue1 % ivalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BMOD:
VMLOG(info, "OP_BMOD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue2 = CLBYTE(ovalue2)->mValue;

                if(bvalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = bvalue1 % bvalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHMOD:
VMLOG(info, "OP_SHMOD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue;
                shvalue2 = CLSHORT(ovalue2)->mValue;

                if(shvalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = shvalue1 % shvalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIMOD:
VMLOG(info, "OP_UIMOD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue;
                uivalue2 = CLUINT(ovalue2)->mValue;

                if(uivalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = uivalue1 % uivalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOMOD:
VMLOG(info, "OP_LOMOD\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue;
                lovalue2 = CLLONG(ovalue2)->mValue;

                if(lovalue2 == 0) {
                    entry_exception_object_with_class_name(info, "DivisionByZeroException", "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = lovalue1 % lovalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILSHIFT:
VMLOG(info, "OP_ILSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue << CLINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLSHIFT:
VMLOG(info, "OP_BLSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue << CLBYTE(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHLSHIFT:
VMLOG(info, "OP_SHLSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue << CLSHORT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UILSHIFT:
VMLOG(info, "OP_UILSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue << CLUINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOLSHIFT:
VMLOG(info, "OP_LOLSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue << CLLONG(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IRSHIFT:
VMLOG(info, "OP_IRSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue >> CLINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BRSHIFT:
VMLOG(info, "OP_BRSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue >> CLBYTE(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHRSHIFT:
VMLOG(info, "OP_SHRSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue >> CLSHORT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIRSHIFT:
VMLOG(info, "OP_UIRSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue >> CLUINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LORSHIFT:
VMLOG(info, "OP_LORSHIFT\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue >> CLLONG(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IAND:
VMLOG(info, "OP_IAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue & CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BAND:
VMLOG(info, "OP_BAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue & CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHAND:
VMLOG(info, "OP_SHAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue & CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIAND:
VMLOG(info, "OP_UIAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue & CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOAND:
VMLOG(info, "OP_LOAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue & CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IXOR:
VMLOG(info, "OP_IXOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue ^ CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BXOR:
VMLOG(info, "OP_BXOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue ^ CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHXOR:
VMLOG(info, "OP_SHXOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue ^ CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIXOR:
VMLOG(info, "OP_UIXOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue ^ CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOXOR:
VMLOG(info, "OP_LOXOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue ^ CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IOR:
VMLOG(info, "OP_IOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue | CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BOR:
VMLOG(info, "OP_BAOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue | CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHOR:
VMLOG(info, "OP_SHOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue | CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIOR:
VMLOG(info, "OP_UIOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue | CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOOR:
VMLOG(info, "OP_LOOR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue | CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IGTR:
VMLOG(info, "OP_IGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue > CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BGTR:
VMLOG(info, "OP_BGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue > CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHGTR:
VMLOG(info, "OP_SHGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue > CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIGTR:
VMLOG(info, "OP_UIGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue > CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOGTR:
VMLOG(info, "OP_LOGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue > CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FGTR:
VMLOG(info, "OP_FGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue > CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DGTR:
VMLOG(info, "OP_DGTR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue > CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IGTR_EQ:
VMLOG(info, "OP_IGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue >= CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BGTR_EQ:
VMLOG(info, "OP_BGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue >= CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHGTR_EQ:
VMLOG(info, "OP_SHGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue >= CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIGTR_EQ:
VMLOG(info, "OP_UIGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue >= CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOGTR_EQ:
VMLOG(info, "OP_LOGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue >= CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FGTR_EQ:
VMLOG(info, "OP_FGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue >= CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DGTR_EQ:
VMLOG(info, "OP_DGTR_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue >= CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILESS:
VMLOG(info, "OP_ILESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue < CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLESS:
VMLOG(info, "OP_BLESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue < CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHLESS:
VMLOG(info, "OP_SHLESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue < CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UILESS:
VMLOG(info, "OP_UILESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue < CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOLESS:
VMLOG(info, "OP_LOLESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue < CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FLESS:
VMLOG(info, "OP_FLESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue < CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DLESS:
VMLOG(info, "OP_DLESS\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue < CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILESS_EQ:
VMLOG(info, "OP_ILESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue <= CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLESS_EQ:
VMLOG(info, "OP_BLESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue <= CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHLESS_EQ:
VMLOG(info, "OP_SHLESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue <= CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UILESS_EQ:
VMLOG(info, "OP_UILESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue <= CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOLESS_EQ:
VMLOG(info, "OP_LOLESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue <= CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FLESS_EQ:
VMLOG(info, "OP_FLESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue <= CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DLESS_EQ:
VMLOG(info, "OP_DLESS_EQ\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue <= CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IEQ:
VMLOG(info, "OP_IEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue == CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BEQ:
VMLOG(info, "OP_BEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue == CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHEQ:
VMLOG(info, "OP_SHEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue == CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UIEQ:
VMLOG(info, "OP_UIEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue == CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOEQ:
VMLOG(info, "OP_LOEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue == CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FEQ:
VMLOG(info, "OP_FEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue == CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DEQ:
VMLOG(info, "OP_DEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue == CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SEQ:
VMLOG(info, "OP_SEQ");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (wcscmp(CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars) == 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_BSEQ:
VMLOG(info, "OP_BSEQ");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (strcmp(CLBYTES(ovalue1)->mChars, (const char*)CLBYTES(ovalue2)->mChars) == 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_INOTEQ:
VMLOG(info, "OP_INOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue != CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BNOTEQ:
VMLOG(info, "OP_BNOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue != CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SHNOTEQ:
VMLOG(info, "OP_SHNOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_short_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSHORT(ovalue1)->mValue != CLSHORT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_UINOTEQ:
VMLOG(info, "OP_UINOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_uint_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLUINT(ovalue1)->mValue != CLUINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LONOTEQ:
VMLOG(info, "OP_LONOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_long_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLLONG(ovalue1)->mValue != CLLONG(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FNOTEQ:
VMLOG(info, "OP_FNOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue != CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DNOTEQ:
VMLOG(info, "OP_DNOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_double_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLDOUBLE(ovalue1)->mValue != CLDOUBLE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SNOTEQ:
VMLOG(info, "OP_SNOTEQ");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (wcscmp(CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars) != 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_BSNOTEQ:
VMLOG(info, "OP_BSNOTEQ");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (strcmp((const char*)CLBYTES(ovalue1)->mChars, (const char*)CLBYTES(ovalue2)->mChars) != 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_COMPLEMENT:
VMLOG(info, "OP_COMPLEMENT\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue1 = ~ivalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_int_object_with_type(ivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                vm_mutex_unlock();
                break;

            case OP_BCOMPLEMENT:
VMLOG(info, "OP_BCOMPLEMENT\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_byte_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue1 = ~bvalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_byte_object_with_type(bvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                vm_mutex_unlock();
                break;

            case OP_SHCOMPLEMENT:
VMLOG(info, "OP_SHCOMPLEMENT\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_short_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                shvalue1 = CLSHORT(ovalue1)->mValue;
                shvalue1 = ~shvalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_short_object_with_type(shvalue1, CLOBJECT_HEADER(ovalue1)->mType);
                vm_mutex_unlock();
                break;

            case OP_UICOMPLEMENT:
VMLOG(info, "OP_UICOMPLEMENT\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_uint_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                uivalue1 = CLUINT(ovalue1)->mValue;
                uivalue1 = ~uivalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_uint_object_with_type(uivalue1, CLOBJECT_HEADER(ovalue1)->mType);
                vm_mutex_unlock();
                break;

            case OP_LOCOMPLEMENT:
VMLOG(info, "OP_LOCOMPLEMENT\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_long_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                lovalue1 = CLLONG(ovalue1)->mValue;
                lovalue1 = ~lovalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_long_object_with_type(lovalue1, CLOBJECT_HEADER(ovalue1)->mType);
                vm_mutex_unlock();
                break;

            case OP_BLEQ:
VMLOG(info, "OP_BLEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_eq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (CLBOOL(ovalue1)->mValue?1:0) == (CLBOOL(ovalue2)->mValue? 1:0);
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLNOTEQ:
VMLOG(info, "OP_BLNOTEQ\n");
                vm_mutex_lock();

                pc++;

                if(null_check_for_neq(info)) {
                    vm_mutex_unlock();
                    break;
                }

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (CLBOOL(ovalue1)->mValue?1:0) != (CLBOOL(ovalue2)->mValue?1:0);
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLANDAND:
VMLOG(info, "OP_BLANDAND\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue && CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLOROR:
VMLOG(info, "OP_BLOROR\n");
                vm_mutex_lock();

                pc++;

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue || CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LOGICAL_DENIAL:
VMLOG(info, "OP_LOGICAL_DENIAL %d\n", ivalue1);
                vm_mutex_lock();

                pc++;

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = !CLBOOL(ovalue1)->mValue;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCINT:
VMLOG(info, "OP_LDCINT\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCBYTE:
VMLOG(info, "OP_LDCBYTE\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_byte_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCFLOAT:
VMLOG(info, "OP_LDCFLOAT\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

                fvalue1 = *(float*)(constant->mConst + ivalue1);

                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);

VMLOG(info, "%f(%d) is created\n", fvalue1, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCWSTR: {
VMLOG(info, "OP_LDCWSTR\n");
                int size;
                wchar_t* wcs;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                wcs = (wchar_t*)(constant->mConst + ivalue1);
                size = wcslen(wcs);

                info->stack_ptr->mObjectValue.mValue = create_string_object(wcs, size, gStringTypeObject, info);
VMLOG(info, "%ls(%d) is created\n", wcs, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_LDCBOOL:
VMLOG(info, "OP_LDCBOOL\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCNULL:
VMLOG(info, "OP_LDCNULL\n");
                vm_mutex_lock();
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_null_object();

VMLOG(info, "null(%d) is created\n", info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDTYPE: {
VMLOG(info, "OP_LDTYPE\n");
                CLObject obj;

                vm_mutex_lock();

                pc++;

                obj = create_type_object_from_bytecodes(&pc, code, constant, info);

                if(obj == 0) {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;
                pc++;

                ivalue2 = info->num_vm_types - ivalue1 -1;

                if(ivalue1 == -1 || ivalue2 < 0 || ivalue2 >= CL_VM_TYPES_MAX) 
                {
                    info->stack_ptr->mObjectValue.mValue = obj;
                    info->stack_ptr++;
                }
                else {
                    push_object(obj, info);


                    if(!solve_generics_types_of_type_object(obj, ALLOC &type1, info->vm_types[ivalue2], info))
                    {
                        pop_object_except_top(info);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    pop_object(info);

                    info->stack_ptr->mObjectValue.mValue = type1;
                    info->stack_ptr++;
                }

                vm_mutex_unlock();
                }
                break;

            case OP_LDCSTR: {
VMLOG(info, "OP_LDCSTR\n");
                int size;
                char* mbs;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                mbs = (char*)(constant->mConst + ivalue1);
                size = strlen(mbs);

                info->stack_ptr->mObjectValue.mValue = create_bytes_object(mbs, size, gBytesTypeObject, info);
VMLOG(info, "%s(%d) is created\n", mbs, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_LDCSHORT:
VMLOG(info, "OP_LDCSHORT\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_short_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCUINT:
VMLOG(info, "OP_LDCUINT\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_uint_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCLONG:
VMLOG(info, "OP_LDCLONG\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                ivalue2 = *pc;
                pc++;

                memcpy(&lovalue1, &ivalue1, sizeof(int));
                memcpy((char*)&lovalue1 + sizeof(int), &ivalue2, sizeof(int));

                info->stack_ptr->mObjectValue.mValue = create_long_object(lovalue1);

VMLOG(info, "%ld(%d) is created\n", lovalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCCHAR:
VMLOG(info, "OP_LDCCHAR\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                info->stack_ptr->mObjectValue.mValue = create_char_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDCDOUBLE:
VMLOG(info, "OP_LDCDOUBLE\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

                dvalue1 = *(double*)(constant->mConst + ivalue1);

                info->stack_ptr->mObjectValue.mValue = create_double_object(dvalue1);

VMLOG(info, "%f(%d) is created\n", dvalue1, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;


            case OP_LDCPATH: {
VMLOG(info, "OP_LDCPATH\n");
                char* mbs;
                char* mbs2;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                mbs = (char*)(constant->mConst + ivalue1);

                if(mbs[0] == '~') {
                    char* home;

                    home = getenv("HOME");

                    if(home) {
                        int size;

                        size = strlen(home) + strlen(mbs) + 1;

                        mbs2 = MALLOC(size);
                        xstrncpy(mbs2, home, size);
                        xstrncat(mbs2, mbs + 1, size);
                    }
                    else {
                        mbs2 = STRDUP(mbs);
                    }
                }
                else {
                    mbs2 = STRDUP(mbs);
                }

                if(!create_string_object_from_ascii_string_with_class_name(&ovalue1, mbs2, "Path", info))
                {
                    vm_mutex_unlock();
                    FREE(mbs2);
                    return FALSE;
                }

                info->stack_ptr->mObjectValue.mValue = ovalue1;
VMLOG(info, "Path Object %s(%d) is created\n", mbs, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;

                FREE(mbs2);
                vm_mutex_unlock();
                }
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
            case OP_OSTORE:
VMLOG(info, "OP_ISTORE\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;
                pc++;

                /// range checking ///
                if(ivalue1 < 0 || var + ivalue1 >= top_of_stack) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfStackException", "Out of range of stack. Clover can't load the variable");
                    vm_mutex_unlock();
                    return FALSE;
                }

VMLOG(info, "OP_STORE %d\n", ivalue1);
                var[ivalue1] = *(info->stack_ptr-1);
                vm_mutex_unlock();
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
VMLOG(info, "OP_ILOAD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;
                pc++;
VMLOG(info, "OP_LOAD %d\n", ivalue1);

                /// range checking ///
                if(ivalue1 < 0 || var + ivalue1 >= top_of_stack) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfStackException", "Out of range of stack. Clover can't load the variable");
                    vm_mutex_unlock();
                    return FALSE;
                }

                *info->stack_ptr = var[ivalue1];
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SRFIELD:
VMLOG(info, "OP_SRFIELD\n");
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;    // target object
                mvalue1 = info->stack_ptr-1;                    // right value

                ivalue1 = *pc;                              // field index
                pc++;

                ivalue2 = *pc;                // class name of field index
                pc++;

                real_class_name = CONS_str(constant, ivalue2);
                klass1 = cl_get_class(real_class_name);

                ASSERT(klass1 != NULL);

                ivalue1 += get_sum_of_non_class_fields_only_super_classes(klass1);

                /// type checking ///
                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                type2 = CLOBJECT_HEADER(ovalue1)->mType;

                if(!substitution_posibility_of_type_object_without_generics(type1, type2, TRUE))
                {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "Exception", "Clover can't access the field because the type of field is invalid.(2)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);

                /// range checking ///
                if(ivalue1 < 0 || ivalue1 >= CLUSEROBJECT(ovalue1)->mNumFields) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfFieldException", "Out of range of field. Clover can't load the field(2)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// access //
                CLUSEROBJECT(ovalue1)->mFields[ivalue1] = *mvalue1;
VMLOG(info, "SRFIELD object %d field num %d value %d\n", (int)ovalue1, ivalue1, mvalue1->mObjectValue.mValue);
                info->stack_ptr-=2;

                *info->stack_ptr = *mvalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LDFIELD:
VMLOG(info, "OP_LDFIELD\n");
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

                ivalue1 = *pc;                // field index
                pc++;

                ivalue2 = *pc;                // class name of field index
                pc++;

                real_class_name = CONS_str(constant, ivalue2);
                klass1 = cl_get_class(real_class_name);

                ASSERT(klass1 != NULL);

                ivalue1 += get_sum_of_non_class_fields_only_super_classes(klass1);

                /// type checking ///
                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                type2 = CLOBJECT_HEADER(ovalue1)->mType;

                if(!substitution_posibility_of_type_object_without_generics(type1, type2, TRUE))
                {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "Exception", "Clover can't access the field because the type of field is invalid.(1)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);

                /// range checking ///
                if(ivalue1 < 0 || ivalue1 >= CLUSEROBJECT(ovalue1)->mNumFields) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfFieldException", "Out of range of field. Clover can't load the field(1)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// access ///
VMLOG(info, "LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);
                info->stack_ptr--;
                *info->stack_ptr = CLUSEROBJECT(ovalue1)->mFields[ivalue1];

                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LD_STATIC_FIELD: 
VMLOG(info, "OP_LD_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;             // class name
                pc++;

                ivalue2 = *pc;             // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named (%s)", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// range checking ///
                if(ivalue2 < 0 || ivalue2 >= klass1->mNumFieldsIncludingSuperClasses) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfFieldException", "Out of range of field. Clover can't load the field(3)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// access ///
                field = klass1->mFields + ivalue2;

                if(!(field->mFlags & CL_STATIC_FIELD)) {
                    entry_exception_object_with_class_name(info, "Exception", "Unexpected error. This field is not static field. ");
                    vm_mutex_unlock();
                    return FALSE;
                }

                *info->stack_ptr = field->uValue.mStaticField;
VMLOG(info, "LD_STATIC_FIELD %d\n", field->uValue.mStaticField.mObjectValue.mValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SR_STATIC_FIELD:
VMLOG(info, "OP_SR_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                    // class name
                pc++;

                ivalue2 = *pc;                    // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get the class named %s3", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// range checking ///
                if(ivalue2 < 0 || ivalue2 >= klass1->mNumFieldsIncludingSuperClasses) {
                    entry_exception_object_with_class_name(info, "OutOfRangeOfFieldException", "Out of range of field. Clover can't load the field(4)");
                    vm_mutex_unlock();
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                if(!(field->mFlags & CL_STATIC_FIELD)) {
                    entry_exception_object_with_class_name(info, "Exception", "Unexpected error. This field is not static field. ");
                    vm_mutex_unlock();
                    return FALSE;
                }

                field->uValue.mStaticField = *(info->stack_ptr-1);
VMLOG(info, "OP_SR_STATIC_FIELD value %d\n", (info->stack_ptr-1)->mObjectValue.mValue);
                vm_mutex_unlock();
                break;

            case OP_NEW_OBJECT:
VMLOG(info, "NEW_OBJECT\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);
                push_object(type2, info);

                klass1 = CLTYPEOBJECT(type2)->mClass;
VMLOG(info, "klass %s\n", REAL_CLASS_NAME(klass1));

                if(klass1->mFlags & CLASS_FLAGS_NATIVE) {
                    create_fun = klass1->mCreateFun;

                    if(create_fun == NULL) {
                        pop_object(info);
                        entry_exception_object_with_class_name(info, "Exception", "Clover can't create object of this native class(%s) because of no creating object function", real_class_name);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    ovalue1 = create_fun(type2, info);
                }
                else {
                    if(!create_user_object(type2, &ovalue1, vm_type, NULL, 0, info)) {
                        pop_object(info);
                        entry_exception_object_with_class_name(info, "Exception", "can't create user object");
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }

                pop_object(info);

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_ARRAY:
VMLOG(info, "OP_NEW_ARRAY\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);
                push_object(type2, info);

                ivalue2 = *pc;                              // number of elements
                pc++;

                stack_ptr2 = info->stack_ptr - ivalue2 -1;
                for(i=0; i<ivalue2; i++) {
                    objects[i] = *stack_ptr2++;
                }

                ovalue1 = create_array_object(type2, objects, ivalue2, info);
VMLOG(info, "new array %d\n", ovalue1);
                pop_object(info);

                info->stack_ptr -= ivalue2;
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_HASH:
VMLOG(info, "OP_NEW_HASH\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);
                push_object(type2, info);

                ivalue2 = *pc;                              // number of elements
                pc++;

                ASSERT(ivalue2 % 2 == 0);

                stack_ptr2 = info->stack_ptr - ivalue2 -1;
                for(i=0; i<ivalue2/2; i++) {
                    keys[i] = *stack_ptr2++;
                    objects[i] = *stack_ptr2++;
                }

                if(!create_hash_object(&ovalue1, type2, keys, objects, ivalue2/2, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }
VMLOG(info, "new hash %d\n", ovalue1);
                pop_object(info);

                info->stack_ptr -= ivalue2;
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_RANGE:
VMLOG(info, "OP_NEW_RANGE\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);
                push_object(type2, info);

                ovalue1 = (info->stack_ptr-3)->mObjectValue.mValue;
                ovalue2 = (info->stack_ptr-2)->mObjectValue.mValue;

                if(!check_type_with_nullable(ovalue1, gIntTypeObject, info)) {
                    vm_mutex_unlock();
                    return FALSE;
                }
                if(!check_type_with_nullable(ovalue2, gIntTypeObject, info)) {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ovalue1 = create_range_object(type2, ovalue1, ovalue2);
VMLOG(info, "new range %d\n", ovalue1);
                pop_object(info);

                info->stack_ptr -= 2;
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_TUPLE:
VMLOG(info, "OP_NEW_ARRAY\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    pop_object_except_top(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                pop_object(info);
                push_object(type2, info);

                ivalue2 = *pc;                              // number of elements
                pc++;

                stack_ptr2 = info->stack_ptr - ivalue2 -1;
                for(i=0; i<ivalue2; i++) {
                    objects[i] = *stack_ptr2++;
                }

                if(!create_user_object(type2, &ovalue1, vm_type, objects, ivalue2, info)) 
                {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "Exception", "can't create user object");
                    vm_mutex_unlock();
                    return FALSE;
                }
VMLOG(info, "new tuple %d\n", ovalue1);
                pop_object(info);

                info->stack_ptr -= ivalue2;
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;


            case OP_NEW_REGEX:
VMLOG(info, "OP_NEW_REGEX\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // regex string
                pc++;

                str2 = CONS_str(constant, ivalue1);

                ivalue2 = *pc;          // global
                pc++;

                ivalue3 = *pc;          // mutiline
                pc++;

                ivalue4 = *pc;          // ignore case
                pc++;

                if(!create_oniguruma_regex_object(&ovalue1, gOnigurumaRegexTypeObject, (OnigUChar*)str2, ivalue4, ivalue3, ivalue2, ONIG_ENCODING_UTF8, info, vm_type)) 
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_BLOCK: {
VMLOG(info, "OP_NEW_BLOCK\n");
                char* const_buf;
                int* code_buf;
                int constant2_len;
                int code2_len;

                vm_mutex_lock();

                pc++;

                ivalue2 = *pc;                      // max stack
                pc++;

                ivalue3 = *pc;                      // num locals
                pc++;

                ivalue4 = *pc;                      // num params
                pc++;

                ivalue8 = *pc;                      // max_block_var_num
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);           // block result type

                memset(params, 0, sizeof(params));
                for(j=0; j<ivalue4; j++) {           // block param types
                    params[j] = create_type_object_from_bytecodes(&pc, code, constant, info);
                    push_object(params[j], info);
                }

                constant2_len = *pc;                // constant pool len
                pc++;
                
                ivalue6 = *pc;                      // constant pool offset
                pc++;

                const_buf = CONS_str(constant, ivalue6);

                code2_len = *pc;                    // byte code len
                pc++;

                ivalue7 = *pc;                      // byte code offset
                pc++;

                code_buf = (int*)CONS_str(constant, ivalue7);

                ivalue9 = *pc;                      // breakable
                pc++;

                ovalue1 = create_block(const_buf, constant2_len, code_buf, code2_len, ivalue2, ivalue3, ivalue4, var, ivalue8, type1, params, ivalue9);

                pop_object(info);
                pop_object_n(info, ivalue4);

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_INVOKE_METHOD:
VMLOG(info, "OP_INVOKE_METHOD\n");
                vm_mutex_lock();

                pc++;

                /// method data ///
                ivalue1 = *pc;                  // real class name offset
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named (%s)", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue2 = *pc;                  // method index
                pc++;

                ivalue3 = *pc;                  // existance of result
                pc++;

                ivalue4 = *pc;                  // num params
                pc++;

                ivalue6 = *pc;                  // method num block type
                pc++;

                ivalue10 = *pc;                 // class method
                pc++;

                ivalue11 = *pc;                 // method name
                pc++;

                ivalue9 = *pc;                  // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_CLASS) {
VMLOG(info, "INVOKE_METHOD_KIND_CLASS\n");
                    type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                    push_object(type1, info);

                    if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                    {
                        pop_object_except_top(info);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    pop_object(info);

                    if(type2 == 0) {
                        entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    if(!get_class_info_from_bytecode(&klass2, &pc, constant, info))
                    {
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }
                else {
VMLOG(info, "INVOKE_METHOD_KIND_OBJECT\n");
                    mvalue1 = info->stack_ptr-ivalue4-ivalue6-1;  // get self

VMLOG(info, "-ivalue4-ivalu6-1 %d\n", -ivalue4-ivalue6-1);

                    type2 = get_type_from_mvalue(mvalue1, info);

                    ASSERT(type2 != 0);

                    klass2 = klass1;
                }

                if(ivalue2 >= 0 && ivalue2 < klass1->mNumMethods) {
                    method = klass1->mMethods + ivalue2;
                    vm_mutex_unlock();
                }
                else {
                    BOOL method_missing_found;

                    vm_mutex_unlock();

                    method_missing_found = FALSE;
                    if(!call_method_missing_method(klass1, &method_missing_found, info, vm_type, CONS_str(constant, ivalue11), ivalue10, ivalue4, ivalue6, ivalue3)) 
                    {
                        return FALSE;
                    }

                    if(method_missing_found) {
                        break;
                    }
                    else {
                        vm_mutex_lock();
                        entry_exception_object_with_class_name(info, "MethodMissingException", "can't get a method which has index of %d from %s", ivalue2, REAL_CLASS_NAME(klass1));
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }

VMLOG(info, "klass1 %s\n", REAL_CLASS_NAME(klass1));
VMLOG(info, "method name (%s)\n", METHOD_NAME(klass1, ivalue2));
VMLOG(info, "the stack before excute_method is ↓");
//SHOW_STACK(info, top_of_stack, var);

//printf("METHOD_NAME2(klass1, method) %s\n", METHOD_NAME2(klass1,method));

                if(!excute_method(method, klass1, klass2, &klass1->mConstPool, ivalue3, info, type2))
                {
                    return FALSE;
                }

VMLOG(info, "klass1 %s\n", REAL_CLASS_NAME(klass1));
VMLOG(info, "method name (%s)\n", METHOD_NAME(klass1, ivalue2));
VMLOG(info, "OP_INVOKE_METHOD is end\n");
                break;

            case OP_INVOKE_VIRTUAL_METHOD: {
VMLOG(info, "OP_INVOKE_VIRTUAL_METHOD\n");
                vm_mutex_lock();

                pc++;

                /// method data ///
                ivalue1 = *pc;           // method name offset
                pc++;

                ivalue2 = *pc;           // method num params
                pc++;

                ivalue11 = *pc;                         // existance of block
                pc++;

                ivalue5 = *pc;  // existance of result
                pc++;

                ivalue6 = *pc;  // super
                pc++;

                ivalue7 = *pc;   // class method
                pc++;

                ivalue8 = *pc;   // method num block
                pc++;

                ivalue9 = *pc;   // object kind
                pc++;

VMLOG(info, "calling method name (%s)\n", CONS_str(constant, ivalue1));

                if(ivalue9 == INVOKE_METHOD_KIND_OBJECT) {
VMLOG(info, "INVOKE_METHOD_KIND_OBJECT\n");
                    ASSERT(ivalue7 == 0);

                    mvalue1 = (info->stack_ptr-ivalue2-ivalue8-1);

                    if(ivalue6) { // super
                        type2 = get_type_from_mvalue(mvalue1, info);
                        ASSERT(type2 != 0);

                        type2 = get_super_from_type_object(type2, info);

                        if(type2 == 0) {
                            entry_exception_object_with_class_name(info, "Exception", "can't get a super class from object #%lu", ovalue1);
                            vm_mutex_unlock();
                            return FALSE;
                        }
                    }
                    else {
                        type2 = get_type_from_mvalue(mvalue1, info);

                        ASSERT(type2 != 0);
                    }

                    klass3 = NULL;
                }
                else {
VMLOG(info, "INVOKE_METHOD_KIND_CLASS\n");

                    type1 = create_type_object_from_bytecodes(&pc, code, constant, info);

                    push_object(type1, info);

                    if(type1 == 0) {
                        pop_object(info);
                        entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                    {
                        pop_object_except_top(info);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    pop_object(info);

                    if(type2 == 0) {
                        entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    if(!get_class_info_from_bytecode(&klass3, &pc, constant, info))
                    {
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    bvalue1 = 0;
                }

                /// get params type from stack ///
                memset(params, 0, sizeof(params));

                for(i=0; i<ivalue2; i++) {
                    mvalue1 = info->stack_ptr-ivalue2-ivalue8 + i;
                    ovalue1 = mvalue1->mObjectValue.mValue;
                    params[i] = CLOBJECT_HEADER(ovalue1)->mType;
                }

                /// get block params type from block object in stack ///
                if(ivalue11) {
                    mvalue1 = info->stack_ptr-ivalue2-ivalue8 + ivalue2;
                    ovalue1 = mvalue1->mObjectValue.mValue; // block object

                    ASSERT(ovalue1 && check_type(ovalue1, gBlockTypeObject, info));

                    memset(params2, 0, sizeof(params2));
                    for(i=0; i<CLBLOCK(ovalue1)->mNumParams; i++) {
                        params2[i] = CLBLOCK(ovalue1)->mParams[i];  // block param types
                    }

                    type1 = CLBLOCK(ovalue1)->mResultType;          // block result type

                    ivalue3 = CLBLOCK(ovalue1)->mNumParams;         // block num params
                }
                else {
                    memset(params2, 0, sizeof(params2));   // block param type
                    type1 = 0;              // block result type
                    ivalue3 = 0;            // block num params
                }

                push_object(type2, info);

                /// searching for method ///
                method = get_virtual_method_with_params(type2, CONS_str(constant, ivalue1), params, ivalue2, &klass2, ivalue7, ivalue11, ivalue3, params2, type1, info);

                pop_object(info);


                if(method == NULL) {
                    BOOL method_missing_found;

                    vm_mutex_unlock();

                    method_missing_found = FALSE;
                    if(!call_method_missing_method(klass2, &method_missing_found, info, vm_type, CONS_str(constant, ivalue1), ivalue7, ivalue2, ivalue8, ivalue5)) 
                    {
                        return FALSE;
                    }

                    if(method_missing_found) {
VMLOG(info, "method_missing_found\n");
//SHOW_STACK(info, NULL, NULL);
                        break;
                    }
                    else {
                        int j;
                        char buf[512];

                        *buf = 0;

                        for(j=0; j<ivalue2; j++) {
                            write_type_name_to_buffer(buf + strlen(buf), 512-strlen(buf), params[j]);
                            if(j != ivalue2-1) xstrncat(buf, ",", 512);
                        }

                        vm_mutex_lock();
                        entry_exception_object_with_class_name(info, "MethodMissingException", "can't get a method named %s.%s(%s)", REAL_CLASS_NAME(CLTYPEOBJECT(type2)->mClass), CONS_str(constant, ivalue1), buf);
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }
                else {
                    vm_mutex_unlock();
                }

                /// call parametor initializer ///
                vm_mutex_lock();
                if(ivalue2 < method->mNumParams) {
                    for(j=ivalue2; j<method->mNumParams; j++) {
                        sCLParamInitializer* initializer = method->mParamInitializers + j;

                        if(initializer->mInitializer.mCode) {
                            if(!param_initializer(&klass2->mConstPool, &initializer->mInitializer, initializer->mLVNum, initializer->mMaxStack, info, vm_type))
                            {
                                vm_mutex_unlock();
                                return FALSE;
                            }
                        }
                    }

                    if(ivalue8) {                       // a block exists
                        int num_param_initializer;
                        CLObject block;
                        
                        num_param_initializer = method->mNumParams - ivalue2;
                        block = (info->stack_ptr-num_param_initializer-1)->mObjectValue.mValue;
                        for(j=0; j<num_param_initializer; j++) {
                            (info->stack_ptr-num_param_initializer-1+j)->mObjectValue.mValue = (info->stack_ptr-num_param_initializer-1+j+1)->mObjectValue.mValue;
                        }

                        (info->stack_ptr-1)->mObjectValue.mValue = block;
                    }
                }
                vm_mutex_unlock();

VMLOG(info, "klass2 %s\n", REAL_CLASS_NAME(klass2));
VMLOG(info, "method name (%s)\n", METHOD_NAME2(klass2, method));
//SHOW_STACK(info, top_of_stack, var);
//SHOW_HEAP(info);

                /// do call method ///

                if(!excute_method(method, klass2, klass3, &klass2->mConstPool, ivalue5, info, type2)) 
                {
                    return FALSE;
                }
VMLOG(info, "OP_INVOKE_VIRTUAL_METHOD end\n");
VMLOG(info, "klass2 %s\n", REAL_CLASS_NAME(klass2));
VMLOG(info, "method name (%s)\n", METHOD_NAME2(klass2, method));

                }
                break;

            case OP_INVOKE_VIRTUAL_CLONE_METHOD:
VMLOG(info, "OP_INVOKE_VIRTUAL_CLONE_METHOD end\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;      // star
                pc++;

                ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

                type1 = CLOBJECT_HEADER(ovalue1)->mType;

                ASSERT(type1 != 0);

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                push_object(type2, info);

                klass1 = CLTYPEOBJECT(type2)->mClass;

                if(((klass1->mFlags & CLASS_FLAGS_STRUCT) && !ivalue1) || (!(klass1->mFlags & CLASS_FLAGS_STRUCT) && ivalue1)) 
                {
                    info->stack_ptr->mObjectValue.mValue = ovalue1;
                    info->stack_ptr++;

                    if(!call_clone_method(klass1, info, type2)) {
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    *(info->stack_ptr-3) = *(info->stack_ptr-1);
                    info->stack_ptr-=2;
                }
                else {
                    pop_object(info);
                }
                vm_mutex_unlock();
                break;

            case OP_CALL_PARAM_INITIALIZER: {
VMLOG(info, "OP_CALL_PARAM_INITIALIZER\n");
                sByteCode code2;

                int* code_buf;
                int code_len;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get the class named %s", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue3 = *pc;              // code len
                pc++;

                ivalue4 = *pc;              // code offset
                pc++;

                ivalue5 = *pc;              // max_stack
                pc++;

                ivalue6 = *pc;              // lv_num
                pc++;

                /// code ///
                code_buf = (int*)CONS_str(constant, ivalue4);
                code_len = ivalue3;

                sByteCode_init(&code2);
                append_buf_to_bytecodes(&code2, code_buf, code_len, FALSE);

                if(!param_initializer(&klass1->mConstPool, &code2, ivalue6, ivalue5, info, vm_type))
                {
                    sByteCode_free(&code2);
                    vm_mutex_unlock();
                    return FALSE;
                }
                sByteCode_free(&code2);
                vm_mutex_unlock();
                }
                break;

            case OP_FOLD_PARAMS_TO_ARRAY: // for variable argument
VMLOG(info, "OP_FOLD_PARAMS_TO_ARRAY");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;          // num variable argument
                pc++;

                ivalue2 = *pc;          // existance of block object
                pc++;

                for(i=0; i<ivalue1; i++) {
                    params[i] = (info->stack_ptr - ivalue2 - ivalue1 + i)->mObjectValue.mValue;
                }

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);  // Array<anonymous>
                push_object(type1, info);

                if(type1 == 0) {
                    pop_object(info);
                    entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a type data");
                    vm_mutex_unlock();
                    return FALSE;
                }

                ovalue1 = create_array_object2(type1, params, ivalue1, info);

                if(ivalue2) {
                    ovalue2 = (info->stack_ptr - 2)->mObjectValue.mValue;  // block object
                    pop_object_n(info, ivalue1 + 1 + 1);
                    push_object(ovalue1, info);
                    push_object(ovalue2, info);
                }
                else {
                    pop_object_n(info, ivalue1 + 1);
                    push_object(ovalue1, info);
                }

//puts("fold");
//show_stack(info);

                vm_mutex_unlock();
                break;

            case OP_IF: {
VMLOG(info, "OP_IF\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// get result of conditional ///
                ivalue1 = CLBOOL(ovalue1)->mValue;
                info->stack_ptr--;

                ivalue2 = *pc;
                pc++;

                if(ivalue1) {
                    pc += ivalue2;
                }
                vm_mutex_unlock();
                }
                break;

            case OP_NOTIF: {
VMLOG(info, "OP_NOTIF\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// get result of conditional ///
                ivalue1 = CLBOOL(ovalue1)->mValue;
                info->stack_ptr--;

                ivalue2 = *pc;
                pc++;

                if(!ivalue1) {
                    pc += ivalue2;
                }

                vm_mutex_unlock();
                }
                break;

            case OP_GOTO:
VMLOG(info, "OP_GOTO\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;
                pc++;

                pc = code->mCode + ivalue1;

                vm_mutex_unlock();
                break;

            case OP_RETURN:
VMLOG(info, "OP_RETURN\n");
                vm_mutex_lock();
                pc++;
VMLOG(info, "KKK");
VMLOG(info, "super %d\n", CLTYPEOBJECT(gIntTypeObject)->mClass->mNumSuperClasses);

                vm_mutex_unlock();
                return TRUE;

            case OP_THROW:
VMLOG(info, "OP_THROW\n");
                vm_mutex_lock();
                pc++;

                vm_mutex_unlock();
                return FALSE;

            case OP_TRY:
VMLOG(info, "OP_TRY\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                  // catch block number
                pc++;

                ivalue2 = *pc;                  // finally block existance
                pc++;

                ivalue3 = *pc;                  // finally block result existance
                pc++;

                mvalue1 = info->stack_ptr - ivalue2 -ivalue1*2 -1;  // try object pointer

                /// get try block object ///
                ovalue1 = mvalue1->mObjectValue.mValue;

                /// get catch block object and the type ///
                for(j=0; j<ivalue1; j++) {
                    catch_blocks[j] = (mvalue1 + 1 + j*2)->mObjectValue.mValue;
                    catch_block_type[j] = (mvalue1 + 1 +j*2 + 1)->mObjectValue.mValue;
                }

                /// get finally block object ///
                if(ivalue2) { 
                    ovalue3 = (info->stack_ptr -1)->mObjectValue.mValue;
                }
                else {
                    ovalue3 = 0;
                }

                /// try block ///
VMLOG(info, "TRY BLOCK START\n");
                result = excute_block(ovalue1, FALSE, info, vm_type);
VMLOG(info, "TRY BLOCK FINISHED. result is %d\n", result);
                if(result) {
                    if(ovalue3 && check_type(ovalue3, gBlockTypeObject, info)) {
VMLOG(info, "FINALLY BLOCK START\n");
                        /// finally ///
                        if(!excute_block(ovalue3, ivalue3, info, vm_type)) {
                            ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                            info->stack_ptr = mvalue1;

                            info->stack_ptr->mObjectValue.mValue = ovalue1;
                            info->stack_ptr++;
                            vm_mutex_unlock();
                            return FALSE;
                        }

                        if(ivalue3) {
                            ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                            info->stack_ptr = mvalue1;

                            info->stack_ptr->mObjectValue.mValue = ovalue1;
                            info->stack_ptr++;
                            goto OP_TRY_END;
                        }
                    }
                }
                else {
                    ovalue2 = (info->stack_ptr - 1)->mObjectValue.mValue; // exception object

/*
                    /// restore stack pointer ///
                    info->stack_ptr = top_of_stack-1;
                    info->stack_ptr->mObjectValue.mValue = ovalue2;
                    info->stack_ptr++;
*/
                    /// catch ///
                    for(j=0; j<ivalue1; j++) {
                        if(check_type(ovalue2, catch_block_type[j], info)) {
VMLOG(info, "CATCH BLOCK STARTS %d\n", j);
                            result = excute_block(catch_blocks[j], FALSE, info, vm_type);
VMLOG(info, "CATCH BLOCK ENDS . result is %d\n", result);

                            if(result) {
                                /// finally ///
                                if(ovalue3 && check_type(ovalue3, gBlockTypeObject, info)) 
                                {
                                    /// finally ///
VMLOG(info, "FINALLY BLOCK STARTS.\n");
                                    if(!excute_block(ovalue3, ivalue3, info, vm_type)) {
                                        ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                        info->stack_ptr = mvalue1;

                                        info->stack_ptr->mObjectValue.mValue = ovalue1;
                                        info->stack_ptr++;
                                        vm_mutex_unlock();
                                        return FALSE;
                                    }
VMLOG(info, "FINALLY BLOCK FINISHED.\n");
                                }

                                if(ivalue3) {
                                    ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                    info->stack_ptr = mvalue1;

                                    info->stack_ptr->mObjectValue.mValue = ovalue1;
                                    info->stack_ptr++;
                                    goto OP_TRY_END;
                                }
                            }
                            else {
                                ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                info->stack_ptr = mvalue1;

                                info->stack_ptr->mObjectValue.mValue = ovalue1;
                                info->stack_ptr++;
                                vm_mutex_unlock();
                                return FALSE;
                            }
                            break;
                        }
                    }

                    /// can't catch the exception ///
                    if(j == ivalue1) {
                        info->stack_ptr = mvalue1;
                        entry_exception_object_with_class_name(info, "Exception", "can't catch the exception");
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }

                info->stack_ptr = mvalue1;

OP_TRY_END:
                vm_mutex_unlock();
                break;

            case OP_INVOKE_BLOCK:
VMLOG(info, "OP_INVOKE_BLOCK\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // block index
                pc++;

                ivalue2 = *pc;         // result existance
                pc++;

                ovalue1 = var[ivalue1].mObjectValue.mValue;

                if(!excute_block(ovalue1, ivalue2, info, vm_type)) 
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
                vm_mutex_unlock();
                break;

            case OP_BREAK_IN_METHOD_BLOCK:
VMLOG(info, "OP_BREAK_IN_METHOD_BLOCK");
                vm_mutex_lock();
                pc++;

                info->existance_of_break = TRUE;

                vm_mutex_unlock();
                return TRUE;

            case OP_OUTPUT_RESULT: 
                vm_mutex_lock();
                pc++;
                if(!output_result_for_interpreter(info)) {
                    vm_mutex_unlock();
                    return FALSE;
                }
                vm_mutex_unlock();
                break;

            case OP_POP:
VMLOG(info, "OP_POP\n");
                vm_mutex_lock();
                info->stack_ptr--;
                pc++;
                vm_mutex_unlock();
                break;

            case OP_POP_N:
VMLOG(info, "OP_POP_N %d\n", ivalue1);
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;
                pc++;

                info->stack_ptr -= ivalue1;

                vm_mutex_unlock();
                break;

            case OP_POP_N_WITHOUT_TOP:
VMLOG(info, "OP_POP_N_WITHOUT_TOP %d\n", ivalue1);
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;
                pc++;
                
                ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

                info->stack_ptr -= ivalue1;

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_DUP:
VMLOG(info, "OP_POP\n");
                vm_mutex_lock();
                pc++;

                *info->stack_ptr = *(info->stack_ptr-1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SWAP:
VMLOG(info, "OP_SWAP\n");
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
                (info->stack_ptr-2)->mObjectValue.mValue = (info->stack_ptr-1)->mObjectValue.mValue;
                (info->stack_ptr-1)->mObjectValue.mValue = ovalue1;

                vm_mutex_unlock();
                break;

            case OP_INC_VALUE:
VMLOG(info, "OP_INC_VALUE\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;
                pc++;

                ivalue2 = CLINT(ovalue1)->mValue + ivalue1;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue2, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_DEC_VALUE:
VMLOG(info, "OP_DEC_VALUE\n");
                vm_mutex_lock();

                pc++;

                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;
                pc++;

                ivalue2 = CLINT(ovalue1)->mValue - ivalue1;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_int_object_with_type(ivalue2, CLOBJECT_HEADER(ovalue1)->mType);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            default:
                cl_print(info, "invalid op code(%d). unexpected error at cl_vm\n", *pc);
                exit(1);
        }
//SHOW_STACK(info, top_of_stack, var);
//SHOW_HEAP(info);
    }

VMLOG(info, "VM finished pc - code->mCode --> %d code->mLen %d\n", pc - code->mCode, code->mLen);

    return TRUE;
}

/// result (TRUE): success (FALSE): threw exception
BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack, int stack_size)
{
    MVALUE* lvar;
    BOOL result;
    sVMInfo info;

    vm_mutex_lock();

    memset(&info, 0, sizeof(info));

    info.stack = CALLOC(1, sizeof(MVALUE)*stack_size);
    info.stack_size = stack_size;
    info.stack_ptr = info.stack;
    lvar = info.stack;
    info.stack_ptr += lv_num;

START_VMLOG(&info);
VMLOG(&info, "cl_main lv_num %d max_stack %d\n", lv_num, max_stack);

    push_vminfo(&info);

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object_with_class_name(&info, "OverflowStackSizeException", "overflow stack size");
        output_exception_message_with_info(&info);
        FREE(info.stack);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

VMLOG(&info, "cl_main lv_num2 %d\n", lv_num);

    vm_mutex_unlock();
    result = cl_vm(code, constant, lvar, &info, 0);
    vm_mutex_lock();

    if(result == FALSE) {
        output_exception_message_with_info(&info); // show exception message
    }
    else {
#ifdef VM_DEBUG
        if(info.stack_ptr != info.stack + lv_num) {
            fprintf(stderr, "invalid stack ptr. An error occurs on cl_main\n");
            exit(2);
        }
#endif
    }

    FREE(info.stack);

    pop_vminfo(&info);

    vm_mutex_unlock();

    return result;
}

// called by create_user_object
BOOL field_initializer(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack, CLObject vm_type)
{
    MVALUE* lvar;
    BOOL vm_result;
    sVMInfo info;
    MVALUE mvalue[CL_FIELD_INITIALIZER_STACK_SIZE];

    vm_mutex_lock();

    memset(&info, 0, sizeof(info));

    info.stack = mvalue;
    memset(&mvalue, 0, sizeof(MVALUE)*CL_FIELD_INITIALIZER_STACK_SIZE);
    info.stack_size = CL_FIELD_INITIALIZER_STACK_SIZE;
    info.stack_ptr = info.stack;

START_VMLOG(&info);
VMLOG(&info, "field_initializer\n");

    push_vminfo(&info);

    lvar = info.stack_ptr;
    info.stack_ptr += lv_num;

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object_with_class_name(&info, "OverflowStackSizeException", "overflow stack size");
        output_exception_message_with_info(&info);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

/*
    info.vm_types[info.num_vm_types++] = vm_type;

    if(info.num_vm_types >= CL_VM_TYPES_MAX) {
        entry_exception_object_with_class_name(&info, "Exception", "overflow method calling nest");
        output_exception_message_with_info(&info);
        pop_vminfo(&info);
        vm_mutex_unlock();
        info.num_vm_types--;
        return FALSE;
    }
*/

    if(code->mLen > 0) {
        vm_result = cl_vm(code, constant, lvar, &info, vm_type);
        *result = *(info.stack_ptr-1);
    }
    else {
        vm_result = TRUE;
        result->mObjectValue.mValue = create_null_object();
    }

#ifdef VM_DEBUG
    if(info.stack_ptr != lvar + lv_num && info.stack_ptr != lvar + lv_num + 1) {
        fprintf(stderr, "invalid stack ptr. An error occurs on field_initializer\n");
        fprintf(stderr, "stack pointer is %d\n", info.stack_ptr - lvar - lv_num -1);
        exit(2);
    }
#endif

    if(!vm_result) {
        output_exception_message_with_info(&info); // show exception message
    }

    info.stack_ptr = lvar;

    pop_vminfo(&info);

    vm_mutex_unlock();

//    info.num_vm_types--;
    return vm_result;
}

static BOOL param_initializer(sConst* constant, sByteCode* code, int lv_num, int max_stack, sVMInfo* info, CLObject vm_type)
{
    MVALUE* lvar;
    BOOL vm_result;
    sVMInfo new_info;
    MVALUE mvalue[CL_PARAM_INITIALIZER_STACK_SIZE];

    vm_mutex_lock();

    memset(&new_info, 0, sizeof(new_info));

    new_info.stack = mvalue;
    memset(&mvalue, 0, sizeof(MVALUE)*CL_PARAM_INITIALIZER_STACK_SIZE);
    new_info.stack_size = CL_PARAM_INITIALIZER_STACK_SIZE;
    new_info.stack_ptr = new_info.stack;

START_VMLOG(&new_info);
VMLOG(&new_info, "field_initializer\n");

    push_vminfo(&new_info);

    lvar = new_info.stack_ptr;
    new_info.stack_ptr += lv_num;

    if(new_info.stack_ptr + max_stack > new_info.stack + new_info.stack_size) {
        entry_exception_object_with_class_name(&new_info, "OverflowStackSizeException", "overflow stack size");
        output_exception_message_with_info(&new_info);
        pop_vminfo(&new_info);
        vm_mutex_unlock();
        return FALSE;
    }

/*
    new_info.vm_types[new_info.num_vm_types++] = vm_type;

    if(new_info.num_vm_types >= CL_VM_TYPES_MAX) {
        entry_exception_object_with_class_name(&new_info, "Exception", "overflow method calling nest");
        output_exception_message_with_info(&new_info);
        pop_vminfo(&new_info);
        vm_mutex_unlock();
        new_info.num_vm_types--;
        return FALSE;
    }
*/

    vm_result = cl_vm(code, constant, lvar, &new_info, vm_type);

    *(info->stack_ptr) = *(new_info.stack_ptr-1);
    info->stack_ptr++;

#ifdef VM_DEBUG
    if(new_info.stack_ptr != lvar + lv_num + 1) {
        fprintf(stderr, "invalid stack ptr. An error occurs on param_initializer\n");
        exit(2);
    }
#endif

    if(!vm_result) {
        output_exception_message_with_info(&new_info); // show exception message
    }

    //new_info.num_vm_types--;

    pop_vminfo(&new_info);

    vm_mutex_unlock();

    return vm_result;
}

// result_type 0: nothing 1: result exists
static BOOL excute_method(sCLMethod* method, sCLClass* klass, sCLClass* class_of_class_method_call, sConst* constant, int result_type, sVMInfo* info, CLObject vm_type)
{
    int real_param_num;
    BOOL result;
    BOOL native_result;
    BOOL external_result;
    int return_count;
    sCLClass* running_class_before;
    sCLMethod* running_method_before;

    vm_mutex_lock();

    running_class_before = info->mRunningClass;
    running_method_before = info->mRunningMethod;

    info->mRunningClass = klass;
    info->mRunningMethod = method;

    real_param_num = method->mNumParams + (method->mFlags & CL_CLASS_METHOD ? 0:1) + method->mNumBlockType;

    info->vm_types[info->num_vm_types++] = vm_type;

    if(info->num_vm_types >= CL_VM_TYPES_MAX) {
        entry_exception_object_with_class_name(info, "Exception", "overflow method calling nest");
        info->num_vm_types--;
        info->mRunningClass = running_class_before;
        info->mRunningMethod = running_method_before;
        vm_mutex_unlock();
        return FALSE;
    }

    if(method->mFlags & CL_ABSTRACT_METHOD) {
        entry_exception_object_with_class_name(info, "Exception", "This is abstract method.");
        info->num_vm_types--;
        info->mRunningClass = running_class_before;
        info->mRunningMethod = running_method_before;
        vm_mutex_unlock();
        return FALSE;
    }

    if(method->mFlags & CL_NATIVE_METHOD) {

VMLOG(info, "native method1\n");
//SHOW_STACK(info, NULL, NULL);
        MVALUE* lvar;
        BOOL synchronized;

        lvar = info->stack_ptr - real_param_num;

        if(info->stack_ptr + method->mMaxStack > info->stack + info->stack_size) {
            entry_exception_object_with_class_name(info, "OverflowStackSizeException", "overflow stack size");
            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        if(method->uCode.mNativeMethod == NULL) {
            entry_exception_object_with_class_name(info, "MethodMissingException", "can't get a native method named %s.%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return FALSE;
        }

        vm_mutex_unlock();
        if(!synchronized) vm_mutex_lock();
        native_result = method->uCode.mNativeMethod(&info->stack_ptr, lvar, info, vm_type, class_of_class_method_call);
        if(!synchronized) vm_mutex_unlock();
        vm_mutex_lock();

        if(native_result) {
            if(result_type == 1) {
                CLObject result_object;
                CLObject type_object;

                result_object = (info->stack_ptr-1)->mObjectValue.mValue;

                /// result type check ///
                type_object = create_type_object_from_cl_type(klass, &method->mResultType, info);
                push_object(type_object, info);

                if(CLTYPEOBJECT(type_object)->mClass == gVoidClass) {
                    type_object = gNullTypeObject;

                    if(!check_type_without_generics(result_object, type_object, info, TRUE)) 
                    {
                        pop_object_except_top(info);
                        info->num_vm_types--;
                        info->mRunningClass = running_class_before;
                        info->mRunningMethod = running_method_before;
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }
                else if(is_generics_param_class(CLTYPEOBJECT(type_object)->mClass)) 
                {
                    /// no check ///
                }
                else {
                    if(!check_type_without_generics(result_object, type_object, info, TRUE)) 
                    {
                        pop_object_except_top(info);
                        info->num_vm_types--;
                        info->mRunningClass = running_class_before;
                        info->mRunningMethod = running_method_before;
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }
                
                pop_object(info);

                info->stack_ptr = lvar;
                info->stack_ptr->mObjectValue.mValue = result_object;
                info->stack_ptr++;
            }
            else {
                info->stack_ptr = lvar;
            }

            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return TRUE;
        }
        else {
            MVALUE* mvalue;

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;

            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return FALSE;
        }
    }
    else {
        MVALUE* lvar;
        BOOL synchronized;
        MVALUE* stack_top;

        lvar = info->stack_ptr - real_param_num;
        if(method->mNumLocals - real_param_num > 0) {
            info->stack_ptr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
        }

VMLOG(info, "method->mNumLocals - real_param_num %d\n", (method->mNumLocals - real_param_num));

        if(info->stack_ptr + method->mMaxStack > info->stack + info->stack_size) {
            entry_exception_object_with_class_name(info, "OverflowStackSizeException", "overflow stack size");
            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        stack_top = info->stack_ptr;
        vm_mutex_unlock();

        if(!synchronized) vm_mutex_lock();
        result = cl_vm(&method->uCode.mByteCodes, constant, lvar, info, vm_type);
        if(!synchronized) vm_mutex_unlock();
        vm_mutex_lock();

        if(!result) {
            MVALUE* mvalue;

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;

            info->num_vm_types--;
            info->mRunningClass = running_class_before;
            info->mRunningMethod = running_method_before;
            vm_mutex_unlock();
            return FALSE;
        }
        else if(result_type == 1) 
        {
            MVALUE* mvalue;

#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top + 1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method2\n");
                exit(2);
            }
#endif

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;
        }
/*
        else if(result_type == 2) {  // dynamic type
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top && info->stack_ptr != stack_top + 1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method3\n");
                fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top -1);
                exit(2);
            }
#endif

            if(strcmp(CONS_str(constant, method->mResultType.mClassNameOffset), "void") == 0)
            {
                info->stack_ptr = lvar;
                (*info->stack_ptr).mObjectValue.mValue = 0;               // uninitialized object
                info->stack_ptr++;
            }
            else {
                MVALUE* mvalue;

                mvalue = info->stack_ptr-1;
                info->stack_ptr = lvar;
                *info->stack_ptr = *mvalue;
                info->stack_ptr++;
            }
        }
*/
        else {
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method4\n");
                fprintf(stderr, "info->stack_ptr - stack_top is %d\n", info->stack_ptr - stack_top);
                exit(2);
            }
#endif
            info->stack_ptr = lvar;
        }

        info->num_vm_types--;
        info->mRunningClass = running_class_before;
        info->mRunningMethod = running_method_before;
        vm_mutex_unlock();
        return result;
    }
}

static BOOL excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type)
{
    int real_param_num;
    MVALUE* lvar;
    BOOL result;
    MVALUE* stack_top;

    vm_mutex_lock();

/*
    info->vm_types[info->num_vm_types++] = vm_type;

    if(info->num_vm_types >= CL_VM_TYPES_MAX) {
        entry_exception_object_with_class_name(info, "Exception", "overflow method calling nest");
        info->num_vm_types--;
        return FALSE;
    }
*/

    real_param_num = CLBLOCK(block)->mNumParams;

    lvar = info->stack_ptr - real_param_num;

    /// copy caller local vars to current local vars ///
    memmove(lvar + CLBLOCK(block)->mNumParentVar, lvar, sizeof(MVALUE)*real_param_num);
    memmove(lvar, CLBLOCK(block)->mParentLocalVar, sizeof(MVALUE)*CLBLOCK(block)->mNumParentVar);

    info->stack_ptr += CLBLOCK(block)->mNumParentVar;

    if(CLBLOCK(block)->mNumLocals - real_param_num > 0) {
        info->stack_ptr += (CLBLOCK(block)->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
    }

    if(info->stack_ptr + CLBLOCK(block)->mMaxStack > info->stack + info->stack_size) {
        info->stack_ptr = lvar;
        entry_exception_object_with_class_name(info, "OverflowStackSizeException", "overflow stack size");
        //info->num_vm_types--;
        vm_mutex_unlock();
        return FALSE;
    }

    info->existance_of_break = FALSE;

    stack_top = info->stack_ptr;
    vm_mutex_unlock();
    result = cl_vm(CLBLOCK(block)->mCode, CLBLOCK(block)->mConstant, lvar, info, vm_type);
VMLOG(info, "returned from VM at excute_block");
    vm_mutex_lock();

    /// restore caller local vars, and restore base of all block stack  ///
    memmove(CLBLOCK(block)->mParentLocalVar, lvar, sizeof(MVALUE)*CLBLOCK(block)->mNumParentVar);

    /// exception ///
    if(!result) {
        MVALUE* mvalue;

        mvalue = info->stack_ptr-1;
        info->stack_ptr = lvar;
        *info->stack_ptr = *mvalue;
        info->stack_ptr++;
    }
    else if(result_existance) {
        MVALUE* mvalue;

        /// block result type is Tuple<bool existance_of_break, RESULTTYPE>
        if(CLBLOCK(block)->mBreakable == 1) {
            CLObject tuple_object;
            CLObject tuple_type;
            CLObject object;
            CLObject bool_object;

#ifdef VM_DEBUG
        if(info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
            fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top -1);
            exit(2);
        }
#endif

            object = (info->stack_ptr-1)->mObjectValue.mValue;

            tuple_type = CLBLOCK(block)->mResultType;

            if(!create_user_object(tuple_type, &tuple_object, vm_type, NULL, 0, info))
            {
                mvalue = info->stack_ptr-1;
                info->stack_ptr = lvar;
                *info->stack_ptr = *mvalue;
                info->stack_ptr++;
                vm_mutex_unlock();
                return FALSE;
            }

            push_object(tuple_object, info);

            bool_object = create_bool_object(info->existance_of_break);

            CLUSEROBJECT(tuple_object)->mFields[0].mObjectValue.mValue = bool_object;
            CLUSEROBJECT(tuple_object)->mFields[1].mObjectValue.mValue = object;

            pop_object(info);

            info->stack_ptr = lvar;
            info->stack_ptr->mObjectValue.mValue = tuple_object;
            info->stack_ptr++;
        }
        /// block result type is bool.
        else if(CLBLOCK(block)->mBreakable == 2) {
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
                fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top);
                exit(2);
            }
#endif
            info->stack_ptr = lvar;
            info->stack_ptr->mObjectValue.mValue = create_bool_object(info->existance_of_break);
            info->stack_ptr++;
        }
        /// block result type is dynamic typing 
        else if(CLBLOCK(block)->mBreakable == 3)
        {
            CLObject block_result_type;
            
            block_result_type = create_type_object_from_cl_type(info->mRunningClass, &info->mRunningMethod->mBlockType.mResultType, info);

            /// block result type is bool.
            if(CLTYPEOBJECT(block_result_type)->mClass == gBoolClass && CLTYPEOBJECT(block_result_type)->mGenericsTypesNum == 0)
            {
#ifdef VM_DEBUG
                if(info->stack_ptr != stack_top) {
                    fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
                    fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top);
                    exit(2);
                }
#endif
                info->stack_ptr = lvar;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(info->existance_of_break);
                info->stack_ptr++;
            }
            /// block result type is Tuple<bool existance_of_break, RESULTTYPE>
            else {
                CLObject tuple_object;
                CLObject tuple_type;
                CLObject object;
                CLObject ovalue;
#ifdef VM_DEBUG
                if(info->stack_ptr != stack_top+1) {
                    fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
                    fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top-1);
                    exit(2);
                }
#endif
                
                object = (info->stack_ptr-1)->mObjectValue.mValue;

                tuple_type = CLBLOCK(block)->mResultType;

                if(!create_user_object(tuple_type, &tuple_object, vm_type, NULL, 0, info))
                {
                    mvalue = info->stack_ptr-1;
                    info->stack_ptr = lvar;
                    *info->stack_ptr = *mvalue;
                    info->stack_ptr++;
                    vm_mutex_unlock();
                    return FALSE;
                }

                ovalue = create_bool_object(info->existance_of_break);
                CLUSEROBJECT(tuple_object)->mFields[0].mObjectValue.mValue = ovalue;
                CLUSEROBJECT(tuple_object)->mFields[1].mObjectValue.mValue = object;
                info->stack_ptr = lvar;
                info->stack_ptr->mObjectValue.mValue = tuple_object;
                info->stack_ptr++;
            }
        }
        /// CLBLOCK(block)->mBreakable == 0
        else {
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top+1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
                fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top-1);
                exit(2);
            }
#endif
            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;
        }
    }
    else {

#ifdef VM_DEBUG
        if(info->stack_ptr != stack_top) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block3\n");
            fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top);
            exit(2);
        }
#endif
        info->stack_ptr = lvar;
    }

    //info->num_vm_types--;

    vm_mutex_unlock();
    return result;
}

BOOL cl_excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type)
{
    return excute_block(block, result_existance, info, vm_type);
}

static BOOL excute_block_with_new_stack(MVALUE* result, CLObject block, BOOL result_existance, sVMInfo* new_info, CLObject vm_type)
{
    BOOL vm_result;
    MVALUE* lvar;
    sByteCode* code;
    sConst* constant;
    MVALUE* stack_top;

    new_info->vm_types[new_info->num_vm_types++] = vm_type;

    if(new_info->num_vm_types >= CL_VM_TYPES_MAX) {
        entry_exception_object_with_class_name(new_info, "Exception", "overflow method calling nest");
        new_info->num_vm_types--;
        vm_mutex_unlock();
        return FALSE;
    }

    lvar = new_info->stack;

    if(new_info->stack_ptr + CLBLOCK(block)->mMaxStack > new_info->stack + new_info->stack_size) {
        entry_exception_object_with_class_name(new_info, "OverflowStackSizeException", "overflow stack size");
        output_exception_message_with_info(new_info); // show exception message
        FREE(new_info->stack);
        pop_vminfo(new_info);
        new_info->num_vm_types--;
        vm_mutex_unlock();
        return FALSE;
    }

    code = CLBLOCK(block)->mCode;
    constant = CLBLOCK(block)->mConstant;

START_VMLOG(new_info);

    stack_top = new_info->stack_ptr;

    vm_mutex_unlock();
    vm_result = cl_vm(code, constant, lvar, new_info, vm_type);
    vm_mutex_lock();

    if(!vm_result) {
/*
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif
*/

        *result = *(new_info->stack_ptr-1);
        output_exception_message_with_info(new_info); // show exception message
    }
    else if(result_existance) {
/*
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif
*/
        *result = *(new_info->stack_ptr-1);
    }
    else {
/*
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif
*/
    }

    FREE(new_info->stack);

    pop_vminfo(new_info);

    new_info->num_vm_types--;
 
    vm_mutex_unlock();

    return vm_result;
}

BOOL cl_excute_block_with_new_stack(MVALUE* result, CLObject block, BOOL result_existance, sVMInfo* new_info, CLObject vm_type)
{
    return excute_block_with_new_stack(result, block, result_existance, new_info, vm_type);
}

BOOL cl_excute_method(sCLMethod* method, sCLClass* klass, sCLClass* class_of_class_method_call, NULLABLE sVMInfo* info, CLObject* result_value)
{
    int result_type;
    sVMInfo info2;
    CLObject vm_type;
    BOOL result;
    int i;
    int param_num;
    MVALUE* stack_ptr;

    memset(&info2, 0, sizeof(sVMInfo));

    info2.stack = CALLOC(1, sizeof(MVALUE)*CL_STACK_SIZE);
    info2.stack_size = CL_STACK_SIZE;
    info2.stack_ptr = info2.stack;
    info2.next_info = NULL;

    if(strcmp(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset), "void") == 0)
    {
        result_type = 0;
    }
    else {
        result_type = 1;
    }

    vm_type = 0;

    push_vminfo(&info2);

    /// copy parametors and a block object ///
    param_num = method->mNumParams + method->mNumBlockType + (method->mFlags & CL_CLASS_METHOD ? 0: 1);

    ASSERT(param_num > 0 && info != NULL || param_num == 0);

    if(info) {
        stack_ptr = info->stack_ptr - param_num;

        for(i=0; i<param_num; i++) {
            *info2.stack_ptr++ = *stack_ptr++;
        }
    }

START_VMLOG(&info2);
VMLOG(&info2, "cl_excute_method(%s.%s)\n", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));

    result = excute_method(method, klass, class_of_class_method_call, &klass->mConstPool, result_type, &info2, vm_type);

    if(!result) 
    {
        if(info) {
            info2.mRunningClass = info->mRunningClass;
            info2.mRunningMethod = info->mRunningMethod;
            *info->stack_ptr = *(info2.stack_ptr-1);  // exception object
            info->stack_ptr++;
        }

        *result_value = (info2.stack_ptr-1)->mObjectValue.mValue;  // exception object
    }
    else {
        if(result_type == 0) {
            *result_value = 0;
        }
        else {
            *result_value = (info2.stack_ptr-1)->mObjectValue.mValue;
        }
    }

    pop_vminfo(&info2);

    FREE(info2.stack);

    return result;
}

