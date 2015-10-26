#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLPointer);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_pointer_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gPointerTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_pointer_object(void* value, int size)
{
    CLObject obj;

    obj = alloc_pointer_object();

    CLPOINTER(obj)->mValue = value;
    CLPOINTER(obj)->mPointer = value;
    CLPOINTER(obj)->mSize = size;

    return obj;
}

CLObject create_pointer_object_with_class_name(void* value, int size, char* class_name, sVMInfo* info)
{
    CLObject type_object;
    CLObject result;

    type_object = create_type_object_with_class_name(class_name);

    push_object(type_object, info);

    result = create_pointer_object(value, size);

    CLOBJECT_HEADER(result)->mType = type_object;

    pop_object(info);

    return result;
}

static CLObject create_pointer_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_pointer_object(NULL, 0);
    push_object(self, info);

    CLOBJECT_HEADER(self)->mType = type_object;

    pop_object(info);

    return self;
}

void initialize_hidden_class_method_of_pointer(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_pointer_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gPointerClass = klass;
        gPointerTypeObject = create_type_object(gPointerClass);
    }
}

BOOL pointer_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;
    if(!check_type(value, gPointerTypeObject, info)) {
        return FALSE;
    }

    CLPOINTER(self)->mValue = CLPOINTER(value)->mValue;
    CLPOINTER(self)->mPointer = CLPOINTER(value)->mPointer;
    CLPOINTER(self)->mSize = CLPOINTER(value)->mSize;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    CLObject self;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    len = snprintf(buf, 128, "%p", CLPOINTER(self)->mValue);
    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_getByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject new_obj;
    CLObject self;
    unsigned char byte;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    if(CLPOINTER(self)->mPointer == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This pointer indicates to null.");
        return FALSE;
    }

    byte = *(unsigned char*)CLPOINTER(self)->mPointer;

    new_obj = create_byte_object(byte);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_getShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject new_obj;
    CLObject self;
    unsigned short value;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    if(CLPOINTER(self)->mPointer == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This pointer indicates to null.");
        return FALSE;
    }

    value = *(unsigned short*)CLPOINTER(self)->mPointer;

    new_obj = create_short_object(value);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_getUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject new_obj;
    CLObject self;
    unsigned int value;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    if(CLPOINTER(self)->mPointer == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This pointer indicates to null.");
        return FALSE;
    }

    value = *(unsigned int*)CLPOINTER(self)->mPointer;

    new_obj = create_uint_object(value);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_getLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject new_obj;
    CLObject self;
    unsigned long value;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    if(CLPOINTER(self)->mPointer == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This pointer indicates to null.");
        return FALSE;
    }

    value = *(unsigned long*)CLPOINTER(self)->mPointer;

    new_obj = create_long_object(value);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_forward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject new_obj;
    CLObject self;
    CLObject size;
    char byte;
    int size_value;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    size = (lvar+1)->mObjectValue.mValue;

    if(!check_type(size, gIntTypeObject, info)) {
        return FALSE;
    }

    size_value = CLINT(size)->mValue;

    if(CLPOINTER(self)->mPointer == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This pointer indicates to null.");
        return FALSE;
    }

    CLPOINTER(self)->mPointer += size_value;

    if(CLPOINTER(self)->mPointer < (char*)CLPOINTER(self)->mValue || CLPOINTER(self)->mPointer >= (char*)CLPOINTER(self)->mValue + CLPOINTER(self)->mSize)
    {
        entry_exception_object_with_class_name(info, "Exception", "This pointer indicates the out of range");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL pointer_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject result;
    CLObject right;
    CLObject self;

    self = lvar->mObjectValue.mValue;   // self

    if(!check_type(self, gPointerTypeObject, info)) {
        return FALSE;
    }

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gPointerTypeObject, info)) {
        return FALSE;
    }

    result = create_bool_object(CLPOINTER(self)->mPointer == CLPOINTER(right)->mPointer);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}
