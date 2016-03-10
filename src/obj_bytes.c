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
    int i;
    int size;

    obj = alloc_bytes_object(type_object, info);

    size = (len+1) * 2;

    CLBYTES(obj)->mChars = MCALLOC(1, size+1);

    memcpy(CLBYTES(obj)->mChars, str, len);
    CLBYTES(obj)->mChars[len] = 0;

    CLBYTES(obj)->mLen = len;
    CLBYTES(obj)->mSize = size;

    CLBYTES(obj)->mPoint = 0;

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
    str = MMALLOC(len+1);
    str[0] = 0;
    for(i=0; i<number; i++) {
        xstrncat((char*)str, (char*)CLBYTES(string)->mChars, len+1);
    }

    result = create_bytes_object(str, len, gBytesTypeObject, info);

    MFREE(str);

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
    MFREE(CLBYTES(self)->mChars);
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

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    buf = CLBYTES(self)->mChars;
    len = CLBYTES(self)->mLen;

    wstr = MCALLOC(1, sizeof(wchar_t)*(len+1));

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "failed to mbstowcs");
        MFREE(wstr);
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    MFREE(wstr);

    return TRUE;
}

void replace_bytes(CLObject bytes, char* buf, int size)
{
    char* chars;

    chars = CLBYTES(bytes)->mChars;

    if(size+1 < CLBYTES(bytes)->mSize) {
        memcpy(chars, buf, size);
        chars[size] = 0;

        CLBYTES(bytes)->mLen = size;
    }
    else {
        int new_size;

        new_size = (size + 1 ) * 2;

        chars = MREALLOC(chars, new_size);

        memcpy(chars, buf, size);
        chars[size] = 0;

        CLBYTES(bytes)->mChars = chars;
        CLBYTES(bytes)->mSize = new_size;
        CLBYTES(bytes)->mLen = size;
    }
}

static void append_bytes(CLObject self, char* str, int len)
{
    int size;
    int new_size;
    char* chars;
    int len2;

    chars = CLBYTES(self)->mChars;
    len2 = CLBYTES(self)->mLen;

    size = CLBYTES(self)->mLen + len + 1;
    if(size < CLBYTES(self)->mSize) {
        memcpy(chars + len2, str, len);
        chars[len2+len] = 0;

        CLBYTES(self)->mLen = len2 + len;
    }
    else {
        new_size = (len + len2 + 1) * 2;
        chars = MREALLOC(chars, new_size);

        memcpy(chars + len2, str, len);
        chars[len2+len] = 0;

        CLBYTES(self)->mChars = chars;
        CLBYTES(self)->mLen = len2 + len;
        CLBYTES(self)->mSize = new_size;
    }
}

BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    char character;
    CLObject ovalue1, ovalue2;
    char* chars;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;
    if(!check_type(ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }
    index = CLINT(ovalue1)->mValue;

    ovalue2 = (lvar+2)->mObjectValue.mValue;
    if(!check_type(ovalue2, gByteTypeObject, info)) {
        return FALSE;
    }
    character = CLBYTE(ovalue2)->mValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        entry_exception_object_with_class_name(info, "RangeException", "rage exception");
        return FALSE;
    }

    chars = CLBYTES(self)->mChars;

    chars[index] = character;

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(character);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    CLObject ovalue1;
    char* chars;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;

    if(!check_type(ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }

    index = CLINT(ovalue1)->mValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }
    else {
        chars = CLBYTES(self)->mChars;

        (*stack_ptr)->mObjectValue.mValue = create_byte_object(chars[index]);
        (*stack_ptr)++;
    }

    return TRUE;
}

BOOL Bytes_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    int len;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gBytesTypeObject, info)) {
        return FALSE;
    }

    replace_bytes(self, CLBYTES(value)->mChars, CLBYTES(value)->mLen);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, right;
    char* chars;
    char* chars2;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    chars = CLBYTES(self)->mChars;

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gBytesTypeObject, info)) {
        return FALSE;
    }

    chars2 = CLBYTES(right)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(strcmp(chars, chars2));
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_append(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, str;
    char* chars;
    char* chars2;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    str = (lvar+1)->mObjectValue.mValue;

    if(!check_type(str, gBytesTypeObject, info)) {
        return FALSE;
    }

    append_bytes(self, CLBYTES(str)->mChars, CLBYTES(str)->mLen);

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_forward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject size;
    int size_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    size = (lvar+1)->mObjectValue.mValue;

    if(!check_type(size, gIntTypeObject, info)) {
        return FALSE;
    }

    size_value = CLINT(size)->mValue;

    CLBYTES(self)->mPoint += size_value;

    if(CLBYTES(self)->mPoint > CLBYTES(self)->mLen) {
        entry_exception_object_with_class_name(info, "Exception", "pointer is out of range");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_backward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject size;
    int size_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    size = (lvar+1)->mObjectValue.mValue;

    if(!check_type(size, gIntTypeObject, info)) {
        return FALSE;
    }

    size_value = CLINT(size)->mValue;

    CLBYTES(self)->mPoint -= size_value;

    if(CLBYTES(self)->mPoint < 0) {
        entry_exception_object_with_class_name(info, "Exception", "pointer is out of range");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_getByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    unsigned char value;
    char* p;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    p = CLBYTES(self)->mChars + CLBYTES(self)->mPoint;
    value = *p;

    (*stack_ptr)->mObjectValue.mValue = create_byte_object(value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_getShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    unsigned short value;
    unsigned short* p;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    p = (unsigned short*)(CLBYTES(self)->mChars + CLBYTES(self)->mPoint);
    value = *p;

    (*stack_ptr)->mObjectValue.mValue = create_short_object(value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_getUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    unsigned int value;
    unsigned int* p;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    p = (unsigned int*)(CLBYTES(self)->mChars + CLBYTES(self)->mPoint);
    value = *p;

    (*stack_ptr)->mObjectValue.mValue = create_uint_object(value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_getLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    unsigned long value;
    unsigned long* p;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    p = (unsigned long*)(CLBYTES(self)->mChars + CLBYTES(self)->mPoint);
    value = *p;

    (*stack_ptr)->mObjectValue.mValue = create_long_object(value);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_setPoint(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject point;
    unsigned long point_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    point = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(point, "long", info)) {
        return FALSE;
    }

    point_value = CLLONG(point)->mValue;

    CLBYTES(self)->mPoint = point_value;

    if(CLBYTES(self)->mPoint < 0 || CLBYTES(self)->mPoint > CLBYTES(self)->mLen) {
        entry_exception_object_with_class_name(info, "Exception", "pointer is out of range");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_point(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    long point;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gBytesTypeObject, info)) {
        return FALSE;
    }

    point = CLBYTES(self)->mPoint;

    (*stack_ptr)->mObjectValue.mValue = create_long_object(point);
    (*stack_ptr)++;

    return TRUE;
}
