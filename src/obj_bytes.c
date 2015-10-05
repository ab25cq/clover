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

static CLObject alloc_bytes_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_bytes_object(char* str, int len, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    char* chars;
    int i;

    obj = alloc_bytes_object(type_object, info);

    CLBYTES(obj)->mChars = MALLOC(len+1);

    memcpy(CLBYTES(obj)->mChars, str, len);
    CLBYTES(obj)->mChars[len] = 0;

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
    char* str;
    int len;
    int i;
    CLObject result;

    len = CLBYTES(string)->mLen * number;
    str = MALLOC(sizeof(char)*(len + 1));
    str[0] = 0;
    for(i=0; i<number; i++) {
        xstrncat((char*)str, CLBYTES(string)->mChars, len+1);
    }

    result = create_bytes_object(str, len, gBytesTypeObject, info);

    FREE(str);

    return result;
}

CLObject create_bytes_object_by_multiply_with_type(CLObject string, int number, sVMInfo* info, CLObject type_object)
{
    CLObject result;

    result = create_bytes_object_by_multiply(string, number, info);

    CLOBJECT_HEADER(result)->mType = type_object;

    return result;
}

void free_bytes_object(CLObject self)
{
    FREE(CLBYTES(self)->mChars);
}

void initialize_hidden_class_method_of_bytes(sCLClass* klass)
{
    klass->mFreeFun = free_bytes_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
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

    buf = CLBYTES(self)->mChars;
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

void replace_bytes(CLObject bytes, char* buf, int size)
{
    unsigned int chars_size;
    CLObject obj2;
    char* chars;
    int i;

    vm_mutex_lock();

    FREE(CLBYTES(bytes)->mChars);

    CLBYTES(bytes)->mChars = MALLOC(size+1);
    memcpy(CLBYTES(bytes)->mChars, buf, size);
    CLBYTES(bytes)->mChars[size] = 0;

    CLBYTES(bytes)->mLen = size;

    vm_mutex_unlock();
}

BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    char character;
    CLObject ovalue1, ovalue2;
    char* chars;

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

    chars = CLBYTES(self)->mChars;

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
    char* chars;

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

    chars = CLBYTES(self)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(chars[index]);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    int len;

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

    FREE(CLBYTES(self)->mChars);

    len = CLBYTES(value)->mLen;

    CLBYTES(self)->mChars = MALLOC(len+1);
    CLBYTES(self)->mLen = len;

    memcpy(CLBYTES(self)->mChars, CLBYTES(value)->mChars, len);
    CLBYTES(self)->mChars[len] = 0;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
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

    chars = CLBYTES(self)->mChars;

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gBytesTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars2 = CLBYTES(right)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(strcmp(chars, chars));
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_toPointer(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;
    void* pointer;
    int chars_size;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    pointer = CLBYTES(self)->mChars;
    chars_size = CLBYTES(self)->mLen;

    new_obj = create_pointer_object(pointer, chars_size);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}
