#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLMethodObject);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject create_method_object(CLObject type_object, sCLClass* klass, sCLMethod* method)
{
    unsigned int size;
    CLObject object;

    size = object_size();

    object = alloc_heap_mem(size, type_object);

    CLMETHOD(object)->mClass = klass;
    CLMETHOD(object)->mMethod = method;

    return object;
}

static CLObject create_method_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_field_object(type_object, NULL, NULL);

    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

void initialize_hidden_class_method_of_method_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_method_object_for_new;
}

BOOL Method_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLMETHOD(self)->mClass = CLMETHOD(value)->mClass;
    CLMETHOD(self)->mMethod = CLMETHOD(value)->mMethod;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isNativeMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_NATIVE_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isClassMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_CLASS_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isPrivateMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_PRIVATE_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isConstructor(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_CONSTRUCTOR);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isSyncronizedMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_SYNCHRONIZED_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isVirtualMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_VIRTUAL_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isAbstractMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_ABSTRACT_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isGenericsNewableConstructor(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_GENERICS_NEWABLE_CONSTRUCTOR);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isProtectedMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_PROTECTED_METHOD);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_isParamVariableArguments(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    method = CLMETHOD(self)->mMethod;

    if(method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(method->mFlags & CL_METHOD_PARAM_VARABILE_ARGUMENTS);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_name(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLMethod* method;
    char* str;
    wchar_t* wstr;
    int wlen;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    str = CONS_str(&klass2->mConstPool, method->mNameOffset);

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wlen, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_path(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLMethod* method;
    char* str;
    wchar_t* wstr;
    int wlen;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    str = CONS_str(&klass2->mConstPool, method->mPathOffset);

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wlen, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_resultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLMethod* method;
    CLObject type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    type_object = create_type_object_from_cl_type(klass2, &method->mResultType, info);
    (*stack_ptr)->mObjectValue.mValue = type_object;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_blockResultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    sCLMethod* method;
    CLObject type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    if(method->mBlockType.mResultType.mClassNameOffset == 0) {
        type_object = create_type_object_with_class_name("void");
    }
    else {
        type_object = create_type_object_from_cl_type(klass2, &method->mBlockType.mResultType, info);
    }
    (*stack_ptr)->mObjectValue.mValue = type_object;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_parametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass2;
    CLObject array;
    CLObject type_object2;
    sCLMethod* method;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<method->mNumParams; i++) {
        CLObject element;
        sCLType* param;

        param = method->mParamTypes + i;

        element = create_type_object_from_cl_type(klass2, param, info);

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_blockParametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass2;
    CLObject array;
    CLObject type_object2;
    sCLMethod* method;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<method->mBlockType.mNumParams; i++) {
        CLObject element;
        sCLType* param;

        param = method->mBlockType.mParamTypes + i;

        element = create_type_object_from_cl_type(klass2, param, info);

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Method_exceptions(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass2;
    CLObject array;
    CLObject type_object2;
    sCLMethod* method;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<method->mNumException; i++) {
        CLObject element;
        char* exceptin_class_name;

        exceptin_class_name = CONS_str(&klass2->mConstPool, method->mExceptionClassNameOffset[i]);

        element = create_type_object_with_class_name(exceptin_class_name);

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

static BOOL type_checking_of_method(sCLClass* klass, sCLMethod* method, CLObject params, sVMInfo* info) 
{
    int i;

    if(method->mNumParams != CLARRAY(params)->mLen) {
        return FALSE;
    }

    for(i=0; i<method->mNumParams; i++) {
        CLObject method_param;
        CLObject param;
        CLObject type_object;

        method_param = create_type_object_from_cl_type(klass, &method->mParamTypes[i], info);
        push_object(method_param, info);

        param = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;

        type_object = CLOBJECT_HEADER(param)->mType;

        if(!substitution_posibility_of_type_object(method_param, type_object, TRUE))
        {
            pop_object(info);
            return FALSE;
        }

        pop_object(info);
    }

    return TRUE;
}

BOOL Method_invokeMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject object;
    CLObject type_params;
    CLObject params;
    CLObject result_value;
    CLObject type_object;
    int i;
    sCLClass* klass2;
    sCLMethod* method;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Method", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    object = (lvar+1)->mObjectValue.mValue;

    type_params = create_type_object_with_class_name("Array$1");
    push_object(type_params, info);
    type_object = create_type_object_with_class_name("anonymous");
    CLTYPEOBJECT(type_params)->mGenericsTypes[0] = type_object;
    CLTYPEOBJECT(type_params)->mGenericsTypesNum = 1;

    params = (lvar+2)->mObjectValue.mValue; // param

    if(!check_type(params, type_params, info)) {
        pop_object_except_top(info);
        vm_mutex_unlock();
        return FALSE;
    }

    pop_object(info);

    klass2 = CLMETHOD(self)->mClass;
    method = CLMETHOD(self)->mMethod;

    if(klass2 == NULL || method == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    /// type checking ///
    if(!type_checking_of_method(klass2, method, params, info)) {
        entry_exception_object_with_class_name(info, "Exception", "type error of method parametors");
        vm_mutex_unlock();
        return FALSE;
    }

    /// expand params to the stack ///
    if(!(method->mFlags & CL_CLASS_METHOD)) {
        (*stack_ptr)->mObjectValue.mValue = object;
        (*stack_ptr)++;
    }

    for(i=0; i<CLARRAY(params)->mLen; i++) {
        (*stack_ptr)->mObjectValue.mValue = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;
        (*stack_ptr)++;
    }

    /// excute method ///
    if(!cl_excute_method(method, klass2, klass2, info, &result_value)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result_value;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
