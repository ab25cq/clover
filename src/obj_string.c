#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLString);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static unsigned int chars_object_size(unsigned int len2)
{
    unsigned int size;

    size = sizeof(sCLStringData) - sizeof(wchar_t)*DUMMY_ARRAY_SIZE + sizeof(wchar_t) * len2;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

BOOL create_buffer_from_string_object(CLObject str, ALLOC char** result, sVMInfo* info)
{
    wchar_t* wstr;
    int len;

    wstr = CLSTRING_DATA(str)->mChars;
    len = CLSTRING(str)->mLen;

    *result = CALLOC(1, MB_LEN_MAX * (len + 1));

    if((int)wcstombs(*result, wstr, MB_LEN_MAX * (len+1)) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs on converting string");
        return FALSE;
    }

    return TRUE;
}

static CLObject alloc_string_object(unsigned int len2, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    CLObject obj2;
    unsigned int size;
    unsigned int chars_size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    push_object(obj, info);

    chars_size = chars_object_size(len2);
    obj2 = alloc_heap_mem(chars_size, 0);
    CLSTRING(obj)->mData = obj2;

    pop_object(info);

    return obj;
}

CLObject create_string_object(wchar_t* str, int len, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    wchar_t* chars;
    int i;

    obj = alloc_string_object(len+1, type_object, info);

    chars = CLSTRING_DATA(obj)->mChars;
    for(i=0; i<len; i++) {
        chars[i] = str[i];
    }
    chars[i] = 0;

    CLSTRING(obj)->mLen = len;

    return obj;
}

BOOL create_string_object_from_ascii_string(CLObject* result, char* str, CLObject type_object, sVMInfo* info)
{
    int wlen;
    wchar_t* wstr;

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        FREE(wstr);
        return FALSE;
    }

    *result = create_string_object(wstr, wlen, type_object, info);

    FREE(wstr);

    return TRUE;
}

static CLObject create_string_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_string_object(L"", 0, type_object, info);
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

CLObject create_string_object_by_multiply(CLObject string, int number, sVMInfo* info)
{
    wchar_t* wstr;
    int len;
    int i;
    CLObject result;

    len = CLSTRING(string)->mLen * number;
    wstr = CALLOC(1, sizeof(wchar_t)*(len + 1));
    wstr[0] = 0;
    for(i=0; i<number; i++) {
        wcsncat(wstr, CLSTRING_DATA(string)->mChars, len-wcslen(wstr));
    }

    result = create_string_object(wstr, len, gStringTypeObject, info);

    FREE(wstr);

    return result;
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
    wchar_t* chars;

    obj_size = object_size(obj);

    len = CLSTRING(obj)->mLen;

    size = (len + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    chars = CLSTRING_DATA(obj)->mChars;
    if((int)wcstombs(str, chars, size) >= 0) {
        VMLOG(info, " (len %d) (%s)\n", len, str);
    }

    FREE(str);
}

static void mark_string_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject obj2;

    obj2 = CLSTRING(object)->mData;

    mark_object(obj2, mark_flg);
}

void initialize_hidden_class_method_of_string(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_string_object;
    klass->mMarkFun = mark_string_object;
    klass->mCreateFun = create_string_object_for_new;
}

BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(wcslen(chars));
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    CLObject ovalue1;
    wchar_t* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;

    if(!check_type(ovalue1, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    index = CLINT(ovalue1)->mValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
        entry_exception_object(info, gExRangeClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(chars[index]);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    wchar_t character;
    CLObject ovalue1, ovalue2;
    wchar_t* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;
    if(!check_type(ovalue1, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    index = CLINT(ovalue1)->mValue;

    ovalue2 = (lvar+2)->mObjectValue.mValue;
    if(!check_type(ovalue2, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    character = (wchar_t)CLINT(ovalue2)->mValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
        entry_exception_object(info, gExRangeClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;
    chars[index] = character;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(character);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

BOOL String_toBytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* wstr;
    int len;
    char* buf;
    int buf_len;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    wstr = CLSTRING_DATA(self)->mChars;
    len = CLSTRING(self)->mLen;

    buf = CALLOC(1, MB_LEN_MAX * (len + 1));

    if((int)wcstombs(buf, wstr, MB_LEN_MAX * (len+1)) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs on converting string");
        FREE(buf);
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_bytes_object((unsigned char*)buf, strlen(buf), gBytesTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    FREE(buf);
    vm_mutex_unlock();

    return TRUE;
}

BOOL String_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    unsigned int chars_size;
    CLObject obj2;
    CLObject new_obj;
    wchar_t* chars;
    wchar_t* chars2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars_size = chars_object_size(CLSTRING(value)->mLen+1);
    obj2 = alloc_heap_mem(chars_size, 0);
    CLSTRING(self)->mData = obj2;
    CLSTRING(self)->mLen = CLSTRING(value)->mLen;

    chars = CLSTRING_DATA(self)->mChars;
    chars2 = CLSTRING_DATA(value)->mChars;

    for(i=0; i<CLSTRING(value)->mLen; i++) {
        chars[i] = chars2[i];
    }
    chars[i] = 0;

    new_obj = create_string_object(CLSTRING_DATA(value)->mChars, CLSTRING(value)->mLen, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_string_object(CLSTRING_DATA(self)->mChars, CLSTRING(self)->mLen, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, right;
    wchar_t* chars;
    wchar_t* chars2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars2 = CLSTRING_DATA(right)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(wcscmp(chars, chars));
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

