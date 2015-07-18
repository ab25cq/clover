#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBytes);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static unsigned int chars_object_size(unsigned int len2)
{
    unsigned int size;

    size = sizeof(sCLBytesData) - sizeof(unsigned char) * DUMMY_ARRAY_SIZE + sizeof(unsigned char) * len2;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_bytes_object(unsigned int len2, CLObject type_object, sVMInfo* info)
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
    CLBYTES(obj)->mData = obj2;

    pop_object(info);

    return obj;
}

CLObject create_bytes_object(unsigned char* str, int len, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned char* chars;
    int i;

    obj = alloc_bytes_object(len+1, type_object, info);

    chars = CLBYTES_DATA(obj)->mChars;
    for(i=0; i<len; i++) {
        chars[i] = str[i];
    }
    chars[i] = 0;

    CLBYTES(obj)->mLen = len;

    return obj;
}

static CLObject create_bytes_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_bytes_object("", 0, type_object, info);
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

CLObject create_bytes_object_by_multiply(CLObject string, int number, sVMInfo* info)
{
    unsigned char* str;
    int len;
    int i;
    CLObject result;

    len = CLBYTES(string)->mLen * number;
    str = CALLOC(1, sizeof(char)*(len + 1));
    str[0] = 0;
    for(i=0; i<number; i++) {
        xstrncat((char*)str, CLBYTES_DATA(string)->mChars, len+1);
    }

    result = create_bytes_object(str, len, gBytesTypeObject, info);

    FREE(str);

    return result;
}

static void mark_bytes_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject obj2;

    obj2 = CLBYTES(object)->mData;

    mark_object(obj2, mark_flg);
}

void initialize_hidden_class_method_of_bytes(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_bytes_object;
    klass->mCreateFun = create_bytes_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gBytesClass = klass;
        gBytesTypeObject = create_type_object(gBytesClass);
    }
}

BOOL Bytes_Bytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(CLBYTES(self)->mLen);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    char* buf;
    int len;
    wchar_t* wstr;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    buf = CLBYTES_DATA(self)->mChars;
    len = CLBYTES(self)->mLen;

    wstr = CALLOC(1, sizeof(wchar_t)*(len+1));

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "failed to mbstowcs");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    FREE(wstr);
    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    unsigned char character;
    CLObject ovalue1, ovalue2;
    unsigned char* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
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
    if(!check_type(ovalue2, gByteTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    character = CLBYTE(ovalue2)->mValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        entry_exception_object_with_class_name(info, "RangeException", "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLBYTES_DATA(self)->mChars;

    chars[index] = character;

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(character);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    CLObject ovalue1;
    unsigned char* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;

    if(!check_type(ovalue1, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    index = CLINT(ovalue1)->mValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        entry_exception_object_with_class_name(info, "RangeException", "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLBYTES_DATA(self)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(chars[index]);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    unsigned int chars_size;
    CLObject obj2;
    CLObject new_obj;
    unsigned char* chars;
    unsigned char* chars2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars_size = chars_object_size(CLBYTES(value)->mLen+1);
    obj2 = alloc_heap_mem(chars_size, 0);
    CLBYTES(self)->mData = obj2;
    CLBYTES(self)->mLen = CLBYTES(value)->mLen;

    chars = CLBYTES_DATA(self)->mChars;
    chars2 = CLBYTES_DATA(value)->mChars;

    for(i=0; i<CLBYTES(value)->mLen; i++) {
        chars[i] = chars2[i];
    }
    chars[i] = 0;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_bytes_object(CLBYTES_DATA(self)->mChars, CLBYTES(self)->mLen, gBytesTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, right;
    char* chars;
    char* chars2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLBYTES_DATA(self)->mChars;

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars2 = CLBYTES_DATA(right)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(strcmp(chars, chars));
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

