#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

static void add_dependences_with_node_type(sCLClass* klass, sCLNodeType* node_type)
{
    int i;

    add_dependence_class(klass, node_type->mClass);

    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        add_dependences_with_node_type(klass, node_type->mGenericsTypes[i]);
    }
}

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLNodeType* super_klass)
{
    int i;

    if(super_klass == NULL || super_klass->mClass == NULL) {
        return FALSE;
    }

    if(super_klass->mClass->mNumSuperClasses >= SUPER_CLASS_MAX) {
        return FALSE;
    }

    for(i=0; i<super_klass->mClass->mNumSuperClasses; i++) {
        clone_cl_type2(&klass->mSuperClasses[i], &super_klass->mClass->mSuperClasses[i], klass, super_klass->mClass);
    }

    create_cl_type_from_node_type2(ALLOC &klass->mSuperClasses[i], super_klass, klass);
    klass->mNumSuperClasses = super_klass->mClass->mNumSuperClasses + 1;

    add_dependences_with_node_type(klass, super_klass);

    if(super_klass->mClass->mFlags & CLASS_FLAGS_NATIVE) {
        klass->mFlags |= CLASS_FLAGS_NATIVE;
    }

    return TRUE;
}

// result (TRUE) --> success (FLASE) --> overflow implemented interface number
BOOL add_implemented_interface(sCLClass* klass, sCLNodeType* interface)
{
    if(klass->mNumImplementedInterfaces >= IMPLEMENTED_INTERFACE_MAX) {
        return FALSE;
    }

    create_cl_type_from_node_type2(ALLOC &klass->mImplementedInterfaces[klass->mNumImplementedInterfaces], interface, klass);
    klass->mNumImplementedInterfaces++;

    add_dependences_with_node_type(klass, interface);

    return TRUE;
}

// result (TRUE) --> success (FLASE) --> overflow included module number
BOOL add_included_module(sCLClass* klass, sCLModule* module) 
{
    if(klass->mNumIncludedModules >= INCLUDED_MODULE_MAX) {
        return FALSE;
    }

    create_cl_type_from_module(ALLOC &klass->mIncludedModules[klass->mNumIncludedModules], module, klass);
    klass->mNumIncludedModules++;

    return TRUE;
}

static BOOL type_identity_of_cl_type_with_solving_generics(sCLNodeType* klass1, sCLType* type1, sCLNodeType* klass2, sCLType* type2)
{
    sCLNodeType* left_type;
    sCLNodeType* left_type2;
    sCLNodeType* right_type;
    sCLNodeType* right_type2;

    left_type = ALLOC create_node_type_from_cl_type(type1, klass1->mClass);
    right_type = ALLOC create_node_type_from_cl_type(type2, klass2->mClass);

    ASSERT(left_type->mClass != NULL && right_type->mClass != NULL);

    if(!solve_generics_types_for_node_type(left_type, ALLOC &left_type2, klass1)) 
    {
        return FALSE;
    }

    if(!solve_generics_types_for_node_type(right_type, ALLOC &right_type2, klass2))
    {
        return FALSE;
    }

    /// anonymous is special ///
    if(type_identity(left_type2, gAnonymousType) || type_identity(right_type2, gAnonymousType)) 
    {
        return TRUE;
    }
    else {
        return type_identity(left_type2, right_type2);
    }
}

BOOL check_the_same_parametor_of_two_methods(sCLNodeType* klass1, sCLMethod* method1, sCLNodeType* klass2, sCLMethod* method2)
{
    int i;
    int j;

    if(method1->mNumParams != method2->mNumParams) {
        return FALSE;
    }

    for(i=0; i<method1->mNumParams; i++) {
        if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mParamTypes[i], klass2, &method2->mParamTypes[i]))
        {
            return FALSE;
        }
    }

    if(method1->mNumBlockType != method2->mNumBlockType) {
        return FALSE;
    }

    if(method1->mNumBlockType == 1) {
        if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mBlockType.mResultType, klass2, &method2->mBlockType.mResultType))
        {
            return FALSE;
        }

        if(method1->mBlockType.mNumParams != method2->mBlockType.mNumParams) {
            return FALSE;
        }

        for(i=0; i<method1->mBlockType.mNumParams; i++) {
            if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mBlockType.mParamTypes[i], klass2, &method2->mBlockType.mParamTypes[i]))
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

        real_class_name = CONS_str(&klass1->mClass->mConstPool, method1->mExceptionClassNameOffset[i]);
        exception_class1 = cl_get_class(real_class_name);
        if(exception_class1 == NULL) {
            return FALSE;
        }

        for(j=0; j<method2->mNumException; j++) {
            sCLClass* exception_class2;
            char* real_class_name;

            real_class_name = CONS_str(&klass2->mClass->mConstPool, method2->mExceptionClassNameOffset[j]);

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

static BOOL check_the_same_interface_of_two_methods(sCLNodeType* klass1, sCLMethod* method1, sCLNodeType* klass2, sCLMethod* method2, BOOL constructor)
{
    int i;
    int j;
    char* name1;
    char* name2;

    name1 = CONS_str(&klass1->mClass->mConstPool, method1->mNameOffset);
    name2 = CONS_str(&klass2->mClass->mConstPool, method2->mNameOffset);

    if(strcmp(name1, name2) != 0) {
        return FALSE;
    }

    if(!constructor && !type_identity_of_cl_type_with_solving_generics(klass1, &method1->mResultType, klass2, &method2->mResultType))
    {
        return FALSE;
    }

    if(method1->mNumParams != method2->mNumParams) {
        return FALSE;
    }

    for(i=0; i<method1->mNumParams; i++) {
        if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mParamTypes[i], klass2, &method2->mParamTypes[i]))
        {
            return FALSE;
        }
    }

    if(method1->mNumBlockType != method2->mNumBlockType) {
        return FALSE;
    }

    if(method1->mNumBlockType == 1) {
        if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mBlockType.mResultType, klass2, &method2->mBlockType.mResultType))
        {
            return FALSE;
        }

        if(method1->mBlockType.mNumParams != method2->mBlockType.mNumParams) {
            return FALSE;
        }

        for(i=0; i<method1->mBlockType.mNumParams; i++) {
            if(!type_identity_of_cl_type_with_solving_generics(klass1, &method1->mBlockType.mParamTypes[i], klass2, &method2->mBlockType.mParamTypes[i]))
            {
                return FALSE;
            }
        }
    }

/*
    if(method1->mNumException != method2->mNumException) {
        int j;
        for(j=0; j<method2->mNumException; j++) {
            sCLClass* exception_class2;
            char* real_class_name;

            real_class_name = CONS_str(&klass2->mClass->mConstPool, method2->mExceptionClassNameOffset[j]);

            exception_class2 = cl_get_class(real_class_name);

        }
        return FALSE;
    }

    for(i=0; i<method1->mNumException; i++) {
        sCLClass* exception_class1;
        char* real_class_name;

        real_class_name = CONS_str(&klass1->mClass->mConstPool, method1->mExceptionClassNameOffset[i]);
        exception_class1 = cl_get_class(real_class_name);
        if(exception_class1 == NULL) {
            return FALSE;
        }

        for(j=0; j<method2->mNumException; j++) {
            sCLClass* exception_class2;
            char* real_class_name;

            real_class_name = CONS_str(&klass2->mClass->mConstPool, method2->mExceptionClassNameOffset[j]);

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
*/

    return TRUE;
}

BOOL check_implemented_interface_between_super_classes(sCLNodeType* klass, sCLNodeType* interface, sCLMethod* method)
{
    int j;
    int k;

    for(j=0;j<klass->mClass->mNumSuperClasses; j++) {
        sCLNodeType* super_class;

        super_class = ALLOC create_node_type_from_cl_type(&klass->mClass->mSuperClasses[j], klass->mClass);

        ASSERT(super_class->mClass != NULL);

        for(k=0; k<super_class->mClass->mNumMethods; k++) {
            sCLMethod* method2;

            method2 = super_class->mClass->mMethods + k;

            if(!(method2->mFlags & CL_ABSTRACT_METHOD)) {
                if(check_the_same_interface_of_two_methods(interface, method, super_class, method2, method->mFlags & CL_CONSTRUCTOR))
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

BOOL check_implemented_interface(sCLNodeType* klass, sCLNodeType* interface)
{
    int i, j, k;

    for(i=0; i<interface->mClass->mNumMethods; i++) {
        sCLMethod* method;

        method = interface->mClass->mMethods + i;

        if(!check_implemented_interface_between_super_classes(klass, interface, method)) 
        {
            for(j=0; j<klass->mClass->mNumMethods; j++) {
                sCLMethod* method2;

                method2 = klass->mClass->mMethods + j;

                if(check_the_same_interface_of_two_methods(interface, method, klass, method2, method->mFlags & CL_CONSTRUCTOR))
                {
                    break;
                }
            }

            if(j == klass->mClass->mNumMethods) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL check_implemented_interface_without_super_class(sCLNodeType* klass, sCLNodeType* interface)
{
    int i, j;

    for(i=0; i<interface->mClass->mNumMethods; i++) {
        sCLMethod* method;

        method = interface->mClass->mMethods + i;

        for(j=0; j<klass->mClass->mNumMethods; j++) {
            sCLMethod* method2;

            method2 = klass->mClass->mMethods + j;

            if(check_the_same_interface_of_two_methods(interface, method, klass, method2, method->mFlags & CL_CONSTRUCTOR))
            {
                break;
            }
        }

        if(j == klass->mClass->mNumMethods) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL check_implemented_interface2_core(sCLClass* klass, sCLNodeType* interface)
{
    int i;

    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        sCLNodeType* interface2;

        interface2 = create_node_type_from_cl_type(&klass->mImplementedInterfaces[i], klass);

        ASSERT(interface2->mClass != NULL);

        if(substitution_posibility(interface, interface2)) {
            return TRUE;
        }
    }

    return FALSE;
}

// result (TRUE) --> implemeted this interface (FALSE) --> not implemented this interface
BOOL check_implemented_interface2(sCLClass* klass, sCLNodeType* interface)
{
    int i;

    if(klass == NULL) {
        return FALSE;
    }

    if(is_dynamic_typing_class(klass)) {
        return TRUE;
    }

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);

        super = cl_get_class(real_class_name);

        ASSERT(super != NULL);

        if(check_implemented_interface2_core(super, interface)) {
            return TRUE;
        }
    }

    if(check_implemented_interface2_core(klass, interface)) {
        return TRUE;
    }

    return FALSE;
}

static BOOL is_abstract_method_implemented(sCLNodeType* klass, sCLNodeType* klass_of_method, sCLMethod* method)
{
    if(klass_of_method->mClass->mFlags & CLASS_FLAGS_INTERFACE) {
        return FALSE;
    }

    int i;
    for(i=0; i<klass->mClass->mNumMethods; i++) {
        sCLMethod* method2;

        method2 = klass->mClass->mMethods + i;

        if(!(method2->mFlags & CL_ABSTRACT_METHOD)) {
            if(check_the_same_interface_of_two_methods(klass, method2, klass_of_method, method, FALSE))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL check_implemented_abstract_methods(sCLNodeType* klass, char** not_implemented_method_name)
{
    int i,j, k, l;

    for(i=0; i<klass->mClass->mNumSuperClasses; i++) {
        sCLNodeType* super_class;

        super_class = create_node_type_from_cl_type(&klass->mClass->mSuperClasses[i], klass->mClass);

        if(super_class->mClass->mFlags & CLASS_FLAGS_ABSTRACT) {
            for(int k=0; k<super_class->mClass->mNumMethods; k++) {
                sCLMethod* method;

                method = super_class->mClass->mMethods + k;

                if(method->mFlags & CL_ABSTRACT_METHOD) {
                    for(j=i+1; j<klass->mClass->mNumSuperClasses; j++) {
                        sCLNodeType* super_class2;

                        super_class2 = create_node_type_from_cl_type(&klass->mClass->mSuperClasses[j], klass->mClass);

                        if(is_abstract_method_implemented(super_class2, super_class, method)) {
                            break;
                        }
                    }

                    if(j == klass->mClass->mNumSuperClasses) {
                        if(!is_abstract_method_implemented(klass, super_class, method)) {
                            *not_implemented_method_name = METHOD_NAME2(super_class->mClass, method);
                            return FALSE;
                        }
                    }
                }
            }
        }
    }

    *not_implemented_method_name = NULL;

    return TRUE;
}

static BOOL is_already_contained_on_dependeces(sCLClass* klass, sCLClass* dependence_class)
{
    int i;

    for(i=0; i<klass->mNumDependences; i++) {
        if(strcmp(REAL_CLASS_NAME(dependence_class), CONS_str(&klass->mConstPool, klass->mDependencesOffset[i])) == 0)
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

        klass->mDependencesOffset = xxrealloc(klass->mDependencesOffset, sizeof(int)*klass->mSizeDependences, sizeof(int)*new_size);
        memset(klass->mDependencesOffset + klass->mSizeDependences, 0, sizeof(int)*(new_size-klass->mSizeDependences));

        klass->mSizeDependences = new_size;
    }

    klass->mDependencesOffset[klass->mNumDependences] = append_str_to_constant_pool(&klass->mConstPool, REAL_CLASS_NAME(dependence_class), FALSE);
    klass->mNumDependences++;
}


BOOL is_generics_param_type(sCLNodeType* node_type)
{
    int i;

    if(is_generics_param_class(node_type->mClass)) {
        return TRUE;
    }

    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        if(is_generics_param_type(node_type->mGenericsTypes[i]))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL is_parent_class(sCLClass* klass1, sCLClass* klass2) 
{
    int i;
    for(i=0; i<klass1->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass1->mConstPool, klass1->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == klass2) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL is_this_giving_type_parametor(sCLNodeType* caller_class, sCLClass* klass, sCLNodeType* type_)
{
    if(caller_class->mClass == klass || is_parent_class(caller_class->mClass, klass))
    {
        return FALSE;
    }

    return TRUE;
}

int get_generics_param_number(sCLClass* klass)
{
    int i;

    if(is_generics_param_class(klass)) {
        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            if(klass == gGParamClass[i]) {
                return i;
            }
        }
    }

    return -1;
}

sCLGenericsParamTypes* get_generics_param_types(sCLClass* klass, sCLClass* caller_class, sCLMethod* caller_method)
{
    int generics_param_num;

    generics_param_num = get_generics_param_number(klass);

    if(generics_param_num != -1) {
        if(caller_class == NULL || generics_param_num >= caller_class->mGenericsTypesNum) {
            return NULL;
        }
        else {
            return caller_class->mGenericsTypes + generics_param_num;
        }
    }
    else {
        return NULL;
    }
}

static sGenericsParamPattern* gGenericsTypePatterns;
static int gNumGenericsTypePattern;
static int gSizeGenericsTypePattern;

static void init_generics_type_pattern()
{
    gSizeGenericsTypePattern = 16;
    gGenericsTypePatterns = CALLOC(1, sizeof(sGenericsParamPattern)*gSizeGenericsTypePattern);
    gNumGenericsTypePattern = 0;
}

static sGenericsParamPattern* new_generics_type_pattern()
{
    if(gNumGenericsTypePattern >= gSizeGenericsTypePattern) {
        int new_size;

        new_size = gSizeGenericsTypePattern * 2;

        gGenericsTypePatterns = REALLOC(gGenericsTypePatterns, sizeof(sGenericsParamPattern)*new_size);

        memset(gGenericsTypePatterns + gSizeGenericsTypePattern, 0, sizeof(sGenericsParamPattern)*(new_size - gSizeGenericsTypePattern));

        gSizeGenericsTypePattern = new_size;
    }

    gGenericsTypePatterns[gNumGenericsTypePattern].mNumber = gNumGenericsTypePattern;

    return gGenericsTypePatterns + gNumGenericsTypePattern++;
}

void show_generics_pattern(sGenericsParamPattern* pattern)
{
    int i;
    for(i=0; i<pattern->mGenericsTypesNum; i++) {
        show_node_type(pattern->mGenericsTypes[i]);
        if(i-1!= pattern->mGenericsTypesNum) printf(",");
    }
}

static void create_generics_param_type_pattern_core(int i, sGenericsParamPattern** pattern, sCLNodeType* caller_class)
{
    for(; i<caller_class->mClass->mGenericsTypesNum; i++) 
    {
        sCLGenericsParamTypes* generics_param_type = caller_class->mClass->mGenericsTypes + i;
        
        if(generics_param_type->mExtendsType.mClassNameOffset != 0) 
        {
            int k;
            sGenericsParamPattern* new_pattern;

            (*pattern)->mGenericsTypes[i] = alloc_node_type();
            (*pattern)->mGenericsTypes[i]->mClass = gGParamClass[i];
            (*pattern)->mGenericsTypesNum = i + 1;

            create_generics_param_type_pattern_core(i+1, pattern, caller_class);

            new_pattern = new_generics_type_pattern();

            for(k=0; k<i; k++) {
                new_pattern->mGenericsTypes[k] = (*pattern)->mGenericsTypes[k];
            }
            new_pattern->mGenericsTypesNum = i;

            (*pattern) = new_pattern;

            (*pattern)->mGenericsTypes[i] = ALLOC create_node_type_from_cl_type(&generics_param_type->mExtendsType, caller_class->mClass);
            (*pattern)->mGenericsTypesNum = i + 1;
        }
        else if(generics_param_type->mNumImplementsTypes > 0) {
            int j;
            sGenericsParamPattern pattern_saved;

            (*pattern)->mGenericsTypes[i] = alloc_node_type();
            (*pattern)->mGenericsTypes[i]->mClass = gGParamClass[i];
            (*pattern)->mGenericsTypesNum = i + 1;

            pattern_saved = **pattern;

            create_generics_param_type_pattern_core(i+1, pattern, caller_class);

            for(j=0; j<generics_param_type->mNumImplementsTypes; j++)
            {
                int k;
                sGenericsParamPattern* new_pattern;

                new_pattern = new_generics_type_pattern();

                for(k=0; k<i; k++) {
                    new_pattern->mGenericsTypes[k] = pattern_saved.mGenericsTypes[k];
                }
                new_pattern->mGenericsTypesNum = i;

                (*pattern) = new_pattern;

                (*pattern)->mGenericsTypes[i] = ALLOC create_node_type_from_cl_type(&generics_param_type->mImplementsTypes[j], caller_class->mClass);

                (*pattern)->mGenericsTypesNum = i + 1;

                create_generics_param_type_pattern_core(i+1, pattern, caller_class);
            }
            break;
        }
        else {
            (*pattern)->mGenericsTypes[i] = alloc_node_type();
            (*pattern)->mGenericsTypes[i]->mClass = gGParamClass[i];
            (*pattern)->mGenericsTypesNum = i + 1;
        }
    }
}

void create_generics_param_type_pattern(ALLOC sGenericsParamPattern** generics_type_patterns, int* generics_type_pattern_num, sCLNodeType* caller_class)
{
    int i;
    sGenericsParamPattern* pattern;

    init_generics_type_pattern();

    i = 0;
    pattern = new_generics_type_pattern();
    create_generics_param_type_pattern_core(i, &pattern, caller_class);

    *generics_type_patterns = gGenericsTypePatterns;
    *generics_type_pattern_num = gNumGenericsTypePattern;
}

//////////////////////////////////////////////////
// fields
//////////////////////////////////////////////////
// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected, char* name, sCLNodeType* node_type)
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
        klass->mFields = xxrealloc(klass->mFields, sizeof(sCLField)*klass->mSizeFields, sizeof(sCLField)*new_size);
        memset(klass->mFields + klass->mSizeFields, 0, sizeof(sCLField)*(new_size-klass->mSizeFields));
        klass->mSizeFields = new_size;
    }

    field = klass->mFields + klass->mNumFields;

    field->mFlags = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0) | (protected ? CL_PROTECTED_FIELD:0);

    field->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name, FALSE);    // field name

    create_cl_type_from_node_type2(ALLOC &field->mType, node_type, klass);
    add_dependences_with_node_type(klass, node_type);

    field->mFieldIndex = 0;

    klass->mNumFields++;
    
    return TRUE;
}

/// set field index ///
void set_field_index(sCLClass* klass, char* name, BOOL class_field)
{
    sCLField* field;

    field = get_field(klass, name, class_field);

    if(class_field) {
        field->mFieldIndex = get_field_index(klass, name, TRUE);
    }
    else {
        field->mFieldIndex = get_field_index_including_super_classes_without_class_field(klass, name);
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

// result is seted on this parametors(sCLNodeType** result)
// if solving generics type is failer, this function returns FALSE
// if type_ is 0, this function doesn't solve the generics type
BOOL get_field_type(sCLClass* klass, sCLField* field, ALLOC sCLNodeType** result, sCLNodeType* type_)
{
    sCLNodeType* node_type;

    ASSERT(field != NULL);

    node_type = ALLOC create_node_type_from_cl_type(&field->mType, klass);
    
    if(type_) {
        if(type_->mGenericsTypesNum != type_->mClass->mGenericsTypesNum) {
            return FALSE;
        }

        if(!solve_generics_types_for_node_type(node_type, ALLOC result, type_)) {
            return FALSE;
        }
    }
    else {
        *result = node_type;
    }

    return TRUE;
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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
sCLField* get_field_including_super_classes(sCLNodeType* klass, char* field_name, sCLNodeType** founded_class, BOOL class_field, sCLNodeType** field_type, sCLNodeType* type_)
{
    sCLField* field;
    sCLNodeType* current_type;
    int i;

    current_type = type_;

    for(i=klass->mClass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLNodeType* super_class;
        sCLNodeType* solved_super_class;
        
        super_class = ALLOC create_node_type_from_cl_type(&klass->mClass->mSuperClasses[i], klass->mClass);

        ASSERT(super_class != NULL);  // checked on load time

        if(!solve_generics_types_for_node_type(super_class, &solved_super_class, current_type))
        {
            return NULL;
        }

        current_type = solved_super_class;

        field = get_field(super_class->mClass, field_name, class_field);

        if(field) { 
            sCLNodeType* field_type2;
            field_type2 = ALLOC create_node_type_from_cl_type(&field->mType, super_class->mClass);
        
            if(!solve_generics_types_for_node_type(field_type2, ALLOC field_type, current_type)) 
            {
                return NULL;
            }

            *founded_class = super_class; 
            return field; 
        }
    }

    field = get_field(klass->mClass, field_name, class_field);

    if(field) { 
        sCLNodeType* field_type2;
        field_type2 = ALLOC create_node_type_from_cl_type(&field->mType, klass->mClass);

        if(!solve_generics_types_for_node_type(field_type2, ALLOC field_type, type_)) 
        {
            return NULL;
        }

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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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

/////////////////////////////////////////////////////////////////////////
// methods
////////////////////////////////////////////////////////////////////////
static BOOL check_method_params(sCLMethod* method, sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type)
{
    if(strcmp(METHOD_NAME2(klass->mClass, method), method_name) == 0) {
        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) 
        {

            /// type checking ///
            if(method->mNumParams == -1) {              // no type checking of method params
                return TRUE;
            }
            else if(num_params == method->mNumParams) {
                int j, k;

                for(j=0; j<num_params; j++ ) {
                    sCLNodeType* param;
                    sCLNodeType* solved_param;

                    param = ALLOC create_node_type_from_cl_type(&method->mParamTypes[j], klass->mClass);

                    if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_)) 
                    {
                        return FALSE;
                    }
                    if(!substitution_posibility(solved_param, class_params[j])) {
                        return FALSE;
                    }
                }

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                    if(block_num > 0) {
                        sCLNodeType* result_block_type;
                        sCLNodeType* solved_result_block_type;

                        result_block_type = ALLOC create_node_type_from_cl_type(&method->mBlockType.mResultType, klass->mClass);

                        if(!solve_generics_types_for_node_type(result_block_type, ALLOC &solved_result_block_type, type_))
                        {
                            return FALSE;
                        }

                        if(!substitution_posibility(solved_result_block_type, block_type)) {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        sCLNodeType* param;
                        sCLNodeType* solved_param;

                        param = ALLOC create_node_type_from_cl_type(&method->mBlockType.mParamTypes[k], klass->mClass);

                        if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_))
                        {
                            return FALSE;
                        }

                        if(!substitution_posibility(solved_param, block_param_type[k])) {
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
sCLMethod* get_method_with_type_params(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, ALLOC sCLNodeType** result_type)
{
    int i;

    if(start_point < klass->mClass->mNumMethods) {
        for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
            sCLMethod* method;
            
            method = klass->mClass->mMethods + i;

            if(check_method_params(method, klass, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type))
            {
                sCLNodeType* result_type2;
                
                result_type2 = ALLOC create_node_type_from_cl_type(&method->mResultType, klass->mClass);

                if(!solve_generics_types_for_node_type(result_type2, result_type, type_))
                {
                    return NULL;
                }

                return method;
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, sCLNodeType** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, ALLOC sCLNodeType** result_type)
{
    sCLNodeType* current_type;
    sCLNodeType* solved_class_params[CL_METHOD_PARAM_MAX];
    int i;

    current_type = type_;

    for(i=0; i<num_params; i++) {
        solved_class_params[i] = class_params[i];
    }

    for(i=klass->mClass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLNodeType* super_class;
        sCLMethod* method;
        sCLNodeType* solved_super_class;
        int j;
        
        super_class = create_node_type_from_cl_type(&klass->mClass->mSuperClasses[i], klass->mClass);

        ASSERT(super_class != NULL);

        if(!solve_generics_types_for_node_type(super_class, &solved_super_class, current_type))
        {
            return NULL;
        }

        current_type = solved_super_class;

        for(j=0; j<num_params; j++) {
            ASSERT(j < CL_METHOD_PARAM_MAX);
            if(!solve_generics_types_for_node_type(solved_class_params[j], ALLOC &solved_class_params[j], current_type))
            {
                return NULL;
            }
        }

        method = get_method_with_type_params(super_class, method_name, solved_class_params, num_params, search_for_class_method, current_type, super_class->mClass->mNumMethods-1, block_num, block_num_params, block_param_type, block_type, result_type);

        if(method) {
            *founded_class = super_class;
            return method;
        }
    }

    *founded_class = NULL;

    return NULL;
}

static BOOL check_method_params_with_param_initializer(sCLMethod* method, sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, sCLNodeType* generics_solving_type, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer)
{
    *used_param_num_with_initializer = 0;

    if(strcmp(METHOD_NAME2(klass->mClass, method), method_name) == 0) {
        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) {
            /// type checking ///
            if(method->mFlags & CL_METHOD_PARAM_VARABILE_ARGUMENTS)  // variable arguments
            {
                int j;

                if(num_params < method->mNumParams-1) {
                    return FALSE;
                }

                for(j=0; j<method->mNumParams-1; j++) {
                    sCLNodeType* param;
                    sCLNodeType* solved_param;

                    param = ALLOC create_node_type_from_cl_type(&method->mParamTypes[j], klass->mClass);

                    if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_)) 
                    {
                        return FALSE;
                    }

                    if(generics_solving_type) {
                        if(!solve_generics_types_for_node_type(solved_param, ALLOC &solved_param, generics_solving_type)) 
                        {
                            return FALSE;
                        }
                    }

                    if(!substitution_posibility(solved_param, class_params[j])) 
                    {
                        return FALSE;
                    }
                }

                return TRUE;
            }
            else if(num_params == method->mNumParams) {
                int j, k;

                for(j=0; j<num_params; j++ ) {
                    sCLNodeType* param;
                    sCLNodeType* solved_param;

                    param = ALLOC create_node_type_from_cl_type(&method->mParamTypes[j], klass->mClass);

                    if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_)) 
                    {
                        return FALSE;
                    }

                    if(generics_solving_type) {
                        if(!solve_generics_types_for_node_type(solved_param, ALLOC &solved_param, generics_solving_type)) 
                        {
                            return FALSE;
                        }
                    }

                    if(!substitution_posibility(solved_param, class_params[j])) 
                    {
                        return FALSE;
                    }
                }

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                    if(block_num > 0) {
                        sCLNodeType* result_block_type;
                        sCLNodeType* solved_result_block_type;

                        result_block_type = ALLOC create_node_type_from_cl_type(&method->mBlockType.mResultType, klass->mClass);

                        if(!solve_generics_types_for_node_type(result_block_type, ALLOC &solved_result_block_type, type_))
                        {
                            return FALSE;
                        }

                        if(generics_solving_type) {
                            if(!solve_generics_types_for_node_type(solved_result_block_type, ALLOC &solved_result_block_type, generics_solving_type))
                            {
                                return FALSE;
                            }
                        }

                        if(!substitution_posibility(solved_result_block_type, block_type)) {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        sCLNodeType* param;
                        sCLNodeType* solved_param;

                        param = ALLOC create_node_type_from_cl_type(&method->mBlockType.mParamTypes[k], klass->mClass);

                        if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_))
                        {
                            return FALSE;
                        }

                        if(generics_solving_type) {
                            if(!solve_generics_types_for_node_type(solved_param, ALLOC &solved_param, generics_solving_type))
                            {
                                return FALSE;
                            }
                        }

                        if(!substitution_posibility(solved_param, block_param_type[k])) {
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
                    sCLNodeType* param;
                    sCLNodeType* solved_param;

                    param = ALLOC create_node_type_from_cl_type(&method->mParamTypes[j], klass->mClass);

                    if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_)) 
                    {
                        return FALSE;
                    }

                    if(generics_solving_type) {
                        if(!solve_generics_types_for_node_type(solved_param, ALLOC &solved_param, generics_solving_type)) 
                        {
                            return FALSE;
                        }
                    }

                    if(!substitution_posibility(solved_param, class_params[j])) {
                        return FALSE;
                    }
                }

                *used_param_num_with_initializer = method->mNumParams - num_params;

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {

                    if(block_num > 0) {
                        sCLNodeType* result_block_type;
                        sCLNodeType* solved_result_block_type;

                        result_block_type = ALLOC create_node_type_from_cl_type(&method->mBlockType.mResultType, klass->mClass);

                        if(!solve_generics_types_for_node_type(result_block_type, ALLOC &solved_result_block_type, type_))
                        {
                            return FALSE;
                        }

                        if(generics_solving_type) {
                            if(!solve_generics_types_for_node_type(solved_result_block_type, ALLOC &solved_result_block_type, generics_solving_type))
                            {
                                return FALSE;
                            }
                        }

                        if(!substitution_posibility(solved_result_block_type, block_type)) {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        sCLNodeType* param;
                        sCLNodeType* solved_param;

                        param = ALLOC create_node_type_from_cl_type(&method->mBlockType.mParamTypes[k], klass->mClass);

                        if(!solve_generics_types_for_node_type(param, ALLOC &solved_param, type_))
                        {
                            return FALSE;
                        }

                        if(generics_solving_type) {
                            if(!solve_generics_types_for_node_type(solved_param, ALLOC &solved_param, generics_solving_type))
                            {
                                return FALSE;
                            }
                        }

                        if(!substitution_posibility(solved_param, block_param_type[k])) {
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
sCLMethod* get_method_with_type_params_and_param_initializer(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, sCLNodeType* generics_solving_type, int start_point, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type)
{
    int i;

    if(start_point < klass->mClass->mNumMethods) {
        for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
            sCLMethod* method;
            
            method = klass->mClass->mMethods + i;

            if(check_method_params_with_param_initializer(method, klass, method_name, class_params, num_params, search_for_class_method, type_, generics_solving_type, block_num, block_num_params, block_param_type, block_type, used_param_num_with_initializer))
            {
                sCLNodeType* result_type2;
                
                result_type2 = ALLOC create_node_type_from_cl_type(&method->mResultType, klass->mClass);


                if(!solve_generics_types_for_node_type(result_type2, result_type, type_))
                {
                    return NULL;
                }

                return method;
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer_on_super_classes(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, sCLNodeType** founded_class, BOOL search_for_class_method, sCLNodeType* type_, sCLNodeType* generics_solving_type, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type)
{
    sCLNodeType* current_type;
    sCLNodeType* solved_class_params[CL_METHOD_PARAM_MAX];
    int i;

    current_type = type_;

    for(i=0; i<num_params; i++) {
        solved_class_params[i] = class_params[i];
    }

    for(i=klass->mClass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLNodeType* super_class;
        sCLNodeType* solved_super_class;
        sCLMethod* method;
        int j;

        super_class = ALLOC create_node_type_from_cl_type(&klass->mClass->mSuperClasses[i], klass->mClass);

        ASSERT(super_class != NULL);  // checked on load time

        if(!solve_generics_types_for_node_type(super_class, &solved_super_class, current_type))
        {
            return NULL;
        }

        current_type = solved_super_class;

        for(j=0; j<num_params; j++) {
            ASSERT(j < CL_METHOD_PARAM_MAX);

            (void)solve_generics_types_for_node_type(solved_class_params[j], ALLOC &solved_class_params[j], current_type);

            // if it can not be solved generics, no solve the generics type
        }

        method = get_method_with_type_params_and_param_initializer(super_class, method_name, solved_class_params, num_params, search_for_class_method, current_type, generics_solving_type, super_class->mClass->mNumMethods-1, block_num, block_num_params, block_param_type, block_type, used_param_num_with_initializer, result_type);

        if(method) {
            *founded_class = solved_super_class;
            return method;
        }
    }

    *founded_class = NULL;

    return NULL;
}

// no solve generics type
ALLOC sCLNodeType* get_result_type_of_method(sCLNodeType* klass, sCLMethod* method)
{
    return ALLOC create_node_type_from_cl_type(&method->mResultType, klass->mClass);
}

static BOOL add_method_to_virtual_method_table_core(sCLClass* klass, char* method_name, int method_index)
{
    int hash;
    sVMethodMap* item;

    hash = get_hash(method_name) % klass->mSizeVirtualMethodMap;

    item = klass->mVirtualMethodMap + hash;

    while(1) {
        if(item->mMethodName[0] == 0) {
            xstrncpy(item->mMethodName, method_name, CL_VMT_NAME_MAX);
            item->mMethodIndex = method_index;
            break;
        }
        else {
            /// for mixin, we want to order greater index to get first when searching
            if(method_index > item->mMethodIndex) {
                sVMethodMap item2;

                item2 = *item;

                xstrncpy(item->mMethodName, method_name, CL_VMT_NAME_MAX);
                item->mMethodIndex = method_index;

                xstrncpy(method_name, item2.mMethodName, CL_VMT_NAME_MAX);
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
    sVMethodMap* item;

    if(klass->mSizeVirtualMethodMap <= klass->mNumVirtualMethodMap * 2) {
        /// rehash ///
        if(!resizse_vmm(klass)) {
            return FALSE;
        }
    }

    if(!add_method_to_virtual_method_table_core(klass, method_name, method_index))
    {
        return FALSE;
    }

    klass->mNumVirtualMethodMap++;

    return TRUE;
}

static void create_method_path(char* result, int result_size, sCLMethod* method, sCLClass* klass)
{
    int i;

    xstrncpy(result, REAL_CLASS_NAME(klass), result_size);
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

// result (TRUE) --> success (FALSE) --> overflow methods number
// last parametor returns the method which is added
BOOL create_method(sCLClass* klass, sCLMethod** method)
{
    if(klass->mNumMethods >= CL_METHODS_MAX) {
        return FALSE;
    }
    if(klass->mNumMethods >= klass->mSizeMethods) {
        const int new_size = klass->mSizeMethods * 2;
        klass->mMethods = xxrealloc(klass->mMethods, sizeof(sCLMethod)*klass->mSizeMethods, sizeof(sCLMethod)*new_size);
        memset(klass->mMethods + klass->mSizeMethods, 0, sizeof(sCLMethod)*(new_size-klass->mSizeMethods));
        klass->mSizeMethods = new_size;
    }

    *method = klass->mMethods + klass->mNumMethods;

    return TRUE;
}

static BOOL is_this_clone_method(sCLClass* klass, sCLMethod* method)
{
    sCLType* result_type;

    result_type = &method->mResultType;

    if(strcmp(METHOD_NAME2(klass, method), "clone") == 0
        && method->mNumParams == 0
        && !(method->mFlags & CL_CLASS_METHOD)
        && strcmp(CONS_str(&klass->mConstPool, result_type->mClassNameOffset), REAL_CLASS_NAME(klass)) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL is_this_method_missing_method(sCLClass* klass, sCLMethod* method)
{
    sCLType* result_type;

    result_type = &method->mResultType;

    if(strcmp(METHOD_NAME2(klass, method), "methodMissing") == 0
        && !(method->mFlags & CL_CLASS_METHOD)
        && method->mNumParams == 3)
    {
        sCLNodeType* param_types[3];
        sCLNodeType* node_type;

        int i;
        for(i=0; i<3; i++) {
            param_types[i] = create_node_type_from_cl_type(&method->mParamTypes[i], klass);
        }

        /// method_name ///
        if(!substitution_posibility(param_types[0], gStringType)) {
            return FALSE;
        }

        /// params ///
        node_type = alloc_node_type();
        node_type->mClass = cl_get_class("Array$1");
        ASSERT(node_type->mClass != NULL);
        node_type->mGenericsTypesNum = 1;
        node_type->mGenericsTypes[0] = gAnonymousType;

        if(!substitution_posibility(param_types[1], node_type)) {
            return FALSE;
        }

        /// method_block ///
        node_type = alloc_node_type();
        node_type->mClass = gBlockClass;
        node_type->mGenericsTypesNum = 0;

        if(!substitution_posibility(param_types[2], node_type)) {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

static BOOL is_this_method_missing_method_of_class_method(sCLClass* klass, sCLMethod* method)
{
    sCLType* result_type;

    result_type = &method->mResultType;

    if(strcmp(METHOD_NAME2(klass, method), "methodMissing") == 0
        && method->mFlags & CL_CLASS_METHOD
        && method->mNumParams == 3)
    {
        sCLNodeType* param_types[3];
        sCLNodeType* node_type;

        int i;
        for(i=0; i<3; i++) {
            param_types[i] = create_node_type_from_cl_type(&method->mParamTypes[i], klass);
        }

        /// method_name ///
        if(!substitution_posibility(param_types[0], gStringType)) {
            return FALSE;
        }

        /// params ///
        node_type = alloc_node_type();
        node_type->mClass = cl_get_class("Array$1");
        ASSERT(node_type->mClass != NULL);
        node_type->mGenericsTypesNum = 1;
        node_type->mGenericsTypes[0] = gAnonymousType;

        if(!substitution_posibility(param_types[1], node_type)) {
            return FALSE;
        }

        /// method_block ///
        node_type = alloc_node_type();
        node_type->mClass = gBlockClass;
        node_type->mGenericsTypesNum = 0;

        if(!substitution_posibility(param_types[2], node_type)) {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

// result: (NULL) not found (sCLMethod*) found
sCLMethod* get_clone_method(sCLClass* klass)
{
    int i;

    for(i=klass->mNumMethods-1; i>=0; i--) {
        sCLMethod* method;

        method = klass->mMethods + i;

        if(is_this_clone_method(klass, method)) {
            return method;
        }
    }

    return NULL;
}

void add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, BOOL generics_newable, char* name, sCLNodeType* result_type, BOOL constructor, sCLMethod* method)
{
    method->mFlags = (static_ ? CL_CLASS_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (protected_ ? CL_PROTECTED_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0) | (synchronized_ ? CL_SYNCHRONIZED_METHOD:0) | (constructor ? CL_CONSTRUCTOR:0) | (virtual_ ? CL_VIRTUAL_METHOD:0) | (abstract_ ? CL_ABSTRACT_METHOD:0) | (generics_newable ? CL_GENERICS_NEWABLE_CONSTRUCTOR|CL_VIRTUAL_METHOD:0);
    method->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name, FALSE);

    create_cl_type_from_node_type2(&method->mResultType, result_type, klass);

    add_dependences_with_node_type(klass, result_type);

    method->mParamTypes = NULL;
    method->mParamInitializers = NULL;
    method->mNumParams = 0;
    method->mNumParamInitializer = 0;

    method->mNumLocals = 0;

    method->mNumBlockType = 0;
    memset(&method->mBlockType, 0, sizeof(method->mBlockType));
}

// result (TRUE) --> success (FALSE) --> overflow parametor number
BOOL add_param_to_method(sCLClass* klass, sCLNodeType** class_params, MANAGED sByteCode* code_params, int* max_stack_params, int* lv_num_params, int num_params, sCLMethod* method, int block_num, char* block_name, sCLNodeType* bt_result_type, sCLNodeType** bt_class_params, int bt_num_params, char* name, BOOL variable_arguments)
{
    char* buf;
    int i;
    sCLBlockType* block_type;
    int path_max;

    if(num_params > 0) {
        int num_param_initializer;

        method->mParamTypes = CALLOC(1, sizeof(sCLType)*num_params);

        for(i=0; i<num_params; i++) {
            create_cl_type_from_node_type2(&method->mParamTypes[i], class_params[i], klass);
            add_dependences_with_node_type(klass, class_params[i]);
        }

        method->mParamInitializers = CALLOC(1, sizeof(sCLParamInitializer)*num_params);

        num_param_initializer = 0;

        for(i=0; i<num_params; i++) {
            if(code_params[i].mCode) num_param_initializer++;

            method->mParamInitializers[i].mInitializer = MANAGED code_params[i];
            method->mParamInitializers[i].mMaxStack = max_stack_params[i];
            method->mParamInitializers[i].mLVNum = lv_num_params[i];
        }

        method->mNumParams = num_params;
        method->mNumParamInitializer = num_param_initializer;
    }
    else {
        method->mParamTypes = NULL;
        method->mParamInitializers = NULL;
        method->mNumParams = 0;
        method->mNumParamInitializer = 0;
    }

    /// variable arguments ///
    if(variable_arguments) {
        method->mFlags |= CL_METHOD_PARAM_VARABILE_ARGUMENTS;
    }

    method->mNumLocals = 0;

    method->mNumBlockType = 0;
    memset(&method->mBlockType, 0, sizeof(method->mBlockType));

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    if(!add_method_to_virtual_method_table(klass, name, klass->mNumMethods, num_params)) 
    {
        return FALSE;
    }

    /// add block type ///
    if(block_num) {
        method->mNumBlockType++;
        ASSERT(method->mNumBlockType == 1);

        /// block type result type ///
        block_type = &method->mBlockType;

        block_type->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, block_name, FALSE);

        create_cl_type_from_node_type2(&block_type->mResultType, bt_result_type, klass);
        add_dependences_with_node_type(klass, bt_result_type);

        /// param types //
        block_type->mNumParams = bt_num_params;
        if(block_type->mNumParams > 0) {
            block_type->mParamTypes = CALLOC(1, sizeof(sCLType)*bt_num_params);
        }
        else {
            block_type->mParamTypes = NULL;
        }

        for(i=0; i<bt_num_params; i++) {
            create_cl_type_from_node_type2(&block_type->mParamTypes[i], bt_class_params[i], klass);

            add_dependences_with_node_type(klass, bt_class_params[i]);
        }
    }

    /// make method path ///
    path_max = CL_METHOD_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX + CL_REAL_CLASS_NAME_MAX + CL_REAL_CLASS_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX;
    buf = CALLOC(1, sizeof(char)*(path_max+1));;

    create_method_path(buf, path_max, method, klass);

    method->mPathOffset = append_str_to_constant_pool(&klass->mConstPool, buf, FALSE);

    FREE(buf);

    if(is_this_clone_method(klass, method)) {
        klass->mCloneMethodIndex = klass->mNumMethods;
    }
    else if(is_this_method_missing_method(klass, method)) {
        klass->mMethodMissingMethodIndex = klass->mNumMethods;
    }
    else if(is_this_method_missing_method_of_class_method(klass, method)) {
        klass->mMethodMissingMethodIndexOfClassMethod = klass->mNumMethods;
    }

    klass->mNumMethods++;

    return TRUE;
}

BOOL add_generics_param_type_to_method(sCLClass* klass, sCLMethod* method, char* name, sCLNodeType* extends_type, char num_implements_types, sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX])
{
    sCLGenericsParamTypes* param_types;
    int i;

    if(method->mGenericsTypesNum >= CL_GENERICS_CLASS_PARAM_MAX) {
        return FALSE;
    }

    /// check the same name class parametor is defined ///
    for(i=0; i<method->mGenericsTypesNum; i++) {
        param_types = method->mGenericsTypes + i;

        if(strcmp(CONS_str(&klass->mConstPool, param_types->mNameOffset), name) == 0)
        {
            return FALSE;
        }
    }

    param_types = method->mGenericsTypes + method->mGenericsTypesNum;

    param_types->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name, FALSE);

    method->mGenericsTypesNum++;

    if(extends_type) { 
        create_cl_type_from_node_type2(&param_types->mExtendsType, extends_type, klass);
    }

    param_types->mNumImplementsTypes = num_implements_types;
    for(i=0; i<num_implements_types; i++) {
        create_cl_type_from_node_type2(&param_types->mImplementsTypes[i], implements_types[i], klass);
    }

    return TRUE;
}

// result: (TRUE) success (FALSE) overflow exception number
BOOL add_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class)
{
    char* real_class_name;

    if(method->mNumException >= CL_METHOD_EXCEPTION_MAX) {
        return FALSE;
    }

    real_class_name = REAL_CLASS_NAME(exception_class);
    method->mExceptionClassNameOffset[method->mNumException++] = append_str_to_constant_pool(&klass->mConstPool, real_class_name, FALSE);

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

        if(substitution_posibility_of_class(klass2, exception_class)) {
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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

// return method parametor number
int get_method_num_params(sCLMethod* method)
{
    return method->mNumParams;
}

ALLOC char** get_class_names()
{
    int i;
    char** result;
    int result_size;
    int result_num;

    result_size = 128;
    result = CALLOC(1, sizeof(char*)*result_size);
    result_num = 0;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                *(result+result_num) = CONS_str(&klass->mConstPool, klass->mClassNameOffset);
                result_num++;

                if(result_num >= result_size) {
                    result_size *= 2;
                    result = REALLOC(result, sizeof(char*)*result_size);
                }
                klass = next_klass;
            }
        }
    }

    *(result+result_num) = NULL;
    result_num++;

    if(result_num >= result_size) {
        result_size *= 2;
        result = REALLOC(result, sizeof(char*)*result_size);
    }

    return result;
}

ALLOC char** get_method_names(sCLClass* klass)
{
    int i;
    char** result;
    int result_size;
    int result_num;

    result_size = 128;
    result = CALLOC(1, sizeof(char*)*result_size);
    result_num = 0;

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;

        method = klass->mMethods + i;

        *(result+result_num) = METHOD_NAME2(klass, method);
        result_num++;

        if(result_num >= result_size) {
            result_size *= 2;
            result = REALLOC(result, sizeof(char*)*result_size);
        }
    }

    *(result+result_num) = NULL;
    result_num++;

    if(result_num >= result_size) {
        result_size *= 2;
        result = REALLOC(result, sizeof(char*)*result_size);
    }

    return result;
}

ALLOC ALLOC char** get_method_names_with_arguments(sCLClass* klass)
{
    int i;
    char** result;
    int result_size;
    int result_num;

    result_size = 128;
    result = CALLOC(1, sizeof(char*)*result_size);
    result_num = 0;

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;
        sBuf buf;
        int j;

        method = klass->mMethods + i;

        sBuf_init(&buf);

        sBuf_append_str(&buf, METHOD_NAME2(klass, method));
        sBuf_append_str(&buf, "(");

        for(j=0; j<method->mNumParams; j++) {
            sCLType* param;
            char* param_type;
            sCLNodeType* node_type;
            char* argment_names;

            param = method->mParamTypes + j;

            node_type = create_node_type_from_cl_type(param, klass);

            argment_names = ALLOC node_type_to_buffer(node_type);

            sBuf_append_str(&buf, argment_names);

            if(j!=method->mNumParams-1) sBuf_append_str(&buf, ",");

            FREE(argment_names);
        }

        sBuf_append_str(&buf, ")");

        *(result+result_num) = MANAGED buf.mBuf;
        result_num++;

        if(result_num >= result_size) {
            result_size *= 2;
            result = REALLOC(result, sizeof(char*)*result_size);
        }
    }

    *(result+result_num) = NULL;
    result_num++;

    if(result_num >= result_size) {
        result_size *= 2;
        result = REALLOC(result, sizeof(char*)*result_size);
    }

    return result;
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

static void write_long_long_value_to_buffer(sBuf* buf, long long value)
{
    sBuf_append(buf, &value, sizeof(long long));
}

static void write_type_to_buffer(sBuf* buf, sCLType* type)
{
    int j;

    write_int_value_to_buffer(buf, type->mClassNameOffset);

    write_int_value_to_buffer(buf, type->mStar);

    write_char_value_to_buffer(buf, type->mGenericsTypesNum);
    for(j=0; j<type->mGenericsTypesNum; j++) {
        write_type_to_buffer(buf, type->mGenericsTypes[j]);
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

    write_int_value_to_buffer(buf, field->mFieldIndex);
}

static void write_generics_param_types_to_buffer(sBuf* buf, sCLGenericsParamTypes* generics_param_types)
{
    int i;

    write_int_value_to_buffer(buf, generics_param_types->mNameOffset);

    write_type_to_buffer(buf, &generics_param_types->mExtendsType);
    
    write_char_value_to_buffer(buf, generics_param_types->mNumImplementsTypes);

    for(i=0; i<generics_param_types->mNumImplementsTypes; i++) {
        write_type_to_buffer(buf, generics_param_types->mImplementsTypes + i);
    }
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

    write_char_value_to_buffer(buf, method->mGenericsTypesNum);
    for(i=0; i<method->mGenericsTypesNum; i++) {
        write_generics_param_types_to_buffer(buf, &method->mGenericsTypes[i]);
    }
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

    write_long_long_value_to_buffer(buf, klass->mFlags);

    /// save constant pool ///
    write_int_value_to_buffer(buf, klass->mConstPool.mLen);

    sBuf_append(buf, klass->mConstPool.mConst, klass->mConstPool.mLen);

    /// save class name offset
    write_char_value_to_buffer(buf, klass->mNameSpaceOffset);
    write_char_value_to_buffer(buf, klass->mClassNameOffset);
    write_char_value_to_buffer(buf, klass->mRealClassNameOffset);

    /// save fields
    klass->mNumFieldsIncludingSuperClasses = get_field_num_including_super_classes(klass);

    write_int_value_to_buffer(buf, klass->mNumFields);
    write_int_value_to_buffer(buf, klass->mNumFieldsIncludingSuperClasses);
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
        write_type_to_buffer(buf, &klass->mSuperClasses[i]);
    }

    /// write implement interfaces ///
    write_char_value_to_buffer(buf, klass->mNumImplementedInterfaces);
    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        write_type_to_buffer(buf, &klass->mImplementedInterfaces[i]);
    }

    /// write included modules ///
    write_char_value_to_buffer(buf, klass->mNumIncludedModules);
    for(i=0; i<klass->mNumIncludedModules; i++) {
        write_type_to_buffer(buf, &klass->mIncludedModules[i]);
    }

    /// write class params name of generics ///
    write_char_value_to_buffer(buf, klass->mGenericsTypesNum);
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        write_generics_param_types_to_buffer(buf, &klass->mGenericsTypes[i]);
    }

    /// write dependences ///
    write_int_value_to_buffer(buf, klass->mNumDependences);

    for(i=0; i<klass->mNumDependences; i++) {
        write_int_value_to_buffer(buf, klass->mDependencesOffset[i]);
    }

    /// write virtual method table ///
    write_virtual_method_map(buf, klass);

    /// write clone method index ///
    write_int_value_to_buffer(buf, klass->mCloneMethodIndex);

    /// write clone method index ///
    write_int_value_to_buffer(buf, klass->mMethodMissingMethodIndex);

    /// write clone method index ///
    write_int_value_to_buffer(buf, klass->mMethodMissingMethodIndexOfClassMethod);
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

void save_all_modified_classes()
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
                        printf("failed to write this class(%s)\n", REAL_CLASS_NAME(klass));
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
    
    //printf("-+- %s -+-\n", REAL_CLASS_NAME(klass));

    printf("ClassNameOffset %d (%s)\n", klass->mClassNameOffset, CONS_str(&klass->mConstPool, klass->mClassNameOffset));
    printf("NameSpaceOffset %d (%s)\n", klass->mNameSpaceOffset, CONS_str(&klass->mConstPool, klass->mNameSpaceOffset));
    printf("RealClassNameOffset %d (%s)\n", klass->mRealClassNameOffset, CONS_str(&klass->mConstPool, klass->mRealClassNameOffset));

    printf("num fields %d\n", klass->mNumFields);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset));
        ASSERT(super_class);  // checked on load time
        printf("SuperClass[%d] %s\n", i, REAL_CLASS_NAME(super_class));
    }

    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mFlags & CL_STATIC_FIELD) {
            printf("field number %d --> %s static field %d\n", i, FIELD_NAME(klass, i), klass->mFields[i].uValue.mStaticField.mObjectValue.mValue);
        }
        else {
            printf("field number %d --> %s\n", i, FIELD_NAME(klass, i));
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        int j;

        printf("--- method number %d ---\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(&klass->mConstPool, klass->mMethods[i].mNameOffset));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(&klass->mConstPool, klass->mMethods[i].mPathOffset));
        printf("result type %s\n", CONS_str(&klass->mConstPool, klass->mMethods[i].mResultType.mClassNameOffset));
        printf("num params %d\n", klass->mMethods[i].mNumParams);
        printf("num locals %d\n", klass->mMethods[i].mNumLocals);
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            printf("%d. %s\n", j, CONS_str(&klass->mConstPool, klass->mMethods[i].mParamTypes[j].mClassNameOffset));
        }

        if(klass->mMethods[i].mFlags & CL_CLASS_METHOD) {
            printf("static method\n");
        }

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
            printf("native methods %p\n", klass->mMethods[i].uCode.mNativeMethod);
        }
        else {
            printf("length of bytecodes %d\n", klass->mMethods[i].uCode.mByteCodes.mLen);
        }

        show_method(klass, klass->mMethods + i);
    }
}

void show_class_list_on_compile_time()
{
    int i;

    printf("-+- class list -+-\n");
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;

            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                printf("%s\n", REAL_CLASS_NAME(klass));
                klass = next_klass;
            }
        }
    }
}

void show_node_type(sCLNodeType* node_type)
{
    int i;

    if(node_type == NULL) {
        printf("NULL");
    }
    else if(node_type->mGenericsTypesNum == 0) {
        printf("%s", REAL_CLASS_NAME(node_type->mClass));
    }
    else {
        if(node_type->mClass == NULL) {
            printf("NULL<");
        }
        else {
            printf("%s<", REAL_CLASS_NAME(node_type->mClass));
        }
        for(i=0; i<node_type->mGenericsTypesNum; i++) {
            show_node_type(node_type->mGenericsTypes[i]);
            if(i != node_type->mGenericsTypesNum-1) { printf(","); }
        }
        printf(">");
    }
}

void show_type(sCLClass* klass, sCLType* type)
{
    sCLNodeType* node_type;

    node_type = ALLOC create_node_type_from_cl_type(type, klass);

    show_node_type(node_type);
}

void show_method(sCLClass* klass, sCLMethod* method)
{
    int i;

    /// result ///
    show_type(klass, &method->mResultType);
    printf(" ");

    /// name ///
    printf("%s(", METHOD_NAME2(klass, method));

    /// params ///
    for(i=0; i<method->mNumParams; i++) {
        show_type(klass, &method->mParamTypes[i]);

        if(i != method->mNumParams-1) printf(",");
    }

    /// block ///
    if(method->mNumBlockType == 0) {
        printf(") with no block\n");
    }
    else {
        printf(") with ");
        show_type(klass, &method->mBlockType.mResultType);
        printf(" block {|");

        for(i=0; i<method->mBlockType.mNumParams; i++) {
            show_type(klass, &method->mBlockType.mParamTypes[i]);

            if(i != method->mBlockType.mNumParams-1) printf(",");
        }

        printf("|}\n");
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


static void set_special_class_to_global_pointer_of_type(sCLClass* klass, int parametor_num)
{
    if(strcmp(REAL_CLASS_NAME(klass), "void") == 0) {
        gVoidType->mClass = klass;
        gVoidClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "int") == 0) {
        gIntType->mClass = klass;
        gIntClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "byte") == 0) {
        gByteType->mClass = klass;
        gByteClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "short") == 0) {
        gShortType->mClass = klass;
        gShortClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "uint") == 0) {
        gUIntType->mClass = klass;
        gUIntClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "long") == 0) {
        gLongType->mClass = klass;
        gLongClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "char") == 0) {
        gCharType->mClass = klass;
        gCharClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "double") == 0) {
        gDoubleType->mClass = klass;
        gDoubleClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "float") == 0) {
        gFloatType->mClass = klass;
        gFloatClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "bool") == 0) {
        gBoolType->mClass = klass;
        gBoolClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Null") == 0) {
        gNullType->mClass = klass;
        gNullClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Object") == 0) {
        gObjectType->mClass = klass;
        gObjectClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Array$1") == 0) {
        gArrayType->mClass = klass;
        gArrayType->mGenericsTypesNum = 1;
        gArrayType->mGenericsTypes[0]->mClass = gGParamClass[0];
        gArrayClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$1") == 0) {
        gTupleType[0]->mClass = klass;
        gTupleType[0]->mGenericsTypesNum = 1;
        gTupleType[0]->mGenericsTypes[0]->mClass = gGParamClass[0];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$2") == 0) {
        gTupleType[1]->mClass = klass;
        gTupleType[1]->mGenericsTypesNum = 2;
        gTupleType[1]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[1]->mGenericsTypes[1]->mClass = gGParamClass[1];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$3") == 0) {
        gTupleType[2]->mClass = klass;
        gTupleType[2]->mGenericsTypesNum = 3;
        gTupleType[2]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[2]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[2]->mGenericsTypes[2]->mClass = gGParamClass[2];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$4") == 0) {
        gTupleType[3]->mClass = klass;
        gTupleType[3]->mGenericsTypesNum = 3;
        gTupleType[3]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[3]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[3]->mGenericsTypes[2]->mClass = gGParamClass[2];
        gTupleType[3]->mGenericsTypes[3]->mClass = gGParamClass[3];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$5") == 0) {
        gTupleType[4]->mClass = klass;
        gTupleType[4]->mGenericsTypesNum = 5;
        gTupleType[4]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[4]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[4]->mGenericsTypes[2]->mClass = gGParamClass[2];
        gTupleType[4]->mGenericsTypes[3]->mClass = gGParamClass[3];
        gTupleType[4]->mGenericsTypes[4]->mClass = gGParamClass[4];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$6") == 0) {
        gTupleType[5]->mClass = klass;
        gTupleType[5]->mGenericsTypesNum = 6;
        gTupleType[5]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[5]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[5]->mGenericsTypes[2]->mClass = gGParamClass[2];
        gTupleType[5]->mGenericsTypes[3]->mClass = gGParamClass[3];
        gTupleType[5]->mGenericsTypes[4]->mClass = gGParamClass[4];
        gTupleType[5]->mGenericsTypes[5]->mClass = gGParamClass[5];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$7") == 0) {
        gTupleType[6]->mClass = klass;
        gTupleType[6]->mGenericsTypesNum = 7;
        gTupleType[6]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[6]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[6]->mGenericsTypes[2]->mClass = gGParamClass[2];
        gTupleType[6]->mGenericsTypes[3]->mClass = gGParamClass[3];
        gTupleType[6]->mGenericsTypes[4]->mClass = gGParamClass[4];
        gTupleType[6]->mGenericsTypes[5]->mClass = gGParamClass[5];
        gTupleType[6]->mGenericsTypes[6]->mClass = gGParamClass[6];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Tuple$8") == 0) {
        gTupleType[7]->mClass = klass;
        gTupleType[7]->mGenericsTypesNum = 8;
        gTupleType[7]->mGenericsTypes[0]->mClass = gGParamClass[0];
        gTupleType[7]->mGenericsTypes[1]->mClass = gGParamClass[1];
        gTupleType[7]->mGenericsTypes[2]->mClass = gGParamClass[2];
        gTupleType[7]->mGenericsTypes[3]->mClass = gGParamClass[3];
        gTupleType[7]->mGenericsTypes[4]->mClass = gGParamClass[4];
        gTupleType[7]->mGenericsTypes[5]->mClass = gGParamClass[5];
        gTupleType[7]->mGenericsTypes[6]->mClass = gGParamClass[6];
        gTupleType[7]->mGenericsTypes[7]->mClass = gGParamClass[7];
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Range") == 0) {
        gRangeType->mClass = klass;
        gRangeClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Bytes") == 0) {
        gBytesType->mClass = klass;
        gBytesClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Hash$2") == 0) {
        gHashType->mClass = klass;
        gHashType->mGenericsTypesNum = 2;
        gHashType->mGenericsTypes[0]->mClass = gGParamClass[0];
        gHashType->mGenericsTypes[1]->mClass = gGParamClass[1];
        gHashClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Block") == 0) {
        gBlockType->mClass = klass;
        gBlockClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "String") == 0) {
        gStringType->mClass = klass;
        gStringClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Thread") == 0) {
        gThreadType->mClass = klass;
        gThreadClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Exception") == 0) {
        gExceptionType->mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "OnigurumaRegex") == 0) {
        gRegexType->mClass = klass;
        gOnigurumaRegexClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Type") == 0) {
        gTypeType->mClass = klass;
        gTypeClass = klass;
    }
    else if(strlen(REAL_CLASS_NAME(klass)) == 14 && strstr(REAL_CLASS_NAME(klass), "GenericsParam") == REAL_CLASS_NAME(klass) && REAL_CLASS_NAME(klass)[13] >= '0' && REAL_CLASS_NAME(klass)[13] <= '7') 
    {
        int number;

        number = REAL_CLASS_NAME(klass)[13] - '0';

        ASSERT(number >= 0 && number < CL_GENERICS_CLASS_PARAM_MAX);

        gGParamTypes[number]->mClass = klass;
        gGParamClass[number] = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "anonymous") == 0) {
        gAnonymousType->mClass = klass;
        gAnonymousClass = klass;
    }
}

// result (TRUE): success (FALSE):overflow table
BOOL add_generics_param_type_name(sCLClass* klass, char* name)
{
    sCLGenericsParamTypes* param_types;
    int i;

    if(klass->mGenericsTypesNum >= CL_GENERICS_CLASS_PARAM_MAX) {
        return FALSE;
    }

    /// check the same name class parametor is defined ///
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        param_types = klass->mGenericsTypes + i;

        if(strcmp(CONS_str(&klass->mConstPool, param_types->mNameOffset), name) == 0)
        {
            return FALSE;
        }
    }

    param_types = klass->mGenericsTypes + klass->mGenericsTypesNum;

    param_types->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name, FALSE);

    klass->mGenericsTypesNum++;

    return TRUE;
}

// result (TRUE): success (FALSE):not found the param type name
BOOL add_generics_param_type(sCLClass* klass, char* name, sCLNodeType* extends_type, char num_implements_types, sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX])
{
    sCLGenericsParamTypes* param_types;
    int i;

    /// get param types ///
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        param_types = klass->mGenericsTypes + i;

        if(strcmp(CONS_str(&klass->mConstPool, param_types->mNameOffset), name) == 0)
        {
            break;
        }
    }

    if(i == klass->mGenericsTypesNum) {
        return FALSE;
    }

    param_types = klass->mGenericsTypes + i;

    if(extends_type) { 
        create_cl_type_from_node_type2(&param_types->mExtendsType, extends_type, klass);
    }

    param_types->mNumImplementsTypes = num_implements_types;
    for(i=0; i<num_implements_types; i++) {
        create_cl_type_from_node_type2(&param_types->mImplementsTypes[i], implements_types[i], klass);
    }

    return TRUE;
}

sCLClass* alloc_class_on_compile_time(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL native_, BOOL struct_, BOOL enum_, int parametor_num)
{
    sCLClass* klass;

    klass = alloc_class(namespace, class_name, private_, abstract_, interface, dynamic_typing_, final_, native_, struct_, enum_, parametor_num);

    set_special_class_to_global_pointer_of_type(klass, parametor_num);

    return klass;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class_from_classpath_on_compile_time(char* real_class_name, BOOL solve_dependences)
{
    sCLClass* result;

    result = load_class_from_classpath(real_class_name, solve_dependences);
    if(result) {
        if(!entry_alias_of_class(result)) {
            return NULL;
        }
        if(!add_compile_data(result, 1, result->mNumMethods, kCompileTypeLoad, result->mGenericsTypesNum)) {
            return FALSE;
        }
        set_special_class_to_global_pointer_of_type(result, result->mGenericsTypesNum);
    }

    return result;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL solve_dependences, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name, parametor_num);

    return load_class_from_classpath_on_compile_time(real_class_name, solve_dependences);
}

// result: (TRUE) success (FALSE) faield
BOOL load_fundamental_classes_on_compile_time()
{
    /// For gHashType and gArrayType, gTupleType GenericsParam must be loaded at first ///
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

        snprintf(real_class_name, CL_REAL_CLASS_NAME_MAX, "GenericsParam%d", i);

        load_class_from_classpath_on_compile_time(real_class_name, TRUE);
    }

    load_class_from_classpath_on_compile_time("anonymous", TRUE);

    load_class_from_classpath_on_compile_time("void", TRUE);
    load_class_from_classpath_on_compile_time("int", TRUE);
    load_class_from_classpath_on_compile_time("byte", TRUE);
    load_class_from_classpath_on_compile_time("short", TRUE);
    load_class_from_classpath_on_compile_time("uint", TRUE);
    load_class_from_classpath_on_compile_time("long", TRUE);
    load_class_from_classpath_on_compile_time("Bytes", TRUE);
    load_class_from_classpath_on_compile_time("char", TRUE);
    load_class_from_classpath_on_compile_time("float", TRUE);
    load_class_from_classpath_on_compile_time("double", TRUE);
    load_class_from_classpath_on_compile_time("bool", TRUE);
    load_class_from_classpath_on_compile_time("String", TRUE);
    load_class_from_classpath_on_compile_time("Array$1", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$1", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$2", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$3", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$4", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$5", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$6", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$7", TRUE);
    load_class_from_classpath_on_compile_time("Tuple$8", TRUE);
    load_class_from_classpath_on_compile_time("Hash$2", TRUE);

    load_class_from_classpath_on_compile_time("Object", TRUE);
    load_class_from_classpath_on_compile_time("Class", TRUE);
    load_class_from_classpath_on_compile_time("Field", TRUE);
    load_class_from_classpath_on_compile_time("Method", TRUE);

    load_class_from_classpath_on_compile_time("Exception", TRUE);
    load_class_from_classpath_on_compile_time("Type", TRUE);

    load_class_from_classpath_on_compile_time("NullPointerException", TRUE);
    load_class_from_classpath_on_compile_time("InvalidRegexException", TRUE);
    load_class_from_classpath_on_compile_time("RangeException", TRUE);
    load_class_from_classpath_on_compile_time("ConvertingStringCodeException", TRUE);
    load_class_from_classpath_on_compile_time("ClassNotFoundException", TRUE);
    load_class_from_classpath_on_compile_time("IOException", TRUE);
    load_class_from_classpath_on_compile_time("OverflowException", TRUE);
    load_class_from_classpath_on_compile_time("MethodMissingException", TRUE);
    load_class_from_classpath_on_compile_time("OutOfRangeOfStackException", TRUE);
    load_class_from_classpath_on_compile_time("OutOfRangeOfFieldException", TRUE);
    load_class_from_classpath_on_compile_time("OverflowStackSizeException", TRUE);

    load_class_from_classpath_on_compile_time("Thread", TRUE);
    load_class_from_classpath_on_compile_time("Block", TRUE);
    load_class_from_classpath_on_compile_time("Range", TRUE);
    load_class_from_classpath_on_compile_time("Regex", TRUE);
    load_class_from_classpath_on_compile_time("Encoding", TRUE);
    load_class_from_classpath_on_compile_time("Enum", TRUE);
    load_class_from_classpath_on_compile_time("GenericsParametor", TRUE);
    load_class_from_classpath_on_compile_time("OnigurumaRegex", TRUE);

    load_class_from_classpath_on_compile_time("Null", TRUE);
    load_class_from_classpath_on_compile_time("Command", TRUE);

    if(!run_all_loaded_class_fields_initializer()) {
        return FALSE;
    }

    return TRUE;
}
