#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>

BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject string;
    int size;
    char* str;

    vm_mutex_lock();

    string = lvar->mObjectValue.mValue;

    if(!check_type(string, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    size = (CLSTRING(string)->mLen + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    if((int)wcstombs(str, CLSTRING_DATA(string)->mChars, size) < 0) {
        FREE(str);
        entry_exception_object(info, gExConvertingStringCodeClass, "error wcstombs on string");
        vm_mutex_unlock();
        return FALSE;
    }

    cl_print(info, "%s", str);

    FREE(str);

    vm_mutex_unlock();

    return TRUE;
}

BOOL Clover_showClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    show_class_list(info);

    return TRUE;
}

BOOL Clover_outputToString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject block;
    sBuf buf;
    BOOL result_existance_of_method;
    wchar_t* wstr;
    char* str;
    int len;
    int wcs_len;
    BOOL result_existance;
    sBuf* cl_print_buffer_before;

    vm_mutex_lock();

    result_existance = FALSE;

    block = lvar->mObjectValue.mValue;

    cl_print_buffer_before = info->print_buffer;
    info->print_buffer = &buf;              // allocate
    sBuf_init(info->print_buffer);

    if(!cl_excute_block(block, result_existance, TRUE, info, vm_type)) {
        FREE(info->print_buffer->mBuf);
        info->print_buffer = cl_print_buffer_before;
        vm_mutex_unlock();
        return FALSE;
    }

    str = info->print_buffer->mBuf;

    len = strlen(str) + 1;
    wstr = MALLOC(sizeof(wchar_t)*len);
    if((int)mbstowcs(wstr, str, len) < 0) {
        FREE(wstr);
        FREE(info->print_buffer->mBuf);

        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on output string");
        info->print_buffer = cl_print_buffer_before;
        vm_mutex_unlock();
        return FALSE;
    }
    wcs_len = wcslen(wstr);

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wcs_len, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);
    FREE(info->print_buffer->mBuf);

    info->print_buffer = cl_print_buffer_before;

    vm_mutex_unlock();

    return TRUE;
}

BOOL cl_call_runtime_method()
{
    sCLClass* clover;
    sCLMethod* method;
    CLObject result_value;
    int i;

    clover = cl_get_class("Clover");

    method = NULL;

    /// search for initiliaze method ///
    for(i=clover->mNumMethods-1; i>=0; i--) {
        sCLType* result;
        sCLClass* result_class;

        method = clover->mMethods + i;

        result = &method->mResultType;
        result_class = cl_get_class(CONS_str(&clover->mConstPool, result->mClassNameOffset));

        if(strcmp(METHOD_NAME2(clover, method), "initialize") == 0 && method->mNumParams == 0 && method->mNumBlockType == 0 && result_class == gBoolClass && result->mGenericsTypesNum == 0 && (method->mFlags & CL_CLASS_METHOD))
        {
            break;
        }
    }

    if(clover && method) {
        return cl_excute_method(method, clover, clover, NULL, &result_value);
    }
    else {
        return FALSE;
    }
}

