#include "clover.h"
#include "common.h"

static unsigned int object_size(unsigned int len2)
{
    unsigned int size;

    size = sizeof(sCLBytes) - sizeof(unsigned char) * DUMMY_ARRAY_SIZE;
    size += sizeof(unsigned char) * len2;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_bytes_object(sCLClass* klass, unsigned int len2)
{
    CLObject obj;
    unsigned int size;

    size = object_size(len2);
    obj = alloc_heap_mem(size, klass);

    return obj;
}

CLObject create_bytes_object(sCLClass* klass, unsigned char* str, int len)
{
    CLObject obj;
    unsigned char* data;
    int i;

    obj = alloc_bytes_object(klass, len+1);

    data = CLBYTES(obj)->mData;

    for(i=0; i<len; i++) {
        data[i] = str[i];
    }
    data[i] = 0;

    CLBYTES(obj)->mLen = len;

    return obj;
}

void initialize_hidden_class_method_of_bytes(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}

BOOL Bytes_Bytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue;

    (*stack_ptr)->mObjectValue = self;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    self = lvar->mObjectValue;

    (*stack_ptr)->mIntValue = CLBYTES(self)->mLen;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Bytes_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char* buf;
    int len;
    wchar_t* wstr;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue; // self

    buf = CLBYTES(self)->mData;
    len = CLBYTES(self)->mLen;

    wstr = CALLOC(1, sizeof(wchar_t)*(len+1));

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "failed to mbstowcs");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    FREE(wstr);
    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_items(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    int index;

    vm_mutex_lock();

    self = lvar->mObjectValue;
    index = (lvar+1)->mIntValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        entry_exception_object(info, gExRangeType.mClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mByteValue = CLBYTES(self)->mData[index];
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    int index;
    unsigned char character;

    vm_mutex_lock();

    self = lvar->mObjectValue;
    index = (lvar+1)->mIntValue;
    character = (lvar+2)->mByteValue;

    if(index < 0) index += CLBYTES(self)->mLen;

    if(index < 0 || index >= CLBYTES(self)->mLen) {
        entry_exception_object(info, gExRangeType.mClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    CLBYTES(self)->mData[index] = character;

    (*stack_ptr)->mByteValue = CLBYTES(self)->mData[index];
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

