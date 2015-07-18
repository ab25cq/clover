#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject status_code;
    char status_code_value;

    vm_mutex_lock();

    status_code = lvar->mObjectValue.mValue;

    if(!check_type(status_code, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(status_code)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "status_code is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }
    else if(CLINT(status_code)->mValue >= 0xff) {
        /// exception ///
        entry_exception_object_with_class_name(info, "RangeException", "status_code is greater than 255");
        vm_mutex_unlock();
        return FALSE;
    }

    status_code_value = CLINT(status_code)->mValue;

    cl_final();
    exit(status_code_value);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_getenv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject env;
    wchar_t* env_wstr;
    char* env_str;
    int size;
    char* str;
    wchar_t* wstr;
    int wcs_len;
    CLObject object;

    vm_mutex_lock();

    /// params ///
    env = lvar->mObjectValue.mValue;

    if(!check_type(env, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    /// go ///
    env_wstr = CLSTRING_DATA(env)->mChars;

    size = (CLSTRING(env)->mLen + 1) * MB_LEN_MAX;
    env_str = MALLOC(size);

    if((int)wcstombs(env_str, env_wstr, size) < 0) {
        FREE(env_str);
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error wcstombs on converting string");
        vm_mutex_unlock();
        return FALSE;
    }

    str = getenv(env_str);

    FREE(env_str);

    wcs_len = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wcs_len);

    if((int)mbstowcs(wstr, str, wcs_len) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wcs_len, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_sleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    result = sleep(CLINT(time)->mValue);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_msleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;
    struct timespec req;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    req.tv_sec = 0;
    req.tv_nsec = 1000000 * CLINT(time)->mValue;     // 1ms

    result = nanosleep(&req, NULL);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_nanosleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;
    struct timespec req;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    req.tv_sec = 0;
    req.tv_nsec = 1 * CLINT(time)->mValue;     // 1nano

    result = nanosleep(&req, NULL);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_srand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject seed;

    vm_mutex_lock();

    seed = lvar->mObjectValue.mValue;

    if(!check_type(seed, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    srand(CLINT(seed)->mValue);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_rand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result;

    vm_mutex_lock();

    result = rand();

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_time(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    time_t result;

    vm_mutex_lock();

    result = time(NULL);

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_execv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject command;
    CLObject params;
    char* command_name;
    char** param_names;
    int i;
    int len;

    command = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(command, "String", info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "String", info)) {
        return FALSE;
    }

    /// make parametors ///
    if(!create_buffer_from_string_object(command, ALLOC &command_name, info)) {
        return FALSE;
    }

    len = CLARRAY(params)->mLen;
    param_names = CALLOC(sizeof(char*)*(len+2), 1);

    param_names[0] = command_name;

    for(i=0; i<len; i++) {
        CLObject str;

        str = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;

        if(!create_buffer_from_string_object(str, ALLOC &param_names[i+1], info)) {
            int j;
            FREE(command_name);
            for(j=0; j<i; j++) {
                FREE(param_names[j]);
            }
            FREE(param_names);
            return FALSE;
        }
    }

    param_names[i+1] = NULL;

    execv(command_name, param_names);

    FREE(command_name);

    for(i=0; i<len; i++) {
        FREE(param_names[i]);
    }
    FREE(param_names);

    return TRUE;
}

BOOL System_execvp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject command;
    CLObject params;
    char* command_name;
    char** param_names;
    int i;
    int len;

    command = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(command, "String", info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "String", info)) {
        return FALSE;
    }

    /// make parametors ///
    if(!create_buffer_from_string_object(command, ALLOC &command_name, info)) {
        return FALSE;
    }

    len = CLARRAY(params)->mLen;
    param_names = CALLOC(sizeof(char*)*(len+2), 1);

    param_names[0] = command_name;

    for(i=0; i<len; i++) {
        CLObject str;

        str = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;

        if(!create_buffer_from_string_object(str, ALLOC &param_names[i+1], info)) {
            int j;
            FREE(command_name);
            for(j=0; j<i; j++) {
                FREE(param_names[j]);
            }
            FREE(param_names);
            return FALSE;
        }
    }

    param_names[i+1] = NULL;

    execvp(command_name, param_names);

    FREE(command_name);

    for(i=0; i<len; i++) {
        FREE(param_names[i]);
    }
    FREE(param_names);

    return TRUE;
}

BOOL System_fork(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int pid;
    CLObject block;

    block = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(block, "Block", info)) {
        return FALSE;
    }

    /// child process ///
    if((pid = fork()) == 0) {
        vm_mutex_unlock();
        new_vm_mutex();         // avoid to dead lock
        vm_mutex_lock();

        BOOL result_existance;

        if(!cl_excute_block(block, result_existance, info, vm_type)) 
        {
            vm_mutex_unlock();
            return FALSE;
        }

        vm_mutex_unlock();
        exit(0);
    }

    if(pid < 0) {
        entry_exception_object_with_class_name(info, "Exception", "fork(2) is failed");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(pid);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_wait(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    pid_t pid;
    int status;
    CLObject wait_status_object;
    CLObject status_object;
    CLObject type_object;
    CLObject tuple_object;
    CLObject pid_object;

    pid = wait(&status);

    if(!create_user_object_with_class_name("WaitStatus", &wait_status_object, vm_type, info)) 
    {
        return FALSE;
    }

    push_object(wait_status_object, info);

    status_object = create_int_object(status);

    CLUSEROBJECT(wait_status_object)->mFields[0].mObjectValue.mValue = status_object;

    if(!create_user_object_with_class_name("Tuple$2", &tuple_object, vm_type, info)) 
    {
        pop_object_except_top(info);
        return FALSE;
    }

    push_object(tuple_object, info);

    pid_object = create_int_object(pid);

    CLUSEROBJECT(tuple_object)->mFields[0].mObjectValue.mValue = pid_object;
    CLUSEROBJECT(tuple_object)->mFields[1].mObjectValue.mValue = wait_status_object;

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = tuple_object;
    (*stack_ptr)++;

    return TRUE;
}
