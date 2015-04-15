#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLClassObject);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject create_class_object(CLObject type_object, CLObject klass)
{
    unsigned int size;
    CLObject object;

    size = object_size();

    object = alloc_heap_mem(size, type_object);

    CLCLASSOBJECT(object)->mClass = klass;

    return object;
}

static CLObject create_class_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_class_object(type_object, create_null_object());

    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void mark_class_object(CLObject object, unsigned char* mark_flg)
{
    CLObject object2 = CLCLASSOBJECT(object)->mClass;
    if(object2 != 0) {
        mark_object(object2, mark_flg);
    }
}

void initialize_hidden_class_method_of_class_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_class_object;
    klass->mCreateFun = create_class_object_for_new;
}

BOOL Class_newInstance(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object;
    CLObject type_object2;
    sCLClass* klass2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    /// don't solve the generics type ///
    if(info->num_vm_types < 2 || !include_generics_param_type(type_object))
    {
        type_object2 = type_object;
    }
    /// solve the generics type ///
    else {
        if(!solve_generics_types_of_type_object(type_object, ALLOC &type_object2, info->vm_types[info->num_vm_types-2], info))
        {
            vm_mutex_unlock();
            return FALSE;
        }
    }

    push_object(type_object2, info);

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    if(klass2->mFlags & CLASS_FLAGS_SPECIAL_CLASS || is_parent_special_class(klass2))
    {
        fCreateFun create_fun;

        create_fun = klass2->mCreateFun;

        if(create_fun == NULL) {
            pop_object(info);
            entry_exception_object(info, gExceptionClass, "can't create object of this special class(%s) because of no creating object function\n", REAL_CLASS_NAME(klass2));
            vm_mutex_unlock();
            return FALSE;
        }

        new_obj = create_fun(type_object2, info);
    }
    else {
        if(!create_user_object(type_object2, &new_obj, vm_type, NULL, 0, info)) 
        {
            pop_object(info);
            entry_exception_object(info, gExceptionClass, "can't create user object\n");
            vm_mutex_unlock();
            return FALSE;
        }
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object2;
    CLObject new_obj;
    wchar_t* wstr;
    int wlen;
    char* str;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    str = REAL_CLASS_NAME(klass2);

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_string_object(wstr, wlen, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    FREE(wstr);

    return TRUE;
}

BOOL Class_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject value;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(value, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    CLCLASSOBJECT(self)->mClass = CLCLASSOBJECT(value)->mClass;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_fields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass;
    CLObject array;
    CLObject field_type_object;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    type_object = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Field", NULL, 0, info);

    push_object(array, info);

    field_type_object = create_type_object_with_class_name("Field");

    push_object(field_type_object, info);

    klass = CLTYPEOBJECT(type_object)->mClass;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;
        CLObject element;

        field = klass->mFields + i;

        element = create_field_object(field_type_object, klass, field);

        add_to_array(array, element, info);
    }

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_methods(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass;
    CLObject array;
    CLObject method_type_object;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    type_object = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Method", NULL, 0, info);

    push_object(array, info);

    method_type_object = create_type_object_with_class_name("Method");

    push_object(method_type_object, info);

    klass = CLTYPEOBJECT(type_object)->mClass;

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;
        CLObject element;

        method = klass->mMethods + i;

        element = create_method_object(method_type_object, klass, method);

        add_to_array(array, element, info);
    }

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_isSpecialClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object2;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    new_obj = create_bool_object((klass2->mFlags & CLASS_FLAGS_SPECIAL_CLASS) ? 1:0);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_isInterface(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object2;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    new_obj = create_bool_object((klass2->mFlags & CLASS_FLAGS_INTERFACE) ? 1:0);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_isAbstractClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object2;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    new_obj = create_bool_object((klass2->mFlags & CLASS_FLAGS_ABSTRACT) ? 1:0);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_isFinalClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object2;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    new_obj = create_bool_object((klass2->mFlags & CLASS_FLAGS_FINAL) ? 1:0);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_isStruct(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object2;
    sCLClass* klass2;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(type_object2)->mClass;

    new_obj = create_bool_object((klass2->mFlags & CLASS_FLAGS_STRUCT) ? 1:0);

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_superClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass;
    CLObject array;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(type_object2)->mClass;

    if(klass == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        CLObject element;
        sCLType* super_class;

        super_class = klass->mSuperClasses + i;

        element = create_type_object_from_cl_type(klass, super_class, info);

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_implementedInterfaces(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass;
    CLObject array;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(type_object2)->mClass;

    if(klass == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        CLObject element;
        sCLType* interface;

        interface = klass->mImplementedInterfaces + i;

        element = create_type_object_from_cl_type(klass, interface, info);

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_classDependences(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass;
    CLObject array;
    CLObject class_object_type_object;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(type_object2)->mClass;

    if(klass == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Class", NULL, 0, info);

    push_object(array, info);

    class_object_type_object = create_type_object_with_class_name("Class");
    push_object(class_object_type_object, info);

    for(i=0; i<klass->mNumDependences; i++) {
        CLObject element;
        char* depended_class_name;
        CLObject type_object;
        CLObject klass2;

        depended_class_name = CONS_str(&klass->mConstPool, klass->mDependencesOffset[i]);

        klass2 = create_type_object_with_class_name(depended_class_name);

        push_object(klass2, info);

        element = create_class_object(class_object_type_object, klass2);

        add_to_array(array, element, info);

        pop_object(info);
    }

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Class_toType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_type_object_from_other_type_object(type_object, info);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL create_generics_type_object(CLObject* result, sCLGenericsParamTypes* generics_param_type, CLObject vm_type, sVMInfo* info, sCLClass* klass)
{
    CLObject type_object;
    CLObject implemented_interfaces;
    CLObject extends_type;
    CLObject type_object2;
    int i;

    type_object = create_type_object_with_class_name("GenericsParametor");
    push_object(type_object, info);

    if(!create_user_object(type_object, result, vm_type, NULL, 0, info)) {
        pop_object_except_top(info);
        return FALSE;
    }

    pop_object(info);

    push_object(*result, info);

    implemented_interfaces = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(implemented_interfaces, info);

    for(i=0; i<generics_param_type->mNumImplementsTypes; i++) {
        sCLType* implements_type;
        CLObject implements_type_object;

        implements_type = generics_param_type->mImplementsTypes + i;

        implements_type_object = create_type_object_from_cl_type(klass, implements_type, info);

        add_to_array(implemented_interfaces, implements_type_object, info);
    }

    CLUSEROBJECT(*result)->mFields[0].mObjectValue.mValue = implemented_interfaces;

    pop_object(info);

    if(generics_param_type->mExtendsType.mClassNameOffset != 0) {
        extends_type = create_type_object_from_cl_type(klass, &generics_param_type->mExtendsType, info);
        CLUSEROBJECT(*result)->mFields[1].mObjectValue.mValue = extends_type;
    }
    else {
        extends_type = create_null_object();
        CLUSEROBJECT(*result)->mFields[1].mObjectValue.mValue = extends_type;
    }

    pop_object(info);

    return TRUE;
}

BOOL Class_genericsParametorTypes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    sCLClass* klass;
    CLObject array;
    CLObject type_object;
    CLObject type_object2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type_with_class_name(self, "Class", info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    
    type_object2 = CLCLASSOBJECT(self)->mClass;

    if(!check_type_with_class_name(type_object2, "Type", info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(type_object2)->mClass;

    if(klass == NULL) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        vm_mutex_unlock();
        return FALSE;
    }

    array = create_array_object_with_element_class_name("GenericsParametor", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<klass->mGenericsTypesNum; i++) {
        sCLGenericsParamTypes* generics_param_type;
        CLObject generics_param_type_object;

        generics_param_type = klass->mGenericsTypes + i;

        if(!create_generics_type_object(&generics_param_type_object, generics_param_type, vm_type, info, klass))
        {
            pop_object_except_top(info);
            return FALSE;
        }

        add_to_array(array, generics_param_type_object, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
