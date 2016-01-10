#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLParser);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_parser_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

CLObject create_parser_object(CLObject string, CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_parser_object(type_object, info);

    if(string == 0) {
        CLPARSER(obj)->mSize = 0;
        CLPARSER(obj)->mPoint = 0;
        CLPARSER(obj)->mPointedObject = 0;
    }
    else {
        CLPARSER(obj)->mSize = CLSTRING(string)->mLen;
        CLPARSER(obj)->mPoint = 0;
        CLPARSER(obj)->mPointedObject = string;
    }

    return obj;
}

static CLObject create_parser_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_parser_object(0, type_object, info);

    return obj;
}

static void mark_parser_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject object2;

    object2 = CLPARSER(object)->mPointedObject;
    mark_object(object2, mark_flg);
}

void initialize_hidden_class_method_of_parser(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_parser_object;
    klass->mCreateFun = create_parser_object_for_new;
}

BOOL Parser_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "Parser", info)) {
        return FALSE;
    }

    CLPARSER(self)->mSize = CLPARSER(value)->mSize;
    CLPARSER(self)->mPoint = CLPARSER(value)->mPoint;
    CLPARSER(self)->mPointedObject = CLPARSER(value)->mPointedObject;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_setString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "String", info)) {
        return FALSE;
    }

    CLPARSER(self)->mSize = CLSTRING(value)->mLen;
    CLPARSER(self)->mPoint = 0;
    CLPARSER(self)->mPointedObject = value;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = CLPARSER(self)->mPointedObject;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_point(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;
    unsigned long point;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    point = CLPARSER(self)->mPoint;

    (*stack_ptr)->mObjectValue.mValue = create_long_object(point);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_setPoint(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject point;
    CLObject result;
    unsigned long point_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    point = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(point, "long", info)) {
        return FALSE;
    }

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    point_value = CLLONG(point)->mValue;

    if(point_value > CLPARSER(self)->mSize) {
        entry_exception_object_with_class_name(info, "Exception", "Out of range");
        return FALSE;
    }

    CLPARSER(self)->mPoint = point_value;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_end(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;
    unsigned long point;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    result = create_bool_object(CLPARSER(self)->mPoint >= CLPARSER(self)->mSize);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_getString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject num;
    int num_value;
    wchar_t* pointer;
    CLObject result;
    CLObject string;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    num = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(num, "int", info)) {
        return FALSE;
    }

    /// Clover to C Value ///
    num_value = CLINT(num)->mValue;
    string = CLPARSER(self)->mPointedObject;
    pointer = CLSTRING_DATA(string)->mChars + CLPARSER(self)->mPoint;

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }
    if(num_value < 1) {
        entry_exception_object_with_class_name(info, "Exception", "num should be greater than 0");
        return FALSE;
    }

    if(CLPARSER(self)->mPoint + num_value > CLPARSER(self)->mSize) {
        num_value = CLPARSER(self)->mSize - CLPARSER(self)->mPoint;
    }

    result = create_string_object(pointer, num_value, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_getChar(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject result;
    wchar_t wc;
    wchar_t* pointer;
    CLObject string;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    /// Clover to C Value ///
    string = CLPARSER(self)->mPointedObject;
    pointer = CLSTRING_DATA(string)->mChars + CLPARSER(self)->mPoint;

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    wc = *pointer;

    result = create_char_object(wc);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_forward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject num;
    int num_value;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    num = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(num, "int", info)) {
        return FALSE;
    }

    /// Clover to C Value ///
    num_value = CLINT(num)->mValue;

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    CLPARSER(self)->mPoint += num_value;

    if(CLPARSER(self)->mPoint > CLPARSER(self)->mSize) {
        CLPARSER(self)->mPoint = CLPARSER(self)->mSize;
    }

    result = self;

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Parser_backward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject num;
    int num_value;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Parser", info)) {
        return FALSE;
    }

    num = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(num, "int", info)) {
        return FALSE;
    }

    /// Clover to C Value ///
    num_value = CLINT(num)->mValue;

    if(CLPARSER(self)->mPointedObject == 0) {
        entry_exception_object_with_class_name(info, "Exception", "Parser pointed null object");
        return FALSE;
    }

    CLPARSER(self)->mPoint -= num_value;

    if(CLPARSER(self)->mPoint < 0) {
        CLPARSER(self)->mPoint = 0;
    }

    result = self;

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}
