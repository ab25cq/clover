#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

//////////////////////////////////////////////////
// get class
//////////////////////////////////////////////////
unsigned int get_hash(char* name)
{
    unsigned int hash;
    char* p;
    
    hash = 0;
    p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

sCLClass* gClassHashList[CLASS_HASH_SIZE];

BOOL is_valid_class_pointer(void* class_pointer)
{
    int i;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;

            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                if(klass == class_pointer) {
                    return TRUE;
                }
                klass = next_klass;
            }
        }
    }

    return FALSE;
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name)
{
    unsigned int hash;
    sCLClass* klass;
    
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    klass = gClassHashList[hash];

    while(klass) {
        if(strcmp(REAL_CLASS_NAME(klass), real_class_name) == 0) {
            return klass;
        }
        else {
            klass = klass->mNextClass;
        }
    }

    return klass;
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_)
{
    int i;
    sCLClass* klass;

    klass = cl_get_class(real_class_name);

    if(type_) {
        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            if(klass == gAnonymousType[i].mClass) { 
                if(i < type_->mGenericsTypesNum) {
                    return type_->mGenericsTypes[i];
                }
                else {
                    return NULL;
                }
            }
        }
    }


    return klass;
}

void create_real_class_name(char* result, int result_size, char* namespace, char* class_name)
{
    if(namespace[0] == 0) {
        xstrncpy(result, class_name, result_size);
    }
    else {
        xstrncpy(result, namespace, result_size);
        xstrncat(result, "::", result_size);
        xstrncat(result, class_name, result_size);
    }
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    result = cl_get_class(real_class_name);
    if(result == NULL) {
        /// default namespace ///
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, "", class_name);
        return cl_get_class(real_class_name);
    }
    else {
        return result;
    }
}

//////////////////////////////////////////////////
// alloc class
//////////////////////////////////////////////////
static void add_class_to_class_table(char* namespace, char* class_name, sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    /// added this to class table ///
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;
}

static void remove_class_from_class_table(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sCLClass* klass;
    sCLClass* previous_klass;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    /// added this to class table ///
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    klass = gClassHashList[hash];
    previous_klass = NULL;
    while(klass) {
        if(strcmp(REAL_CLASS_NAME(klass), real_class_name) == 0) {
            if(previous_klass) {
                previous_klass->mNextClass = klass->mNextClass;
            }
            else {
                gClassHashList[hash] = klass->mNextClass;
            }
        }

        previous_klass = klass;
        klass = klass->mNextClass;
    }
}

static void initialize_hidden_class_method_and_flags(char* namespace, char* class_name, sCLClass* klass)
{
    if(strcmp(namespace, "") == 0) {
        /// special classes ///
        if(strcmp(class_name, "Array") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_array(klass);
        }
        else if(strcmp(class_name, "Hash") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_hash(klass);
        }
        else if(strcmp(class_name, "String") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_string(klass);
        }
        else if(strcmp(class_name, "ClassName") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_class_name(klass);
        }
        else if(strcmp(class_name, "Block") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_block(klass);
        }
        else if(strcmp(class_name, "Thread") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_thread(klass);
        }
        else if(strcmp(class_name, "Mutex") == 0) {
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_mutex(klass);
        }
        /// immediate value classes ///
        else if(strcmp(class_name, "void") == 0 || strcmp(class_name, "int") == 0 || strcmp(class_name, "float") == 0 || strcmp(class_name, "bool") == 0 || strcmp(class_name, "null") == 0) 
        {
            klass->mFlags |= CLASS_FLAGS_IMMEDIATE_VALUE_CLASS;
            initialize_hidden_class_method_of_immediate_value(klass);
        }
        /// user class ///
        else {
            initialize_hidden_class_method_of_user_object(klass);
        }
    }
    /// user class ///
    else {
        initialize_hidden_class_method_of_user_object(klass);
    }
}

// result should be not NULL
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL open_, char* generics_types[CL_CLASS_TYPE_VARIABLE_MAX], int generics_types_num)
{
    sCLClass* klass;
    sCLClass* klass2;
    int i;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    /// if there is a class which is entried already, return the class
    klass2 = cl_get_class_with_namespace(namespace, class_name);
    
    if(klass2) {
        return klass2;
    }

    klass = CALLOC(1, sizeof(sCLClass));

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(namespace, class_name, klass);

    sConst_init(&klass->mConstPool);

    klass->mFlags |= (private_ ? CLASS_FLAGS_PRIVATE:0) | (open_ ? CLASS_FLAGS_OPEN:0);

    klass->mSizeMethods = 4;
    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);
    klass->mSizeFields = 4;
    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    memset(klass->mSuperClassesOffset, 0, sizeof(klass->mSuperClassesOffset));  // paranoia
    klass->mNumSuperClasses = 0; // paranoia

    klass->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, class_name);  // class name
    klass->mNameSpaceOffset = append_str_to_constant_pool(&klass->mConstPool, namespace);   // namespace 

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);
    klass->mRealClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);  // real class name

    add_class_to_class_table(namespace, class_name, klass);

    klass->mGenericsTypesNum = generics_types_num;

    for(i=0; i<generics_types_num; i++) {
        klass->mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, generics_types[i]);
    }

    klass->mSizeDependences = 4;
    klass->mDepedencesOffset = CALLOC(1, sizeof(int)*klass->mSizeDependences);
    klass->mNumDependences = 0;

    klass->mNumVirtualMethodMap = 0;
    klass->mSizeVirtualMethodMap = 4;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);

    if(strcmp(REAL_CLASS_NAME(klass), "void") == 0) {
        gVoidType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "int") == 0) {
        gIntType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "float") == 0) {
        gFloatType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "bool") == 0) {
        gBoolType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Object") == 0) {
        gObjectType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "String") == 0) {
        gStringType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "ClassName") == 0) {
        gClassNameType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Array") == 0) {
        gArrayType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Exception") == 0) {
        gExceptionType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Hash") == 0) {
        gHashType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Block") == 0) {
        gBlockType.mClass = klass;
    }

    return klass;
}

static void free_class(sCLClass* klass)
{
    sConst_free(&klass->mConstPool);

    if(klass->mMethods) {
        int i;

        for(i=0; i<klass->mNumMethods; i++) {
            sCLMethod* method = klass->mMethods + i;
            if(method->mParamTypes) FREE(method->mParamTypes);

            if(!(method->mFlags & CL_NATIVE_METHOD) && method->uCode.mByteCodes.mCode != NULL) {
                sByteCode_free(&method->uCode.mByteCodes);
            }

            if(method->mNumBlockType > 0) {
                if(method->mBlockType.mParamTypes) FREE(method->mBlockType.mParamTypes);
            }
        }
        FREE(klass->mMethods);
    }

    if(klass->mFields) {
        int i;
        for(i=0; i<klass->mNumFields; i++) {
            sCLField* field = klass->mFields + i;

            if(field->mInitializar.mSize > 0) {
                sByteCode_free(&field->mInitializar);
            }
        }
        FREE(klass->mFields);
    }

    if(klass->mDepedencesOffset) {
        FREE(klass->mDepedencesOffset);
    }

    if(klass->mVirtualMethodMap) {
        FREE(klass->mVirtualMethodMap);
    }

    FREE(klass);
}

// left_type is stored calss. right_type is class of value.
BOOL substition_posibility_of_class(sCLClass* left_type, sCLClass* right_type)
{
ASSERT(left_type != NULL);
ASSERT(right_type != NULL);

    /// null type is special ///
    if(right_type == gNullType.mClass) {
        if(search_for_super_class(left_type, gObjectType.mClass)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        if(left_type != right_type) {
            if(!search_for_super_class(right_type, left_type)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// left_type is stored type. right_type is value type.
BOOL substition_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
ASSERT(left_type->mClass != NULL);
ASSERT(right_type->mClass != NULL);

    /// null type is special ///
    if(right_type->mClass == gNullType.mClass) {
        if(search_for_super_class(left_type->mClass, gObjectType.mClass)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        int i;

        if(left_type->mClass != right_type->mClass) {
            if(!search_for_super_class(right_type->mClass, left_type->mClass)) {
                return FALSE;
            }
        }
        if(left_type->mGenericsTypesNum != right_type->mGenericsTypesNum) {
            return FALSE;
        }

        for(i=0; i<left_type->mGenericsTypesNum; i++) {
            if(!substition_posibility_of_class(left_type->mGenericsTypes[i], right_type->mGenericsTypes[i])) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// fields
//////////////////////////////////////////////////
int get_static_fields_num(sCLClass* klass)
{
    int static_field_num;
    int i;

    static_field_num = 0;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD) {
            static_field_num++;
        }
    }

    return static_field_num;
}

static int get_static_fields_num_including_super_class(sCLClass* klass)
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

    static_field_num += get_static_fields_num(klass);

    return static_field_num;
}

BOOL run_fields_initializer(CLObject object, sCLClass* klass)
{
    int i;
    int static_field_num;

    static_field_num = 0;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;
        int max_stack;
        int lv_num;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD) {
            static_field_num++;
        }
        else if(cl_field->mInitializar.mSize > 0) {
            MVALUE result;

            lv_num = cl_field->mInitializarLVNum;
            max_stack = cl_field->mInitializarMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializar, &klass->mConstPool, lv_num, max_stack)) {
                return FALSE;
            }

            CLUSEROBJECT(object)->mFields[i-static_field_num] = result;
        }
    }

    return TRUE;
}

static BOOL run_class_fields_initializer(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;
        int max_stack;
        int lv_num;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD) {
            MVALUE result;

            lv_num = cl_field->mInitializarLVNum;
            max_stack = cl_field->mInitializarMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializar, &klass->mConstPool, lv_num, max_stack)) {
                return FALSE;
            }

            cl_field->uValue.mStaticField = result;
        }
    }

    return TRUE;
}

static void mark_class_fields_of_class(sCLClass* klass, unsigned char* mark_flg)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = &klass->mFields[i];

        if(field->mFlags & CL_STATIC_FIELD) {
            mark_object(field->uValue.mStaticField.mObjectValue, mark_flg);
        }
    }
}

static void mark_class_fields_of_class_and_super_class(sCLClass* klass, unsigned char* mark_flg)
{
    int i;

    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        mark_class_fields_of_class(super_class, mark_flg);
    }

    mark_class_fields_of_class(klass, mark_flg);
}

void mark_class_fields(unsigned char* mark_flg)
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                mark_class_fields_of_class_and_super_class(klass, mark_flg);
                klass = next_klass;
            }
        }
    }
}

static int get_sum_of_fields_on_super_classes(sCLClass* klass)
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

// return field number
int get_field_num_including_super_classes(sCLClass* klass)
{
    return get_sum_of_fields_on_super_classes(klass) + klass->mNumFields;
}

// return field number
int get_field_num_including_super_classes_without_class_field(sCLClass* klass)
{
    return get_field_num_including_super_classes(klass) - get_static_fields_num_including_super_class(klass);
}

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name, BOOL class_field)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;
        BOOL class_field2;

        field = klass->mFields + i;
        class_field2 = field->mFlags & CL_STATIC_FIELD;
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0 && class_field == class_field2) {
            return klass->mFields + i;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// methods
///////////////////////////////////////////////////////////////////////////////
void alloc_bytecode_of_method(sCLMethod* method)
{
    sByteCode_init(&method->uCode.mByteCodes);
}

// result is setted on (sCLClass** result_class)
// result (TRUE) success on solving or not solving (FALSE) error on solving the generic type
BOOL solve_generics_types(sCLClass* klass, sCLNodeType* type_, sCLClass** result_class)
{
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(klass == gAnonymousType[i].mClass) { 
            if(i < type_->mGenericsTypesNum) {
                *result_class = type_->mGenericsTypes[i];
                return TRUE;
            }
            else {
                *result_class = NULL;
                return FALSE;
            }
        }
    }

    *result_class = klass;
    return TRUE;
}

// result: (FALSE) fail (TRUE) success
// result of parametors is setted on the result
// if type_ is NULL, it is not solved with generics type.
BOOL cl_type_to_node_type(sCLNodeType* result, sCLType* cl_type, sCLNodeType* type_, sCLClass* klass)
{
    char* real_class_name;
    int i;
    
    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);
    result->mClass = cl_get_class(real_class_name);

    if(result->mClass == NULL) {
        return FALSE;
    }

    if(type_) {
        if(!solve_generics_types(result->mClass, type_, &result->mClass)) {
            return FALSE;
        }
    }

    result->mGenericsTypesNum = cl_type->mGenericsTypesNum;

    for(i=0; i<cl_type->mGenericsTypesNum; i++) {
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, cl_type->mGenericsTypesOffset[i]);
        result->mGenericsTypes[i] = cl_get_class(real_class_name);

        if(result->mGenericsTypes[i] == NULL) {
            return FALSE;
        }

        if(type_) {
            if(!solve_generics_types(result->mGenericsTypes[i], type_, &result->mGenericsTypes[i])) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL check_method_params(sCLMethod* method, sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
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
        }
    }

    return FALSE;
}

void create_real_method_name(char* real_method_name, int real_method_name_size, char* method_name, int num_params)
{
    char num_params_buf[16];

    snprintf(num_params_buf, 16, "%d", num_params);

    xstrncpy(real_method_name, method_name, real_method_name_size);
    xstrncat(real_method_name, num_params_buf, real_method_name_size);
}

static sCLMethod* search_for_method_from_virtual_method_table(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
    char real_method_name[CL_VMT_NAME_MAX+1];
    int hash;
    sVMethodMap* item;
    int i;

    create_real_method_name(real_method_name, CL_VMT_NAME_MAX, method_name, num_params);

    hash = get_hash(real_method_name) % klass->mSizeVirtualMethodMap;

    item = klass->mVirtualMethodMap + hash;

    while(1) {
        if(item->mMethodName[0] == 0) {
            break;
        }
        else {
            sCLMethod* method;

            method = klass->mMethods + item->mMethodIndex;

            if(check_method_params(method, klass, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type))
            {
                return method;
            }
            else {
                item++;

                if(item == klass->mVirtualMethodMap + klass->mSizeVirtualMethodMap) {
                    item = klass->mVirtualMethodMap;
                }
                else if(item == klass->mVirtualMethodMap + hash) {
                    break;
                }
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
// if type_ is NULL, don't solve generics type
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
    sCLMethod* result;

    result = search_for_method_from_virtual_method_table(klass, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type);

    *founded_class = klass;
    
    if(result == NULL) {
        int i;

        for(i=klass->mNumSuperClasses-1; i>=0; i--) {
            char* real_class_name;
            sCLClass* super_class;
            
            real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
            super_class = cl_get_class(real_class_name);

            ASSERT(super_class != NULL);     // checked on load time

            result = search_for_method_from_virtual_method_table(super_class, method_name, class_params, num_params, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type);
            if(result) {
                *founded_class = super_class;
                break;
            }
        }
    }

    //ASSERT(result != NULL);

    return result;
}

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == searched_class) {
            return TRUE;                // found
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////
// native method
//////////////////////////////////////////////////
struct sNativeMethodStruct {
    unsigned int mHash;
    const char* mPath;
    fNativeMethod mFun;
};

typedef struct sNativeMethodStruct sNativeMethod;

// manually sort is needed
sNativeMethod gNativeMethods[] = {
    { 854, "Array.add", Array_add },
    { 918, "Mutex.run", Mutex_run },
    { 1044, "int.to_str", int_to_str },
    { 1068, "Array.Array", Array_Array },
    { 1078, "Thread.join", Thread_join },
    { 1091, "String.char", String_char },
    { 1103, "Array.items", Array_items },
    { 1108, "Mutex.Mutex", Mutex_Mutex },
    { 1127, "bool.to_int", bool_to_int },
    { 1127, "int.to_bool", int_to_bool },
    { 1133, "System.exit", System_exit },
    { 1141, "bool.to_str", bool_to_str },
    { 1142, "null.to_int", null_to_int },
    { 1156, "null.to_str", null_to_str },
    { 1199, "Array.length", Array_length },
    { 1222, "Clover.print", Clover_print },
    { 1228, "System.sleep", System_sleep },
    { 1233, "float.to_int", float_to_int },
    { 1239, "null.to_bool", null_to_bool },
    { 1246, "Thread.Thread", Thread_Thread },
    { 1247, "float.to_str", float_to_str },
    { 1308, "String.String", String_String },
    { 1309, "String.append", String_append },
    { 1319, "String.length", String_length },
    { 1340, "System.getenv", System_getenv },
    { 1476, "Object.is_child", Object_is_child },
    { 1600, "ClassName.to_str", ClassName_to_str },
    { 1691, "Object.class_name", Object_class_name },
    { 1711, "Object.instanceof", Object_instanceof } ,
    { 1959, "Clover.show_classes", Clover_show_classes },
    { 2116, "Clover.output_to_str", Clover_output_to_str }, 
};

static fNativeMethod get_native_method(char* path)
{
    unsigned int hash;
    unsigned int top;
    unsigned int bot;
    int n;

    hash = get_hash(path);

    top = 0;
    bot = sizeof(gNativeMethods) / sizeof(sNativeMethod);

    while(1) {
        unsigned int mid = (top + bot) / 2;

        if(gNativeMethods[mid].mHash == hash) {
            n = mid;
            while(1) {
                if(strcmp(gNativeMethods[n].mPath, path) == 0) {
                    return gNativeMethods[n].mFun;
                }
                else if(gNativeMethods[n].mHash != hash) {
                    break;
                }
                n++;
            }

            n = mid;
            while(1) {
                if(strcmp(gNativeMethods[n].mPath, path) == 0) {
                    return gNativeMethods[n].mFun;
                }
                else if(gNativeMethods[n].mHash != hash) {
                    break;
                }
                n--;
            }

            return NULL;
        }

        if(mid == top) break;
        if(hash < gNativeMethods[mid].mHash) {
            bot = mid;
        }
        else {
            top = mid;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////
// load and save class
//////////////////////////////////////////////////
static BOOL read_from_file(int f, void* buf, size_t size)
{
    size_t size2;

    size2 = read(f, buf, size);

    if(size2 < size) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_char_from_file(int fd, char* c)
{
    return read_from_file(fd, c, sizeof(char));
}

static BOOL read_int_from_file(int fd, int* n)
{
    return read_from_file(fd, n, sizeof(int));
}

static BOOL read_type_from_file(int fd, sCLType* type)
{
    int j;
    char c;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    type->mClassNameOffset = n;

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    type->mGenericsTypesNum = c;
    for(j=0; j<type->mGenericsTypesNum; j++) {
        if(!read_int_from_file(fd, &n)) {
            return FALSE;
        }

        type->mGenericsTypesOffset[j] = n;
    }

    return TRUE;
}

static BOOL read_params_from_file(int fd, int* num_params, sCLType** param_types)
{
    int j;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    
    *num_params = n;

    if(*num_params > 0) {
        *param_types = CALLOC(1, sizeof(sCLType)*(*num_params));
        for(j=0; j<*num_params; j++) {
            if(!read_type_from_file(fd, (*param_types) + j)) {
                return FALSE;
            }
        }
    }
    else {
        *param_types = NULL;
    }

    return TRUE;
}

static BOOL read_block_type_from_file(int fd, sCLBlockType* block_type)
{
    int n;

    if(!read_type_from_file(fd, &block_type->mResultType)) {
        return FALSE;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    block_type->mNameOffset = n;

    if(!read_params_from_file(fd, &block_type->mNumParams, &block_type->mParamTypes)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_field_from_file(int fd, sCLField* field)
{
    int n;
    int len_bytecodes;
    int* bytecodes;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mFlags = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mNameOffset = n;

    field->uValue.mStaticField.mIntValue = 0; //read_mvalue_from_buffer(p, file_data);

    /// initializer ////
    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    len_bytecodes = n;

    if(len_bytecodes > 0) {
        sByteCode_init(&field->mInitializar);
        bytecodes = MALLOC(sizeof(int)*len_bytecodes);

        if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
            FREE(bytecodes);
            return FALSE;
        }

        append_buf_to_bytecodes(&field->mInitializar, bytecodes, len_bytecodes);

        FREE(bytecodes);
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mInitializarLVNum = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    field->mInitializarMaxStack = n;

    if(!read_type_from_file(fd, &field->mType)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_method_from_buffer(sCLClass* klass, sCLMethod* method, int fd)
{
    int n;
    int i;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mFlags = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mNameOffset = n;
    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mPathOffset = n;

    if(method->mFlags & CL_NATIVE_METHOD) {
        char* method_path;

        method_path = CONS_str(&klass->mConstPool, method->mPathOffset);

        method->uCode.mNativeMethod = get_native_method(method_path);
        if(method->uCode.mNativeMethod == NULL) {
            vm_error("native method(%s) is not found\n", method_path);
        }
    }
    else {
        int len_bytecodes;

        alloc_bytecode_of_method(method);

        if(!read_int_from_file(fd, &n)) {
            return FALSE;
        }
        len_bytecodes = n;

        if(len_bytecodes > 0) {
            int* bytecodes = MALLOC(sizeof(int)*len_bytecodes);

            if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
                FREE(bytecodes);
                return FALSE;
            }

            append_buf_to_bytecodes(&method->uCode.mByteCodes, bytecodes, len_bytecodes);

            FREE(bytecodes);
        }
    }

    if(!read_type_from_file(fd, &method->mResultType)) {
        return FALSE;
    }
    if(!read_params_from_file(fd, &method->mNumParams, &method->mParamTypes)) {
        return FALSE;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumLocals = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mMaxStack = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumBlockType = n;

    if(method->mNumBlockType >= 1) {
        if(!read_block_type_from_file(fd, &method->mBlockType)) {
            return FALSE;
        }
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumException = n;

    for(i=0; i<method->mNumException; i++) {
        if(!read_int_from_file(fd, &n)) {
            return FALSE;
        }
        method->mExceptionClassNameOffset[i] = n;
    }

    return TRUE;
}

BOOL read_virtual_method_map(int fd, sCLClass* klass)
{
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    klass->mSizeVirtualMethodMap = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    klass->mNumVirtualMethodMap = n;

    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);

    return read_from_file(fd, klass->mVirtualMethodMap, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);
}

static sCLClass* read_class_from_file(int fd)
{
    int i;
    sCLClass* klass;
    int const_pool_len;
    char* buf;
    int n;
    char c;

    klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mFlags = n;

    /// load constant pool ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    const_pool_len = n;

    buf = MALLOC(const_pool_len);

    if(!read_from_file(fd, buf, const_pool_len)) {
        FREE(buf);
        return NULL;
    }

    append_buf_to_constant_pool(&klass->mConstPool, buf, const_pool_len);

    FREE(buf);

    /// load namespace offset ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNameSpaceOffset = c;

    /// load class name offset ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mClassNameOffset = c;

    /// load real class name offset //
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mRealClassNameOffset = c;

    /// load fields ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    klass->mSizeFields = klass->mNumFields = n;

    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    for(i=0; i<klass->mNumFields; i++) {
        if(!read_field_from_file(fd, klass->mFields + i)) {
            return FALSE;
        }
    }

    /// load methods ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    klass->mSizeMethods = klass->mNumMethods = n;

    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        if(!read_method_from_buffer(klass, klass->mMethods + i, fd)) {
            return NULL;
        }
    }

    /// load super classes ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNumSuperClasses = c;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        if(!read_int_from_file(fd, &n)) {
            return NULL;
        }
        klass->mSuperClassesOffset[i] = n;
    }

    /// load class params of generics ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mGenericsTypesNum = c;
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        if(!read_int_from_file(fd, &n)) {
            return NULL;
        }
        klass->mGenericsTypesOffset[i] = n;
    }

    /// load dependences ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mNumDependences = n;
    klass->mDepedencesOffset = CALLOC(1, sizeof(int)*klass->mNumDependences);
    for(i=0; i<klass->mNumDependences; i++) {
        if(!read_int_from_file(fd, &n)) {
            return NULL;
        }
        klass->mDepedencesOffset[i] = n;
    }

    /// write virtual method table ///
    if(!read_virtual_method_map(fd, klass)) {
        return NULL;
    }

    return klass;
}

static BOOL check_method_and_field_types_offset(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = klass->mFields + i;
        sCLClass* klass2;
        
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));

        if(klass2 == NULL) {
            if(load_class_from_classpath(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));
                return FALSE;
            }
        }
    }

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method = klass->mMethods + i;
        sCLClass* klass2;
        int j;

        /// result type ///
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));

        if(klass2 == NULL) {
            if(load_class_from_classpath(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));
                return FALSE;
            }
        }

        /// param types ///
        for(j=0; j<method->mNumParams; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset)); 

            if(klass2 == NULL) {
                if(load_class_from_classpath(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset));
                    return FALSE;
                }
            }
        }

        /// block type ///
        if(method->mNumBlockType == 1) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));

            if(klass2 == NULL) {
                if(load_class_from_classpath(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));
                    return FALSE;
                }
            }

            for(j=0; j<method->mBlockType.mNumParams; j++) {
                klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));

                if(klass2 == NULL) {
                    if(load_class_from_classpath(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset), TRUE) == NULL)
                    {
                        vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));
                        return FALSE;
                    }
                }
            }
        }

        /// exception ///
        for(j=0; j<method->mNumException; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]));

            if(klass2 == NULL) {
                if(load_class_from_classpath(CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]));
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

static BOOL check_super_class_offsets(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        
        super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));

        if(super_class == NULL) {
            if(load_class_from_classpath(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL check_dependece_offsets(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumDependences; i++) {
        sCLClass* dependence_class;
        
        dependence_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]));
        if(dependence_class == NULL) {
            if(load_class_from_classpath(CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]));
                return FALSE;
            }
        }
    }

    return TRUE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class(char* file_name, BOOL resolve_dependences)
{
    sCLClass* klass;
    sCLClass* klass2;
    int fd;
    char c;
    int size;

    fd = open(file_name, O_RDONLY);

    if(fd < 0) {
        return NULL;
    }

    /// check magic number. Is this Clover object file? ///
    if(!read_from_file(fd, &c, 1) || c != 12) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 17) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 33) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 79) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'C') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'L') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'O') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'V') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'E') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'R') { close(fd); return NULL; }

    klass = read_class_from_file(fd);
    close(fd);

    if(klass == NULL) {
        return NULL;
    }

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    /// if there is a class which is entried to class table already, return the class
    klass2 = cl_get_class_with_namespace(NAMESPACE_NAME(klass), CLASS_NAME(klass));
    
    if(klass2) {
        free_class(klass);
        return klass2;
    }

    //// add class to class table ///
    add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    if(resolve_dependences) {
        if(!check_super_class_offsets(klass)){
            vm_error("can't load class %s because of the super classes\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }

        if(!check_method_and_field_types_offset(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }

        if(!check_dependece_offsets(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }
    }

    /// call class field initializer ///
    if(!run_class_fields_initializer(klass)) {
        remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
        free_class(klass);
        return NULL;
    }

    return klass;
}

// result : (TRUE) found (FALSE) not found
// set class file path on class_file arguments
static BOOL search_for_class_file_from_class_name(char* class_file, unsigned int class_file_size, char* real_class_name)
{
    sCLClass* clover;
    MVALUE mvalue;
    int i;
    char* cwd;

    /// default search path ///
    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        if(i == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clo", DATAROOTDIR, real_class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clo", DATAROOTDIR, real_class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }

    /// current working directory ///
    cwd = getenv("PWD");
    if(cwd == NULL) {
        fprintf(stderr, "PWD environment path is NULL\n");
        return FALSE;
    }

    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        if(i == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clo", cwd, real_class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clo", cwd, real_class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }

/*
    /// user search path ///
    clover = cl_get_class("Clover");

    if(clover == NULL) {
        fprintf(stderr, "can't load Clover class\n");
        return FALSE;
    }

    if(cl_get_class_field(clover, "class_file_path", gArrayType.mClass, &mvalue)) {
        CLObject array;
        int j;

        array = mvalue.mObjectValue;

        for(j=0; j<CLARRAY(array)->mLen; j++) {
            char class_file_path[PATH_MAX];
            MVALUE mvalue2;
            CLObject item;
            int k;

            if(!cl_get_array_element(array, j, gStringType.mClass, &mvalue2)) {
                continue;
            }

            item = mvalue2.mObjectValue;

            if((int)wcstombs(class_file_path, CLSTRING(item)->mChars, PATH_MAX) < 0) {
                return FALSE;
            }
            
            for(k=CLASS_VERSION_MAX; k>=1; k--) {
                if(k == 1) {
                    snprintf(class_file, class_file_size, "%s/%s.clo", class_file_path, real_class_name);
                }
                else {
                    snprintf(class_file, class_file_size, "%s/%s#%d.clo", class_file_path, real_class_name, k);
                }

                if(access(class_file, F_OK) == 0) {
                    return TRUE;
                }
            }
        }
    }
*/

    return FALSE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_from_classpath(char* real_class_name, BOOL resolve_dependences)
{
    char class_file[PATH_MAX];

    if(!search_for_class_file_from_class_name(class_file, PATH_MAX, real_class_name)) {
        return NULL;
    }

    return load_class(class_file, resolve_dependences);
}

//////////////////////////////////////////////////
// show class
//////////////////////////////////////////////////
void show_class_list()
{
    int i;

    cl_print("-+- class list -+-\n");
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;

            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                cl_print("%s\n", REAL_CLASS_NAME(klass));
                klass = next_klass;
            }
        }
    }
}

//////////////////////////////////////////////////
// accessor function
//////////////////////////////////////////////////
// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass)
{
    if(klass->mNumSuperClasses > 0) {
        char* real_class_name;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[klass->mNumSuperClasses-1]);
        return cl_get_class(real_class_name);
    }
    else {
        return NULL;
    }
}

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
sCLNodeType gIntType;      // foudamental classes
sCLNodeType gFloatType;
sCLNodeType gVoidType;
sCLNodeType gBoolType;

sCLNodeType gObjectType;
sCLNodeType gStringType;
sCLNodeType gArrayType;
sCLNodeType gHashType;
sCLNodeType gNullType;
sCLNodeType gBlockType;
sCLNodeType gExceptionType;
sCLNodeType gExNullPointerType;
sCLNodeType gExRangeType;
sCLNodeType gExConvertingStringCodeType;
sCLNodeType gExClassNotFoundType;
sCLNodeType gClassNameType;
sCLNodeType gThreadType;

sCLNodeType gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];

static void create_anonymous_classes()
{
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        char class_name[CL_CLASS_NAME_MAX];
        snprintf(class_name, CL_CLASS_NAME_MAX, "anonymous%d", i);

        gAnonymousType[i].mClass = alloc_class("", class_name, FALSE, FALSE, NULL, 0);
    }
}

void class_init()
{
    create_anonymous_classes();
}

void class_final()
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                free_class(klass);
                klass = next_klass;
            }
        }
    }
}

BOOL cl_load_fundamental_classes()
{
    sCLClass* system;
    sCLClass* clover;
    sCLClass* mutex;
    sCLClass* null;

    clover = load_class_from_classpath("Clover", TRUE);
    gVoidType.mClass = load_class_from_classpath("void", TRUE);
    gIntType.mClass = load_class_from_classpath("int", TRUE);
    gFloatType.mClass = load_class_from_classpath("float", TRUE);
    gBoolType.mClass = load_class_from_classpath("bool", TRUE);

    gObjectType.mClass = load_class_from_classpath("Object", TRUE);

    gArrayType.mClass = load_class_from_classpath("Array", TRUE);
    gStringType.mClass = load_class_from_classpath("String", TRUE);
    gHashType.mClass = load_class_from_classpath("Hash", TRUE);

    gBlockType.mClass = load_class_from_classpath("Block", TRUE);
    gExceptionType.mClass = load_class_from_classpath("Exception", TRUE);

    gExNullPointerType.mClass = load_class_from_classpath("NullPointerException", TRUE);
    gExRangeType.mClass = load_class_from_classpath("RangeException", TRUE);
    gExConvertingStringCodeType.mClass = load_class_from_classpath("ConvertingStringCodeException", TRUE);
    gExClassNotFoundType.mClass = load_class_from_classpath("ClassNotFoundException", TRUE);

    gClassNameType.mClass = load_class_from_classpath("ClassName", TRUE);

    gThreadType.mClass = load_class_from_classpath("Thread", TRUE);
    mutex = load_class_from_classpath("Mutex", TRUE);
    system = load_class_from_classpath("System", TRUE);

    gNullType.mClass = load_class_from_classpath("null", TRUE);

    if(gNullType.mClass == NULL || gVoidType.mClass == NULL || gIntType.mClass == NULL || gFloatType.mClass == NULL || gBoolType.mClass == NULL || gObjectType.mClass == NULL || gStringType.mClass == NULL || gBlockType.mClass == NULL || gArrayType.mClass == NULL || gHashType.mClass == NULL || gExceptionType.mClass == NULL || gClassNameType.mClass == NULL || system == NULL || clover == NULL || gThreadType.mClass == NULL || mutex == NULL)
    {
        fprintf(stderr, "can't load fundamental classes\n");
        return FALSE;
    }

    return TRUE;
}
