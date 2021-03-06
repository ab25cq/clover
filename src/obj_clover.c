#define _GNU_SOURCE
#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>

int gArgc = 0;
char** gArgv = NULL;

BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject string;
    int size;
    char* str;

    string = lvar->mObjectValue.mValue;

    if(!check_type(string, gStringTypeObject, info)) {
        return FALSE;
    }

    size = (CLSTRING(string)->mLen + 1) * MB_LEN_MAX;
    str = MMALLOC(size);
    if((int)wcstombs(str, CLSTRING_DATA(string)->mChars, size) < 0) {
        MFREE(str);
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }

    cl_print(info, "%s", str);

    MFREE(str);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Clover_showClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    show_class_list(info);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    cl_gc();

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Clover_outputToString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
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

    result_existance = FALSE;

    block = lvar->mObjectValue.mValue;

    cl_print_buffer_before = info->print_buffer;
    info->print_buffer = &buf;              // allocate
    sBuf_init(info->print_buffer);

    if(!cl_excute_block(block, result_existance, info, vm_type)) {
        MFREE(info->print_buffer->mBuf);
        info->print_buffer = cl_print_buffer_before;
        return FALSE;
    }

    str = info->print_buffer->mBuf;

    len = strlen(str) + 1;
    wstr = MMALLOC(sizeof(wchar_t)*len);
    if((int)mbstowcs(wstr, str, len) < 0) {
        MFREE(wstr);
        MFREE(info->print_buffer->mBuf);

        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        info->print_buffer = cl_print_buffer_before;
        return FALSE;
    }
    wcs_len = wcslen(wstr);

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wcs_len, gStringTypeObject, info);
    (*stack_ptr)++;

    MFREE(wstr);
    MFREE(info->print_buffer->mBuf);

    info->print_buffer = cl_print_buffer_before;

    return TRUE;
}

BOOL Clover_printf(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject format;
    CLObject params;
    sBuf buf;
    char* p;
    int param_num;
    char* format_value;
    int size;

    format = lvar->mObjectValue.mValue;

    if(!check_type(format, gStringTypeObject, info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "anonymous", info)) {
        return FALSE;
    }

    size = (CLSTRING(format)->mLen + 1) * MB_LEN_MAX;
    format_value = MMALLOC(size);
    if((int)wcstombs(format_value, CLSTRING_DATA(format)->mChars, size) < 0) {
        MFREE(format_value);
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }

    p = format_value;

    sBuf_init(&buf);

    param_num = 0;

    while(*p) {
        if(*p == '%') {
            char format2[128+1];
            char* p2;
            BOOL no_conversion;

            no_conversion = FALSE;

            p++;

            p2 = format2;

            *p2++ = '%';

            /// flag characters ///
            while(1) {
                if(*p == '#' || *p == '0' || *p == '-' || *p == ' ' || *p == '+' || *p == '\'') {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// field width ///
            while(1) {
                if(isdigit(*p)) {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// precision ///
            if(*p == '.') {
                *p2++ = *p++;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }

                while(1) {
                    if(isdigit(*p) || *p == '*' || *p == '$') {
                        *p2++ = *p++;

                        if(p2 - format2 >= 128) {
                            entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                            MFREE(buf.mBuf);
                            MFREE(format_value);
                            return FALSE;
                        }
                    }
                    else {
                        break;
                    }
                }
            }

            /// length modifiers ///
            while(1) {
                if((*p == 'h' && *(p+1) == 'h') || (*p == 'l' && *(p+1) == 'l')) {
                    *p2++ = *p++;
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else if(*p == 'h' || *p == 'l' || *p == 'L' || *p == 'j' || *p == 'z' || *p == 't') 
                {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// convertion specifier ///
            if(*p == 'd' || *p == 'i' || *p == 'o' || *p == 'u' || *p == 'x' || *p == 'X' || *p == 'e' || *p == 'E'|| *p == 'f' || *p == 'F' || *p == 'g' || *p == 'G' || *p == 'a' || *p == 'A' || *p == 'c' || *p == 's' || *p == 'p' || *p == 'n' || *p == 'm') 
            {
                *p2++ = *p++;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }
            }
            else if(*p == '%') {
                *p2++ = *p++;

                no_conversion = TRUE;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }
            }
            else {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }

            *p2++ = 0;

            if(p2 - format2 >= 128) {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }

            if(no_conversion) {
                sBuf_append_char(&buf, '%');
            }
            else if(param_num < CLARRAY(params)->mLen) {
                CLObject param;
                char* str;
                CLObject type_object;

                param = CLARRAY_ITEMS2(params, param_num).mObjectValue.mValue;

                type_object = CLOBJECT_HEADER(param)->mType;

                if(substitution_posibility_of_type_object_with_class_name("int", type_object, FALSE, info)) 
                {
                    asprintf(ALLOC &str, format2, CLINT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("byte", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBYTE(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("short", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLSHORT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("uint", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLUINT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("long", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLLONG(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("char", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLCHAR(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("float", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLFLOAT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("double", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLDOUBLE(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("bool", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBOOL(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("pointer", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLPOINTER(param)->mPointer);
                }
                else if(substitution_posibility_of_type_object_with_class_name("String", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLSTRING_DATA(param)->mChars);
                }
                else if(substitution_posibility_of_type_object_with_class_name("Bytes", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBYTES(param)->mChars);
                }
                else if(substitution_posibility_of_type_object_with_class_name("Null", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLNULL(param)->mValue);
                }
                else {
                    entry_exception_object_with_class_name(info, "Exception", "invalid argument type");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }

                sBuf_append_str(&buf, str);

                free(str);

                param_num++;
            }
            else {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }
        }
        else {
            sBuf_append_char(&buf, *p);
            p++;
        }
    }

    cl_print(info, "%s", buf.mBuf);

    MFREE(buf.mBuf);
    MFREE(format_value);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Clover_sprintf(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject format;
    CLObject params;
    CLObject result;
    sBuf buf;
    char* p;
    int param_num;
    char* format_value;
    int size;

    format = lvar->mObjectValue.mValue;

    if(!check_type(format, gStringTypeObject, info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "anonymous", info)) {
        return FALSE;
    }

    size = (CLSTRING(format)->mLen + 1) * MB_LEN_MAX;
    format_value = MMALLOC(size);
    if((int)wcstombs(format_value, CLSTRING_DATA(format)->mChars, size) < 0) {
        MFREE(format_value);
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }

    p = format_value;

    sBuf_init(&buf);

    param_num = 0;

    while(*p) {
        if(*p == '%') {
            char format2[128+1];
            char* p2;
            BOOL no_conversion;

            no_conversion = FALSE;

            p++;

            p2 = format2;

            *p2++ = '%';

            /// flag characters ///
            while(1) {
                if(*p == '#' || *p == '0' || *p == '-' || *p == ' ' || *p == '+' || *p == '\'') {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// field width ///
            while(1) {
                if(isdigit(*p)) {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// precision ///
            if(*p == '.') {
                *p2++ = *p++;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }

                while(1) {
                    if(isdigit(*p) || *p == '*' || *p == '$') {
                        *p2++ = *p++;

                        if(p2 - format2 >= 128) {
                            entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                            MFREE(buf.mBuf);
                            MFREE(format_value);
                            return FALSE;
                        }
                    }
                    else {
                        break;
                    }
                }
            }

            /// length modifiers ///
            while(1) {
                if((*p == 'h' && *(p+1) == 'h') || (*p == 'l' && *(p+1) == 'l')) {
                    *p2++ = *p++;
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else if(*p == 'h' || *p == 'l' || *p == 'L' || *p == 'j' || *p == 'z' || *p == 't') 
                {
                    *p2++ = *p++;

                    if(p2 - format2 >= 128) {
                        entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                        MFREE(buf.mBuf);
                        MFREE(format_value);
                        return FALSE;
                    }
                }
                else {
                    break;
                }
            }

            /// convertion specifier ///
            if(*p == 'd' || *p == 'i' || *p == 'o' || *p == 'u' || *p == 'x' || *p == 'X' || *p == 'e' || *p == 'E'|| *p == 'f' || *p == 'F' || *p == 'g' || *p == 'G' || *p == 'a' || *p == 'A' || *p == 'c' || *p == 's' || *p == 'p' || *p == 'n' || *p == 'm') 
            {
                *p2++ = *p++;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }
            }
            else if(*p == '%') {
                *p2++ = *p++;

                no_conversion = TRUE;

                if(p2 - format2 >= 128) {
                    entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }
            }
            else {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }

            *p2++ = 0;

            if(p2 - format2 >= 128) {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }


            if(no_conversion) {
                sBuf_append_char(&buf, '%');
            }
            else if(param_num < CLARRAY(params)->mLen) {
                CLObject param;
                char* str;
                CLObject type_object;

                param = CLARRAY_ITEMS2(params, param_num).mObjectValue.mValue;

                type_object = CLOBJECT_HEADER(param)->mType;

//push_object(params, info);

                if(substitution_posibility_of_type_object_with_class_name("int", type_object, FALSE, info)) 
                {
                    asprintf(ALLOC &str, format2, CLINT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("byte", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBYTE(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("short", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLSHORT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("uint", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLUINT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("long", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLLONG(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("char", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLCHAR(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("float", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLFLOAT(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("double", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLDOUBLE(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("bool", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBOOL(param)->mValue);
                }
                else if(substitution_posibility_of_type_object_with_class_name("pointer", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLPOINTER(param)->mPointer);
                }
                else if(substitution_posibility_of_type_object_with_class_name("String", type_object, FALSE, info)) 
                {
                    asprintf(ALLOC &str, format2, CLSTRING_DATA(param)->mChars);
                }
                else if(substitution_posibility_of_type_object_with_class_name("Bytes", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLBYTES(param)->mChars);
                }
                else if(substitution_posibility_of_type_object_with_class_name("Null", type_object, FALSE, info)) {
                    asprintf(ALLOC &str, format2, CLNULL(param)->mValue);
                }
                else {
                    entry_exception_object_with_class_name(info, "Exception", "invalid argument type");
                    MFREE(buf.mBuf);
                    MFREE(format_value);
                    return FALSE;
                }

                sBuf_append_str(&buf, str);

                free(str);

                param_num++;
            }
            else {
                entry_exception_object_with_class_name(info, "Exception", "invalid format string");
                MFREE(buf.mBuf);
                MFREE(format_value);
                return FALSE;
            }
        }
        else {
            sBuf_append_char(&buf, *p);
            p++;
        }
    }
    
    if(!create_string_object_from_ascii_string(&result, buf.mBuf, gStringTypeObject, info))
    {
        MFREE(buf.mBuf);
        MFREE(format_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result; // push result
    (*stack_ptr)++;

    MFREE(buf.mBuf);
    MFREE(format_value);

    return TRUE;
}

BOOL Clover_getCloverArgv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    int i;

    if(gArgc == 0) {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }
    else {
        CLObject result;
        int i;

        result = create_array_object_with_element_class_name("String", NULL, 0, info);
        push_object(result, info);

        for(i=0; i<gArgc; i++) {
            CLObject element;

            if(!create_string_object_from_ascii_string(&element, gArgv[i], gStringTypeObject, info))
            {
                return FALSE;
            }

            add_to_array(result, element, info);
        }

        pop_object(info);

        (*stack_ptr)->mObjectValue.mValue = result;
        (*stack_ptr)++;
    }

    return TRUE;
}
