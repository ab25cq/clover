#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>

BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    MVALUE* status_code;

    vm_mutex_lock();

    status_code = lvar;

    if(status_code->mIntValue <= 0) {
        entry_exception_object(info, gExRangeType.mClass, "status_code is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }
    else if(status_code->mIntValue >= 0xff) {
        /// exception ///
        entry_exception_object(info, gExRangeType.mClass, "status_code is greater than 255");
        vm_mutex_unlock();
        return FALSE;
    }

    cl_final();
    exit((char)status_code->mIntValue);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_getenv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
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
    env = lvar->mObjectValue;

    /// go ///
    env_wstr = CLSTRING(env)->mChars;

    size = (CLSTRING(env)->mLen + 1) * MB_LEN_MAX;
    env_str = MALLOC(size);

    if((int)wcstombs(env_str, env_wstr, size) < 0) {
        FREE(env_str);
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "wcstombs");
        vm_mutex_unlock();
        return FALSE;
    }

    str = getenv(env_str);

    FREE(env_str);

    wcs_len = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wcs_len);

    if((int)mbstowcs(wstr, str, wcs_len) < 0) {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "mbstowcs");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue  = create_string_object(gStringType.mClass, wstr, wcs_len);
    (*stack_ptr)++;

    FREE(wstr);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_sleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    MVALUE* time;
    unsigned int result;

    vm_mutex_lock();

    time = lvar;

    if(time->mIntValue <= 0) {
        entry_exception_object(info, gExRangeType.mClass, "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    result = sleep(time->mIntValue);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mIntValue = (int)result;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
