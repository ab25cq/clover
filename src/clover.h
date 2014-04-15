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
    int mSize;
    int mLen;
};

typedef struct sBufStruct sBuf;

/// virtual machine data ///
#define OP_IADD 1
#define OP_LDCINT 2
#define OP_LDCFLOAT 3
#define OP_LDCWSTR 4
#define OP_ASTORE 5
#define OP_ISTORE 6
#define OP_FSTORE 7
#define OP_OSTORE 8
#define OP_ALOAD 9
#define OP_ILOAD 10
#define OP_FLOAD 11
#define OP_OLOAD 12
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
#define OP_NOTIF 60
#define OP_GOTO 61
#define OP_NEW_BLOCK 62
#define OP_INVOKE_BLOCK 63
#define OP_POP 64
#define OP_POP_N 65
#define OP_SEQ 66
#define OP_SNOTEQ 67
#define OP_REVERT 68

struct sByteCodeStruct {
    int* mCode;
    int mSize;
    int mLen;
};

typedef struct sByteCodeStruct sByteCode;

#define CLASS_NAME(klass) ((klass)->mConstPool.mConst + (klass)->mClassNameOffset)
#define NAMESPACE_NAME(klass) ((klass)->mConstPool.mConst + (klass)->mNameSpaceOffset)
#define REAL_CLASS_NAME(klass) ((klass)->mConstPool.mConst + (klass)->mRealClassNameOffset)
#define FIELD_NAME(klass, n) ((klass)->mConstPool.mConst + (klass)->mFields[(n)].mNameOffset)
#define METHOD_NAME(klass, n) ((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mNameOffset)
#define METHOD_NAME2(klass, method) ((klass)->mConstPool.mConst + (method)->mNameOffset)
#define METHOD_PATH(klass, n) ((klass)->mConstPool.mConst + (klass)->mMethods[(n)].mPathOffset)

struct sConstStruct {
    char* mConst;
    int mSize;
    int mLen;
};

typedef struct sConstStruct sConst;

#define CONS_str(constant, offset) (char*)((constant)->mConst + offset)

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
#define CL_LOCAL_VARIABLE_MAX 64                    // max number of local variables
#define CL_METHODS_MAX 0xefffffff
#define CL_BLOCKS_MAX 96
#define CL_FIELDS_MAX 0xefffffff
#define CL_METHOD_PARAM_MAX 16                      // max number of param
#define CL_ARRAY_ELEMENTS_MAX 32                    // max number of array elements(constant array value)
#define CL_GENERICS_CLASS_PARAM_MAX 8               // max number of generics class param
#define CL_BLOCK_NEST_MAX 50

#define CL_NAMESPACE_NAME_MAX 32 // max length of namespace
#define CL_CLASS_NAME_MAX 32    // max length of class name
#define CL_VARIABLE_NAME_MAX 32   
#define CL_REAL_CLASS_NAME_MAX (CL_NAMESPACE_NAME_MAX + CL_CLASS_NAME_MAX + 1)
#define CL_METHOD_NAME_MAX 32   // max length of method or field name
#define CL_METHOD_NAME_WITH_PARAMS_MAX (CL_METHOD_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX)
#define CL_CLASS_TYPE_VARIABLE_MAX CL_CLASS_NAME_MAX
#define CL_ELSE_IF_MAX 32
#define CL_BREAK_MAX 32
#define METHOD_BLOCK_NEST_MAX 0x00ff

#define WORDSIZ 128

#define CLASS_HASH_SIZE 256

struct sCLTypeStruct {
    int mClassNameOffset;                                  // real class name(offset of constant pool)
    char mGenericsTypesNum;
    int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];  // real class name(offset of constant pool)
};

typedef struct sCLTypeStruct sCLType;

/// field flags ///
#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02

struct sCLFieldStruct {
    int mFlags;
    int mNameOffset;   // offset of constant pool

    union {
        MVALUE mStaticField;
        int mOffset;
    } uValue;

    sByteCode mInitializar;
    int mInitializarLVNum;
    int mInitializarMaxStack;

    sCLType mType;
};

typedef struct sCLFieldStruct sCLField;

typedef BOOL (*fNativeMethod)(MVALUE** stack_ptr, MVALUE* lvar);

struct sVarTableStruct;

/// block type ///
struct sCLBlockTypeStruct {
    sCLType mResultType;

    int mNameOffset;

    int mNumParams;
    sCLType* mParamTypes;
};

typedef struct sCLBlockTypeStruct sCLBlockType;

/// method flags ///
#define CL_NATIVE_METHOD 0x01
#define CL_CLASS_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04
#define CL_CONSTRUCTOR 0x08
#define CL_EXTERNAL_METHOD 0x10

struct sCLMethodStruct {
    int mFlags;
    int mNameOffset;     // offset of constant pool
    int mPathOffset;     // offset of constant pool

    union {
        sByteCode mByteCodes;
        fNativeMethod mNativeMethod;
    } uCode;

    sCLType mResultType;     // offset of constant pool(real class name --> namespace$class_name)

    int mNumParams;
    sCLType* mParamTypes;

    int mNumBlockType;
    sCLBlockType mBlockType;

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
    int mFlags;
    sConst mConstPool;

    char mNameSpaceOffset;   // Offset of constant pool
    char mClassNameOffset;   // Offset of constant pool
    char mRealClassNameOffset;   // Offset of constant pool

    sCLField* mFields;
    int mNumFields;
    int mSizeFields;

    sCLMethod* mMethods;
    int mNumMethods;
    int mSizeMethods;

    int mSuperClassesOffset[SUPER_CLASS_MAX];
    char mNumSuperClasses;

    void (*mFreeFun)(CLObject self);
    void (*mShowFun)(CLObject self);
    void (*mMarkFun)(CLObject self, unsigned char* mark_flg);

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list

    char mGenericsTypesNum;
    int mGenericsTypesOffset[CL_GENERICS_CLASS_PARAM_MAX];            // class type variable offset

sVMethodMap* mVirtualMethodMap;
int mNumVMethodMap;
int mSizeVMethodMap;
};

typedef struct sCLClassStruct sCLClass;

struct sCLNodeTypeStruct {
    sCLClass* mClass;
    char mGenericsTypesNum;
    sCLClass* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
};

typedef struct sCLNodeTypeStruct sCLNodeType;

#define NAMESPACE_NAME_MAX 32

struct sVarStruct {
    char mName[CL_METHOD_NAME_MAX];
    int mIndex;
    sCLNodeType mType;

    int mBlockLevel;
};

typedef struct sVarStruct sVar;

struct sVarTableStruct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    int mVarNum;
    int mBlockVarNum;

    int mBlockLevel;
};

typedef struct sVarTableStruct sVarTable;

struct sCLNameSpaceStruct {
    char mName[NAMESPACE_NAME_MAX];
    sCLClass* mClassHashList[CLASS_HASH_SIZE];

    struct sCLNameSpaceStruct* mNextNameSpace;  // next namespace in hash table linked list
};

typedef struct sCLNameSpaceStruct sCLNameSpace;

#define DUMMY_ARRAY_SIZE 32

/// object header ///
struct sCLObjectHeaderStruct {
    int mExistence;                      // for gabage collection
    int mHeapMemSize;
    struct sCLClassStruct* mClass;
};

typedef struct sCLObjectHeaderStruct sCLObjectHeader;

#define CLOBJECT_HEADER(obj) ((sCLObjectHeader*)object_to_ptr((obj)))

struct sCLUserObjectStruct {
    sCLObjectHeader mHeader;
    MVALUE mFields[DUMMY_ARRAY_SIZE];
};

typedef struct sCLUserObjectStruct sCLUserObject;

#define CLUSEROBJECT(obj) ((sCLUserObject*)object_to_ptr((obj)))

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

struct sCLBlockStruct {
    sCLObjectHeader mHeader;

    sByteCode* mCode;
    sConst* mConstant;

    BOOL mStaticMethodBlock;
    int mMaxStack;
    int mNumLocals;
    int mNumParams;

    MVALUE* mLocalVar;
    int mNumVars;
};

typedef struct sCLBlockStruct sCLBlock;

#define CLBLOCK(obj) ((sCLBlock*)object_to_ptr((obj)))

struct sCLExceptionStruct {
    sCLObjectHeader mHeader;

    CLObject mMessage;
};

typedef struct sCLExceptionStruct sCLException;

#define CLEXCEPTION(obj) ((sCLEXceptin*)object_to_ptr((obj)))

/// clover functions ///

// result: (TRUE) success (FALSE) failed. should exit from process
BOOL cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class);
void cl_final();

BOOL cl_eval_file(char* file_name);

void cl_create_clc_file();
BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack);
BOOL cl_excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params);
BOOL cl_excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, BOOL static_method_block);

void cl_gc();

int cl_print(char* msg, ...);
void cl_puts(char* str);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name, BOOL auto_load);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_, BOOL auto_load);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name, BOOL auto_load);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name, BOOL auto_load);

int cl_get_method_index(sCLClass* klass, char* method_name);
    // result: (-1) --> not found (non -1) --> method index

BOOL run_fields_initializar(CLObject object, sCLClass* klass);

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_class_field(sCLClass* klass, char* field_name, sCLClass* field_class, MVALUE* result);

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_array_element(CLObject array, int index, sCLClass* element_class, MVALUE* result);

// result: (FALSE) there are errors (TRUE) success
// when result is success, output and err_output is allocaled as string
// if there are compile errors, a flag named compile_error is setted on TRUE
BOOL cl_compile(char** files, int num_files, BOOL* compile_error, ALLOC char** output, ALLOC char** err_output);

#endif

