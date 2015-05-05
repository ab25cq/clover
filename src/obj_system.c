#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <time.h>

BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject status_code;

    vm_mutex_lock();

    status_code = lvar->mObjectValue.mValue;

    if(!check_type(status_code, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(status_code)->mValue <= 0) {
        entry_exception_object(info, gExRangeClass, "status_code is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }
    else if(CLINT(status_code)->mValue >= 0xff) {
        /// exception ///
        entry_exception_object(info, gExRangeClass, "status_code is greater than 255");
        vm_mutex_unlock();
        return FALSE;
    }

    cl_final();
    exit((char)CLINT(status_code)->mValue);

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
        entry_exception_object(info, gExConvertingStringCodeClass, "error wcstombs on converting string");
        vm_mutex_unlock();
        return FALSE;
    }

    str = getenv(env_str);

    FREE(env_str);

    wcs_len = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wcs_len);

    if((int)mbstowcs(wstr, str, wcs_len) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
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
        entry_exception_object(info, gExRangeClass, "time is lesser equals than 0");
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
        entry_exception_object(info, gExRangeClass, "time is lesser equals than 0");
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
        entry_exception_object(info, gExRangeClass, "time is lesser equals than 0");
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

