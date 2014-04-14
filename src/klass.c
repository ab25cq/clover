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

static sCLClass* gClassHashList[CLASS_HASH_SIZE];

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name, BOOL auto_load)
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

    if(auto_load) {
        return load_class_from_classpath(real_class_name, TRUE);
    }

    return klass;
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_, BOOL auto_load)
{
    int i;
    sCLClass* klass;
    
    klass = cl_get_class(real_class_name, auto_load);

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
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name, BOOL auto_load)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    result = cl_get_class(real_class_name, auto_load);
    if(result == NULL) {
        /// default namespace ///
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, "", class_name);
        return cl_get_class(real_class_name, auto_load);
    }
    else {
        return result;
    }
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name, BOOL auto_load)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    return cl_get_class(real_class_name, auto_load);
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

static void initialize_hidden_class_method_and_flags(char* class_name, sCLClass* klass)
{
    /// some special class ///
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
    else if(strcmp(class_name, "Block") == 0) {
        klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
        initialize_hidden_class_method_of_block(klass);
    }
    else if(strcmp(class_name, "void") == 0 || strcmp(class_name, "int") == 0 || strcmp(class_name, "float") == 0 || strcmp(class_name, "bool") == 0) 
    {
        klass->mFlags |= CLASS_FLAGS_IMMEDIATE_VALUE_CLASS;
        initialize_hidden_class_method_of_immediate_value(klass);
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
    klass2 = cl_get_class_with_namespace(namespace, class_name, FALSE);
    
    if(klass2) {
        return klass2;
    }

    klass = CALLOC(1, sizeof(sCLClass));

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(class_name, klass);

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

    klass->mNumVMethodMap = 0;   // paranoia
    klass->mSizeVMethodMap = 3;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVMethodMap);

    klass->mGenericsTypesNum = generics_types_num;

    for(i=0; i<generics_types_num; i++) {
        klass->mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, generics_types[i]);
    }

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
    else if(strcmp(REAL_CLASS_NAME(klass), "Array") == 0) {
        gArrayType.mClass = klass;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Hash") == 0) {
        gHashType.mClass = klass;
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

            if(!(method->mFlags & (CL_NATIVE_METHOD|CL_EXTERNAL_METHOD)) && method->uCode.mByteCodes.mCode != NULL) {
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

    if(klass->mVirtualMethodMap) {
        FREE(klass->mVirtualMethodMap);
    }

    FREE(klass);
}

static BOOL add_external_program_to_class(sCLClass* klass, char* external_progmra_name)
{
    sCLMethod* method;

    if(gStringType.mClass == NULL) {
        cl_print("unexpected error. can't import external program because string class doesn't have loaded yet\n");
        return FALSE;
    }

    if(!add_method(klass, TRUE, FALSE, FALSE, TRUE, external_progmra_name, &gStringType, NULL, -1, FALSE, &method))
    {
        cl_print("overflow method table\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL seek_external_programs(sCLClass* klass, char* path)
{
    DIR* dir = opendir(path);
    if(dir) {
        struct dirent* entry;

        while((entry = readdir(dir)) != 0) {
#if defined(__CYGWIN__)
            char entry2[PATH_MAX];
            char path2[PATH_MAX];
            struct stat stat_;

            if(strstr(entry->d_name, ".exe") == entry->d_name + strlen(entry->d_name) - 4) {
                strcpy(entry2, entry->d_name);

                entry2[strlen(entry2)-4] = 0;
            }
            else {
                strcpy(entry2, entry->d_name);
            }

            snprintf(path2, PATH_MAX, "%s/%s", path, entry2);

            memset(&stat_, 0, sizeof(struct stat));
            if(stat(path2, &stat_) == 0) {
                if(strcmp(entry2, ".") != 0
                    && strcmp(entry2, "..") != 0
                    &&
                    (stat_.st_mode & S_IXUSR ||stat_.st_mode & S_IXGRP||stat_.st_mode & S_IXOTH))
                {
                    if(!add_external_program_to_class(klass, entry2)) {
                        return FALSE;
                    }
                }
            }
#else
            char path2[PATH_MAX];
            struct stat stat_;

            snprintf(path2, PATH_MAX, "%s/%s", path, entry->d_name);
            
            memset(&stat_, 0, sizeof(struct stat));
            if(stat(path2, &stat_) == 0) {
                if(strcmp(entry->d_name, ".") != 0
                    && strcmp(entry->d_name, "..") != 0
                    &&
                    (stat_.st_mode & S_IXUSR ||stat_.st_mode & S_IXGRP||stat_.st_mode & S_IXOTH))
                {
                    if(!add_external_program_to_class(klass, entry->d_name)) {
                        return FALSE;
                    }
                }
            }
#endif
        }
    }

    return TRUE;
}

// result TRUE: (success) FALSE: (can't get external program. $PATH is NULL or something)
BOOL import_external_program(sCLClass* klass)
{
    char buf[PATH_MAX+1];
    char* path;
    char* p;
    
    path = getenv("PATH");
    if(path == NULL) {
        cl_print("PATH is NULL\n");
        return FALSE;
    }

    p = path;
    while(1) {
        char* p2;

        if((p2 = strstr(p, ":")) != NULL) {
            int size = p2 - p;
            if(size > PATH_MAX) {
                size = PATH_MAX;
            }
            memcpy(buf, p, size);
            buf[size] = 0;

            p = p2 + 1;

            if(!seek_external_programs(klass, buf)) {
                return FALSE;
            }
        }
        else {
            strncpy(buf, p, PATH_MAX);

            if(!seek_external_programs(klass, buf)) {
                return FALSE;
            }
            break;
        }
    }

    return TRUE;
}

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLClass* super_klass)
{
    if(super_klass->mNumSuperClasses >= SUPER_CLASS_MAX) {
        return FALSE;
    }

    /// copy super class tables from the super class to this class ///
    if(super_klass->mNumSuperClasses > 0) {
        klass->mNumSuperClasses = super_klass->mNumSuperClasses;
        memcpy(klass->mSuperClassesOffset, super_klass->mSuperClassesOffset, sizeof(int)*klass->mNumSuperClasses);
    }

    klass->mSuperClassesOffset[klass->mNumSuperClasses] = append_str_to_constant_pool(&klass->mConstPool, REAL_CLASS_NAME(super_klass));
    klass->mNumSuperClasses++;

    return TRUE;
}

/*
// result (TRUE) --> success (FALSE) --> overflow method table number
static BOOL add_method_to_virtual_method_table(sCLClass* klass)
{
    if(klass->mNumVMethodMap >= CL_METHODS_MAX) {
        return FALSE;
    }

    if(klass->mNumVMethodMap >= klass->mSizeVMethodMap) {
        int size = klass->mSizeVMethodMap;
        klass->mSizeVMethodMap *= 2;

        klass->mVirtualMethodMap = REALLOC(klass->mVirtualMethodMap, sizeof(sVMethodMap)*klass->mSizeVMethodMap));
        memset(klass->mVirtualMethodMap + sizeof(sVMethodMap) * size, 0, sizeof(sVMethodMap)*(klass->mSizeVMethodMaps - size))
    }

    klass->mVirtualMethodMap[klass->mNumVMethodMap].mSuperClassIndex = ;
    klass->mVirtualMethodMap[klass->mNumVMethodMap++].mMethodIndex = ;
}
*/

// result is setted on (sCLClass** result_class)
// result (TRUE) success on solving or not solving (FALSE) error on solving the generic type
static BOOL solve_generics_types(sCLClass* klass, sCLNodeType* type_, sCLClass** result_class)
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
    /// there is compatibility of immediate value classes in the name space which is different ///
    if( ((left_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mFlags & CLASS_FLAGS_SPECIAL_CLASS)))
    {
        if(strcmp(CLASS_NAME(left_type), CLASS_NAME(right_type)) != 0) {
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
    /// there is compatibility of immediate value classes and special classin the name space which is different ///
    else if(((left_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS))) 
    {
        int i;

        if(strcmp(CLASS_NAME(left_type->mClass), CLASS_NAME(right_type->mClass)) != 0) {
            return FALSE;
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

BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    /// there is compatibility of immediate value classes and special classin the name space which is different ///
    if(((left_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS))) 
    {
        if(strcmp(CLASS_NAME(left_type->mClass), CLASS_NAME(right_type->mClass)) != 0) {
            return FALSE;
        }
    }
    else {
        if(!type_identity(left_type, right_type)) {
            return FALSE;
        }
    }

    return TRUE;
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

static int get_static_fields_num_on_super_class(sCLClass* klass)
{
    int i;
    int static_field_num;

    static_field_num = 0;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name, FALSE);

        ASSERT(super_class != NULL);     // checked on load time

        static_field_num += get_static_fields_num(super_class);
    }

    return static_field_num;
}

int get_static_fields_num_including_super_class(sCLClass* klass)
{
    int i;
    int static_field_num;

    static_field_num = 0;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name, FALSE);

        ASSERT(super_class != NULL);     // checked on load time

        static_field_num += get_static_fields_num(super_class);
    }

    static_field_num += get_static_fields_num(klass);

    return static_field_num;
}

BOOL run_fields_initializar(CLObject object, sCLClass* klass)
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

            if(!field_initializar(&result, &cl_field->mInitializar, &klass->mConstPool, lv_num, max_stack)) {
                return FALSE;
            }

            CLUSEROBJECT(object)->mFields[i-static_field_num] = result;
        }
    }

    return TRUE;
}

BOOL run_class_fields_initializar(sCLClass* klass)
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

            if(!field_initializar(&result, &cl_field->mInitializar, &klass->mConstPool, lv_num, max_stack)) {
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
        super_class = cl_get_class(real_class_name, FALSE);

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

    field->mType.mGenericsTypesNum = type_->mGenericsTypesNum;

    for(i=0; i<field->mType.mGenericsTypesNum; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_->mGenericsTypes[i]), CLASS_NAME(type_->mGenericsTypes[i]));
        field->mType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
    }

    klass->mNumFields++;
    
    return TRUE;
}

// result (TRUE) --> success (FALSE) --> can't find a field which is indicated by an argument
BOOL add_field_initializar(sCLClass* klass, BOOL static_, char* name, MANAGED sByteCode initializar_code, sVarTable* lv_table, int max_stack)
{
    sCLField* field;

    field = get_field(klass, name, static_);

    if(field == NULL) {
        sByteCode_free(&initializar_code);
        return FALSE;
    }

    field->mInitializar = MANAGED initializar_code;
    field->mInitializarLVNum = lv_table->mVarNum + lv_table->mBlockVarNum;
    field->mInitializarMaxStack = max_stack;
    
    return TRUE;
}

static int get_sum_of_fields_on_super_clasess(sCLClass* klass)
{
    int sum = 0;
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name, FALSE);

        ASSERT(super_class != NULL);     // checked on load time

        sum += super_class->mNumFields;
    }

    return sum;
}

static int get_sum_of_fields_on_super_clasess_without_class_fields(sCLClass* klass)
{
    return get_sum_of_fields_on_super_clasess(klass) - get_static_fields_num_on_super_class(klass);
}

// return field number
int get_field_num_including_super_classes(sCLClass* klass)
{
    return get_sum_of_fields_on_super_clasess(klass) + klass->mNumFields;
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
        super_class = cl_get_class(real_class_name, FALSE);

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
        super_class = cl_get_class(real_class_name, FALSE);

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
        super_class = cl_get_class(real_class_name, FALSE);

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

// result is seted on this parametors(sCLNodeType* result)
// if the field is not found, result->mClass is setted on NULL
void get_field_type(sCLClass* klass, sCLField* field, sCLNodeType* result, sCLNodeType* type_)
{
    if(field) {
        int i;

        result->mClass = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset), FALSE);

        if(type_) {
            if(!solve_generics_types(result->mClass, type_, &result->mClass)) {
                memset(result, 0, sizeof(sCLNodeType));
                return;
            }
        }

        result->mGenericsTypesNum = field->mType.mGenericsTypesNum;

        for(i=0; i<field->mType.mGenericsTypesNum; i++) {
            result->mGenericsTypes[i] = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mGenericsTypesOffset[i]), FALSE);

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

///////////////////////////////////////////////////////////////////////////////
// methods
///////////////////////////////////////////////////////////////////////////////
// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
// last parametor returns the method which is added
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, BOOL external, char* name, sCLNodeType* result_type, sCLNodeType* class_params, int num_params, BOOL constructor, sCLMethod** method)
{
    int i, j;
    int path_max;
    char* buf;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

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
    (*method)->mFlags = (static_ ? CL_CLASS_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0) | (constructor ? CL_CONSTRUCTOR:0) | (external ? CL_EXTERNAL_METHOD:0);

    (*method)->mNameOffset = append_str_to_constant_pool(&klass->mConstPool, name);

    path_max = CL_METHOD_NAME_MAX + CL_CLASS_NAME_MAX + 2;
    buf = MALLOC(sizeof(char)*path_max);;
    snprintf(buf, path_max, "%s.%s", CLASS_NAME(klass), name);

    (*method)->mPathOffset = append_str_to_constant_pool(&klass->mConstPool, buf);

    FREE(buf);

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mClass), CLASS_NAME(result_type->mClass));
    (*method)->mResultType.mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    (*method)->mResultType.mGenericsTypesNum = result_type->mGenericsTypesNum;
    for(i=0; i<(*method)->mResultType.mGenericsTypesNum; i++) {
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mGenericsTypes[i]), CLASS_NAME(result_type->mGenericsTypes[i]));

        (*method)->mResultType.mGenericsTypesOffset[i] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
    }


    if(num_params > 0) {
        (*method)->mParamTypes = CALLOC(1, sizeof(sCLType)*num_params);

        for(i=0; i<num_params; i++) {
            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mClass), CLASS_NAME(class_params[i].mClass));
            (*method)->mParamTypes[i].mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

            (*method)->mParamTypes[i].mGenericsTypesNum = class_params[i].mGenericsTypesNum;

            for(j=0; j<(*method)->mParamTypes[i].mGenericsTypesNum; j++) {
                create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mGenericsTypes[j]), CLASS_NAME(class_params[i].mGenericsTypes[j]));
                (*method)->mParamTypes[i].mGenericsTypesOffset[j] = append_str_to_constant_pool(&klass->mConstPool, real_class_name);
            }
        }
        (*method)->mNumParams = num_params;
    }
    else {
        (*method)->mParamTypes = NULL;
        (*method)->mNumParams = num_params;
    }

    klass->mNumMethods++;

    (*method)->mNumLocals = 0;

    (*method)->mNumBlockType = 0;
    memset(&(*method)->mBlockType, 0, sizeof((*method)->mBlockType));

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    return TRUE;
}

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

void alloc_bytecode_of_method(sCLMethod* method)
{
    sByteCode_init(&method->uCode.mByteCodes);
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

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_from_index(sCLClass* klass, int method_index)
{
    if(method_index < 0 || method_index >= klass->mNumMethods) {
        return NULL;
    }

    return klass->mMethods + method_index;
}

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type)
{
    int i;

    if(start_point < klass->mNumMethods) {
        for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
            if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
                sCLMethod* method;
                
                method = klass->mMethods + i;

                if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) {
                    /// type checking ///
                    if(method->mNumParams == -1) {              // no type checking of method params
                        return method;
                    }
                    else if(num_params == method->mNumParams) {
                        int j, k;

                        for(j=0; j<num_params; j++ ) {
                            sCLNodeType param;

                            memset(&param, 0, sizeof(param));

                            if(!cl_type_to_node_type(&param, &method->mParamTypes[j], type_, klass)) {
                                return NULL;
                            }

                            if(!substition_posibility(&param, &class_params[j])) {
                                return NULL;
                            }
                        }

                        if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                            if(block_num > 0) {
                                sCLNodeType result_block_type;

                                memset(&result_block_type, 0, sizeof(result_block_type));

                                if(!cl_type_to_node_type(&result_block_type, &method->mBlockType.mResultType, type_, klass)) {
                                    return NULL;
                                }

                                if(!substition_posibility(&result_block_type, block_type)) {
                                    return NULL;
                                }
                            }
                            
                            for(k=0; k<block_num_params; k++) {
                                sCLNodeType param;

                                memset(&param, 0, sizeof(param));

                                if(!cl_type_to_node_type(&param, &method->mBlockType.mParamTypes[k], type_ , klass)) {
                                    return NULL;
                                }

                                if(!substition_posibility(&param, &block_param_type[k])) {
                                    return NULL;
                                }
                            }

                            return method;
                        }
                    }
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

    result = get_method_with_type_params(klass, method_name, class_params, num_params, search_for_class_method, type_, klass->mNumMethods-1, block_num, block_num_params, block_param_type, block_type);
    *founded_class = klass;

    if(result == NULL) {
        result = get_method_with_type_params_on_super_classes(klass, method_name, class_params, num_params, founded_class, search_for_class_method, type_, block_num, block_num_params, block_param_type, block_type);
    }

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
        super_class = cl_get_class(real_class_name, FALSE);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == searched_class) {
            return TRUE;                // found
        }
    }

    return FALSE;
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
            result->mClass = cl_get_class(real_class_name, FALSE);
            ASSERT(result->mClass != NULL);
            result->mGenericsTypesNum = method->mParamTypes[param_num].mGenericsTypesNum;

            for(i=0; i<result->mGenericsTypesNum; i++) {
                char* real_class_name = CONS_str(&klass->mConstPool, method->mParamTypes[param_num].mGenericsTypesOffset[i]);
                result->mGenericsTypes[i] = cl_get_class(real_class_name, FALSE);
                ASSERT(result->mGenericsTypes[i] != NULL);
            }
        }
    }
    else {
        result->mClass = NULL;
        result->mGenericsTypesNum = 0;
    }
}

// result: (FALSE) can't solve a generics type (TRUE) success
// if type_ is NULL, don't solve generics type
BOOL get_result_type_of_method(sCLClass* klass, sCLMethod* method, sCLNodeType* result, sCLNodeType* type_)
{
    char* real_class_name;
    int i;
    
    real_class_name = CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset);
    result->mClass = cl_get_class(real_class_name, FALSE);

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
        result->mGenericsTypes[i] = cl_get_class(real_class_name, FALSE);

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
        super_class = cl_get_class(real_class_name, FALSE);

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

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** founded_class)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        sCLMethod* method;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name, FALSE);

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

// return method parametor number
int get_method_num_params(sCLMethod* method)
{
    return method->mNumParams;
}

//////////////////////////////////////////////////
// native method
//////////////////////////////////////////////////
struct sNativeMethodStruct {
    unsigned int mHash;
    fNativeMethod mFun;
};

typedef struct sNativeMethodStruct sNativeMethod;

// manually sort is needed
sNativeMethod gNativeMethods[] = {
    { 814, int_to_s },
    { 854, Array_add },
    { 867, Clover_gc },
    { 911, bool_to_s },
    { 1017, float_to_s },
    { 1068, Array_Array },
    { 1091, String_char },
    { 1103, Array_items },
    { 1107, Clover_exit },
    { 1126, float_floor },
    { 1199, Array_length },
    { 1202, Clover_sleep },
    { 1222, Clover_print },
    { 1308, String_String },
    { 1309, String_append },
    { 1314, Clover_getenv },
    { 1319, String_length },
    { 1410, Clover_compile },
    { 1691, Object_class_name },
    { 1723, Object_show_class },
    { 1886, Clover_output_to_s }, 
    { 1959, Clover_show_classes }
};

static fNativeMethod get_native_method(char* name)
{
    unsigned int hash;
    unsigned int top;
    unsigned int bot;

    hash = get_hash(name);

    top = 0;
    bot = sizeof(gNativeMethods) / sizeof(sNativeMethod);

    while(1) {
        unsigned int mid = (top + bot) / 2;
        
        if(gNativeMethods[mid].mHash == hash) {
            return gNativeMethods[mid].mFun;
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

static void write_char_value_to_buffer(sBuf* buf, char value)
{
    sBuf_append_char(buf, value);
}

static BOOL read_char_from_file(int fd, char* c)
{
    return read_from_file(fd, c, sizeof(char));
}

static void write_int_value_to_buffer(sBuf* buf, int value)
{
    sBuf_append(buf, &value, sizeof(int));
}

static BOOL read_int_from_file(int fd, int* n)
{
    return read_from_file(fd, n, sizeof(int));
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

static void write_params_to_buffer(sBuf* buf, int num_params, sCLType* param_types)
{
    int j;

    write_int_value_to_buffer(buf, num_params);
    for(j=0; j<num_params; j++) {
        write_type_to_buffer(buf, param_types + j);
    }
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

static void write_block_type_to_buffer(sBuf* buf, sCLBlockType* block_type)
{
    write_type_to_buffer(buf, &block_type->mResultType);
    write_int_value_to_buffer(buf, block_type->mNameOffset);
    write_params_to_buffer(buf, block_type->mNumParams, block_type->mParamTypes);
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

static void write_field_to_buffer(sBuf* buf, sCLField* field)
{
    write_int_value_to_buffer(buf, field->mFlags);
    write_int_value_to_buffer(buf, field->mNameOffset);
    //write_mvalue_to_buffer(buf, field->uValue.mStaticField);

    write_int_value_to_buffer(buf, field->mInitializar.mLen);
    if(field->mInitializar.mLen > 0) {
        sBuf_append(buf, field->mInitializar.mCode, sizeof(int)*field->mInitializar.mLen);
    }

    write_int_value_to_buffer(buf, field->mInitializarLVNum);
    write_int_value_to_buffer(buf, field->mInitializarMaxStack);

    write_type_to_buffer(buf, &field->mType);
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

    /// initializar ////
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

static void write_method_to_buffer(sBuf* buf, sCLMethod* method)
{
    write_int_value_to_buffer(buf, method->mFlags);
    write_int_value_to_buffer(buf, method->mNameOffset);

    write_int_value_to_buffer(buf, method->mPathOffset);

    if(method->mFlags & (CL_NATIVE_METHOD|CL_EXTERNAL_METHOD)) {
    }
    else {
        write_int_value_to_buffer(buf, method->uCode.mByteCodes.mLen);
        if(method->uCode.mByteCodes.mLen > 0) {
            sBuf_append(buf, method->uCode.mByteCodes.mCode, sizeof(int)*method->uCode.mByteCodes.mLen);
        }
    }

    write_type_to_buffer(buf, &method->mResultType);
    write_params_to_buffer(buf, method->mNumParams, method->mParamTypes);

    write_int_value_to_buffer(buf, method->mNumLocals);
    write_int_value_to_buffer(buf, method->mMaxStack);

    write_int_value_to_buffer(buf, method->mNumBlockType);
    if(method->mNumBlockType >= 1) {
        write_block_type_to_buffer(buf, &method->mBlockType);
    }
}

static BOOL read_method_from_buffer(sCLClass* klass, sCLMethod* method, int fd)
{
    int n;

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
    else if(method->mFlags & CL_EXTERNAL_METHOD) {
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

    return TRUE;
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

    /// write class params name of generics ///
    write_char_value_to_buffer(buf, klass->mGenericsTypesNum);
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        write_int_value_to_buffer(buf, klass->mGenericsTypesOffset[i]);
    }
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

    return klass;
}

static BOOL check_method_and_field_types_offset(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = klass->mFields + i;
        sCLClass* klass2;
        
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset), TRUE); // auto load

        if(klass2 == NULL) {
            vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));
            return FALSE;
        }
    }

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method = klass->mMethods + i;
        sCLClass* klass2;
        int j;

        /// result type ///
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset), TRUE); // auto load

        if(klass2 == NULL) {
            vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));
            return FALSE;
        }

        /// param types ///
        for(j=0; j<method->mNumParams; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset), TRUE);   // auto load

            if(klass2 == NULL) {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset));
                return FALSE;
            }
        }

        /// block type ///
        if(method->mNumBlockType == 1) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset), TRUE);   // auto load

            if(klass2 == NULL) {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));
                return FALSE;
            }

            for(j=0; j<method->mBlockType.mNumParams; j++) {
                klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset), TRUE);   // auto load

                if(klass2 == NULL) {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

BOOL check_super_class_offsets(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        
        super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]), TRUE);

        if(super_class == NULL) {
            vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));
            return FALSE;
        }
    }

    return TRUE;
}

// (FALSE) --> failed to write (TRUE) --> success
BOOL save_class(sCLClass* klass)
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

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class(char* file_name, BOOL resolve_dependences) 
{
    char* class_name;
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

    class_name = CLASS_NAME(klass);

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(class_name, klass);

    /// if there is a class which is entried to class table already, return the class
    klass2 = cl_get_class_with_namespace(NAMESPACE_NAME(klass), CLASS_NAME(klass), FALSE);
    
    if(klass2) {
        free_class(klass);
        return klass2;
    }

    //// add class to class table ///
    add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    if(!check_super_class_offsets(klass)){
        vm_error("can't load class %s because of the super classes\n", REAL_CLASS_NAME(klass));
        remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
        free_class(klass);
        return NULL;
    }

    if(!resolve_dependences) {
        if(!check_method_and_field_types_offset(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }
    }

    /// call class field initializar ///
    if(!run_class_fields_initializar(klass)) {
        return NULL;
    }

    return klass;
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
                    if(!save_class(klass)) {
                        cl_print("failed to write this class(%s)\n", REAL_CLASS_NAME(klass));
                    }
                }
                klass = next_klass;
            }
        }
    }
}

// result : (TRUE) found (FALSE) not found
// set class file path on class_file arguments
static BOOL search_for_class_file_from_class_name(char* class_file, unsigned int class_file_size, char* class_name)
{
    sCLClass* clover;
    MVALUE mvalue;
    int i;

    /// default search path ///
    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        if(i == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clo", DATAROOTDIR, class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clo", DATAROOTDIR, class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }

    /// user search path ///
    clover = cl_get_class("Clover", FALSE);

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
                    snprintf(class_file, class_file_size, "%s/%s.clo", class_file_path, class_name);
                }
                else {
                    snprintf(class_file, class_file_size, "%s/%s#%d.clo", class_file_path, class_name, k);
                }

                if(access(class_file, F_OK) == 0) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_from_classpath(char* class_name, BOOL resolve_dependences)
{
    char class_file[PATH_MAX];

    if(!search_for_class_file_from_class_name(class_file, PATH_MAX, class_name)) {
        return NULL;
    }

    return load_class(class_file, resolve_dependences);
}

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
        sCLClass* super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]), FALSE);
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
        else if(klass->mMethods[i].mFlags & CL_EXTERNAL_METHOD) {
            cl_print("external method\n");
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
    node_type.mClass = cl_get_class(real_class_name, FALSE);

    node_type.mGenericsTypesNum = type->mGenericsTypesNum;

    for(i=0; i<type->mGenericsTypesNum; i++) {
        real_class_name = CONS_str(&klass->mConstPool, type->mGenericsTypesOffset[i]);
        node_type.mGenericsTypes[i] = cl_get_class(real_class_name, FALSE);
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

//////////////////////////////////////////////////
// accessor function
//////////////////////////////////////////////////
// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass)
{
    if(klass->mNumSuperClasses > 0) {
        char* real_class_name;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[klass->mNumSuperClasses-1]);
        return cl_get_class(real_class_name, FALSE);
    }
    else {
        return NULL;
    }
}

// result: (FALSE) fail (TRUE) success
// result of parametors is setted on the result
// if type_ is NULL, it is not solved with generics type.
BOOL cl_type_to_node_type(sCLNodeType* result, sCLType* cl_type, sCLNodeType* type_, sCLClass* klass)
{
    char* real_class_name;
    int i;
    
    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);
    result->mClass = cl_get_class(real_class_name, FALSE);

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
        result->mGenericsTypes[i] = cl_get_class(real_class_name, FALSE);

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

void increase_class_version(sCLClass* klass)
{
    char version;

    version = CLASS_VERSION(klass);
    version++;
    klass->mFlags = (klass->mFlags & ~CLASS_FLAGS_VERSION) | version;
}

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
sCLNodeType gIntType;      // foudamental classes
sCLNodeType gFloatType;
sCLNodeType gVoidType;
sCLNodeType gObjectType;
sCLNodeType gStringType;
sCLNodeType gArrayType;
sCLNodeType gHashType;
sCLNodeType gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];
sCLNodeType gNullType;
sCLNodeType gBoolType;
sCLNodeType gBlockType;

static void create_anonymous_classes()
{
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        char class_name[CL_CLASS_NAME_MAX];
        snprintf(class_name, CL_CLASS_NAME_MAX, "anonymous%d", i);

        gAnonymousType[i].mClass = alloc_class("", class_name, FALSE, FALSE, NULL, 0);
    }
}

static void create_null_class()
{
    char class_name[CL_CLASS_NAME_MAX];
    snprintf(class_name, CL_CLASS_NAME_MAX, "null");

    gNullType.mClass = alloc_class("", class_name, FALSE, FALSE, NULL, 0);
}

BOOL class_init(BOOL load_foundamental_class)
{
    create_null_class();
    create_anonymous_classes();

    if(load_foundamental_class) {
        sCLClass* block;
        sCLClass* system;
        sCLClass* clover;

        gVoidType.mClass = load_class_from_classpath("void", TRUE);
        gIntType.mClass = load_class_from_classpath("int", TRUE);
        gFloatType.mClass = load_class_from_classpath("float", TRUE);
        gBoolType.mClass = load_class_from_classpath("bool", TRUE);

        gObjectType.mClass = load_class_from_classpath("Object", TRUE);
        gStringType.mClass = load_class_from_classpath("String", TRUE);
        gBlockType.mClass = load_class_from_classpath("Block", TRUE);
        gArrayType.mClass = load_class_from_classpath("Array", TRUE);
        gHashType.mClass = load_class_from_classpath("Hash", TRUE);

        block = load_class_from_classpath("Block", TRUE);
        system = load_class_from_classpath("System", TRUE);

        clover = load_class_from_classpath("Clover", TRUE);

        if(gVoidType.mClass == NULL || gIntType.mClass == NULL || gFloatType.mClass == NULL || gBoolType.mClass == NULL || gObjectType.mClass == NULL || gStringType.mClass == NULL || gBlockType.mClass == NULL || gArrayType.mClass == NULL || gHashType.mClass == NULL || block == NULL || system == NULL || clover == NULL) 
        {
            fprintf(stderr, "can't load fundamental classes\n");
            return FALSE;
        }
    }

    return TRUE;
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
