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
        entry_exception_object(gExNullPointerType.mClass, "Null pointer exception");
        return FALSE;
    }

    size = (CLSTRING(string->mObjectValue)->mLen + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    if((int)wcstombs(str, CLSTRING(string->mObjectValue)->mChars, size) < 0) {
        FREE(str);
        entry_exception_object(gExceptionType.mClass, "wcstombs");
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

    if(!cl_excute_method_block(block, NULL, result_existance, TRUE)) {
        FREE(gCLPrintBuffer->mBuf);
        return FALSE;
    }

    str = gCLPrintBuffer->mBuf;

    len = strlen(str) + 1;
    wstr = MALLOC(sizeof(wchar_t)*len);
    if((int)mbstowcs(wstr, str, len) < 0) {
        FREE(wstr);
        FREE(gCLPrintBuffer->mBuf);

        entry_exception_object(gExceptionType.mClass, "mbstowcs");
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



