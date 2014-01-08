#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

//////////////////////////////////////////////////
// get class
//////////////////////////////////////////////////

uint get_hash(char* name)
{
    uint hash = 0;
    char* p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

sCLClass* gClassHashList[CLASS_HASH_SIZE];

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name)
{
    const int hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    sCLClass* klass = gClassHashList[hash];

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

static void create_real_class_name(char* result, int result_size, char* namespace, char* class_name)
{
    xstrncpy(result, namespace, result_size);
    xstrncat(result, "$", result_size);
    xstrncat(result, class_name, result_size);
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    sCLClass* result = cl_get_class(real_class_name);
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
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    /// added this to class table ///
    const int hash = get_hash(real_class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;
}

static void remove_class_from_class_table(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    /// added this to class table ///
    const int hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    sCLClass* klass = gClassHashList[hash];
    sCLClass* previous_klass = NULL;
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

// result must be not NULL; this is for compiler.c
sCLClass* alloc_class(char* namespace, char* class_name)
{
    sCLClass* klass = CALLOC(1, sizeof(sCLClass));

    /// immediate class is special ///
    if(strcmp(class_name, "void") == 0 || strcmp(class_name, "int") == 0 || strcmp(class_name, "float") == 0 || strcmp(class_name, "String") == 0) {
        klass->mFlags |= CLASS_FLAGS_IMMEDIATE_VALUE_CLASS;
    }

    sConst_init(&klass->mConstPool);

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

    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    klass->mRealClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, real_class_name);  // real class name

    add_class_to_class_table(namespace, class_name, klass);

    klass->mNumVMethodMap = 0;   // paranoia
    klass->mSizeVMethodMap = 3;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVMethodMap);

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

            if(!(method->mFlags & CL_NATIVE_METHOD) && method->mByteCodes.mCode != NULL) {
                sByteCode_free(&method->mByteCodes);
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
        memcpy(klass->mSuperClassesOffset, super_klass->mSuperClassesOffset, sizeof(uint)*klass->mNumSuperClasses);
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
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, BOOL virtual_, BOOL override_, char* name, sCLClass* result_type, sCLClass* class_params[], uint num_params, BOOL constructor)
{
    if(klass->mNumMethods >= CL_METHODS_MAX) {
        return FALSE;
    }

    if(klass->mNumMethods >= klass->mSizeMethods) {
        const int new_size = klass->mSizeMethods * 2;
        klass->mMethods = REALLOC(klass->mMethods, sizeof(sCLMethod)*new_size);
        memset(klass->mMethods + klass->mSizeMethods, 0, sizeof(sCLMethod)*(new_size-klass->mSizeMethods));
        klass->mSizeMethods = new_size;
    }

    sCLMethod* method = klass->mMethods + klass->mNumMethods;
    method->mFlags = (static_ ? CL_STATIC_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0) | (constructor ? CL_CONSTRUCTOR:0) | (virtual_ || override_ ? CL_VIRTUAL_METHOD:0);

    method->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);

    const int path_max = CL_METHOD_NAME_MAX + CL_CLASS_NAME_MAX + 2;
    char buf[path_max];
    snprintf(buf, path_max, "%s.%s", CLASS_NAME(klass), name);

    method->mPathOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, buf);

    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(result_type), CLASS_NAME(result_type));

    method->mResultType = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, real_class_name);

    method->mParamTypes = CALLOC(1, sizeof(uint)*num_params);

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    int i;
    for(i=0; i<num_params; i++) {
        method->mParamTypes[i] = klass->mConstPool.mLen;

        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(class_params[i]), CLASS_NAME(class_params[i]));
        sConst_append_str(&klass->mConstPool, real_class_name);
    }
    method->mNumParams = num_params;

    klass->mNumMethods++;

    method->mNumLocals = 0;

    return TRUE;
}

void alloc_bytecode(sCLMethod* method)
{
    sByteCode_init(&method->mByteCodes);
}

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, char* name, sCLClass* type_)
{
    if(klass->mNumFields >= CL_FIELDS_MAX) {
        return FALSE;
    }
    if(klass->mNumFields >= klass->mSizeFields) {
        const int new_size = klass->mSizeFields * 2;
        klass->mFields = REALLOC(klass->mFields, sizeof(sCLField)*new_size);
        memset(klass->mFields + klass->mSizeFields, 0, sizeof(sCLField)*(new_size-klass->mSizeFields));
        klass->mSizeFields = new_size;
    }

    sCLField* field = klass->mFields + klass->mNumFields;

    field->mFlags = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

    field->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);    // field name

    field->mClassNameOffset = klass->mConstPool.mLen;

    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(type_), CLASS_NAME(type_));

    sConst_append_str(&klass->mConstPool, real_class_name);

    klass->mNumFields++;
    
    return TRUE;
}

//////////////////////////////////////////////////
// native method
//////////////////////////////////////////////////
static BOOL Clover_load(MVALUE* stack, MVALUE* stack_ptr)
{
    MVALUE* value = stack_ptr-1;

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    char* str = MALLOC(size);
    wcstombs(str, CLASTART(value->mObjectValue), size);

    if(!load_object_file(str)) {
        printf("can't load this class(%s)\n", str);
    }

    FREE(str);

    return TRUE;
}

static BOOL Clover_print(MVALUE* stack, MVALUE* stack_ptr)
{
    MVALUE* value = stack_ptr-1;

    if(value->mIntValue == 0) {
        /// exception ///
puts("throw exception");
return TRUE;
    }

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    char* str = MALLOC(size);
    wcstombs(str, CLASTART(value->mObjectValue), size);

    printf("%s\n", str);

    FREE(str);

    return TRUE;
}

static BOOL Clover_compile(MVALUE* stack, MVALUE* stack_ptr)
{
/*
    MVALUE* value = stack_ptr-1;

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    char* fname = MALLOC(size);
    wcstombs(fname, CLASTART(value->mObjectValue), size);

    int fd = open(fname, O_RDONLY);

    if(fd < 0) {
        fprintf(stderr, "can't open %s.\n", fname);
        FREE(fname);
        return FALSE;
    }

    sBuf buf;
    sBuf_init(&buf);

    while(1) {
        char buf2[BUFSIZ];
        int size = read(fd, buf2, BUFSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&buf, buf2, size);
    }

    close(fd);

    int sline = 1;
    if(!cl_parse_class(buf.mBuf, fname, &sline)) {
        FREE(buf.mBuf);
        FREE(fname);
        return FALSE;
    }

    FREE(buf.mBuf);
    FREE(fname);
*/

    return TRUE;
}

static BOOL Clover_gc(MVALUE* stack, MVALUE* stack_ptr)
{
puts("running gc...");
    cl_gc();

    return TRUE;
}

static BOOL Clover_show_heap(MVALUE* stack, MVALUE* stack_ptr)
{
    show_heap();

    return TRUE;
}

static BOOL Clover_show_classes(MVALUE* stack, MVALUE* stack_ptr)
{
    printf("-+- class list -+-\n");
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;
                printf("%s\n", REAL_CLASS_NAME(klass));
show_class(klass);
                klass = next_klass;
            }
        }
    }

    return TRUE;
}


static BOOL String_length(MVALUE* stack, MVALUE* stack_ptr)
{
puts("Hello String_length");

    return TRUE;
}

static BOOL int_to_s(MVALUE* stack, MVALUE* stack_ptr)
{
    MVALUE* self = stack_ptr-1; // self

    char buf[128];
    int len = snprintf(buf, 128, "%d", self->mIntValue);

    wchar_t wstr[len+1];
    mbstowcs(wstr, buf, len+1);

    CLObject new_obj = create_string_object(wstr, len);

    gCLStackPtr--; // delete self
    gCLStackPtr->mObjectValue = new_obj;  // push result
    gCLStackPtr++;

    return TRUE;
}

static BOOL float_floor(MVALUE* stack, MVALUE* stack_ptr)
{
puts("float_floor");

    return TRUE;
}

typedef struct {
    uint mHash;
    fNativeMethod mFun;
} sNativeMethod;

// manually sort is needed
sNativeMethod gNativeMethods[] = {
    { 814, int_to_s },
    { 867, Clover_gc },
    { 1081, Clover_load },
    { 1126, float_floor },
    { 1222, Clover_print },
    { 1319, String_length },
    { 1410, Clover_compile },
    { 1623, Clover_show_heap }, 
    { 1959, Clover_show_classes }
};

static fNativeMethod get_native_method(char* name)
{
    uint hash = get_hash(name);

    uint top = 0;
    uint bot = sizeof(gNativeMethods) / sizeof(sNativeMethod);

    while(1) {
        uint mid = (top + bot) / 2;
        
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
static ALLOC uchar* load_file(char* file_name)
{
    int f = open(file_name, O_RDONLY);

    if(f < 0) {
        return NULL;
    }

    sBuf buf;
    sBuf_init(&buf);

    while(1) {
        char buf2[BUFSIZ];
        int size = read(f, buf2, BUFSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&buf, buf2, size);
    }

    return (uchar*)ALLOC buf.mBuf;
}

static sCLClass* read_class_from_buffer(uchar** p)
{
    int i;

    sCLClass* klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mFlags = *(uint*)(*p);
    (*p) += sizeof(uint);

    /// load constant pool ///
    const int const_pool_len = *(uint*)(*p);
    (*p) += sizeof(uint);

    sConst_append(&klass->mConstPool, *p, const_pool_len);
    (*p) += const_pool_len;

    /// load namespace offset ///
    klass->mNameSpaceOffset = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    /// load class name offset ///
    klass->mClassNameOffset = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    /// load real class name offset //
    klass->mRealClassNameOffset = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    /// load fields ///
    klass->mSizeFields = klass->mNumFields = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mFlags = *(uint*)(*p);
        (*p) += sizeof(uint);
        klass->mFields[i].mNameOffset = *(uint*)(*p);
        (*p) += sizeof(uint);
        klass->mFields[i].mStaticField = *(MVALUE*)(*p);
        (*p) += sizeof(MVALUE);
        klass->mFields[i].mClassNameOffset = *(uint*)(*p);
        (*p) += sizeof(uint);
    }

    /// load methods ///
    klass->mSizeMethods = klass->mNumMethods = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        klass->mMethods[i].mFlags = *(uint*)(*p);
        (*p) += sizeof(uint);

        klass->mMethods[i].mNameOffset = *(uint*)(*p);
        (*p) += sizeof(int);

        klass->mMethods[i].mPathOffset = *(uint*)(*p);
        (*p) += sizeof(int);

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
            char* method_path = CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset);

            klass->mMethods[i].mNativeMethod = get_native_method(method_path);
            if(klass->mMethods[i].mNativeMethod == NULL) {
                fprintf(stderr, "native method(%s) is not found\n", method_path);
            }
        }
        else {
            sByteCode_init(&klass->mMethods[i].mByteCodes);

            const int len_bytecodes = *(uint*)(*p);
            (*p) += sizeof(uint);
            sByteCode_append(&klass->mMethods[i].mByteCodes, *p, len_bytecodes);
            (*p) += len_bytecodes;
        }

        klass->mMethods[i].mResultType = *(uint*)(*p);
        (*p) += sizeof(uint);

        const int param_num = *(uint*)(*p);
        klass->mMethods[i].mNumParams = param_num;
        (*p) += sizeof(uint);

        if(param_num == 0) 
            klass->mMethods[i].mParamTypes = NULL;
        else 
            klass->mMethods[i].mParamTypes = CALLOC(1, sizeof(uint)*param_num);

        int j;
        for(j=0; j<param_num; j++) {
            klass->mMethods[i].mParamTypes[j] = *(uint*)(*p);
            (*p) += sizeof(int);
        }

        const int local_num = *(uint*)(*p);
        klass->mMethods[i].mNumLocals = local_num;
        (*p) += sizeof(uint);

        const int max_stack = *(uint*)(*p);
        klass->mMethods[i].mMaxStack = max_stack;
        (*p) += sizeof(uint);
    }

    /// load super classes ///
    klass->mNumSuperClasses = *(uchar*)(*p);
    (*p) += sizeof(uchar);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        klass->mSuperClassesOffset[i] = *(uint*)(*p);
        (*p) += sizeof(uint);
    }

    return klass;
}

static void write_class_to_buffer(sCLClass* klass, sBuf* buf)
{
    uint flags = klass->mFlags & ~CLASS_FLAGS_MODIFIED;
    sBuf_append(buf, &flags, sizeof(uint));

    /// save constant pool ///
    sBuf_append(buf, &klass->mConstPool.mLen, sizeof(uint));
    sBuf_append(buf, klass->mConstPool.mConst, klass->mConstPool.mLen);

    /// save class name offset
    sBuf_append(buf, &klass->mNameSpaceOffset, sizeof(uchar));
    sBuf_append(buf, &klass->mClassNameOffset, sizeof(uchar));
    sBuf_append(buf, &klass->mRealClassNameOffset, sizeof(uchar));

    /// save fields
    sBuf_append(buf, &klass->mNumFields, sizeof(uchar));
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sBuf_append(buf, &klass->mFields[i].mFlags, sizeof(uint));
        sBuf_append(buf, &klass->mFields[i].mNameOffset, sizeof(uint));
        sBuf_append(buf, &klass->mFields[i].mStaticField, sizeof(MVALUE));
        sBuf_append(buf, &klass->mFields[i].mClassNameOffset, sizeof(uint));
    }

    /// save methods
    sBuf_append(buf, &klass->mNumMethods, sizeof(uchar));
    for(i=0; i<klass->mNumMethods; i++) {
        sBuf_append(buf, &klass->mMethods[i].mFlags, sizeof(uint));
        sBuf_append(buf, &klass->mMethods[i].mNameOffset, sizeof(uint));

        sBuf_append(buf, &klass->mMethods[i].mPathOffset, sizeof(uint));

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
        }
        else {
            sBuf_append(buf, &klass->mMethods[i].mByteCodes.mLen, sizeof(uint));
            sBuf_append(buf, klass->mMethods[i].mByteCodes.mCode, klass->mMethods[i].mByteCodes.mLen);
        }

        sBuf_append(buf, &klass->mMethods[i].mResultType, sizeof(uint));
        sBuf_append(buf, &klass->mMethods[i].mNumParams, sizeof(uint));

        int j;
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            sBuf_append(buf, &klass->mMethods[i].mParamTypes[j], sizeof(uint));
        }

        sBuf_append(buf, &klass->mMethods[i].mNumLocals, sizeof(uint));
        sBuf_append(buf, &klass->mMethods[i].mMaxStack, sizeof(uint));
    }

    /// write super classes ///
    sBuf_append(buf, &klass->mNumSuperClasses, sizeof(uchar));

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sBuf_append(buf, &klass->mSuperClassesOffset[i], sizeof(uint));
    }
}

BOOL check_super_class_offsets(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class = cl_get_class(CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]));

        if(super_class == NULL) {
            return FALSE;
        }
    }

    return TRUE;
}

// result: (FALSE) --> file not found (TRUE) --> success
BOOL load_object_file(char* file_name)
{
    int i;

    uchar* file_data = ALLOC load_file(file_name);
    uchar* p = file_data;

    if(p == NULL) {
        return FALSE;
    }

    /// check magic number. Is this Clover object file? ///
    if(*p++ != 12) { FREE(file_data); return FALSE; }
    if(*p++ != 17) { FREE(file_data); return FALSE; }
    if(*p++ != 33) { FREE(file_data); return FALSE; }
    if(*p++ != 79) { FREE(file_data); return FALSE; }
    if(*p++ != 'C') { FREE(file_data); return FALSE; }
    if(*p++ != 'L') { FREE(file_data); return FALSE; }
    if(*p++ != 'O') { FREE(file_data); return FALSE; }
    if(*p++ != 'V') { FREE(file_data); return FALSE; }
    if(*p++ != 'E') { FREE(file_data); return FALSE; }
    if(*p++ != 'R') { FREE(file_data); return FALSE; }

    uint num_classes = *(uint*)p;
    p += sizeof(uint);

    sCLClass* classes[num_classes];

    for(i=0; i<num_classes; i++) {
        sCLClass* klass = read_class_from_buffer(&p);
        add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);
        classes[i] = klass;
printf("loaded class %s::%s...\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));
    }

    for(i=0; i<num_classes; i++) {
        if(!check_super_class_offsets(classes[i])) {
printf("remove class(%s::%s) because there is not the super classes\n", NAMESPACE_NAME(classes[i]), CLASS_NAME(classes[i]));
            remove_class_from_class_table(NAMESPACE_NAME(classes[i]), CLASS_NAME(classes[i]));
            free_class(classes[i]);
        }
    }

    FREE(file_data);

    return TRUE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class(char* file_name)
{
    int i;

    uchar* file_data = ALLOC load_file(file_name);
    uchar* p = file_data;

    if(p == NULL) {
        return NULL;
    }

    sCLClass* klass = read_class_from_buffer(&p);

    if(check_super_class_offsets(klass)){
        add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

printf("loaded class %s::%s...\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));
    }
    else {
        free_class(klass);

printf("can't load class %s::%s because of super classes\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));
    }

    FREE(file_data);

    return klass;
}

// if a modified class exists, save it. this is used by compiler.c
// (FALSE) --> failed to write (TRUE) --> success
BOOL save_object_file(char* file_name)
{
    sBuf buf;
    sBuf_init(&buf);

    /// write magic number ///
    char magic_number[16];
    magic_number[0] = 12;
    magic_number[1] = 17;
    magic_number[2] = 33;
    magic_number[3] = 79;

    strcpy(magic_number + 4, "CLOVER");
    sBuf_append(&buf, magic_number, sizeof(char)*10);

    /// count modified classes ///
    uint num_classes = 0;

    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;

                if(klass->mFlags & CLASS_FLAGS_MODIFIED) {
                    num_classes++;
                }

                klass = next_klass;
            }
        }
    }

    sBuf_append(&buf, &num_classes, sizeof(uint));

    /// add class data to buffer ///
printf("writing modified classes...\n");
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;

                if(klass->mFlags & CLASS_FLAGS_MODIFIED) {
printf("writing %s::%s\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));
                    write_class_to_buffer(klass, &buf);
                }

                klass = next_klass;
            }
        }
    }

    /// write ///
    int f = open(file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    uint total_size = 0;
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

// (FALSE) --> failed to write (TRUE) --> success
BOOL save_class(sCLClass* klass, char* file_name)
{
    sBuf buf;
    sBuf_init(&buf);

    write_class_to_buffer(klass, &buf);

    /// write ///
    int f = open(file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    uint total_size = 0;
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

void show_class(sCLClass* klass)
{
    printf("-+- %s::%s -+-\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));

/*
    /// show constant pool ///
    printf("constant len %d\n", klass->mConstPool.mLen);
    show_constants(&klass->mConstPool);
*/

    char* p = klass->mConstPool.mConst;

    while(p - klass->mConstPool.mConst < klass->mConstPool.mLen) {
        uchar type = *p;
        p++;

        switch(type) {
            case CONSTANT_STRING: {
                uint len = *(uint*)p;
                p+=sizeof(int);
                char* str = p;
                p+=len;
                printf("len (%d) str (%s)\n", len, str);
                }
                break;

            case CONSTANT_INT: {
                uint num = *(uint*)p;
                p+=sizeof(int);

                printf("value (%d)\n", num);
                }
                break;
        }
    }

    printf("ClassNameOffset %d (%s)\n", klass->mClassNameOffset, CONS_str(klass->mConstPool, klass->mClassNameOffset));
    printf("NameSpaceOffset %d (%s)\n", klass->mNameSpaceOffset, CONS_str(klass->mConstPool, klass->mNameSpaceOffset));
    printf("RealClassNameOffset %d (%s)\n", klass->mRealClassNameOffset, CONS_str(klass->mConstPool, klass->mRealClassNameOffset));

    printf("num fields %d\n", klass->mNumFields);

    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class = cl_get_class(CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]));
        ASSERT(super_class);  // checked on load time
        printf("SuperClass[%d] %s\n", i, REAL_CLASS_NAME(super_class));
    }

    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mFlags & CL_STATIC_FIELD) {
            printf("field number %d --> %s static field %d\n", i, FIELD_NAME(klass, i), klass->mFields[i].mStaticField.mIntValue);
        }
        else {
            printf("field number %d --> %s\n", i, FIELD_NAME(klass, i));
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        printf("--- method number %d ---\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mNameOffset));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset));
        printf("result type %s\n", CONS_str(klass->mConstPool, klass->mMethods[i].mResultType));
        printf("num params %d\n", klass->mMethods[i].mNumParams);
        printf("num locals %d\n", klass->mMethods[i].mNumLocals);
        int j;
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            printf("%d. %s\n", j, CONS_str(klass->mConstPool, klass->mMethods[i].mParamTypes[j]));
        }

        if(klass->mMethods[i].mFlags & CL_STATIC_METHOD) {
            printf("static method\n");
        }

        if(klass->mMethods[i].mFlags & CL_NATIVE_METHOD) {
            printf("native methods %p\n", klass->mMethods[i].mNativeMethod);
        }
        else {
            printf("length of bytecodes %d\n", klass->mMethods[i].mByteCodes.mLen);
int j;
for(j=0; j<klass->mMethods[i].mByteCodes.mLen; j++) {
    printf("(%d) ", klass->mMethods[i].mByteCodes.mCode[j]);
}
puts("");
        }
    }
}

void show_all_classes()
{
    printf("-+- class list -+-\n");
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;
                printf("%s::%s\n", NAMESPACE_NAME(klass), CLASS_NAME(klass));
                klass = next_klass;
            }
        }
    }
}

//////////////////////////////////////////////////
// accessor function
//////////////////////////////////////////////////

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

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, char* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return klass->mMethods + i;
        }
    }

    return NULL;
}

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_with_params(sCLClass* klass, char* method_name, sCLClass** class_params, uint num_params)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method = klass->mMethods + i;

            /// type checking ///
            if(num_params == method->mNumParams) {
                int j;
                for(j=0; j<num_params; j++ ) {
                    char* real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[j]);
                    sCLClass* class_param2 = cl_get_class(real_class_name);

                    if(class_param2 == NULL || !type_checking(class_param2, class_params[j])) {
                        break;
                    }
                }

                if(j == num_params) {
                    return method;
                }
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLClass** class_params, uint num_params, sCLClass** found_class)
{
    sCLMethod* result = get_method_with_params(klass, method_name, class_params, num_params);
    *found_class = klass;

    if(result == NULL) {
        result = get_method_with_params_on_super_classes(klass, method_name, class_params, num_params, found_class);
    }

    return result;
}

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        sCLClass* super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == searched_class) {
            return TRUE;                // found
        }
    }

    return FALSE;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, char* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index_with_params(sCLClass* klass, char* method_name, sCLClass** class_params, uint num_params)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method = klass->mMethods + i;

            /// type checking ///
            if(num_params == method->mNumParams) {
                int j;
                for(j=0; j<num_params; j++) {
                    char* real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[j]);
                    sCLClass* class_param2 = cl_get_class(real_class_name);

                    if(class_param2 == NULL || !type_checking(class_param2, class_params[j])) {
                        break;
                    }
                }

                if(j == num_params) {
                    return i;
                }
            }
        }
    }

    return -1;
}

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* get_method_param_types(sCLClass* klass, sCLMethod* method, int param_num)
{
    if(klass != NULL && method != NULL) {
        if(param_num >= 0 && param_num < method->mNumParams && method->mParamTypes != NULL) {
            char* real_class_name = CONS_str(klass->mConstPool, method->mParamTypes[param_num]);
            return cl_get_class(real_class_name);
        }
    }

    return NULL;
}

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* get_method_result_type(sCLClass* klass, sCLMethod* method)
{
    char* real_class_name = CONS_str(klass->mConstPool, method->mResultType);
    return cl_get_class(real_class_name);
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_with_params_on_super_classes(sCLClass* klass, char* method_name, sCLClass* class_params[], uint num_params, sCLClass** found_class)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        sCLClass* super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        sCLMethod* method = get_method_with_params(super_class, method_name, class_params, num_params);

        if(method) {
            *found_class = super_class;
            return method;
        }
    }

    *found_class = NULL;

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** found_class)
{
    int i;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name = CONS_str(klass->mConstPool, klass->mSuperClassesOffset[i]);
        sCLClass* super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);  // checked on load time

        sCLMethod* method = get_method(super_class, method_name);

        if(method) {
            *found_class = super_class;
            return method;
        }
    }

    *found_class = NULL;

    return NULL;
}

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
sCLClass* gIntClass;      // foudamental classes
sCLClass* gStringClass;
sCLClass* gFloatClass;
sCLClass* gVoidClass;
sCLClass* gCloverClass;

void class_init(BOOL load_foundamental_class)
{
    if(load_foundamental_class) {
        if(!load_object_file(DATAROOTDIR "/Foudamental.clo")) {
            fprintf(stderr, "can't load Foudamental classes. abort");
            exit(1);
        }
        gVoidClass = cl_get_class_with_namespace("", "void");
        gIntClass = cl_get_class_with_namespace("", "int");
        gFloatClass = cl_get_class_with_namespace("", "float");
        gStringClass = cl_get_class_with_namespace("", "String");
        gCloverClass  = cl_get_class_with_namespace("", "Clover");
    }
}

void class_final()
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;
                free_class(klass);
                klass = next_klass;
            }
        }
    }
}
