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
struct sBufStruct {
    char* mBuf;
    unsigned int mSize;
    unsigned int mLen;
};

typedef struct sBufStruct sBuf;

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
#define OP_NEW_STRING 25
#define OP_NEW_HASH 26
#define OP_LOGICAL_DENIAL 27
#define OP_COMPLEMENT 28
#define OP_ISUB 29
#define OP_FSUB 30
#define OP_IMULT 31
#define OP_FMULT 32
#define OP_IDIV 33
#define OP_FDIV 34
#define OP_IMOD 35
#define OP_ILSHIFT 36
#define OP_IRSHIFT 37
#define OP_IGTR 38
#define OP_FGTR 39
#define OP_IGTR_EQ 40
#define OP_FGTR_EQ 41
#define OP_ILESS 42
#define OP_FLESS 43
#define OP_ILESS_EQ 44
#define OP_FLESS_EQ 45
#define OP_IEQ 46
#define OP_FEQ 47
#define OP_INOTEQ 48
#define OP_FNOTEQ 49
#define OP_IAND 50
#define OP_IXOR 51
#define OP_IOR 52
#define OP_IANDAND 53
#define OP_IOROR 54
#define OP_FANDAND 55
#define OP_FOROR 56
#define OP_INC_VALUE 57
#define OP_DEC_VALUE 58
#define OP_IF 59
#define OP_BREAK 60
#define OP_JUMP 61
#define OP_GOTO 62

struct sByteCodeStruct {
    unsigned char* mCode;
    unsigned int mSize;
    unsigned int mLen;
};

typedef struct sByteCodeStruct sByteCode;

#define CONSTANT_INT 1
#define CONSTANT_FLOAT 2
#define CONSTANT_STRING 3
#define CONSTANT_WSTRING 4

#define CONS_type(constants, offset) *((constants).mConst + (offset))
#define CONS_str_len(constants, offset) *(int*)((constants).mConst + (offset) + 1)
#define CONS_str(constants, offset) (char*)((constants).mConst + (offset) + 1 + sizeof(int))
#define CONS_int(constants, offset) *(int*)((constants).mConst + (offset) + 1)
#define CONS_float(constants, offset) *(float*)((constants).mConst + (offset) + 1)

struct sConstStruct {
    char* mConst;
    unsigned int mSize;
    unsigned int mLen;
};

typedef struct sConstStruct sConst;

typedef unsigned long CLObject;

struct sCLClassStruct;

union MVALUE_UNION {
    char mCharValue;
    int mIntValue;
    float mFloatValue;
    long mLongValue;
    CLObject mObjectValue;
    struct sCLClassStruct* mClassRef;
};

typedef union MVALUE_UNION MVALUE;

// limits:
#define CL_LOCAL_VARIABLE_MAX 64 // max number of local variables
#define CL_METHODS_MAX 64
#define CL_BLOCKS_MAX 96
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
#define CL_ELSE_IF_MAX 32
#define CL_BREAK_MAX 32

#define WORDSIZ 128

#define CLASS_HASH_SIZE 256

struct sCLTypeStruct {
    unsigned int mClassNameOffset;                                  // real class name(offset of constant pool)
    char mGenericsTypesNum;
    unsigned int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];  // real class name(offset of constant pool)
};

typedef struct sCLTypeStruct sCLType;

/// field flags ///
#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02

struct sCLFieldStruct {
    unsigned int mFlags;
    unsigned int mNameOffset;   // offset of constant pool

    union {
        MVALUE mStaticField;
        unsigned int mOffset;
    } uValue;

    sCLType mType;
};

typedef struct sCLFieldStruct sCLField;

typedef BOOL (*fNativeMethod)(MVALUE** stack_ptr, MVALUE* lvar);

/// block ///
struct sCLBlockStruct {
    sByteCode mByteCodes;
    sConst mConstPool;

    unsigned int mMaxStack;
    unsigned int mNumParams;
    unsigned int mNumLocals;
    BOOL mResultExistance;
};

typedef struct sCLBlockStruct sCLBlock;

/// method flags ///
#define CL_NATIVE_METHOD 0x01
#define CL_CLASS_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04
#define CL_CONSTRUCTOR 0x08

struct sCLMethodStruct {
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
};

typedef struct sCLMethodStruct sCLMethod;

/// class flags ///
#define CLASS_FLAGS_INTERFACE 0x0100
#define CLASS_FLAGS_IMMEDIATE_VALUE_CLASS 0x0400
#define CLASS_FLAGS_PRIVATE 0x0800
#define CLASS_FLAGS_OPEN 0x1000
#define CLASS_FLAGS_MODIFIED 0x2000
#define CLASS_FLAGS_SPECIAL_CLASS 0x4000
#define CLASS_FLAGS_VERSION 0x00ff
#define CLASS_VERSION(klass) (klass->mFlags & CLASS_FLAGS_VERSION)
#define CLASS_VERSION_MAX 255

#define SUPER_CLASS_MAX 8

#define NUM_DEFINITION_MAX 128

struct sVMethodMapStruct {
    char mSuperClassIndex;
    char mMethodIndex;
};

typedef struct sVMethodMapStruct sVMethodMap;

struct sCLClassStruct {
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
    void (*mShowFun)(CLObject self);
    void (*mMarkFun)(CLObject self, unsigned char* mark_flg);

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list

    char mGenericsTypesNum;
    unsigned int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];            // class type variable offset

sVMethodMap* mVirtualMethodMap;
char mNumVMethodMap;
char mSizeVMethodMap;
};

typedef struct sCLClassStruct sCLClass;

struct sCLNodeTypeStruct {
    sCLClass* mClass;
    char mGenericsTypesNum;
    sCLClass* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
};

typedef struct sCLNodeTypeStruct sCLNodeType;

#define CLASS_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mClassNameOffset + 1 + sizeof(int))
#define NAMESPACE_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mNameSpaceOffset + 1 + sizeof(int))
#define REAL_CLASS_NAME(klass) (char*)((klass)->mConstPool.mConst + (klass)->mRealClassNameOffset + 1 + sizeof(int))
#define FIELD_NAME(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mFields[(n)].mNameOffset + 1 + sizeof(int))
#define METHOD_NAME(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mNameOffset + 1 + sizeof(int))
#define METHOD_NAME2(klass, method) (char*)((klass)->mConstPool.mConst + (method)->mNameOffset + 1 + sizeof(int))
#define METHOD_PATH(klass, n) (char*)((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mPathOffset + 1 + sizeof(int))

struct sVarStruct {
    char mName[CL_METHOD_NAME_MAX];
    int mIndex;
    sCLNodeType mType;
};

typedef struct sVarStruct sVar;

struct sVarTableStruct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    int mVarNum;
    int mBlockVarNum;
};

typedef struct sVarTableStruct sVarTable;

#define NAMESPACE_NAME_MAX 32

struct sCLNameSpaceStruct {
    char mName[NAMESPACE_NAME_MAX];
    sCLClass* mClassHashList[CLASS_HASH_SIZE];

    struct sCLNameSpaceStruct* mNextNameSpace;  // next namespace in hash table linked list
};

typedef struct sCLNameSpaceStruct sCLNameSpace;

#define DUMMY_ARRAY_SIZE 32

/// object header ///
struct sCLObjectHeaderStruct {
    char mExistence;                      // for gabage collection
    int mHeapMemSize;
    struct sCLClassStruct* mClass;
};

typedef struct sCLObjectHeaderStruct sCLObjectHeader;

#define CLOBJECT_HEADER(obj) ((sCLObjectHeader*)object_to_ptr((obj)))

struct sCLObjectStruct {
    sCLObjectHeader mHeader;
    MVALUE mFields[DUMMY_ARRAY_SIZE];
};

typedef struct sCLObjectStruct sCLObject;

#define CLOBJECT(obj) ((sCLObject*)object_to_ptr((obj)))

struct sCLArrayStruct {
    sCLObjectHeader mHeader;

    CLObject mItems;
    int mSize;
    int mLen;
};

typedef struct sCLArrayStruct sCLArray;

#define CLARRAY(obj) ((sCLArray*)object_to_ptr((obj)))

struct sCLArrayItemsStruct {
    sCLObjectHeader mHeader;
    MVALUE mData[DUMMY_ARRAY_SIZE];
};

typedef struct sCLArrayItemsStruct sCLArrayItems;

#define CLARRAYITEMS(obj) ((sCLArrayItems*)object_to_ptr((obj)))

#define CLARRAY_ITEMS(obj, i) ((CLARRAYITEMS(CLARRAY((obj))->mItems)->mData)[i])

struct sCLStringStruct {
    sCLObjectHeader mHeader;
    int mLen;
    wchar_t mChars[DUMMY_ARRAY_SIZE];
};

typedef struct sCLStringStruct sCLString;

#define CLSTRING(obj) ((sCLString*)object_to_ptr((obj)))

struct sCLHashStruct {
    sCLObjectHeader mHeader;
};

typedef struct sCLHashStruct sCLHash;

/// clover functions ///
void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class);
void cl_final();

BOOL cl_eval_file(char* file_name);

void cl_create_clc_file();
BOOL cl_eval(char* cmdline, char* sname, int* sline);
BOOL cl_main(sByteCode* code, sConst* constant, unsigned int lv_num, unsigned int max_stack);
BOOL cl_excute_method(sCLMethod* method, sConst* constant, BOOL result_existance, sCLNodeType* generics_type);
void cl_gc();
void cl_print(char* msg, ...);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_);

sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name);
    // result: (NULL) --> not found (non NULL) --> (sCLClass*)

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name);

unsigned int cl_get_method_index(sCLClass* klass, unsigned char* method_name);
    // result: (-1) --> not found (non -1) --> method index

#endif

