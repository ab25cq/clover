#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLFieldObject);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject create_field_object(CLObject type_object, sCLClass* klass, sCLField* field)
{
    unsigned int size;
    CLObject object;

    size = object_size();

    object = alloc_heap_mem(size, type_object);

    CLFIELD(object)->mClass = klass;
    CLFIELD(object)->mField = field;

    return object;
}

static CLObject create_field_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_field_object(type_object, NULL, NULL);

    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_field_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_field_object_for_new;
}

BOOL Field_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    int i;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "Field", info)) {
        return FALSE;
    }

    CLFIELD(self)->mClass = CLFIELD(value)->mClass;
    CLFIELD(self)->mField = CLFIELD(value)->mField;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_isStaticField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLField* field;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    field = CLFIELD(self)->mField;

    if(field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(field->mFlags & CL_STATIC_FIELD);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_isPrivateField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLField* field;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    field = CLFIELD(self)->mField;

    if(field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(field->mFlags & CL_PRIVATE_FIELD);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_isProtectedField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLField* field;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    field = CLFIELD(self)->mField;

    if(field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(field->mFlags & CL_PROTECTED_FIELD);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_name(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLField* field;
    char* str;
    wchar_t* wstr;
    int wlen;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    klass2 = CLFIELD(self)->mClass;
    field = CLFIELD(self)->mField;

    if(klass2 == NULL || field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    str = CONS_str(&klass2->mConstPool, field->mNameOffset);

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        FREE(wstr);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wlen, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);

    return TRUE;
}

BOOL Field_fieldType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLField* field;
    CLObject type_object;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    klass2 = CLFIELD(self)->mClass;
    field = CLFIELD(self)->mField;

    if(klass2 == NULL || field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    type_object = create_type_object_from_cl_type(klass2, &field->mType, info);
    (*stack_ptr)->mObjectValue.mValue = type_object;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_get(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject object;
    sCLClass* klass2;
    sCLField* field;
    CLObject* value;
    int field_index;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    object = (lvar+1)->mObjectValue.mValue;

    klass2 = CLFIELD(self)->mClass;
    field = CLFIELD(self)->mField;

    if(klass2 == NULL || field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(klass2->mFlags & CLASS_FLAGS_NATIVE) {
        entry_exception_object_with_class_name(info, "Exception", "The class of this field is native class, this method can't get a field value from native classes");
        return FALSE;
    }

    field_index = field->mFieldIndex;

    if(field->mFlags & CL_STATIC_FIELD) {
        (*stack_ptr)->mObjectValue.mValue = field->uValue.mStaticField.mObjectValue.mValue;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = CLUSEROBJECT(object)->mFields[field_index].mObjectValue.mValue;
    }
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_set(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject object;
    sCLClass* klass2;
    sCLField* field;
    int field_index;
    CLObject value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    object = (lvar+1)->mObjectValue.mValue;
    value = (lvar+2)->mObjectValue.mValue;

    klass2 = CLFIELD(self)->mClass;
    field = CLFIELD(self)->mField;

    if(klass2 == NULL || field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(klass2->mFlags & CLASS_FLAGS_NATIVE) {
        entry_exception_object_with_class_name(info, "Exception", "The class of this field is native class, this method can't get a field value from native classes");
        return FALSE;
    }

    field_index = field->mFieldIndex;

    if(field->mFlags & CL_STATIC_FIELD) {
        field->uValue.mStaticField.mObjectValue.mValue = value;
    }
    else {
        CLUSEROBJECT(object)->mFields[field_index].mObjectValue.mValue = value;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Field_index(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int field_index;
    sCLClass* klass2;
    sCLField* field;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Field", info)) {
        return FALSE;
    }

    /// Convert Clover object to C value ///
    klass2 = CLFIELD(self)->mClass;
    field = CLFIELD(self)->mField;

    if(klass2 == NULL || field == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }

    if(klass2->mFlags & CLASS_FLAGS_NATIVE) {
        entry_exception_object_with_class_name(info, "Exception", "The class of this field is native class, this method can't get a field value from native classes");
        return FALSE;
    }

    field_index = field->mFieldIndex;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(field_index);
    (*stack_ptr)++;

    return TRUE;
}
