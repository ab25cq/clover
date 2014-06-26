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

void string_append(CLObject string, char* str, int n)
{
}

static void show_string_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size;
    int len;
    int size;
    char* str;

    obj_size = object_size(obj);

    len = CLSTRING(obj)->mLen;

    size = (len + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    if((int)wcstombs(str, CLSTRING(obj)->mChars, size) >= 0) {
        VMLOG(info, " (len %d) (%s)\n", len, str);
    }

    FREE(str);
}

void initialize_hidden_class_method_of_string(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_string_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue;

    (*stack_ptr)->mObjectValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue;

    (*stack_ptr)->mIntValue = wcslen(CLSTRING(self)->mChars);
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    int index;

    vm_mutex_lock();

    self = lvar->mObjectValue;
    index = (lvar+1)->mIntValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
        entry_exception_object(info, gExRangeType.mClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mIntValue = CLSTRING(self)->mChars[index];
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    return TRUE;
}

