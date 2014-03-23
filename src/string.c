#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size(unsigned int len2)
{
    unsigned int size;

    size = sizeof(sCLString) - sizeof(wchar_t) * DUMMY_ARRAY_SIZE;
    size += sizeof(wchar_t) * len2;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_string_object(sCLClass* klass, unsigned int len2)
{
    CLObject obj;
    unsigned int size;

    size = object_size(len2);
    obj = alloc_heap_mem(size, klass);

    return obj;
}

CLObject create_string_object(sCLClass* klass, wchar_t* str, int len)
{
    CLObject obj;
    wchar_t* data;
    int i;

    obj = alloc_string_object(klass, len+1);

    data = CLSTRING(obj)->mChars;

    for(i=0; i<len; i++) {
        data[i] = str[i];
    }
    data[i] = 0;

    CLSTRING(obj)->mLen = len;

    return obj;
}

static void show_string_object(CLObject obj)
{
    unsigned int obj_size;
    int len;
    wchar_t* data2;
    int size;
    char* str;

    cl_print(" class name (String) ");
    obj_size = object_size(obj);

    len = CLSTRING(obj)->mLen;

    data2 = MALLOC(sizeof(wchar_t)*len + 1);
    memcpy(data2, CLSTRING(obj)->mChars, sizeof(wchar_t)*len);
    data2[len] = 0;

    size = (len + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    wcstombs(str, data2, size);

    cl_print(" (len %d) (%s)\n", len, str);

    FREE(data2);
    FREE(str);
}

void initialize_hidden_class_method_of_string(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_string_object;
    klass->mMarkFun = NULL;
}

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar)
{
    CLObject self;

    self = lvar->mObjectValue;

    (*stack_ptr)->mIntValue = wcslen(CLSTRING(self)->mChars);
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar)
{
    CLObject self;
    int index;

    self = lvar->mObjectValue;
    index = (lvar+1)->mObjectValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
puts("range exception");
return FALSE;
    }

    (*stack_ptr)->mIntValue = CLSTRING(self)->mChars[index];
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

