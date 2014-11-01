#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

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

static void initialize_hidden_class_method_and_flags(sCLClass* klass)
{
    switch(CLASS_KIND(klass)) {
        case CLASS_KIND_VOID:
        case CLASS_KIND_INT:
        case CLASS_KIND_FLOAT:
        case CLASS_KIND_BOOL:
        case CLASS_KIND_NULL:
        case CLASS_KIND_BYTE:
            klass->mFlags |= CLASS_FLAGS_IMMEDIATE_VALUE_CLASS;
            initialize_hidden_class_method_of_immediate_value(klass);
            break;

        case CLASS_KIND_ARRAY:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_array(klass);
            break;

        case CLASS_KIND_HASH:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_hash(klass);
            break;

        case CLASS_KIND_STRING:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_string(klass);
            break;

        case CLASS_KIND_BYTES:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_bytes(klass);
            break;

        case CLASS_KIND_CLASSNAME:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_class_name(klass);
            break;

        case CLASS_KIND_BLOCK:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_block(klass);
            break;

        case CLASS_KIND_THREAD:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_thread(klass);
            break;

        case CLASS_KIND_MUTEX:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_mutex(klass);
            break;

        case CLASS_KIND_TYPE:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_type(klass);
            break;

        case CLASS_KIND_FILE:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_file(klass);
            break;

        case CLASS_KIND_REGULAR_FILE:
            klass->mFlags |= CLASS_FLAGS_SPECIAL_CLASS;
            initialize_hidden_class_method_of_regular_file(klass);
            break;

        default:
            initialize_hidden_class_method_of_user_object(klass);
            break;
    }
}

sCLClass* gVoidClass;
sCLClass* gIntClass;
sCLClass* gByteClass;
sCLClass* gIntClass;
sCLClass* gByteClass;
sCLClass* gFloatClass;
sCLClass* gBoolClass;
sCLClass* gNullClass;
sCLClass* gObjectClass;
sCLClass* gArrayClass;
sCLClass* gBytesClass;
sCLClass* gHashClass;
sCLClass* gBlockClass;
sCLClass* gClassNameClass;
sCLClass* gTypeClass;
sCLClass* gStringClass;
sCLClass* gThreadClass;
sCLClass* gExceptionClass;
sCLClass* gExNullPointerClass;
sCLClass* gExRangeClass;
sCLClass* gExConvertingStringCodeClass;
sCLClass* gExClassNotFoundClass;
sCLClass* gExIOClass;
sCLClass* gExCantSolveGenericsTypeClass;
sCLClass* gExOverflowClass;
sCLClass* gAnonymousClass[CL_GENERICS_CLASS_PARAM_MAX];
//sCLClass* gMAnonymousClass[CL_GENERICS_CLASS_PARAM_MAX];

static void set_special_class_to_global_pointer(sCLClass* klass)
{
    switch(CLASS_KIND(klass)) {
        case CLASS_KIND_VOID:
            gVoidClass = klass;
            break;
            
        case CLASS_KIND_INT :
            gIntClass = klass;
            break;

        case CLASS_KIND_BYTE :
            gByteClass = klass;
            break;

        case CLASS_KIND_FLOAT :
            gFloatClass = klass;
            break;

        case CLASS_KIND_BOOL :
            gBoolClass = klass;
            break;

        case CLASS_KIND_NULL :
            gNullClass = klass;
            break;

        case CLASS_KIND_OBJECT :
            gObjectClass = klass;
            break;

        case CLASS_KIND_ARRAY :
            gArrayClass = klass;
            break;

        case CLASS_KIND_BYTES :
            gBytesClass = klass;
            break;

        case CLASS_KIND_HASH :
            gHashClass = klass;
            break;

        case CLASS_KIND_BLOCK :
            gBlockClass = klass;
            break;

        case CLASS_KIND_CLASSNAME :
            gClassNameClass = klass;
            break;

        case CLASS_KIND_STRING :
            gStringClass = klass;
            break;

        case CLASS_KIND_THREAD :
            gThreadClass = klass;
            break;

        case CLASS_KIND_TYPE :
            gTypeClass = klass;
            break;

        case CLASS_KIND_EXCEPTION :
            gExceptionClass = klass;
            break;

        case CLASS_KIND_NULL_POINTER_EXCEPTION:
            gExNullPointerClass = klass;
            break;

        case CLASS_KIND_RANGE_EXCEPTION:
            gExRangeClass = klass;
            break;

        case CLASS_KIND_CONVERTING_STRING_CODE_EXCEPTION:
            gExConvertingStringCodeClass = klass;
            break;

        case CLASS_KIND_CLASS_NOT_FOUND_EXCEPTION:
            gExClassNotFoundClass = klass;
            break;

        case CLASS_KIND_IO_EXCEPTION:
            gExIOClass = klass;
            break;

        case CLASS_KIND_OVERFLOW_EXCEPTION:
            gExOverflowClass = klass;
            break;

        case CLASS_KIND_CANT_SOLVE_GENERICS_TYPE:
            gExCantSolveGenericsTypeClass = klass;
            break;

        case CLASS_KIND_ANONYMOUS: {
            int anonymous_num;

            anonymous_num = (REAL_CLASS_NAME(klass)[9] - '0');

            ASSERT(anonymous_num >= 0 && anonymous_num < CL_GENERICS_CLASS_PARAM_MAX); // This is checked at alloc_class() which is written on klass.c

            gAnonymousClass[anonymous_num] = klass;
            }
            break;

/*
        case CLASS_KIND_MANONYMOUS: {
            int anonymous_num;

            anonymous_num = (REAL_CLASS_NAME(klass)[10] - '0');

            ASSERT(anonymous_num >= 0 && anonymous_num < CL_GENERICS_CLASS_PARAM_MAX); // This is checked at alloc_class() which is written on klass.c

            gMAnonymousClass[anonymous_num] = klass;
            }
            break;
*/
    }
}

// result should be not NULL
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface)
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

    sConst_init(&klass->mConstPool);

    klass->mFlags |= (private_ ? CLASS_FLAGS_PRIVATE:0) | (interface ? CLASS_FLAGS_INTERFACE:0) | (abstract_ ? CLASS_FLAGS_ABSTRACT:0);

    klass->mSizeMethods = 4;
    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);
    klass->mSizeFields = 4;
    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    memset(klass->mSuperClasses, 0, sizeof(klass->mSuperClasses));
    klass->mNumSuperClasses = 0;

    memset(klass->mImplementedInterfaces, 0, sizeof(klass->mImplementedInterfaces));
    klass->mNumImplementedInterfaces = 0;

    klass->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, class_name);  // class name
    klass->mNameSpaceOffset = append_str_to_constant_pool(&klass->mConstPool, namespace);   // namespace 

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);
    klass->mRealClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);  // real class name

    add_class_to_class_table(namespace, class_name, klass);

    klass->mSizeDependences = 4;
    klass->mDepedencesOffset = CALLOC(1, sizeof(int)*klass->mSizeDependences);
    klass->mNumDependences = 0;

    klass->mNumVirtualMethodMap = 0;
    klass->mSizeVirtualMethodMap = 4;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);

    if(strcmp(REAL_CLASS_NAME(klass), "void") == 0) {
        klass->mFlags |= CLASS_KIND_VOID;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "int") == 0) {
        klass->mFlags |= CLASS_KIND_INT;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "byte") == 0) {
        klass->mFlags |= CLASS_KIND_BYTE;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "float") == 0) {
        klass->mFlags |= CLASS_KIND_FLOAT;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "bool") == 0) {
        klass->mFlags |= CLASS_KIND_BOOL;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "null") == 0) {
        klass->mFlags |= CLASS_KIND_NULL;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Object") == 0) {
        klass->mFlags |= CLASS_KIND_OBJECT;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "String") == 0) {
        klass->mFlags |= CLASS_KIND_STRING;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Bytes") == 0) {
        klass->mFlags |= CLASS_KIND_BYTES;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "ClassName") == 0) {
        klass->mFlags |= CLASS_KIND_CLASSNAME;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Array") == 0) {
        klass->mFlags |= CLASS_KIND_ARRAY;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Exception") == 0) {
        klass->mFlags |= CLASS_KIND_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "NullPointerException") == 0) {
        klass->mFlags |= CLASS_KIND_NULL_POINTER_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "RangeException") == 0) {
        klass->mFlags |= CLASS_KIND_RANGE_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "ConvertingStringCodeException") == 0) {
        klass->mFlags |= CLASS_KIND_CONVERTING_STRING_CODE_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "ClassNotFoundException") == 0) {
        klass->mFlags |= CLASS_KIND_CLASS_NOT_FOUND_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "IOException") == 0) {
        klass->mFlags |= CLASS_KIND_IO_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "OverflowException") == 0) {
        klass->mFlags |= CLASS_KIND_OVERFLOW_EXCEPTION;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "CantSolveGenericsType") == 0) {
        klass->mFlags |= CLASS_KIND_CANT_SOLVE_GENERICS_TYPE;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Hash") == 0) {
        klass->mFlags |= CLASS_KIND_HASH;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Block") == 0) {
        klass->mFlags |= CLASS_KIND_BLOCK;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Thread") == 0) {
        klass->mFlags |= CLASS_KIND_THREAD;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Mutex") == 0) {
        klass->mFlags |= CLASS_KIND_MUTEX;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "Type") == 0) {
        klass->mFlags |= CLASS_KIND_TYPE;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "File") == 0) {
        klass->mFlags |= CLASS_KIND_FILE;
    }
    else if(strcmp(REAL_CLASS_NAME(klass), "RegularFile") == 0) {
        klass->mFlags |= CLASS_KIND_REGULAR_FILE;
    }
    else if(strstr(REAL_CLASS_NAME(klass), "Anonymous") == REAL_CLASS_NAME(klass)) 
    {
        char* class_name;

        class_name = REAL_CLASS_NAME(klass);

        if(strlen(class_name) == 10) {
            int anonymous_num;

            anonymous_num = class_name[9] - '0';

            if(anonymous_num >= 0 && anonymous_num < CL_GENERICS_CLASS_PARAM_MAX)
            {
                klass->mFlags |= CLASS_KIND_ANONYMOUS;
            }
        }
    }
/*
    else if(strstr(REAL_CLASS_NAME(klass), "MAnonymous") == REAL_CLASS_NAME(klass)) 
    {
        char* class_name;

        class_name = REAL_CLASS_NAME(klass);

        if(strlen(class_name) == 11) {
            int anonymous_num;

            anonymous_num = class_name[10] - '0';

            if(anonymous_num >= 0 && anonymous_num < CL_GENERICS_CLASS_PARAM_MAX)
            {
                klass->mFlags |= CLASS_KIND_MANONYMOUS;
            }
        }
    }
*/

    set_special_class_to_global_pointer(klass);

    /// set hidden flags to special classes ///
    initialize_hidden_class_method_and_flags(klass);

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
            if(method->mParamInitializers) {
                int i;
                for(i=0; i<method->mNumParams; i++) {
                    if(method->mParamInitializers[i].mInitializer.mCode) {
                        sByteCode_free(&method->mParamInitializers[i].mInitializer);
                    }
                }
                FREE(method->mParamInitializers);
            }

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

            if(field->mInitializer.mSize > 0) {
                sByteCode_free(&field->mInitializer);
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

//////////////////////////////////////////////////
// fields
//////////////////////////////////////////////////
BOOL is_parent_immediate_value_class(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
            return TRUE;
        }
    }

    return FALSE;
}

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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        static_field_num += get_static_fields_num(super_class);
    }

    static_field_num += get_static_fields_num(klass);

    return static_field_num;
}

BOOL run_fields_initializer(CLObject object, sCLClass* klass, CLObject vm_type)
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
        else if(cl_field->mInitializer.mSize > 0) {
            MVALUE result;

            lv_num = cl_field->mInitializerLVNum;
            max_stack = cl_field->mInitializerMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializer, &klass->mConstPool, lv_num, max_stack, vm_type)) {
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

            lv_num = cl_field->mInitializerLVNum;
            max_stack = cl_field->mInitializerMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializer, &klass->mConstPool, lv_num, max_stack, 0)) {
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
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

void create_real_method_name(char* real_method_name, int real_method_name_size, char* method_name, int num_params)
{
    char num_params_buf[16];

    snprintf(num_params_buf, 16, "%d", num_params);

    xstrncpy(real_method_name, method_name, real_method_name_size);
    xstrncat(real_method_name, num_params_buf, real_method_name_size);
}

static BOOL solve_generics_types_of_class(sCLClass* klass, sCLClass** result, CLObject type_object)
{
    int i;

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(klass == gAnonymousClass[i]) {
            if(i < CLTYPEOBJECT(type_object)->mGenericsTypesNum) {
                CLObject type_object2;

                type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
                *result = CLTYPEOBJECT(type_object2)->mClass;
                return TRUE;
            }
            else {
                *result = klass; // error
                return FALSE;
            }
        }
    }

    *result = klass; // no solved

    return TRUE;
}

static BOOL check_method_params(sCLMethod* method, sCLClass* klass, char* method_name, char** class_params, int num_params, BOOL search_for_class_method, int block_num, int block_num_params, char** block_param_type, char* block_type, CLObject type_object, CLObject vm_type)
{
    if(strcmp(METHOD_NAME2(klass, method), method_name) ==0) {
        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) 
        {
            /// type checking ///
            if(method->mNumParams == -1) {              // no type checking of method params
                return TRUE;
            }
            else if(num_params == method->mNumParams) {
                int j, k;

                for(j=0; j<num_params; j++ ) {
                    sCLClass* klass_of_param;
                    sCLClass* solved_klass_of_param;
                    sCLClass* klass_of_param2;
                    sCLClass* solved_klass_of_param2;
                    char* real_class_name;

                    real_class_name = CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset);
                    klass_of_param = cl_get_class(real_class_name);

                    klass_of_param2 = cl_get_class(class_params[j]);

                    if(klass_of_param == NULL || klass_of_param2 == NULL) {
                        return FALSE;
                    }

                    if(!solve_generics_types_of_class(klass_of_param, &solved_klass_of_param, type_object))
                    {
                        return FALSE;
                    }

                    if(!solve_generics_types_of_class(klass_of_param2, &solved_klass_of_param2, vm_type))
                    {
                        return FALSE;
                    }

                    if(!substitution_posibility_of_class(solved_klass_of_param, solved_klass_of_param2))
                    {
                        return FALSE;
                    }
                }

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                    if(block_num > 0) {
                        char* real_class_name;
                        sCLClass* klass_of_param;
                        sCLClass* solved_klass_of_param;
                        sCLClass* klass_of_param2;
                        sCLClass* solved_klass_of_param2;

                        real_class_name = CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset);
                        klass_of_param = cl_get_class(real_class_name);

                        klass_of_param2 = cl_get_class(block_type);

                        if(klass_of_param == NULL || klass_of_param2 == NULL) {
                            return FALSE;
                        }

                        if(!solve_generics_types_of_class(klass_of_param, &solved_klass_of_param, type_object))
                        {
                            return FALSE;
                        }
                        
                        if(!solve_generics_types_of_class(klass_of_param2, &solved_klass_of_param2, vm_type))
                        {
                            return FALSE;
                        }

                        if(!substitution_posibility_of_class(solved_klass_of_param, solved_klass_of_param2))
                        {
                            return FALSE;
                        }
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        char* real_class_name;
                        sCLClass* klass_of_param;
                        sCLClass* solved_klass_of_param;
                        sCLClass* klass_of_param2;
                        sCLClass* solved_klass_of_param2;

                        real_class_name = CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[k].mClassNameOffset);
                        klass_of_param = cl_get_class(real_class_name);

                        klass_of_param2 = cl_get_class(block_param_type[k]);

                        if(klass_of_param == NULL || klass_of_param2 == NULL) {
                            return FALSE;
                        }

                        if(!solve_generics_types_of_class(klass_of_param, &solved_klass_of_param, type_object))
                        {
                            return FALSE;
                        }
                        
                        if(!solve_generics_types_of_class(klass_of_param2, &solved_klass_of_param2, vm_type))
                        {
                            return FALSE;
                        }

                        if(!substitution_posibility_of_class(solved_klass_of_param, solved_klass_of_param2))
                        {
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

static sCLMethod* search_for_method_from_virtual_method_table(CLObject type_object, char* method_name, char** class_params, int num_params, BOOL search_for_class_method, int block_num, int block_num_params, char** block_param_type, char* block_type, CLObject vm_type)
{
    char real_method_name[CL_VMT_NAME_MAX+1];
    int hash;
    sVMethodMap* item;
    int i;
    sCLClass* klass;

    klass = CLTYPEOBJECT(type_object)->mClass;

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

            if(check_method_params(method, klass, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, type_object, vm_type))
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
sCLMethod* get_virtual_method_with_params(CLObject type_object, char* method_name, char** class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, int block_num, int block_num_params, char** block_param_type, char* block_type,sVMInfo* info, CLObject vm_type)
{
    sCLMethod* result;
    int i;
    sCLClass* klass;

    push_object(type_object, info);

    klass = CLTYPEOBJECT(type_object)->mClass;

    result = search_for_method_from_virtual_method_table(type_object, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, vm_type);

    *founded_class = klass;
    
    if(result == NULL) {
        int i;

        for(i=klass->mNumSuperClasses-1; i>=0; i--) {
            CLObject new_type_object;
            CLObject solved_new_type_object;

            new_type_object = get_type_object_from_cl_type(&klass->mSuperClasses[i], klass, info);

            push_object(new_type_object, info);

            if(!solve_generics_types_of_type_object(new_type_object, ALLOC &solved_new_type_object, type_object, info)) {
                remove_object(info, 2);
                return NULL;
            }

            pop_object(info);
            pop_object(info);

            type_object = solved_new_type_object;
            push_object(type_object, info);

            result = search_for_method_from_virtual_method_table(type_object, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, vm_type);

            if(result) {
                *founded_class = CLTYPEOBJECT(type_object)->mClass;
                break;
            }
        }
    }

    //ASSERT(result != NULL);

    pop_object(info);

    return result;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == searched_class) {
            return TRUE;                // found
        }
    }

    return FALSE;
}

static BOOL search_for_implemeted_interface_core(sCLClass* klass, sCLClass* interface) 
{
    int i;

    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        char* real_class_name;
        sCLClass* interface2;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mImplementedInterfaces[i].mClassNameOffset);
        interface2 = cl_get_class(real_class_name);

        ASSERT(interface2 != NULL);     // checked on load time

        if(interface == interface2) {
            return TRUE;                // found
        }

        /// search for the super class of this interface ///
        if(search_for_super_class(interface2, interface)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL search_for_implemeted_interface(sCLClass* klass, sCLClass* interface)
{
    int i;

    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(search_for_implemeted_interface_core(super_class, interface)) {
            return TRUE;
        }
    }

    if(search_for_implemeted_interface_core(klass, interface)) {
        return TRUE;
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
    { 1159, "Thread.join()", Thread_join },
    { 1189, "Mutex.Mutex()", Mutex_Mutex },
    { 1208, "bool.to_int()", bool_to_int },
    { 1208, "int.to_bool()", int_to_bool },
    { 1216, "int.to_byte()", int_to_byte },
    { 1216, "byte.to_int()", byte_to_int } ,
    { 1223, "null.to_int()", null_to_int },
    { 1280, "Array.length()", Array_length },
    { 1288, "Bytes.length()", Bytes_length },
    { 1314, "float.to_int()", float_to_int },
    { 1320, "null.to_bool()", null_to_bool },
    { 1391, "Bytes.char(int)", Bytes_char },
    { 1400, "String.length()", String_length },
    { 1443, "int.to_string()", int_to_string },
    { 1503, "String.char(int)", String_char },
    { 1515, "Array.items(int)", Array_items },
    { 1540, "bool.to_string()", bool_to_string },
    { 1545, "System.exit(int)", System_exit },
    { 1548, "byte.to_string()", byte_to_string },
    { 1555, "null.to_string()", null_to_string },
    { 1585, "File.write(Bytes)", File_write },
    { 1631, "Bytes.to_string()", Bytes_to_string },
    { 1631, "String.to_bytes()", String_to_bytes }, 
    { 1640, "System.sleep(int)", System_sleep },
    { 1646, "float.to_string()", float_to_string },
    { 1681, "Mutex.run()void{}", Mutex_run },
    { 1772, "Object.class_name()", Object_class_name },
    { 1934, "Clover.print(String)", Clover_print },
    { 1952, "Array.add(Anonymous0)", Array_add },
    { 1955, "Array._constructor()", Array_Array },
    { 1999, "ClassName.to_string()", ClassName_to_string },
    { 2021, "String.append(String)", String_append },
    { 2040, "Clover.show_classes()", Clover_show_classes },
    { 2052, "System.getenv(String)", System_getenv },
    { 2075, "String._constructor()", String_String },
    { 2189, "Bytes.replace(int,byte)", Bytes_replace },
    { 2196, "String.replace(int,int)", String_replace },
    { 2444, "Object.is_child(ClassName)", Object_is_child },
    { 2679, "Object.instanceof(ClassName)", Object_instanceof } ,
    { 2726, "Thread._constructor()void{}", Thread_Thread },
    { 3197, "Clover.output_to_string()void{}", Clover_output_to_string }, 
    { 4020, "RegularFile.RegularFile(String,String,int)", RegularFile_RegularFile },
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
        type->mGenericsTypes[j] = allocate_cl_type();
        if(!read_type_from_file(fd, type->mGenericsTypes[j])) {
            return FALSE;
        }
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

static BOOL read_param_initializer_from_file(int fd, sCLParamInitializer* param_initializer)
{
    int len_bytecodes;
    int n;

    if(!read_int_from_file(fd, &len_bytecodes)) {
        return FALSE;
    }

    if(len_bytecodes > 0) {
        int* bytecodes;

        sByteCode_init(&param_initializer->mInitializer);
        bytecodes = MALLOC(sizeof(int)*len_bytecodes);

        if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
            FREE(bytecodes);
            return FALSE;
        }

        append_buf_to_bytecodes(&param_initializer->mInitializer, bytecodes, len_bytecodes);

        FREE(bytecodes);
    }
    else {
        param_initializer->mInitializer.mCode = NULL;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    param_initializer->mMaxStack = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    param_initializer->mLVNum = n;

    return TRUE;
}

static BOOL read_param_initializers_from_file(int fd, int* num_params, sCLParamInitializer** param_initializer)
{
    int j;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    
    *num_params = n;

    if(*num_params > 0) {
        *param_initializer = CALLOC(1, sizeof(sCLParamInitializer)*(*num_params));
        for(j=0; j<*num_params; j++) {
            if(!read_param_initializer_from_file(fd, (*param_initializer) + j)) 
            {
                return FALSE;
            }
        }
    }
    else {
        *param_initializer = NULL;
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
        sByteCode_init(&field->mInitializer);
        bytecodes = MALLOC(sizeof(int)*len_bytecodes);

        if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
            FREE(bytecodes);
            return FALSE;
        }

        append_buf_to_bytecodes(&field->mInitializer, bytecodes, len_bytecodes);

        FREE(bytecodes);
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mInitializerLVNum = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    field->mInitializerMaxStack = n;

    if(!read_type_from_file(fd, &field->mType)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_method_from_buffer(sCLClass* klass, sCLMethod* method, int fd)
{
    int n;
    int i;
    char c;

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
    if(!read_param_initializers_from_file(fd, &method->mNumParams, &method->mParamInitializers)) {
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

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    method->mNumParamInitializer = c;

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }
    method->mGenericsTypesNum = c;
    for(i=0; i<method->mGenericsTypesNum; i++) {
        if(!read_generics_param_types(fd, &method->mGenericsTypes[i])) {
            return FALSE;
        }
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

BOOL read_generics_param_types(int fd, sCLGenericsParamTypes* generics_param_types)
{
    int i;
    int n;
    char c;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    generics_param_types->mNameOffset = n;

    if(!read_type_from_file(fd, &generics_param_types->mExtendsType)) {
        return FALSE;
    }

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    generics_param_types->mNumImplementsTypes = c;

    for(i=0; i<generics_param_types->mNumImplementsTypes; i++) {
        if(!read_type_from_file(fd, generics_param_types->mImplementsTypes + i)) {
            return FALSE;
        }
    }

    return TRUE;
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
        if(!read_type_from_file(fd, klass->mSuperClasses + i)) {
            return NULL;
        }
    }

    /// load implement interfaces ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNumImplementedInterfaces = c;
    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        if(!read_type_from_file(fd, &klass->mImplementedInterfaces[i])) {
            return NULL;
        }
    }

    /// load class params of generics ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mGenericsTypesNum = c;
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        if(!read_generics_param_types(fd, &klass->mGenericsTypes[i])) {
            return NULL;
        }
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
static sCLClass* load_class(char* file_name, BOOL solve_dependences)
{
    sCLClass* klass;
    sCLClass* klass2;
    int fd;
    char c;
    int size;

    /// check the existance of the load class ///
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

    /// set hidden flags to special classes ///
    initialize_hidden_class_method_and_flags(klass);

    //// add class to class table ///
    add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    if(solve_dependences) {
        if(!check_dependece_offsets(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }
    }

    set_special_class_to_global_pointer(klass);

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

    cwd = getenv("PWD");
    if(cwd == NULL) {
        fprintf(stderr, "PWD environment path is NULL\n");
        return FALSE;
    }

    for(i=CLASS_VERSION_MAX; i>=1; i--) {
        /// default search path ///
        if(i == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clo", DATAROOTDIR, real_class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clo", DATAROOTDIR, real_class_name, i);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }

        /// current working directory ///
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

    return FALSE;
}

static void get_namespace_and_class_name_from_real_class_name(char* namespace, char* class_name, char* real_class_name)
{
    char* p;
    char* p2;
    char* p3;
    BOOL flag;

    p2 = strstr(real_class_name, "::");

    if(p2) {
        memcpy(namespace, real_class_name, p2 - real_class_name);
        namespace[p2 -real_class_name] = 0;

        xstrncpy(class_name, p2 + 2, CL_CLASS_NAME_MAX);
    }
    else {
        *namespace = 0;
        xstrncpy(class_name, real_class_name, CL_CLASS_NAME_MAX);
    }
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_from_classpath(char* real_class_name, BOOL solve_dependences)
{
    char class_file_path[PATH_MAX];
    sCLClass* klass;
    char class_name[CL_CLASS_NAME_MAX + 1];
    char namespace[CL_NAMESPACE_NAME_MAX + 1];

    if(!search_for_class_file_from_class_name(class_file_path, PATH_MAX, real_class_name)) {
        return NULL;
    }

    get_namespace_and_class_name_from_real_class_name(namespace, class_name, real_class_name);

    /// if there is a class which has been entried to class table already, return the class
    klass = cl_get_class_with_namespace(namespace, class_name);
    
    if(klass) {
        return klass;
    }

    return load_class(class_file_path, solve_dependences);
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
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[klass->mNumSuperClasses-1].mClassNameOffset);
        return cl_get_class(real_class_name);
    }
    else {
        return NULL;
    }
}

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
CLObject gTypeObject = 0;

void class_init()
{
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
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

        snprintf(real_class_name, CL_REAL_CLASS_NAME_MAX, "Anonymous%d", i);

        load_class_from_classpath(real_class_name, TRUE);

/*
        snprintf(real_class_name, CL_REAL_CLASS_NAME_MAX, "MAnonymous%d", i);

        load_class_from_classpath(real_class_name, TRUE);
*/
    }

    load_class_from_classpath("Type", TRUE);
    gTypeObject = create_type_object(gTypeClass);

    load_class_from_classpath("Exception", TRUE);
    load_class_from_classpath("NullPointerException", TRUE);
    load_class_from_classpath("RangeException", TRUE);
    load_class_from_classpath("ConvertingStringCodeException", TRUE);
    load_class_from_classpath("ClassNotFoundException", TRUE);
    load_class_from_classpath("IOException", TRUE);
    load_class_from_classpath("OverflowException", TRUE);
    load_class_from_classpath("CantSolveGenericsType", TRUE);

    load_class_from_classpath("Block", TRUE);
    load_class_from_classpath("null", TRUE);

    load_class_from_classpath("void", TRUE);
    load_class_from_classpath("int", TRUE);
    load_class_from_classpath("byte", TRUE);
    load_class_from_classpath("float", TRUE);
    load_class_from_classpath("bool", TRUE);
    load_class_from_classpath("String", TRUE);
    load_class_from_classpath("Array", TRUE);
    load_class_from_classpath("Bytes", TRUE);
    load_class_from_classpath("Hash", TRUE);

    load_class_from_classpath("ClassName", TRUE);

    load_class_from_classpath("Object", TRUE);

    load_class_from_classpath("Clover", TRUE);
/*
    load_class_from_classpath("Thread", TRUE);
    load_class_from_classpath("Mutex", TRUE);
    load_class_from_classpath("System", TRUE);
*/

    return TRUE;
}
