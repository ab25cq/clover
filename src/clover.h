#ifndef CLOVER_H
#define CLOVER_H

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////
// clover definition
/////////////////////////////////////////////////////
/// resizable buffer
typedef struct {
    char* mBuf;
    unsigned int mSize;
    unsigned int mLen;
} sBuf;

/// virtual machine data ///
#define OP_IADD 1
#define OP_LDC 2
#define OP_ASTORE 3
#define OP_ISTORE 4
#define OP_FSTORE 5
#define OP_OSTORE 6
#define OP_ALOAD 7
#define OP_ILOAD 8
#define OP_FLOAD 9
#define OP_OLOAD 10
#define OP_POP 11
#define OP_POP_N 12
#define OP_SADD 13
#define OP_FADD 14
#define OP_INVOKE_METHOD 15
#define OP_INVOKE_INHERIT 16
#define OP_RETURN 18
#define OP_NEW_OBJECT 19
#define OP_LDFIELD 20
#define OP_LD_STATIC_FIELD 21
#define OP_SRFIELD 22
#define OP_SR_STATIC_FIELD 23
#define OP_NEW_ARRAY 24

typedef struct {
    unsigned char* mCode;
    unsigned int mSize;
    unsigned int mLen;
} sByteCode;

#define CONSTANT_INT 1
#define CONSTANT_STRING 2
#define CONSTANT_WSTRING 3

#define CONS_type(constants, offset) *((constants).mConst + (offset))
#define CONS_str_len(constants, offset) *(int*)((constants).mConst + (offset) + 1)
#define CONS_str(constants, offset) (char*)((constants).mConst + (offset) + 1 + sizeof(int))
#define CONS_int(constants, offset) *(int*)((constants).mConst + (offset) + 1)

typedef struct {
    char* mConst;
    unsigned int mSize;
    unsigned int mLen;
} sConst;

typedef unsigned long CLObject;

struct sCLClassStruct;

typedef union {
    unsigned char mCharValue;
    unsigned int mIntValue;
    unsigned long mLongValue;
    CLObject mObjectValue;
    struct sCLClassStruct* mClassRef;
} MVALUE;

// limits:
#define CL_LOCAL_VARIABLE_MAX 64 // max number of local variables
#define CL_METHODS_MAX 64
#define CL_FIELDS_MAX 64
#define CL_METHOD_PARAM_MAX 16       // max number of param
#define CL_ARRAY_ELEMENTS_MAX 32     // max number of array elements(constant array value)
#define CL_GENERICS_CLASS_PARAM_MAX 8    // max number of generics class param

#define CL_NAMESPACE_NAME_MAX 32 // max length of namespace
#define CL_CLASS_NAME_MAX 32    // max length of class name
#define CL_REAL_CLASS_NAME_MAX (CL_NAMESPACE_NAME_MAX + CL_CLASS_NAME_MAX + 1)
#define CL_METHOD_NAME_MAX 32   // max length of method or field name
#define CL_METHOD_NAME_WITH_PARAMS_MAX (CL_METHOD_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX)
#define CL_CLASS_TYPE_VARIABLE_MAX CL_CLASS_NAME_MAX

#define WORDSIZ 128

#define CLASS_HASH_SIZE 256

typedef struct {
    unsigned int mClassNameOffset;                                  // real class name(offset of constant pool)
    char mGenericsTypesNum;
    unsigned int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];  // real class name(offset of constant pool)
} sCLType;

/// field flags ///
#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02

typedef struct {
    unsigned int mFlags;
    unsigned int mNameOffset;   // offset of constant pool

    union {
        MVALUE mStaticField;
        unsigned int mOffset;
    } uValue;

    sCLType mType;
} sCLField;

typedef BOOL (*fNativeMethod)(MVALUE* stack_ptr, MVALUE* lvar);

/// method flags ///
#define CL_NATIVE_METHOD 0x01
#define CL_CLASS_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04
#define CL_CONSTRUCTOR 0x08

typedef struct {
    unsigned int mFlags;
    unsigned int mNameOffset;     // offset of constant pool
    unsigned int mPathOffset;     // offset of constant pool

    union {
        sByteCode mByteCodes;
        fNativeMethod mNativeMethod;
    } uCode;

    sCLType mResultType;     // offset of constant pool(real class name --> namespace$class_name)

    int mNumParams;
    sCLType* mParamTypes;

    int mNumLocals;      // number of local variables

    int mMaxStack;
} sCLMethod;

/// class flags ///
#define CLASS_FLAGS_INTERFACE 0x0100
#define CLASS_FLAGS_IMMEDIATE_VALUE_CLASS 0x0400
#define CLASS_FLAGS_PRIVATE 0x0800
#define CLASS_FLAGS_OPEN 0x1000
#define CLASS_FLAGS_MODIFIED 0x2000
#define CLASS_FLAGS_VERSION 0x00ff
#define CLASS_VERSION(klass) (klass->mFlags & CLASS_FLAGS_VERSION)
#define CLASS_VERSION_MAX 255

#define SUPER_CLASS_MAX 8

#define NUM_DEFINITION_MAX 128

typedef struct {
    char mSuperClassIndex;
    char mMethodIndex;
} sVMethodMap;

typedef struct sCLClassStruct {
    unsigned int mFlags;
    sConst mConstPool;

    unsigned char mNameSpaceOffset;   // Offset of constant pool
    unsigned char mClassNameOffset;   // Offset of constant pool
    unsigned char mRealClassNameOffset;   // Offset of constant pool

    sCLField* mFields;
    char mNumFields;
    char mSizeFields;

    sCLMethod* mMethods;
    char mNumMethods;
    char mSizeMethods;

    unsigned int mSuperClassesOffset[SUPER_CLASS_MAX];
    char mNumSuperClasses;

    void (*mFreeFun)(CLObject self);

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list

    char mGenericsTypesNum;
    unsigned int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];            // class type variable offset

sVMethodMap* mVirtualMethodMap;
char mNumVMethodMap;
char mSizeVMethodMap;
} sCLClass;

typedef struct {
    sCLClass* mClass;
    char mGenericsTypesNum;
    sCLClass* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
} sCLNodeType;

#define CLASS_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mClassNameOffset + 1 + sizeof(int))
#define NAMESPACE_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mNameSpaceOffset + 1 + sizeof(int))
#define REAL_CLASS_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mRealClassNameOffset + 1 + sizeof(int))
#define FIELD_NAME(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mFields[(n)].mNameOffset + 1 + sizeof(int))
#define METHOD_NAME(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mNameOffset + 1 + sizeof(int))
#define METHOD_NAME2(klass, method) (char*)((klass)->mConstPool.mConst + (method)->mNameOffset + 1 + sizeof(int))
#define METHOD_PATH(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mPathOffset + 1 + sizeof(int))

void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class);
void cl_final();

BOOL cl_eval_file(char* file_name);

void cl_create_clc_file();
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, BOOL flg_main, int* err_num, int* max_stack, char* current_namespace);
BOOL cl_eval(char* cmdline, char* sname, int* sline);
BOOL cl_main(sByteCode* code, sConst* constant, unsigned int lv_num, unsigned int max_stack);
BOOL cl_excute_method(sCLMethod* method, sConst* constant, BOOL result_existance, sCLNodeType* generics_type);
void cl_gc();

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_);

sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name);
    // result: (NULL) --> not found (non NULL) --> (sCLClass*)

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name);

unsigned int cl_get_method_index(sCLClass* klass, unsigned char* method_name);
    // result: (-1) --> not found (non -1) --> method index

#define NAMESPACE_NAME_MAX 32

struct sCLNameSpaceStruct {
    char mName[NAMESPACE_NAME_MAX];
    sCLClass* mClassHashList[CLASS_HASH_SIZE];

    struct sCLNameSpaceStruct* mNextNameSpace;  // next namespace in hash table linked list
};

typedef struct sCLNameSpaceStruct sCLNameSpace;

#endif

