#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLClass* super_klass)
{
    int i;

    if(super_klass->mNumSuperClasses >= SUPER_CLASS_MAX) {
        return FALSE;
    }

    for(i=0; i<super_klass->mNumSuperClasses; i++) {
        klass->mSuperClassesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, CONS_str(&super_klass->mConstPool, super_klass->mSuperClassesOffset[i]));
    }

    klass->mSuperClassesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, REAL_CLASS_NAME(super_klass));
    klass->mNumSuperClasses = super_klass->mNumSuperClasses + 1;

    add_dependence_class(klass, super_klass);

    return TRUE;
}

// result (TRUE) --> success (FLASE) --> overflow implemented interface number
BOOL add_implemented_interface(sCLClass* klass, sCLClass* interface)
{
    if(interface->mNumImplementedInterfaces >= IMPLEMENTED_INTERFACE_MAX) {
        return FALSE;
    }

    klass->mImplementedInterfacesOffset[klass->mNumImplementedInterfaces] = append_str_to_constant_pool(&klass->mConstPool, REAL_CLASS_NAME(interface));
    klass->mNumImplementedInterfaces++;

    add_dependence_class(klass, interface);

    return TRUE;
}

static BOOL type_identity_of_cl_type(sCLClass* klass1, sCLType* type1, sCLClass* klass2, sCLType* type2)
{
    sCLNodeType node_type1;
    sCLNodeType node_type2;

    if(!cl_type_to_node_type(&node_type1, type1, NULL, klass1)) {
        return FALSE;
    }
    if(!cl_type_to_node_type(&node_type2, type2, NULL, klass2)) {
        return FALSE;
    }

    if(!type_identity(&node_type1, &node_type2)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL check_same_interface_of_two_methods(sCLClass* klass1, sCLMethod* method1, sCLClass* klass2, sCLMethod* method2)
{
    int i;
    int j;
    char* name1;
    char* name2;

    name1 = CONS_str(&klass1->mConstPool, method1->mNameOffset);
    name2 = CONS_str(&klass2->mConstPool, method2->mNameOffset);

    if(strcmp(name1, name2) != 0) {
        return FALSE;
    }

    if(!type_identity_of_cl_type(klass1, &method1->mResultType, klass2, &method2->mResultType))
    {
        return FALSE;
    }

    if(method1->mNumParams != method2->mNumParams) {
        return FALSE;
    }

    for(i=0; i<method1->mNumParams; i++) {
        if(!type_identity_of_cl_type(klass1, &method1->mParamTypes[i], klass2, &method2->mParamTypes[i]))
        {
            return FALSE;
        }
    }

    if(method1->mNumBlockType != method2->mNumBlockType) {
        return FALSE;
    }

    if(method1->mNumBlockType == 1) {
        if(!type_identity_of_cl_type(klass1, &method1->mBlockType.mResultType, klass2, &method2->mBlockType.mResultType)) 
        {
            return FALSE;
        }

        if(method1->mBlockType.mNumParams != method2->mBlockType.mNumParams) {
            return FALSE;
        }

        for(i=0; i<method1->mBlockType.mNumParams; i++) {
            if(!type_identity_of_cl_type(klass1, &method1->mBlockType.mParamTypes[i], klass2, &method2->mBlockType.mParamTypes[i]))
            {
                return FALSE;
            }
        }
    }

    if(method1->mNumException != method2->mNumException) {
        return FALSE;
    }

    for(i=0; i<method1->mNumException; i++) {
        sCLClass* exception_class1;
        char* real_class_name;

        real_class_name = CONS_str(&klass1->mConstPool, method1->mExceptionClassNameOffset[i]);
        exception_class1 = cl_get_class(real_class_name);
        if(exception_class1 == NULL) {
            return FALSE;
        }

        for(j=0; j<method2->mNumException; j++) {
            sCLClass* exception_class2;
            char* real_class_name;

            real_class_name = CONS_str(&klass2->mConstPool, method2->mExceptionClassNameOffset[j]);

            exception_class2 = cl_get_class(real_class_name);

            if(exception_class2 == NULL) {
                return FALSE;
            }

            if(exception_class1 == exception_class2) {
                break;
            }
        }

        if(j == method2->mNumException) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL check_implemented_interface_core(sCLClass* klass, sCLClass* interface)
{
    int i;

    for(i=interface->mNumSuperClasses-1; i>=0; i--) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&interface->mConstPool, interface->mSuperClassesOffset[i]);

        super = cl_get_class(real_class_name);

        ASSERT(super != NULL);

        if(!check_implemented_interface_core(klass, super)) {
            return FALSE;
        }
    }

    if(!check_implemented_interface_core(klass, interface)) {
        return FALSE;
    }

    return TRUE;
}

BOOL check_implemented_interface(sCLClass* klass, sCLClass* interface)
{
    int i, j;

    for(i=0; i<interface->mNumMethods; i++) {
        sCLMethod* method;

        method = interface->mMethods + i;

        for(j=0; j<klass->mNumMethods; j++) {
            sCLMethod* method2;

            method2 = klass->mMethods + j;

            if(check_same_interface_of_two_methods(interface, method, klass, method2)) 
            {
                break;
            }
        }

        if(j == klass->mNumMethods) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL check_implemented_abstract_methods_on_the_super_class(sCLClass* klass, sCLClass* super_class)
{
    int i, j;

    for(i=0; i<super_class->mNumMethods; i++) {
        sCLMethod* method;

        method = super_class->mMethods + i;

        if(method->mFlags & CL_ABSTRACT_METHOD) {
            for(j=0; j<klass->mNumMethods; j++) {
                sCLMethod* method2;

                method2 = klass->mMethods + j;

                if(check_same_interface_of_two_methods(super_class, method, klass, method2))
                {
                    break;
                }
            }

            if(j == klass->mNumMethods) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL check_implemented_abstract_methods(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(!check_implemented_abstract_methods_on_the_super_class(klass, super_class))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL is_already_contained_on_dependeces(sCLClass* klass, sCLClass* dependence_class)
{
    int i;

    for(i=0; i<klass->mNumDependences; i++) {
        if(strcmp(REAL_CLASS_NAME(dependence_class), CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i])) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

void add_dependence_class(sCLClass* klass, sCLClass* dependence_class)
{
    int i;

    if(is_already_contained_on_dependeces(klass, dependence_class)) return;

    if(klass->mNumDependences == klass->mSizeDependences) {
        int new_size;

        new_size = klass->mSizeDependences * 2;

        klass->mDepedencesOffset = REALLOC(klass->mDepedencesOffset, sizeof(int)*new_size);
        memset(klass->mDepedencesOffset + klass->mSizeDependences, 0, sizeof(int)*(new_size-klass->mSizeDependences));

        klass->mSizeDependences = new_size;
    }

    klass->mDepedencesOffset[klass->mNumDependences] = append_str_to_constant_pool(&klass->mConstPool, REAL_CLASS_NAME(dependence_class));
    klass->mNumDependences++;
}

BOOL is_parent_special_class(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class->mFlags & CLASS_FLAGS_SPECIAL_CLASS) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL is_parent_immediate_value_class(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2)
{
    int i;

    if(type1->mClass != type2->mClass) {
        return FALSE;
    }

    if(type1->mGenericsTypesNum != type2->mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<type1->mGenericsTypesNum; i++) {
        if(type1->mGenericsTypes[i] != type2->mGenericsTypes[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    if(!type_identity(left_type, right_type)) {
        return FALSE;
    }

    return TRUE;
}


//////////////////////////////////////////////////
// fields
//////////////////////////////////////////////////
// result is seted on this parametors(sCLNodeType* result)
// if the field is not found, result->mClass is setted on NULL
void get_field_type(sCLClass* klass, sCLField* field, sCLNodeType* result, sCLNodeType* type_)
{
    if(field) {
        int i;

        result->mClass = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));

        ASSERT(result->mClass != NULL);

        if(type_) {
            if(!solve_generics_types(result->mClass, type_, &result->mClass)) {
                memset(result, 0, sizeof(sCLNodeType));
                return;
            }
        }

        result->mGenericsTypesNum = field->mType.mGenericsTypesNum;

        for(i=0; i<field->mType.mGenericsTypesNum; i++) {
            result->mGenericsTypes[i] = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mGenericsTypesOffset[i]));

            ASSERT(result->mGenericsTypes[i] != NULL);

            if(type_) {
                if(!solve_generics_types(result->mGenericsTypes[i], type_, &result->mGenericsTypes[i])) {
                    memset(result, 0, sizeof(sCLNodeType));
                    return;
                }
            }
        }
    }
    else {
        memset(result, 0, sizeof(sCLNodeType));
    }
}

// result: (-1) --> not found (non -1) --> field index
static int get_field_index_without_class_field(sCLClass* klass, char* field_name)
{
    int i;
    int static_field_num;

    static_field_num = 0;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;

        field = klass->mFields + i;

        if(field->mFlags & CL_STATIC_FIELD) {
            static_field_num++;
        }
        else {
            if(strcmp(FIELD_NAME(klass, i), field_name) == 0) {
                return i - static_field_num;
            }
        }
    }

    return -1;
}

static int get_static_fields_num_on_super_class(sCLClass* klass)
{
    int i;
    int static_field_num;

    static_field_num = 0;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        static_field_num += get_static_fields_num(super_class);
    }

    return static_field_num;
}

static int get_sum_of_fields_on_super_clasess(sCLClass* klass)
{
    int sum = 0;
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        sum += super_class->mNumFields;
    }

    return sum;
}

static int get_sum_of_fields_on_super_clasess_without_class_fields(sCLClass* klass)
{
    return get_sum_of_fields_on_super_clasess(klass) - get_static_fields_num_on_super_class(klass);
}

// result: (-1) --> not found (non -1) --> field index
int get_field_index_including_super_classes_without_class_field(sCLClass* klass, char* field_name)
{
    int i;
    int field_index;
    int static_field_num;

    static_field_num = 0;
    
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        int field_index;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        field_index = get_field_index_without_class_field(super_class, field_name);

        if(field_index >= 0) {
            return get_sum_of_fields_on_super_clasess_without_class_fields(super_class) + field_index;
        }
    }

    field_index = get_field_index_without_class_field(klass, field_name);

    if(field_index >= 0) {
        return get_sum_of_fields_on_super_clasess_without_class_fields(klass) + field_index;
    }
    else {
        return -1;
    }
}

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, char* field_name, BOOL class_field)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;
        BOOL class_field2;

        field = klass->mFields + i;
        class_field2 = field->mFlags & CL_STATIC_FIELD;

        if(strcmp(FIELD_NAME(klass, i), field_name) == 0 && class_field == class_field2) {
            return i;
        }
    }

    return -1;
}

// result: (NULL) --> not found (non NULL) --> field
// also return the class in which is found the the field 
sCLField* get_field_including_super_classes(sCLClass* klass, char* field_name, sCLClass** founded_class, BOOL class_field)
{
    sCLField* field;
    int i;

    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        field = get_field(super_class, field_name, class_field);

        if(field) { 
            *founded_class = super_class; 
            return field; 
        }
    }

    field = get_field(klass, field_name, class_field);

    if(field) { 
        *founded_class = klass;
        return field;
    }
    else {
        *founded_class = NULL;
        return NULL;
    }
}

// result: (-1) --> not found (non -1) --> field index
int get_field_index_including_super_classes(sCLClass* klass, char* field_name, BOOL class_field)
{
    int i;
    int field_index;
    
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        int field_index;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        field_index = get_field_index(super_class, field_name, class_field);

        if(field_index >= 0) {
            return get_sum_of_fields_on_super_clasess(super_class) + field_index;
        }
    }

    field_index = get_field_index(klass, field_name, class_field);

    if(field_index >= 0) {
        return get_sum_of_fields_on_super_clasess(klass) + field_index;
    }
    else {
        return -1;
    }
}

// result (TRUE) --> success (FALSE) --> can't find a field which is indicated by an argument
BOOL add_field_initializer(sCLClass* klass, BOOL static_, char* name, MANAGED sByteCode initializer_code, sVarTable* lv_table, int max_stack)
{
    sCLField* field;

    field = get_field(klass, name, static_);

    if(field == NULL) {
        sByteCode_free(&initializer_code);
        return FALSE;
    }

    field->mInitializer = MANAGED initializer_code;
    field->mInitializerLVNum = lv_table->mVarNum + lv_table->mMaxBlockVarNum;
    field->mInitializerMaxStack = max_stack;
    
    return TRUE;
}

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, char* name, sCLNodeType* type_)
{
    sCLField* field;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    int i;
    
    if(klass->mNumFields >= CL_FIELDS_MAX) {
        return FALSE;
    }
    if(klass->mNumFields >= klass->mSizeFields) {
        int new_size;
        
        new_size = klass->mSizeFields * 2;
        klass->mFields = REALLOC(klass->mFields, sizeof(sCLField)*new_size);
        memset(klass->mFields + klass->mSizeFields, 0, sizeof(sCLField)*(new_size-klass->mSizeFields));
        klass->mSizeFields = new_size;
    }

    field = klass->mFields + klass->mNumFields;

    field->mFlags = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

    field->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name);    // field name

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_->mClass), CLASS_NAME(type_->mClass));
    field->mType.mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
    add_dependence_class(klass, type_->mClass);

    field->mType.mGenericsTypesNum = type_->mGenericsTypesNum;

    for(i=0; i<field->mType.mGenericsTypesNum; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_->mGenericsTypes[i]), CLASS_NAME(type_->mGenericsTypes[i]));
        field->mType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
        add_dependence_class(klass, type_->mGenericsTypes[i]);
    }

    klass->mNumFields++;
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// methods
///////////////////////////////////////////////////////////////////////////////
// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
    int i;

    if(start_point < klass->mNumMethods) {
        for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
            sCLMethod* method;
            
            method = klass->mMethods + i;

            if(check_method_params(method, klass, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type))
            {
                return method;
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        sCLMethod* method;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        method = get_method_with_type_params(super_class, method_name, class_params, num_params, search_for_class_method, type_, super_class->mNumMethods-1, block_num, block_num_params, block_param_type, block_type);

        if(method) {
            *founded_class = super_class;
            return method;
        }
    }

    *founded_class = NULL;

    return NULL;
}

static BOOL check_method_params_with_param_initializer(sCLMethod* method, sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer)
{
    *used_param_num_with_initializer = 0;

    if(strcmp(METHOD_NAME2(klass, method), method_name) == 0) {
        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) {
            /// type checking ///
            if(method->mNumParams == -1) {              // no type checking of method params
                return TRUE;
            }
            else if(num_params == method->mNumParams) {
                int j, k;

                for(j=0; j<num_params; j++ ) {
                    sCLNodeType param;

                    memset(&param, 0, sizeof(param));

                    if(!cl_type_to_node_type(&param, &method->mParamTypes[j], type_, klass)) {
                        return FALSE;
                    }

                    if(!substition_posibility(&param, &class_params[j])) {
                        return FALSE;
                    }
                }

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                    if(block_num > 0) {
                        sCLNodeType result_block_type;

                        memset(&result_block_type, 0, sizeof(result_block_type));

                        if(!cl_type_to_node_type(&result_block_type, &method->mBlockType.mResultType, type_, klass)) {
                            return FALSE;
                        }

                        if(!substition_posibility(&result_block_type, block_type)) {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        sCLNodeType param;

                        memset(&param, 0, sizeof(param));

                        if(!cl_type_to_node_type(&param, &method->mBlockType.mParamTypes[k], type_ , klass)) {
                            return FALSE;
                        }

                        if(!substition_posibility(&param, &block_param_type[k])) {
                            return FALSE;
                        }
                    }

                    return TRUE;
                }
            }
            else if(num_params < method->mNumParams && num_params+method->mNumParamInitializer >= method->mNumParams)
            {
                int j, k;

                for(j=0; j<num_params; j++) {
                    sCLNodeType param;

                    memset(&param, 0, sizeof(param));

                    if(!cl_type_to_node_type(&param, &method->mParamTypes[j], type_, klass)) {
                        return FALSE;
                    }

                    if(!substition_posibility(&param, &class_params[j])) {
                        return FALSE;
                    }
                }

                *used_param_num_with_initializer = method->mNumParams - num_params;

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {

                    if(block_num > 0) {
                        sCLNodeType result_block_type;

                        memset(&result_block_type, 0, sizeof(result_block_type));

                        if(!cl_type_to_node_type(&result_block_type, &method->mBlockType.mResultType, type_, klass)) {
                            return FALSE;
                        }

                        if(!substition_posibility(&result_block_type, block_type)) {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        sCLNodeType param;

                        memset(&param, 0, sizeof(param));

                        if(!cl_type_to_node_type(&param, &method->mBlockType.mParamTypes[k], type_ , klass)) {
                            return FALSE;
                        }

                        if(!substition_posibility(&param, &block_param_type[k])) {
                            return FALSE;
                        }
                    }

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer)
{
    int i;

    if(start_point < klass->mNumMethods) {
        for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
            sCLMethod* method;
            
            method = klass->mMethods + i;

            if(check_method_params_with_param_initializer(method, klass, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type, used_param_num_with_initializer))
            {
                return method;
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        sCLMethod* method;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        method = get_method_with_type_params_and_param_initializer(super_class, method_name, class_params, num_params, search_for_class_method, type_, super_class->mNumMethods-1, block_num, block_num_params, block_param_type, block_type, used_param_num_with_initializer);

        if(method) {
            *founded_class = super_class;
            return method;
        }
    }

    *founded_class = NULL;

    return NULL;
}

// result: (FALSE) can't solve a generics type (TRUE) success
// if type_ is NULL, don't solve generics type
BOOL get_result_type_of_method(sCLClass* klass, sCLMethod* method, sCLNodeType* result, sCLNodeType* type_)
{
    char* real_class_name;
    int i;
    
    real_class_name = CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset);
    result->mClass = cl_get_class(real_class_name);

    if(result->mClass == NULL) {
        return FALSE;
    }

    if(type_) {
        if(!solve_generics_types(result->mClass, type_, &result->mClass)) {
            return FALSE;
        }
    }

    result->mGenericsTypesNum = method->mResultType.mGenericsTypesNum;

    for(i=0; i<method->mResultType.mGenericsTypesNum; i++) {
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, method->mResultType.mGenericsTypesOffset[i]);
        result->mGenericsTypes[i] = cl_get_class(real_class_name);

        if(result->mGenericsTypes[i] == NULL) {
            return FALSE;
        }

        if(type_) {
            if(!solve_generics_types(result->mGenericsTypes[i], type_, &result->mGenericsTypes[i])) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL add_method_to_virtual_method_table_core(sCLClass* klass, char* real_method_name, int method_index)
{
    int hash;
    sVMethodMap* item;

    hash = get_hash(real_method_name) % klass->mSizeVirtualMethodMap;

    item = klass->mVirtualMethodMap + hash;

    while(1) {
        if(item->mMethodName[0] == 0) {
            xstrncpy(item->mMethodName, real_method_name, CL_VMT_NAME_MAX);
            item->mMethodIndex = method_index;
            break;
        }
        else {
            /// for mixin, we want to order greater index to get first when searching
            if(method_index > item->mMethodIndex) {
                sVMethodMap item2;

                item2 = *item;

                xstrncpy(item->mMethodName, real_method_name, CL_VMT_NAME_MAX);
                item->mMethodIndex = method_index;

                xstrncpy(real_method_name, item2.mMethodName, CL_VMT_NAME_MAX);
                method_index = item2.mMethodIndex;
            }

            item++;

            if(item == klass->mVirtualMethodMap + klass->mSizeVirtualMethodMap) {
                item = klass->mVirtualMethodMap;
            }
            else if(item == klass->mVirtualMethodMap + hash) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL resizse_vmm(sCLClass* klass)
{
    sVMethodMap* vmm_before;
    int size_vmm_before;
    int i;
    
    vmm_before = klass->mVirtualMethodMap;
    size_vmm_before = klass->mSizeVirtualMethodMap;

    klass->mSizeVirtualMethodMap = klass->mNumVirtualMethodMap * 4;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);
    
    /// rehash ///
    for(i=0; i<size_vmm_before; i++) {
        if(vmm_before[i].mMethodName[0] != 0) {
            if(!add_method_to_virtual_method_table_core(klass, vmm_before[i].mMethodName, vmm_before[i].mMethodIndex))
            {
                FREE(vmm_before);
                return FALSE;
            }
        }
    }

    FREE(vmm_before);

    return TRUE;
}

// result (TRUE) --> success (FALSE) --> overflow method table number
static BOOL add_method_to_virtual_method_table(sCLClass* klass, char* method_name, int method_index, int num_params)
{
    int hash;
    char real_method_name[CL_VMT_NAME_MAX+1];
    sVMethodMap* item;

    if(klass->mSizeVirtualMethodMap <= klass->mNumVirtualMethodMap * 2) {
        /// rehash ///
        if(!resizse_vmm(klass)) {
            return FALSE;
        }
    }

    create_real_method_name(real_method_name, CL_VMT_NAME_MAX, method_name, num_params);

    if(!add_method_to_virtual_method_table_core(klass, real_method_name, method_index))
    {
        return FALSE;
    }

    klass->mNumVirtualMethodMap++;

    return TRUE;
}

static void create_method_path(char* result, int result_size, sCLMethod* method, sCLClass* klass)
{
    int i;

    xstrncpy(result, CLASS_NAME(klass), result_size);
    xstrncat(result, ".", result_size);
    xstrncat(result, METHOD_NAME2(klass, method), result_size);

    xstrncat(result, "(", result_size);

    for(i=0; i<method->mNumParams; i++) {
        xstrncat(result, CONS_str(&klass->mConstPool, method->mParamTypes[i].mClassNameOffset), result_size);

        if(i != method->mNumParams-1) xstrncat(result, ",", result_size);
    }

    xstrncat(result, ")", result_size);

    for(i=0; i<method->mNumBlockType; i++) {
        int j;

        xstrncat(result, CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset), result_size);

        xstrncat(result, "{", result_size);

        for(j=0; j<method->mBlockType.mNumParams; j++) {
            xstrncat(result, CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset), result_size);
            if(j != method->mBlockType.mNumParams-1) {
                xstrncat(result, ",", result_size);
            }
        }

        xstrncat(result, "}", result_size);
    }
}

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
// last parametor returns the method which is added
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, MANAGED sByteCode* code_params, int* max_stack_params, int* lv_num_params, int num_params, BOOL constructor, sCLMethod** method, int block_num, char* block_name, sCLNodeType* bt_result_type, sCLNodeType* bt_class_params, int bt_num_params)
{
    int i, j;
    int path_max;
    char* buf;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLBlockType* block_type;

    if(klass->mNumMethods >= CL_METHODS_MAX) {
        return FALSE;
    }
    if(klass->mNumMethods >= klass->mSizeMethods) {
        const int new_size = klass->mSizeMethods * 2;
        klass->mMethods = REALLOC(klass->mMethods, sizeof(sCLMethod)*new_size);
        memset(klass->mMethods + klass->mSizeMethods, 0, sizeof(sCLMethod)*(new_size-klass->mSizeMethods));
        klass->mSizeMethods = new_size;
    }

    *method = klass->mMethods + klass->mNumMethods;
    (*method)->mFlags = (static_ ? CL_CLASS_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0) | (synchronized_ ? CL_SYNCHRONIZED_METHOD:0) | (constructor ? CL_CONSTRUCTOR:0) | (virtual_ ? CL_VIRTUAL_METHOD:0) | (abstract_ ? CL_ABSTRACT_METHOD:0);

    (*method)->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name);

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mClass), CLASS_NAME(result_type->mClass));
    (*method)->mResultType.mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
    add_dependence_class(klass, result_type->mClass);

    (*method)->mResultType.mGenericsTypesNum = result_type->mGenericsTypesNum;
    for(i=0; i<(*method)->mResultType.mGenericsTypesNum; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mGenericsTypes[i]), CLASS_NAME(result_type->mGenericsTypes[i]));

        (*method)->mResultType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
        add_dependence_class(klass, result_type->mGenericsTypes[i]);
    }

    if(num_params > 0) {
        int num_param_initializer;

        (*method)->mParamTypes = CALLOC(1, sizeof(sCLType)*num_params);

        for(i=0; i<num_params; i++) {
            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mClass), CLASS_NAME(class_params[i].mClass));
            (*method)->mParamTypes[i].mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
            add_dependence_class(klass, class_params[i].mClass);

            (*method)->mParamTypes[i].mGenericsTypesNum = class_params[i].mGenericsTypesNum;

            for(j=0; j<(*method)->mParamTypes[i].mGenericsTypesNum; j++) {
                create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mGenericsTypes[j]), CLASS_NAME(class_params[i].mGenericsTypes[j]));
                (*method)->mParamTypes[i].mGenericsTypesOffset[j] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
                add_dependence_class(klass, class_params[i].mGenericsTypes[j]);
            }
        }

        (*method)->mParamInitializers = CALLOC(1, sizeof(sCLParamInitializer)*num_params);

        num_param_initializer = 0;

        for(i=0; i<num_params; i++) {
            if(code_params[i].mCode) num_param_initializer++;

            (*method)->mParamInitializers[i].mInitializer = MANAGED code_params[i];
            (*method)->mParamInitializers[i].mMaxStack = max_stack_params[i];
            (*method)->mParamInitializers[i].mLVNum = lv_num_params[i];
        }

        (*method)->mNumParams = num_params;
        (*method)->mNumParamInitializer = num_param_initializer;
    }
    else {
        (*method)->mParamTypes = NULL;
        (*method)->mParamInitializers = NULL;
        (*method)->mNumParams = 0;
        (*method)->mNumParamInitializer = 0;
    }

    (*method)->mNumLocals = 0;

    (*method)->mNumBlockType = 0;
    memset(&(*method)->mBlockType, 0, sizeof((*method)->mBlockType));

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    if(!add_method_to_virtual_method_table(klass, name, klass->mNumMethods, num_params)) 
    {
        return FALSE;
    }

    /// add block type ///
    if(block_num) {
        (*method)->mNumBlockType++;
        ASSERT((*method)->mNumBlockType == 1);

        /// block type result type ///
        block_type = &(*method)->mBlockType;

        block_type->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, block_name);

        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_result_type->mClass), CLASS_NAME(bt_result_type->mClass));
        block_type->mResultType.mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
        add_dependence_class(klass, bt_result_type->mClass);

        block_type->mResultType.mGenericsTypesNum = bt_result_type->mGenericsTypesNum;
        for(i=0; i<block_type->mResultType.mGenericsTypesNum; i++) {
            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_result_type->mGenericsTypes[i]), CLASS_NAME(bt_result_type->mGenericsTypes[i]));
            block_type->mResultType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
            add_dependence_class(klass, bt_result_type->mGenericsTypes[i]);
        }

        /// param types //
        block_type->mNumParams = bt_num_params;
        if(block_type->mNumParams > 0) {
            block_type->mParamTypes = CALLOC(1, sizeof(sCLType)*bt_num_params);
        }
        else {
            block_type->mParamTypes = NULL;
        }

        for(i=0; i<bt_num_params; i++) {
            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_class_params[i].mClass), CLASS_NAME(bt_class_params[i].mClass));
            block_type->mParamTypes[i].mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
            add_dependence_class(klass, bt_class_params[i].mClass);

            block_type->mParamTypes[i].mGenericsTypesNum = bt_class_params[i].mGenericsTypesNum;

            for(j=0; j<block_type->mParamTypes[i].mGenericsTypesNum; j++) {
                create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_class_params[i].mGenericsTypes[j]), CLASS_NAME(bt_class_params[i].mGenericsTypes[j]));
                block_type->mParamTypes[i].mGenericsTypesOffset[j] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
                add_dependence_class(klass, bt_class_params[i].mGenericsTypes[j]);
            }
        }
    }

    /// make method path ///
    path_max = CL_METHOD_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX + CL_REAL_CLASS_NAME_MAX + CL_REAL_CLASS_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX;
    buf = CALLOC(1, sizeof(char)*(path_max+1));;

    create_method_path(buf, path_max, *method, klass);

    (*method)->mPathOffset = append_str_to_constant_pool(&klass->mConstPool, buf);

    FREE(buf);

    klass->mNumMethods++;

    return TRUE;
}

/*
void add_block_type_to_method(sCLClass* klass, sCLMethod* method, char* block_name, sCLNodeType* bt_result_type, sCLNodeType bt_class_params[], int bt_num_params)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLBlockType* block_type;
    int i;
    int j;

    method->mNumBlockType++;
    ASSERT(method->mNumBlockType == 1);

    /// block type result type ///
    block_type = &method->mBlockType;

    block_type->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, block_name);

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_result_type->mClass), CLASS_NAME(bt_result_type->mClass));
    block_type->mResultType.mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    block_type->mResultType.mGenericsTypesNum = bt_result_type->mGenericsTypesNum;
    for(i=0; i<block_type->mResultType.mGenericsTypesNum; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_result_type->mGenericsTypes[i]), CLASS_NAME(bt_result_type->mGenericsTypes[i]));
        block_type->mResultType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
    }

    /// param types //
    block_type->mNumParams = bt_num_params;
    if(block_type->mNumParams > 0) {
        block_type->mParamTypes = CALLOC(1, sizeof(sCLType)*bt_num_params);
    }
    else {
        block_type->mParamTypes = NULL;
    }

    for(i=0; i<bt_num_params; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_class_params[i].mClass), CLASS_NAME(bt_class_params[i].mClass));
        block_type->mParamTypes[i].mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

        block_type->mParamTypes[i].mGenericsTypesNum = bt_class_params[i].mGenericsTypesNum;

        for(j=0; j<block_type->mParamTypes[i].mGenericsTypesNum; j++) {
            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(bt_class_params[i].mGenericsTypes[j]), CLASS_NAME(bt_class_params[i].mGenericsTypes[j]));
            block_type->mParamTypes[i].mGenericsTypesOffset[j] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
        }
    }
}
*/

// result: (TRUE) success (FALSE) overflow exception number
BOOL add_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class)
{
    char* real_class_name;

    if(method->mNumException >= CL_METHOD_EXCEPTION_MAX) {
        return FALSE;
    }

    real_class_name = REAL_CLASS_NAME(exception_class);
    method->mExceptionClassNameOffset[method->mNumException++] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    add_dependence_class(klass, exception_class);

    return TRUE;
}

BOOL is_method_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class)
{
    int i;


    for(i=0; i<method->mNumException; i++) {
        char* real_class_name;
        sCLClass* klass2;

        real_class_name = CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[i]);
        klass2 = cl_get_class(real_class_name);

        ASSERT(klass2 != NULL);      // checked on load time

        if(substition_posibility_of_class(klass2, exception_class)) {
            return TRUE;
        }
    }

    return FALSE;
}

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, char* method_name)
{
    int i;
    for(i=klass->mNumMethods-1; i>=0; i--) {                    // search for method in reverse because we want to get last defined method
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return klass->mMethods + i;
        }
    }

    return NULL;
}
// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** founded_class)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        sCLMethod* method;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        method = get_method(super_class, method_name);

        if(method) {
            *founded_class = super_class;
            return method;
        }
    }

    *founded_class = NULL;

    return NULL;
}

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_from_index(sCLClass* klass, int method_index)
{
    if(method_index < 0 || method_index >= klass->mNumMethods) {
        return NULL;
    }

    return klass->mMethods + method_index;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, sCLMethod* method)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method2;
        
        method2 = klass->mMethods + i;
        if(method2 == method) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index_from_the_parametor_point(sCLClass* klass, char* method_name, int method_index, BOOL search_for_class_method)
{
    int i;
    if(method_index < 0 ||method_index >= klass->mNumMethods) {
        return -1;
    }

    for(i=method_index; i>=0; i--) {                      // search for method in reverse because we want to get last defined method
        sCLMethod* method;

        method = klass->mMethods + i;

        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) {
            if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
                return i;
            }
        }
    }

    return -1;
}

void get_param_type_of_method(sCLClass* klass, sCLMethod* method, int param_num, sCLNodeType* result)
{
    if(klass != NULL && method != NULL) {
        if(param_num >= 0 && param_num < method->mNumParams && method->mParamTypes != NULL) {
            char* real_class_name;
            int i;
            
            real_class_name = CONS_str(&klass->mConstPool, method->mParamTypes[param_num].mClassNameOffset);
            result->mClass = cl_get_class(real_class_name);

            ASSERT(result->mClass != NULL);

            result->mGenericsTypesNum = method->mParamTypes[param_num].mGenericsTypesNum;

            for(i=0; i<result->mGenericsTypesNum; i++) {
                char* real_class_name = CONS_str(&klass->mConstPool, method->mParamTypes[param_num].mGenericsTypesOffset[i]);
                result->mGenericsTypes[i] = cl_get_class(real_class_name);

                ASSERT(result->mGenericsTypes[i] != NULL);
            }
        }
    }
    else {
        result->mClass = NULL;
        result->mGenericsTypesNum = 0;
    }
}

// return method parametor number
int get_method_num_params(sCLMethod* method)
{
    return method->mNumParams;
}

//////////////////////////////////////////////////
// save class on compiler
//////////////////////////////////////////////////
static void write_char_value_to_buffer(sBuf* buf, char value)
{
    sBuf_append_char(buf, value);
}

static void write_int_value_to_buffer(sBuf* buf, int value)
{
    sBuf_append(buf, &value, sizeof(int));
}

static void write_type_to_buffer(sBuf* buf, sCLType* type)
{
    int j;

    write_int_value_to_buffer(buf, type->mClassNameOffset);

    write_char_value_to_buffer(buf, type->mGenericsTypesNum);
    for(j=0; j<type->mGenericsTypesNum; j++) {
        write_int_value_to_buffer(buf, type->mGenericsTypesOffset[j]);
    }
}

static void write_params_to_buffer(sBuf* buf, int num_params, sCLType* param_types)
{
    int j;

    write_int_value_to_buffer(buf, num_params);
    for(j=0; j<num_params; j++) {
        write_type_to_buffer(buf, param_types + j);
    }
}

static void write_param_initializers_to_buffer(sBuf* buf, int num_params, sCLParamInitializer* param_initilizer)
{
    int j;

    write_int_value_to_buffer(buf, num_params);
    for(j=0; j<num_params; j++) {
        write_int_value_to_buffer(buf, param_initilizer[j].mInitializer.mLen);
        if(param_initilizer[j].mInitializer.mLen > 0) {
            sBuf_append(buf, param_initilizer[j].mInitializer.mCode, sizeof(int)*param_initilizer[j].mInitializer.mLen);
        }
        write_int_value_to_buffer(buf, param_initilizer[j].mMaxStack);
        write_int_value_to_buffer(buf, param_initilizer[j].mLVNum);
    }
}

static void write_block_type_to_buffer(sBuf* buf, sCLBlockType* block_type)
{
    write_type_to_buffer(buf, &block_type->mResultType);
    write_int_value_to_buffer(buf, block_type->mNameOffset);
    write_params_to_buffer(buf, block_type->mNumParams, block_type->mParamTypes);
}

static void write_field_to_buffer(sBuf* buf, sCLField* field)
{
    write_int_value_to_buffer(buf, field->mFlags);
    write_int_value_to_buffer(buf, field->mNameOffset);
    //write_mvalue_to_buffer(buf, field->uValue.mStaticField);

    write_int_value_to_buffer(buf, field->mInitializer.mLen);
    if(field->mInitializer.mLen > 0) {
        sBuf_append(buf, field->mInitializer.mCode, sizeof(int)*field->mInitializer.mLen);
    }

    write_int_value_to_buffer(buf, field->mInitializerLVNum);
    write_int_value_to_buffer(buf, field->mInitializerMaxStack);

    write_type_to_buffer(buf, &field->mType);
}

static void write_method_to_buffer(sBuf* buf, sCLMethod* method)
{
    int i;

    write_int_value_to_buffer(buf, method->mFlags);
    write_int_value_to_buffer(buf, method->mNameOffset);

    write_int_value_to_buffer(buf, method->mPathOffset);

    if(method->mFlags & CL_NATIVE_METHOD) {
    }
    else {
        write_int_value_to_buffer(buf, method->uCode.mByteCodes.mLen);
        if(method->uCode.mByteCodes.mLen > 0) {
            sBuf_append(buf, method->uCode.mByteCodes.mCode, sizeof(int)*method->uCode.mByteCodes.mLen);
        }
    }

    write_type_to_buffer(buf, &method->mResultType);
    write_params_to_buffer(buf, method->mNumParams, method->mParamTypes);
    write_param_initializers_to_buffer(buf, method->mNumParams, method->mParamInitializers);


    write_int_value_to_buffer(buf, method->mNumLocals);
    write_int_value_to_buffer(buf, method->mMaxStack);

    write_int_value_to_buffer(buf, method->mNumBlockType);
    if(method->mNumBlockType >= 1) {
        write_block_type_to_buffer(buf, &method->mBlockType);
    }

    write_int_value_to_buffer(buf, method->mNumException);
    for(i=0; i<method->mNumException; i++) {
        write_int_value_to_buffer(buf, method->mExceptionClassNameOffset[i]);
    }

    write_char_value_to_buffer(buf, method->mNumParamInitializer);
}

static void write_virtual_method_map(sBuf* buf, sCLClass* klass)
{
    write_int_value_to_buffer(buf, klass->mSizeVirtualMethodMap);
    write_int_value_to_buffer(buf, klass->mNumVirtualMethodMap);

    sBuf_append(buf, klass->mVirtualMethodMap, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);
}

static void write_class_to_buffer(sCLClass* klass, sBuf* buf)
{
    int i;

    write_int_value_to_buffer(buf, klass->mFlags);

    /// save constant pool ///
    write_int_value_to_buffer(buf, klass->mConstPool.mLen);
    sBuf_append(buf, klass->mConstPool.mConst, klass->mConstPool.mLen);

    /// save class name offset
    write_char_value_to_buffer(buf, klass->mNameSpaceOffset);
    write_char_value_to_buffer(buf, klass->mClassNameOffset);
    write_char_value_to_buffer(buf, klass->mRealClassNameOffset);

    /// save fields
    write_int_value_to_buffer(buf, klass->mNumFields);
    for(i=0; i<klass->mNumFields; i++) {
        write_field_to_buffer(buf, klass->mFields + i);
    }

    /// save methods
    write_int_value_to_buffer(buf, klass->mNumMethods);
    for(i=0; i<klass->mNumMethods; i++) {
        write_method_to_buffer(buf, klass->mMethods + i);
    }

    /// write super classes ///
    write_char_value_to_buffer(buf, klass->mNumSuperClasses);
    for(i=0; i<klass->mNumSuperClasses; i++) {
        write_int_value_to_buffer(buf, klass->mSuperClassesOffset[i]);
    }

    /// write implement interfaces ///
    write_char_value_to_buffer(buf, klass->mNumImplementedInterfaces);
    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        write_int_value_to_buffer(buf, klass->mImplementedInterfacesOffset[i]);
    }

    /// write class params name of generics ///
    write_char_value_to_buffer(buf, klass->mGenericsTypesNum);
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        write_int_value_to_buffer(buf, klass->mGenericsTypesOffset[i]);
    }

    /// write dependences ///
    write_int_value_to_buffer(buf, klass->mNumDependences);

    for(i=0; i<klass->mNumDependences; i++) {
        write_int_value_to_buffer(buf, klass->mDepedencesOffset[i]);
    }

    /// write virtual method table ///
    write_virtual_method_map(buf, klass);
}

// (FALSE) --> failed to write (TRUE) --> success
static BOOL save_class(sCLClass* klass)
{
    char file_name[PATH_MAX];
    unsigned char version;
    sBuf buf;
    char magic_number[16];
    int f;
    unsigned int total_size;
    
    version = CLASS_VERSION(klass);
    if(version == 0) {
        return FALSE;
    }
    else if(version == 1) {
        snprintf(file_name, PATH_MAX, "%s.clo", REAL_CLASS_NAME(klass));
    }
    else {
        snprintf(file_name, PATH_MAX, "%s#%d.clo", REAL_CLASS_NAME(klass), CLASS_VERSION(klass));
    }

    sBuf_init(&buf);

    /// write magic number ///
    magic_number[0] = 12;
    magic_number[1] = 17;
    magic_number[2] = 33;
    magic_number[3] = 79;

    strcpy(magic_number + 4, "CLOVER");
    sBuf_append(&buf, magic_number, sizeof(char)*10);

    write_class_to_buffer(klass, &buf);

    /// write ///
    f = open(file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    total_size = 0;
    while(total_size < buf.mLen) {
        int size;

        if(buf.mLen - total_size < BUFSIZ) {
            size = write(f, buf.mBuf + total_size, buf.mLen - total_size);
        }
        else {
            size = write(f, buf.mBuf + total_size, BUFSIZ);
        }
        if(size < 0) {
            FREE(buf.mBuf);
            close(f);
            return FALSE;
        }

        total_size += size;
    }
    close(f);

    FREE(buf.mBuf);


    return TRUE;
}

void save_all_modified_class()
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                if(klass->mFlags & CLASS_FLAGS_MODIFIED) {
                    klass->mFlags &= ~CLASS_FLAGS_MODIFIED;
                    if(!save_class(klass)) {
                        cl_print("failed to write this class(%s)\n", REAL_CLASS_NAME(klass));
                    }
                }
                klass = next_klass;
            }
        }
    }
}

void increase_class_version(sCLClass* klass)
{
    char version;

    version = CLASS_VERSION(klass);
    version++;
    klass->mFlags = (klass->mFlags & ~CLASS_FLAGS_VERSION) | version;
}

//////////////////////////////////////////////////
// show classes
//////////////////////////////////////////////////
void show_class(sCLClass* klass)
{
    char* p;
    int i;
    
    //cl_print("-+- %s -+-\n", REAL_CLASS_NAME(klass));

    cl_print("ClassNameOffset %d (%s)\n", klass->mClassNameOffset, CONS_str(&klass->mConstPool, klass->mClassNameOffset));
    cl_print("NameSpaceOffset %d (%s)\n", klass->mNameSpaceOffset, CONS_str(&klass->mConstPool, klass->mNameSpaceOffset));
    cl_print("RealClassNameOffset %d (%s)\n", klass->mRealClassNameOffset, CONS_str(&klass->mConstPool, klass->mRealClassNameOffset));

    cl_print("num fields %d\n", klass->mNumFields);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));
        ASSERT(super_class);  // checked on load time
        cl_print("SuperClass[%d] %s\n", i, REAL_CLASS_NAME(super_class));
    }

    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mFlags & CL_STATIC_FIELD) {
            cl_print("field number %d --> %s static field %d\n", i, FIELD_NAME(klass, i), klass->mFields[i].uValue.mStaticField.mIntValue);
        }
        else {
            cl_print("field number %d --> %s\n", i, FIELD_NAME(klass, i));
        }
    }

    cl_print("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        int j;

        cl_print("--- method number %d ---\n", i);
        cl_print("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(&klass->mConstPool, klass->mMethods[i].mNameOffset));
        cl_print("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(&klass->mConstPool, klass->mMethods[i].mPathOffset));
        cl_print("result type %s\n", CONS_str(&klass->mConstPool, klass->mMethods[i].mResultType.mClassNameOffset));
        cl_print("num params %d\n", klass->mMethods[i].mNumParams);
        cl_print("num locals %d\n", klass->mMethods[i].mNumLocals);
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            cl_print("%d. %s\n", j, CONS_str(&klass->mConstPool, klass->mMethods[i].mParamTypes[j].mClassNameOffset));
        }

        if(klass->mMethods[i].mFlags & CL_CLASS_METHOD) {
            cl_print("static method\n");
        }

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
            cl_print("native methods %p\n", klass->mMethods[i].uCode.mNativeMethod);
        }
        else {
            cl_print("length of bytecodes %d\n", klass->mMethods[i].uCode.mByteCodes.mLen);
        }

        show_method(klass, klass->mMethods + i);
    }
}

void show_node_type(sCLNodeType* type)
{
    int i;

    if(type == NULL) {
        cl_print("NULL");
    }
    else if(type->mGenericsTypesNum == 0) {
        cl_print("%s", REAL_CLASS_NAME(type->mClass));
    }
    else {
        cl_print("%s<", REAL_CLASS_NAME(type->mClass));
        for(i=0; i<type->mGenericsTypesNum; i++) {
            cl_print("%s", REAL_CLASS_NAME(type->mGenericsTypes[i]));
            if(i != type->mGenericsTypesNum-1) { cl_print(","); }
        }
        cl_print(">");
    }
}

void show_type(sCLClass* klass, sCLType* type)
{
    sCLNodeType node_type;
    char* real_class_name;
    int i;


    real_class_name = CONS_str(&klass->mConstPool, type->mClassNameOffset);
    node_type.mClass = cl_get_class(real_class_name);

    node_type.mGenericsTypesNum = type->mGenericsTypesNum;

    for(i=0; i<type->mGenericsTypesNum; i++) {
        real_class_name = CONS_str(&klass->mConstPool, type->mGenericsTypesOffset[i]);
        node_type.mGenericsTypes[i] = cl_get_class(real_class_name);
    }

    show_node_type(&node_type);
}

void show_method(sCLClass* klass, sCLMethod* method)
{
    int i;

    /// result ///
    show_type(klass, &method->mResultType);
    cl_print(" ");

    /// name ///
    cl_print("%s(", METHOD_NAME2(klass, method));

    /// params ///
    for(i=0; i<method->mNumParams; i++) {
        show_type(klass, &method->mParamTypes[i]);

        if(i != method->mNumParams-1) cl_print(",");
    }

    /// block ///
    if(method->mNumBlockType == 0) {
        cl_print(") with no block\n");
    }
    else {
        cl_print(") with ");
        show_type(klass, &method->mBlockType.mResultType);
        cl_print(" block {|");

        for(i=0; i<method->mBlockType.mNumParams; i++) {
            show_type(klass, &method->mBlockType.mParamTypes[i]);

            if(i != method->mBlockType.mNumParams-1) cl_print(",");
        }

        cl_print("|}\n");
    }
}

void show_all_method(sCLClass* klass, char* method_name)
{
    int i;
    for(i=klass->mNumMethods-1; i>=0; i--) {                    // search for method in reverse because we want to get last defined method
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method;
            
            method = klass->mMethods + i;

            show_method(klass, method);
        }
    }
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class_from_classpath_on_compile_time(char* real_class_name, BOOL resolve_dependences)
{
    sCLClass* result;

    result = load_class_from_classpath(real_class_name, resolve_dependences);

    if(result) {
        if(!entry_alias_of_class(result)) {
            return NULL;
        }
        if(!add_compile_data(result, 1, result->mNumMethods, kCompileTypeLoad)) {
            return FALSE;
        }
    }

    return result;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL resolve_dependences)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    return load_class_from_classpath_on_compile_time(real_class_name, resolve_dependences);
}

// result: (TRUE) success (FALSE) faield
BOOL load_fundamental_classes_on_compile_time()
{
    load_class_from_classpath_on_compile_time("void", TRUE);
    load_class_from_classpath_on_compile_time("int", TRUE);
    load_class_from_classpath_on_compile_time("byte", TRUE);
    load_class_from_classpath_on_compile_time("Bytes", TRUE);
    load_class_from_classpath_on_compile_time("float", TRUE);
    load_class_from_classpath_on_compile_time("bool", TRUE);
    load_class_from_classpath_on_compile_time("String", TRUE);
    load_class_from_classpath_on_compile_time("Array", TRUE);
    load_class_from_classpath_on_compile_time("Hash", TRUE);

    load_class_from_classpath_on_compile_time("Object", TRUE);

    load_class_from_classpath_on_compile_time("Exception", TRUE);

    load_class_from_classpath_on_compile_time("NullPointerException", TRUE);
    load_class_from_classpath_on_compile_time("RangeException", TRUE);
    load_class_from_classpath_on_compile_time("ConvertingStringCodeException", TRUE);
    load_class_from_classpath_on_compile_time("ClassNotFoundException", TRUE);
    load_class_from_classpath_on_compile_time("IOException", TRUE);
    load_class_from_classpath_on_compile_time("OverflowException", TRUE);

    load_class_from_classpath_on_compile_time("ClassName", TRUE);
    load_class_from_classpath_on_compile_time("Thread", TRUE);
    load_class_from_classpath_on_compile_time("Block", TRUE);

    load_class_from_classpath_on_compile_time("null", TRUE);

    return TRUE;
}
