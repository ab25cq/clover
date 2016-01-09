#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLStringBuffer);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_string_buffer_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_string_buffer_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    wchar_t* chars;
    int i;
    int size;

    obj = alloc_string_buffer_object(type_object, info);
    size = 1024;

    CLSTRINGBUFFER(obj)->mLen = 0;
    CLSTRINGBUFFER(obj)->mSize = size;
    CLSTRINGBUFFER(obj)->mChars = CALLOC(1, sizeof(wchar_t)*(size+1));

    return obj;
}

static CLObject create_string_buffer_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_string_buffer_object(type_object, info);

    return obj;
}

static void show_string_buffer_object(sVMInfo* info, CLObject obj)
{
    VMLOG(info, " (len %d) (%ls)\n", CLSTRINGBUFFER(obj)->mLen, CLSTRINGBUFFER(obj)->mChars);
}

void free_string_buffer_object(CLObject self)
{
    FREE(CLSTRINGBUFFER(self)->mChars);
}

void initialize_hidden_class_method_of_string_buffer(sCLClass* klass)
{
    klass->mFreeFun = free_string_buffer_object;
    klass->mShowFun = show_string_buffer_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_string_buffer_object_for_new;
}

BOOL StringBuffer_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "StringBuffer", info)) {
        return FALSE;
    }

    if(CLSTRINGBUFFER(value)->mLen+1 < CLSTRINGBUFFER(self)->mSize) {
        wcsncpy(CLSTRINGBUFFER(self)->mChars, CLSTRINGBUFFER(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
        CLSTRINGBUFFER(self)->mLen = CLSTRINGBUFFER(value)->mLen;
    }
    else {
        int size;

        FREE(CLSTRINGBUFFER(self)->mChars);

        size = (CLSTRINGBUFFER(value)->mLen+1) * 2;
        CLSTRINGBUFFER(self)->mSize = size;

        CLSTRINGBUFFER(self)->mChars = CALLOC(1, sizeof(wchar_t)*(size+1));

        wcsncpy(CLSTRINGBUFFER(self)->mChars, CLSTRINGBUFFER(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
        CLSTRINGBUFFER(self)->mLen = CLSTRINGBUFFER(value)->mLen;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL StringBuffer_setValue2(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "String", info)) {
        return FALSE;
    }

    if(CLSTRING(value)->mLen+1 < CLSTRINGBUFFER(self)->mSize) {
        wcsncpy(CLSTRINGBUFFER(self)->mChars, CLSTRING_DATA(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
        CLSTRINGBUFFER(self)->mLen = CLSTRING(value)->mLen;
    }
    else {
        int size;

        FREE(CLSTRINGBUFFER(self)->mChars);

        size = (CLSTRING(value)->mLen+1) * 2;
        CLSTRINGBUFFER(self)->mSize = size;

        CLSTRINGBUFFER(self)->mChars = CALLOC(1, sizeof(wchar_t)*(size+1));

        wcsncpy(CLSTRINGBUFFER(self)->mChars, CLSTRING_DATA(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
        CLSTRINGBUFFER(self)->mLen = CLSTRINGBUFFER(value)->mLen;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL StringBuffer_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;
    wchar_t* wstr;
    int len;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    wstr = CLSTRINGBUFFER(self)->mChars;
    len = CLSTRINGBUFFER(self)->mLen;

    result = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL StringBuffer_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    result = create_int_object(CLSTRINGBUFFER(self)->mLen);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL StringBuffer_append(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    int new_str_len;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "String", info)) {
        return FALSE;
    }

    new_str_len = CLSTRINGBUFFER(self)->mLen + CLSTRING(value)->mLen;

    if(new_str_len+1 < CLSTRINGBUFFER(self)->mSize) {
        wcsncat(CLSTRINGBUFFER(self)->mChars, CLSTRING_DATA(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
        CLSTRINGBUFFER(self)->mLen = new_str_len;
    }
    else {
        int new_size;
        wchar_t* wstr_before;

        new_size = (new_str_len+1) * 2;

        CLSTRINGBUFFER(self)->mChars = REALLOC(CLSTRINGBUFFER(self)->mChars, sizeof(wchar_t)*(new_size+1));
        CLSTRINGBUFFER(self)->mSize = new_size;
        CLSTRINGBUFFER(self)->mLen = new_str_len;

        wcsncpy(CLSTRINGBUFFER(self)->mChars, CLSTRING_DATA(value)->mChars, CLSTRINGBUFFER(self)->mSize+1);
    }

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL StringBuffer_append2(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    int new_str_len;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "StringBuffer", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "char", info)) {
        return FALSE;
    }

    new_str_len = CLSTRINGBUFFER(self)->mLen + 1;

    if(new_str_len+1 < CLSTRINGBUFFER(self)->mSize) {
        int len;

        len = CLSTRINGBUFFER(self)->mLen;

        CLSTRINGBUFFER(self)->mChars[len] = CLCHAR(value)->mValue;
        CLSTRINGBUFFER(self)->mLen++;
    }
    else {
        int new_size;
        wchar_t* wstr_before;
        int len;

        len = CLSTRINGBUFFER(self)->mLen;
        new_size = (new_str_len+1) * 2;

        CLSTRINGBUFFER(self)->mChars = REALLOC(CLSTRINGBUFFER(self)->mChars, sizeof(wchar_t)*(new_size+1));
        CLSTRINGBUFFER(self)->mSize = new_size;
        CLSTRINGBUFFER(self)->mLen = new_str_len;

        CLSTRINGBUFFER(self)->mChars[len] = CLCHAR(value)->mValue;
    }

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

