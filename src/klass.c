#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

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

    return NULL;
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

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    return cl_get_class(real_class_name);
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

// result must be not NULL; this is for compiler.c
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL open_, char* generics_types[CL_CLASS_TYPE_VARIABLE_MAX], int generics_types_num)
{
    sCLClass* klass;
    int i;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

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

    klass->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, class_name);  // class name

    klass->mNameSpaceOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, namespace);   // namespace 

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    klass->mRealClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, real_class_name);  // real class name

    add_class_to_class_table(namespace, class_name, klass);

    klass->mNumVMethodMap = 0;   // paranoia
    klass->mSizeVMethodMap = 3;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVMethodMap);

    klass->mGenericsTypesNum = generics_types_num;

    for(i=0; i<generics_types_num; i++) {
        klass->mGenericsTypesOffset[i] = klass->mConstPool.mLen;
        sConst_append_str(&klass->mConstPool, generics_types[i]);
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

            if(!(method->mFlags & CL_NATIVE_METHOD) && method->uCode.mByteCodes.mCode != NULL) {
                sByteCode_free(&method->uCode.mByteCodes);
            }
        }
        FREE(klass->mMethods);
    }

    if(klass->mFields) {
        FREE(klass->mFields);
    }

    if(klass->mVirtualMethodMap) {
        FREE(klass->mVirtualMethodMap);
    }

    FREE(klass);
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
        memcpy(klass->mSuperClassesOffset, super_klass->mSuperClassesOffset, sizeof(unsigned int)*klass->mNumSuperClasses);
    }

    klass->mSuperClassesOffset[klass->mNumSuperClasses] = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, REAL_CLASS_NAME(super_klass));
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

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, unsigned int num_params, BOOL constructor)
{
    int i, j;
    sCLMethod* method;
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

    method = klass->mMethods + klass->mNumMethods;
    method->mFlags = (static_ ? CL_CLASS_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0) | (constructor ? CL_CONSTRUCTOR:0);

    method->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);

    path_max = CL_METHOD_NAME_MAX + CL_CLASS_NAME_MAX + 2;
    buf = MALLOC(sizeof(char)*path_max);;
    snprintf(buf, path_max, "%s.%s", CLASS_NAME(klass), name);

    method->mPathOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, buf);

    FREE(buf);

    method->mResultType.mClassNameOffset = klass->mConstPool.mLen;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mClass), CLASS_NAME(result_type->mClass));

    sConst_append_str(&klass->mConstPool, real_class_name);

    method->mResultType.mGenericsTypesNum = result_type->mGenericsTypesNum;
    for(i=0; i<method->mResultType.mGenericsTypesNum; i++) {
        method->mResultType.mGenericsTypesOffset[i] = klass->mConstPool.mLen;

        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type->mGenericsTypes[i]), CLASS_NAME(result_type->mGenericsTypes[i]));

        sConst_append_str(&klass->mConstPool, real_class_name);
    }

    method->mParamTypes = CALLOC(1, sizeof(sCLType)*num_params);

    for(i=0; i<num_params; i++) {
        method->mParamTypes[i].mClassNameOffset = klass->mConstPool.mLen;

        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mClass), CLASS_NAME(class_params[i].mClass));
        sConst_append_str(&klass->mConstPool, real_class_name);

        method->mParamTypes[i].mGenericsTypesNum = class_params[i].mGenericsTypesNum;

        for(j=0; j<method->mParamTypes[i].mGenericsTypesNum; j++) {
            method->mParamTypes[i].mGenericsTypesOffset[j] = klass->mConstPool.mLen;

            create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i].mGenericsTypes[j]), CLASS_NAME(class_params[i].mGenericsTypes[j]));
            sConst_append_str(&klass->mConstPool, real_class_name);
        }
    }
    method->mNumParams = num_params;

    klass->mNumMethods++;

    method->mNumLocals = 0;

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    return TRUE;
}

void alloc_bytecode_of_method(sCLMethod* method)
{
    sByteCode_init(&method->uCode.mByteCodes);
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

    field->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);    // field name

    field->mType.mClassNameOffset = klass->mConstPool.mLen;
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_->mClass), CLASS_NAME(type_->mClass));
    sConst_append_str(&klass->mConstPool, real_class_name);

    field->mType.mGenericsTypesNum = type_->mGenericsTypesNum;

    for(i=0; i<field->mType.mGenericsTypesNum; i++) {
        field->mType.mGenericsTypesOffset[i] = klass->mConstPool.mLen;
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_->mGenericsTypes[i]), CLASS_NAME(type_->mGenericsTypes[i]));
        sConst_append_str(&klass->mConstPool, real_class_name);
    }

    klass->mNumFields++;
    
    return TRUE;
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
    { 1081, Clover_load },
    { 1103, Array_items },
    { 1126, float_floor },
    { 1199, Array_length },
    { 1222, Clover_print },
    { 1308, String_String },
    { 1309, String_append },
    { 1319, String_length },
    { 1410, Clover_compile },
    { 1691, Object_class_name },
    { 1723, Object_show_class },
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
// result: (null) --> file not found (char* pointer) --> success
ALLOC char* load_file(char* file_name)
{
    int f;
    sBuf buf;
    
    f = open(file_name, O_RDONLY);

    if(f < 0) {
        return NULL;
    }

    sBuf_init(&buf);

    while(1) {
        char buf2[BUFSIZ];
        int size;
        
        size = read(f, buf2, BUFSIZ);

        if(size < 0) {
            FREE(buf.mBuf);
            return NULL;
        }
        if(size == 0) {
            break;
        }

        sBuf_append(&buf, buf2, size);
    }

    return (char*)ALLOC buf.mBuf;
}

static void write_class_to_buffer(sCLClass* klass, sBuf* buf)
{
    int i, j, k;

    unsigned int flags;

    flags = klass->mFlags;
    sBuf_append(buf, &flags, sizeof(unsigned int));

    /// save constant pool ///
    sBuf_append(buf, &klass->mConstPool.mLen, sizeof(unsigned int));
    sBuf_append(buf, klass->mConstPool.mConst, klass->mConstPool.mLen);

    /// save class name offset
    sBuf_append(buf, &klass->mNameSpaceOffset, sizeof(unsigned char));
    sBuf_append(buf, &klass->mClassNameOffset, sizeof(unsigned char));
    sBuf_append(buf, &klass->mRealClassNameOffset, sizeof(unsigned char));

    /// save fields
    sBuf_append(buf, &klass->mNumFields, sizeof(unsigned char));
    for(i=0; i<klass->mNumFields; i++) {
        sBuf_append(buf, &klass->mFields[i].mFlags, sizeof(unsigned int));
        sBuf_append(buf, &klass->mFields[i].mNameOffset, sizeof(unsigned int));
        sBuf_append(buf, &klass->mFields[i].uValue.mStaticField, sizeof(MVALUE));

        sBuf_append(buf, &klass->mFields[i].mType.mClassNameOffset, sizeof(unsigned int));

        sBuf_append(buf, &klass->mFields[i].mType.mGenericsTypesNum, sizeof(unsigned char));

        for(j=0; j<klass->mFields[i].mType.mGenericsTypesNum; j++) {
            sBuf_append(buf, &klass->mFields[i].mType.mGenericsTypesOffset[j], sizeof(unsigned int));
        }
    }

    /// save methods
    sBuf_append(buf, &klass->mNumMethods, sizeof(unsigned char));
    for(i=0; i<klass->mNumMethods; i++) {
        sBuf_append(buf, &klass->mMethods[i].mFlags, sizeof(unsigned int));
        sBuf_append(buf, &klass->mMethods[i].mNameOffset, sizeof(unsigned int));

        sBuf_append(buf, &klass->mMethods[i].mPathOffset, sizeof(unsigned int));

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
        }
        else {
            sBuf_append(buf, &klass->mMethods[i].uCode.mByteCodes.mLen, sizeof(unsigned int));
            if(klass->mMethods[i].uCode.mByteCodes.mLen > 0) {
                sBuf_append(buf, klass->mMethods[i].uCode.mByteCodes.mCode, klass->mMethods[i].uCode.mByteCodes.mLen);
            }
        }

        sBuf_append(buf, &klass->mMethods[i].mResultType.mClassNameOffset, sizeof(unsigned int));
        sBuf_append(buf, &klass->mMethods[i].mResultType.mGenericsTypesNum, sizeof(unsigned char));
        
        for(j=0; j<klass->mMethods[i].mResultType.mGenericsTypesNum; j++) {
            sBuf_append(buf, &klass->mMethods[i].mResultType.mGenericsTypesOffset, sizeof(unsigned int));
        }

        sBuf_append(buf, &klass->mMethods[i].mNumParams, sizeof(unsigned int));

        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            sBuf_append(buf, &klass->mMethods[i].mParamTypes[j].mClassNameOffset, sizeof(unsigned int));

            sBuf_append(buf, &klass->mMethods[i].mParamTypes[j].mGenericsTypesNum, sizeof(unsigned char));

            for(k=0; k<klass->mMethods[i].mParamTypes[j].mGenericsTypesNum; k++) {
                sBuf_append(buf, &klass->mMethods[i].mParamTypes[j].mGenericsTypesOffset, sizeof(unsigned int));
            }
        }

        sBuf_append(buf, &klass->mMethods[i].mNumLocals, sizeof(unsigned int));
        sBuf_append(buf, &klass->mMethods[i].mMaxStack, sizeof(unsigned int));
    }

    /// write super classes ///
    sBuf_append(buf, &klass->mNumSuperClasses, sizeof(unsigned char));

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sBuf_append(buf, &klass->mSuperClassesOffset[i], sizeof(unsigned int));
    }

    sBuf_append(buf, &klass->mGenericsTypesNum, sizeof(unsigned char));

    for(i=0; i<klass->mGenericsTypesNum; i++) {
        sBuf_append(buf, &klass->mGenericsTypesOffset[i], sizeof(unsigned int));
    }
}

static sCLClass* read_class_from_buffer(char** p)
{
    int i, j, k;
    int const_pool_len;
    sCLClass* klass;

    klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mFlags = *(unsigned int*)(*p);
    (*p) += sizeof(unsigned int);

    /// load constant pool ///
    const_pool_len = *(unsigned int*)(*p);
    (*p) += sizeof(unsigned int);

    sConst_append(&klass->mConstPool, *p, const_pool_len);
    (*p) += const_pool_len;

    /// load namespace offset ///
    klass->mNameSpaceOffset = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    /// load class name offset ///
    klass->mClassNameOffset = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    /// load real class name offset //
    klass->mRealClassNameOffset = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    /// load fields ///
    klass->mSizeFields = klass->mNumFields = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mFlags = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);
        klass->mFields[i].mNameOffset = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);
        klass->mFields[i].uValue.mStaticField = *(MVALUE*)(*p);
        (*p) += sizeof(MVALUE);
        klass->mFields[i].mType.mClassNameOffset = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);

        klass->mFields[i].mType.mGenericsTypesNum = *(unsigned char*)(*p);
        (*p) += sizeof(unsigned char);

        for(j=0; j<klass->mFields[i].mType.mGenericsTypesNum; j++) {
            klass->mFields[i].mType.mGenericsTypesOffset[j] = *(unsigned int*)(*p);
            (*p) += sizeof(unsigned int);
        }
    }

    /// load methods ///
    klass->mSizeMethods = klass->mNumMethods = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        int param_num;
        int local_num;
        int max_stack;

        klass->mMethods[i].mFlags = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);

        klass->mMethods[i].mNameOffset = *(unsigned int*)(*p);
        (*p) += sizeof(int);

        klass->mMethods[i].mPathOffset = *(unsigned int*)(*p);
        (*p) += sizeof(int);

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
            char* method_path;

            method_path = CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset);

            klass->mMethods[i].uCode.mNativeMethod = get_native_method(method_path);
            if(klass->mMethods[i].uCode.mNativeMethod == NULL) {
                fprintf(stderr, "native method(%s) is not found\n", method_path);
            }
        }
        else {
            int len_bytecodes;

            alloc_bytecode_of_method(&klass->mMethods[i]);

            len_bytecodes = *(unsigned int*)(*p);
            (*p) += sizeof(unsigned int);

//printf("%s.%s code size %d\n", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, klass->mMethods + i),len_bytecodes);

            if(len_bytecodes > 0) {
                sByteCode_append(&klass->mMethods[i].uCode.mByteCodes, *p, len_bytecodes);
                (*p) += len_bytecodes;
            }
        }

        klass->mMethods[i].mResultType.mClassNameOffset = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);

        klass->mMethods[i].mResultType.mGenericsTypesNum = *(unsigned char*)(*p);
        (*p) += sizeof(unsigned char);

        for(j=0; j<klass->mMethods[i].mResultType.mGenericsTypesNum; j++) {
            klass->mMethods[i].mResultType.mGenericsTypesOffset[j] = *(unsigned int*)(*p);
            (*p) += sizeof(unsigned int);
        }

        param_num = *(unsigned int*)(*p);
        klass->mMethods[i].mNumParams = param_num;
        (*p) += sizeof(unsigned int);

        if(param_num == 0) 
            klass->mMethods[i].mParamTypes = NULL;
        else {
            klass->mMethods[i].mParamTypes = CALLOC(1, sizeof(sCLType)*param_num);

            for(j=0; j<param_num; j++) {
                klass->mMethods[i].mParamTypes[j].mClassNameOffset = *(unsigned int*)(*p);
                (*p) += sizeof(unsigned int);

                klass->mMethods[i].mParamTypes[j].mGenericsTypesNum = *(unsigned char*)(*p);
                (*p) += sizeof(unsigned char);

                for(k=0; k<klass->mMethods[i].mParamTypes[j].mGenericsTypesNum; k++) {
                    klass->mMethods[i].mParamTypes[j].mGenericsTypesOffset[k] = *(unsigned int*)(*p);
                    (*p) += sizeof(unsigned int);
                }
            }
        }

        local_num = *(unsigned int*)(*p);
        klass->mMethods[i].mNumLocals = local_num;
        (*p) += sizeof(unsigned int);

        max_stack = *(unsigned int*)(*p);
        klass->mMethods[i].mMaxStack = max_stack;
        (*p) += sizeof(unsigned int);
    }

    /// load super classes ///
    klass->mNumSuperClasses = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        klass->mSuperClassesOffset[i] = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);
    }

    klass->mGenericsTypesNum = *(unsigned char*)(*p);
    (*p) += sizeof(unsigned char);

    for(i=0; i<klass->mGenericsTypesNum; i++) {
        klass->mGenericsTypesOffset[i] = *(unsigned int*)(*p);
        (*p) += sizeof(unsigned int);
    }

    return klass;
}

BOOL check_super_class_offsets(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        
        super_class = cl_get_class(CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]));

        if(super_class == NULL) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL check_method_and_field_types_offset(sCLClass* klass)
{
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
        snprintf(file_name, PATH_MAX, "%s.clc", REAL_CLASS_NAME(klass));
    }
    else {
        snprintf(file_name, PATH_MAX, "%s#%d.clc", REAL_CLASS_NAME(klass), CLASS_VERSION(klass));
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
static sCLClass* load_class(char* file_name) 
{
    int i;
    char* file_data;
    char* p;
    char* class_name;
    sCLClass* klass;
    
    file_data = ALLOC load_file(file_name);
    p = file_data;

    if(p == NULL) {
        return NULL;
    }

    /// check magic number. Is this Clover object file? ///
    if(*p++ != 12) { FREE(file_data); return NULL; }
    if(*p++ != 17) { FREE(file_data); return NULL; }
    if(*p++ != 33) { FREE(file_data); return NULL; }
    if(*p++ != 79) { FREE(file_data); return NULL; }
    if(*p++ != 'C') { FREE(file_data); return NULL; }
    if(*p++ != 'L') { FREE(file_data); return NULL; }
    if(*p++ != 'O') { FREE(file_data); return NULL; }
    if(*p++ != 'V') { FREE(file_data); return NULL; }
    if(*p++ != 'E') { FREE(file_data); return NULL; }
    if(*p++ != 'R') { FREE(file_data); return NULL; }

    klass = read_class_from_buffer(&p);

    class_name = CLASS_NAME(klass);

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(class_name, klass);

    if(check_super_class_offsets(klass)){
        add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

//printf("loaded class %s version %d...\n", REAL_CLASS_NAME(klass), CLASS_VERSION(klass));
    }
    else {
        free_class(klass);

printf("can't load class %s because of the super classes\n", REAL_CLASS_NAME(klass));
    }

    FREE(file_data);

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
                        fprintf(stderr, "failed to write this class(%s)\n", REAL_CLASS_NAME(klass));
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
    int i;
    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        if(i == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clc", DATAROOTDIR, class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clc", DATAROOTDIR, class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }
    
    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        if(i == 1) {
            snprintf(class_file, class_file_size, "./%s.clc", class_name);
        }
        else {
            snprintf(class_file, class_file_size, "./%s#%d.clc", class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_from_classpath(char* class_name)
{
    char class_file[PATH_MAX];

    if(!search_for_class_file_from_class_name(class_file, PATH_MAX, class_name)) {
        return NULL;
    }

    return load_class(class_file);
}

void show_class(sCLClass* klass)
{
    char* p;
    int i;
    
    printf("-+- %s -+-\n", REAL_CLASS_NAME(klass));

/*
    /// show constant pool ///
    printf("constant len %d\n", klass->mConstPool.mLen);
    show_constants(&klass->mConstPool);

    p = klass->mConstPool.mConst;

    while(p - klass->mConstPool.mConst < klass->mConstPool.mLen) {
        unsigned char type = *p;
        p++;

        switch(type) {
            case CONSTANT_STRING: {
                unsigned int len;
                char* str;

                len = *(unsigned int*)p;
                p+=sizeof(int);
                str = p;
                p+=len;
                printf("len (%d) str (%s)\n", len, str);
                }
                break;

            case CONSTANT_INT: {
                unsigned int num = *(unsigned int*)p;
                p+=sizeof(int);

                printf("value (%d)\n", num);
                }
                break;
        }
    }
*/

    printf("ClassNameOffset %d (%s)\n", klass->mClassNameOffset, CONS_str(klass->mConstPool, klass->mClassNameOffset));
    printf("NameSpaceOffset %d (%s)\n", klass->mNameSpaceOffset, CONS_str(klass->mConstPool, klass->mNameSpaceOffset));
    printf("RealClassNameOffset %d (%s)\n", klass->mRealClassNameOffset, CONS_str(klass->mConstPool, klass->mRealClassNameOffset));

    printf("num fields %d\n", klass->mNumFields);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class = cl_get_class(CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]));
        ASSERT(super_class);  // checked on load time
        printf("SuperClass[%d] %s\n", i, REAL_CLASS_NAME(super_class));
    }

    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mFlags & CL_STATIC_FIELD) {
            printf("field number %d --> %s static field %d\n", i, FIELD_NAME(klass, i), klass->mFields[i].uValue.mStaticField.mIntValue);
        }
        else {
            printf("field number %d --> %s\n", i, FIELD_NAME(klass, i));
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        int j;

        printf("--- method number %d ---\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mNameOffset));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset));
        printf("result type %s\n", CONS_str(klass->mConstPool, klass->mMethods[i].mResultType.mClassNameOffset));
        printf("num params %d\n", klass->mMethods[i].mNumParams);
        printf("num locals %d\n", klass->mMethods[i].mNumLocals);
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            printf("%d. %s\n", j, CONS_str(klass->mConstPool, klass->mMethods[i].mParamTypes[j].mClassNameOffset));
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

void show_node_type(sCLNodeType* type)
{
    int i;

    if(type->mGenericsTypesNum == 0) {
        printf("%s", REAL_CLASS_NAME(type->mClass));
    }
    else {
        printf("%s<", REAL_CLASS_NAME(type->mClass));
        for(i=0; i<type->mGenericsTypesNum; i++) {
            printf("%s", REAL_CLASS_NAME(type->mGenericsTypes[i]));
            if(i != type->mGenericsTypesNum-1) { printf(","); }
        }
        printf(">");
    }
}

void show_type(sCLClass* klass, sCLType* type)
{
    sCLNodeType node_type;
    char* real_class_name;
    int i;

    real_class_name = CONS_str(klass->mConstPool, type->mClassNameOffset);
    node_type.mClass = cl_get_class(real_class_name);

    node_type.mGenericsTypesNum = type->mGenericsTypesNum;

    for(i=0; i<type->mGenericsTypesNum; i++) {
        real_class_name = CONS_str(klass->mConstPool, type->mGenericsTypesOffset[i]);
        node_type.mGenericsTypes[i] = cl_get_class(real_class_name);
    }

    show_node_type(&node_type);
}

void show_method(sCLClass* klass, sCLMethod* method)
{
    int i;

    /// result ///
    show_type(klass, &method->mResultType);
    printf(" ");
    
    /// name ///
    printf("%s(", METHOD_NAME2(klass, method));

    for(i=0; i<method->mNumParams; i++) {
        show_type(klass, &method->mParamTypes[i]);

        if(i != method->mNumParams-1) printf(",");
    }

    printf(")\n");
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

// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass)
{
    if(klass->mNumSuperClasses > 0) {
        char* real_class_name;
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[klass->mNumSuperClasses-1]);
        return cl_get_class(real_class_name);
    }
    else {
        return NULL;
    }
}

static int get_sum_of_fields_on_super_clasess(sCLClass* klass)
{
    int sum = 0;
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        sum += super_class->mNumFields;
    }

    return sum;
}

// return field number
int get_field_num_including_super_classes(sCLClass* klass)
{
    return get_sum_of_fields_on_super_clasess(klass) + klass->mNumFields;
}

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0) {
            return klass->mFields + i;
        }
    }

    return NULL;
}

// result: (NULL) --> not found (non NULL) --> field
// also return the class in which is found the the field 
sCLField* get_field_including_super_classes(sCLClass* klass, char* field_name, sCLClass** founded_class)
{
    sCLField* field;
    int i;

    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        field = get_field(super_class, field_name);

        if(field) { 
            *founded_class = super_class; 
            return field; 
        }
    }

    field = get_field(klass, field_name);

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
int get_field_index(sCLClass* klass, char* field_name)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> field index
int get_field_index_including_super_classes(sCLClass* klass, char* field_name)
{
    int i;
    int field_index;
    
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        int field_index;
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        field_index = get_field_index(super_class, field_name);

        if(field_index >= 0) {
            return get_sum_of_fields_on_super_clasess(super_class) + field_index;
        }
    }

    field_index = get_field_index(klass, field_name);

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

        result->mClass = cl_get_class(CONS_str(klass->mConstPool, field->mType.mClassNameOffset));

        if(type_) {
            if(!solve_generics_types(result->mClass, type_, &result->mClass)) {
                memset(result, 0, sizeof(sCLNodeType));
                return;
            }
        }

        result->mGenericsTypesNum = field->mType.mGenericsTypesNum;

        for(i=0; i<field->mType.mGenericsTypesNum; i++) {
            result->mGenericsTypes[i] = cl_get_class(CONS_str(klass->mConstPool, field->mType.mGenericsTypesOffset[i]));

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
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point)
{
    int i;
    for(i=start_point; i>=0; i--) {           // search for method in reverse because we want to get last defined method
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method;
            
            method = klass->mMethods + i;

            if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) {
                /// type checking ///
                if(num_params == method->mNumParams) {
                    int j;
                    for(j=0; j<num_params; j++ ) {
                        sCLNodeType class_params2;
                        char* real_class_name;
                        int k;

                        memset(&class_params2, 0, sizeof(class_params2));

                        real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[j].mClassNameOffset);
                        class_params2.mClass = cl_get_class(real_class_name);

                        if(type_) {
                            if(!solve_generics_types(class_params2.mClass, type_, &class_params2.mClass)) {
                                return NULL;
                            }
                        }

                        ASSERT(class_params2.mClass != NULL);

                        class_params2.mGenericsTypesNum = method->mParamTypes[j].mGenericsTypesNum;

                        for(k=0; k<class_params2.mGenericsTypesNum; k++) {
                            char* real_class_name;

                            real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[j].mGenericsTypesOffset[k]);
                            class_params2.mGenericsTypes[k] = cl_get_class(real_class_name);

                            if(type_) {
                                if(!solve_generics_types(class_params2.mGenericsTypes[k], type_, &class_params2.mGenericsTypes[k])) {
                                    return NULL;
                                }
                            }

                            ASSERT(class_params2.mGenericsTypes[k] != NULL);
                        }

                        if(!substition_posibility(&class_params2, &class_params[j])) {
                            break;
                        }
                    }

                    if(j == num_params) {
                        return method;
                    }
                }
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
// if type_ is NULL, don't solve generics type
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_)
{
    sCLMethod* result;

    result = get_method_with_type_params(klass, method_name, class_params, num_params, search_for_class_method, type_, klass->mNumMethods-1);
    *founded_class = klass;

    if(result == NULL) {
        result = get_method_with_type_params_on_super_classes(klass, method_name, class_params, num_params, founded_class, search_for_class_method, type_);
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
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

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

// result should be found
void get_param_type_of_method(sCLClass* klass, sCLMethod* method, int param_num, sCLNodeType* result)
{
    if(klass != NULL && method != NULL) {
        if(param_num >= 0 && param_num < method->mNumParams && method->mParamTypes != NULL) {
            char* real_class_name;
            int i;
            
            real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[param_num].mClassNameOffset);
            result->mClass = cl_get_class(real_class_name);
            ASSERT(result->mClass != NULL);
            result->mGenericsTypesNum = method->mParamTypes[param_num].mGenericsTypesNum;

            for(i=0; i<result->mGenericsTypesNum; i++) {
                char* real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[param_num].mGenericsTypesOffset[i]);
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

// result: (FALSE) can't solve a generics type (TRUE) success
// if type_ is NULL, don't solve generics type
BOOL get_result_type_of_method(sCLClass* klass, sCLMethod* method, sCLNodeType* result, sCLNodeType* type_)
{
    char* real_class_name;
    int i;
    
    real_class_name = CONS_str(klass->mConstPool, method->mResultType.mClassNameOffset);
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

        real_class_name = CONS_str(klass->mConstPool, method->mResultType.mGenericsTypesOffset[i]);
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

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        sCLMethod* method;
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        method = get_method_with_type_params(super_class, method_name, class_params, num_params, search_for_class_method, type_, super_class->mNumMethods-1);

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
        
        real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
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

// return method parametor number
int get_method_num_params(sCLMethod* method)
{
    return method->mNumParams;
}

void increase_class_version(sCLClass* klass)
{
    unsigned char version;

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

void class_init(BOOL load_foundamental_class)
{
    create_null_class();
    create_anonymous_classes();

    if(load_foundamental_class) {
        load_class_from_classpath("Clover");

        gVoidType.mClass = load_class_from_classpath("void");
        gIntType.mClass = load_class_from_classpath("int");
        gFloatType.mClass = load_class_from_classpath("float");
        gBoolType.mClass = load_class_from_classpath("bool");

        gObjectType.mClass = load_class_from_classpath("Object");
        gStringType.mClass = load_class_from_classpath("String");
        gArrayType.mClass = load_class_from_classpath("Array");
        gHashType.mClass = load_class_from_classpath("Hash");
    }
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
