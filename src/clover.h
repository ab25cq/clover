#ifndef CLOVER_H
#define CLOVER_H

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////
// clover definition
/////////////////////////////////////////////////////
typedef long int64;
typedef unsigned long uint64;

typedef unsigned int uint;

typedef short int16;
typedef unsigned short uint16;

typedef float float32;

typedef unsigned char uchar;

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
#define OP_INVOKE_STATIC_METHOD 15
#define OP_INVOKE_METHOD 16
#define OP_RETURN 17
#define OP_NEW_OBJECT 18
#define OP_LDFIELD 19
#define OP_LD_STATIC_FIELD 20
#define OP_SRFIELD 21
#define OP_SR_STATIC_FIELD 22
#define OP_INVOKE_CONSTRUCTOR 23

typedef struct {
    uchar* mCode;
    uint mSize;
    uint mLen;
} sByteCode;

#define CONSTANT_INT 1
#define CONSTANT_STRING 2
#define CONSTANT_WSTRING 3

#define CONS_type(constants, offset) *(constants.mConst + offset)
#define CONS_str_len(constants, offset) *(int*)(constants.mConst + offset + 1)
#define CONS_str(constants, offset) (char*)(constants.mConst + offset + 1 + sizeof(int))
#define CONS_int(constants, offset) *(int*)(constants.mConst + offset + 1)

typedef struct {
    uchar* mConst;
    uint mSize;
    uint mLen;
} sConst;

typedef uint64 CLObject;

struct sCLClassStruct;

typedef union {
    uchar mCharValue;
    uint mIntValue;
    uint64 mLongValue;
    CLObject mObjectValue;
    struct sCLClassStruct* mClassRef;
} MVALUE;

/// field flags ///
#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02

typedef struct {
    uint mHeader;
    uint mNameOffset;   // offset of constant pool

    union {
        MVALUE mStaticField;
        uint mOffset;
    };

    uint mClassNameOffset; // offset of constant pool(real class name --> namespace$class_name)
} sCLField;

typedef BOOL (*fNativeMethod)(MVALUE* stack, MVALUE* stack_ptr);

/// method flags ///
#define CL_NATIVE_METHOD 0x01
#define CL_STATIC_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04
#define CL_CONSTRUCTOR 0x08

typedef struct {
    uint mHeader;
    uint mNameOffset;     // offset of constant pool
    uint mPathOffset;     // offset of constant pool

    union {
        sByteCode mByteCodes;
        fNativeMethod mNativeMethod;
    };

    uint mResultType;     // offset of constant pool(real class name --> namespace$class_name)

    uint mNumParams;
    uint* mParamTypes;    // offset of constant pool(real class name --> namespace$class_name)

    uint mNumLocals;      // number of local variables

    uint mMaxStack;
} sCLMethod;

// limits:
#define CL_LOCAL_VARIABLE_MAX 64 // max number of local variables
#define CL_METHODS_MAX 128
#define CL_FIELDS_MAX 128
#define CL_NAMESPACE_NAME_MAX 32 // max length of namespace
#define CL_CLASS_NAME_MAX 32    // max length of class name
#define CL_REAL_CLASS_NAME_MAX (CL_NAMESPACE_NAME_MAX + CL_CLASS_NAME_MAX + 1)
#define CL_METHOD_NAME_MAX 32   // max length of method or field name
#define CL_METHOD_PARAM_MAX 16   // max number of param
#define CL_METHOD_NAME_REAL_MAX (CL_METHOD_NAME_MAX + 1 + (CL_CLASS_NAME_MAX + 1) * CL_METHOD_PARAM_MAX)
#define WORDSIZ 128

#define CLASS_HASH_SIZE 256

/// class flags ///
#define CLASS_FLAGS_INTERFACE 0x01
#define CLASS_FLAGS_MODIFIED 0x02

typedef struct sCLClassStruct {
    uint mFlags;
    sConst mConstPool;

    uchar mNameSpaceOffset;   // Offset of constant pool
    uchar mClassNameOffset;   // Offset of constant pool
    uchar mRealClassNameOffset;   // Offset of constant pool

    sCLField* mFields;
    uchar mNumFields;
    uchar mSizeFields;

    sCLMethod* mMethods;
    uchar mNumMethods;
    uchar mSizeMethods;

    void (*mFreeFun)(CLObject self);

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list
} sCLClass;

#define CLASS_NAME(klass) (char*)(klass->mConstPool.mConst + klass->mClassNameOffset + 1 + sizeof(int))
#define NAMESPACE_NAME(klass) (char*)(klass->mConstPool.mConst + klass->mNameSpaceOffset + 1 + sizeof(int))
#define REAL_CLASS_NAME(klass) (char*)(klass->mConstPool.mConst + klass->mRealClassNameOffset + 1 + sizeof(int))
#define METHOD_NAME(klass, n) (char*)(klass->mConstPool.mConst + klass->mMethods[n].mNameOffset + 1 + sizeof(int))
#define FIELD_NAME(klass, n) (char*)(klass->mConstPool.mConst + klass->mFields[n].mNameOffset + 1 + sizeof(int))
#define FIELD_CLASS_NAME(klass, n) (char*)(klass->mConstPool.mConst + klass->mFields[n].mClassNameOffset + 1 + sizeof(int))
#define METHOD_PATH(klass, n) (char*)(klass->mConstPool.mConst + klass->mMethods[n].mPathOffset + 1 + sizeof(int))

void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class);
void cl_final();

void cl_create_clc_file();
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, BOOL flg_main, int* err_num, int* max_stack, char* current_namespace);
BOOL cl_eval(char* cmdline, char* sname, int* sline);
BOOL cl_main(sByteCode* code, sConst* constant, uint lv_num, uint max_stack);
BOOL cl_excute_method(sByteCode* code, sConst* constant, uint lv_num, MVALUE* top_of_stack, uint max_stack);
void cl_gc();

void cl_editline_init();
void cl_editline_final();
ALLOC char* editline(char* prompt, char* rprompt);

sCLClass* cl_get_class(char* real_class_name);
    // result: (NULL) --> not found (non NULL) --> (sCLClass*)

sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name);
    // result: (NULL) --> not found (non NULL) --> (sCLClass*)

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name);

uint cl_get_method_index(sCLClass* klass, uchar* method_name);
    // result: (-1) --> not found (non -1) --> method index

#define NAMESPACE_NAME_MAX 32

typedef struct sCLNameSpaceStruct {
    char mName[NAMESPACE_NAME_MAX];
    sCLClass* mClassHashList[CLASS_HASH_SIZE];

    struct sCLNameSpaceStruct* mNextNameSpace;  // next namespace in hash table linked list
} sCLNameSpace;

#endif

