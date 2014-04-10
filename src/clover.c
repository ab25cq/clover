#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>

BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* string;
    int size;
    char* str;

    string = lvar;

    if(string->mIntValue == 0) {
        /// exception ///
cl_print("Null Pointer Exception on Clover.print()");
puts("throw exception");
return FALSE;
    }

    size = (CLSTRING(string->mObjectValue)->mLen + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    if((int)wcstombs(str, CLSTRING(string->mObjectValue)->mChars, size) < 0) {
        FREE(str);
puts("throw exception");
return FALSE;
    }

    cl_print("%s", str);

    FREE(str);

    return TRUE;
}

BOOL Clover_compile(MVALUE** stack_ptr, MVALUE* lvar)
{

    return TRUE;
}

BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar)
{
    cl_gc();

    return TRUE;
}

BOOL Clover_show_classes(MVALUE** stack_ptr, MVALUE* lvar)
{
    show_class_list();

    return TRUE;
}

BOOL Clover_output_to_s(MVALUE** stack_ptr, MVALUE* lvar)
{
    CLObject block;
    sBuf buf;
    BOOL result_existance_of_method;
    wchar_t* wstr;
    char* str;
    int len;
    int wcs_len;
    BOOL result_existance;

    result_existance = FALSE;

    block = lvar->mObjectValue;

    gCLPrintBuffer = &buf;              // allocate
    sBuf_init(gCLPrintBuffer);

    if(!cl_excute_block(block, NULL, result_existance, TRUE)) {
        FREE(gCLPrintBuffer->mBuf);
        return FALSE;
    }

    str = gCLPrintBuffer->mBuf;

    len = strlen(str) + 1;
    wstr = MALLOC(sizeof(wchar_t)*len);
    if((int)mbstowcs(wstr, str, len) < 0) {
puts("throw exception");
FREE(wstr);
FREE(gCLPrintBuffer->mBuf);
return FALSE;
    }
    wcs_len = wcslen(wstr);

    (*stack_ptr)->mObjectValue = create_string_object(gStringType.mClass, wstr, wcs_len);
    (*stack_ptr)++;

    FREE(wstr);
    FREE(gCLPrintBuffer->mBuf);

    gCLPrintBuffer = NULL;

    return TRUE;
}

BOOL Clover_sleep(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* time;
    unsigned int result;

    time = lvar;

    if(time->mIntValue <= 0) {
        /// exception ///
cl_print("time is lesser equals than 0");
puts("throw exception");
return FALSE;
    }

    result = sleep(time->mIntValue);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mIntValue = (int)result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Clover_exit(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* status_code;

    status_code = lvar;

    if(status_code->mIntValue <= 0) {
        /// exception ///
cl_print("status_code is lesser equals than 0");
puts("throw exception");
return FALSE;
    }
    else if(status_code->mIntValue >= 0xff) {
        /// exception ///
cl_print("status_code is greater than 255");
puts("throw exception");
return FALSE;
    }

    exit((char)status_code->mIntValue);

    return TRUE;
}

BOOL Clover_getenv(MVALUE** stack_ptr, MVALUE* lvar)
{
    CLObject env;
    wchar_t* env_wstr;
    char* env_str;
    int size;
    char* str;
    wchar_t* wstr;
    int wcs_len;
    CLObject object;

    /// params ///
    env = lvar->mObjectValue;

    /// go ///
    env_wstr = CLSTRING(env)->mChars;

    size = (CLSTRING(env)->mLen + 1) * MB_LEN_MAX;
    env_str = MALLOC(size);

    if((int)wcstombs(env_str, env_wstr, size) < 0) {
puts("throw exception");
        FREE(env_str);
        return FALSE;
    }

    str = getenv(env_str);

    FREE(env_str);

    wcs_len = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wcs_len);

    if((int)mbstowcs(wstr, str, wcs_len) < 0) {
puts("throw exception");
        FREE(wstr);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue  = create_string_object(gStringType.mClass, wstr, wcs_len);
    (*stack_ptr)++;

    FREE(wstr);

    return TRUE;
}
