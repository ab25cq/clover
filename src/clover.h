#ifndef CLOVER_H
#define CLOVER_H

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <oniguruma.h>

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
#define OP_INVOKE_VIRTUAL_METHOD 15
#define OP_RETURN 18
#define OP_NEW_OBJECT 19
#define OP_LDFIELD 20
#define OP_LD_STATIC_FIELD 21
#define OP_SRFIELD 22
#define OP_SR_STATIC_FIELD 23
#define OP_NEW_ARRAY 24
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
#define OP_BLANDAND 53
#define OP_BLOROR 54
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
#define OP_THROW 69
#define OP_TRY 70
#define OP_LDTYPE 71

#define OP_BADD 73
#define OP_BSUB 74
#define OP_BMULT 75
#define OP_BDIV 76
#define OP_BMOD 77
#define OP_BLSHIFT 78
#define OP_BRSHIFT 79

#define OP_BAND 80
#define OP_BXOR 81
#define OP_BOR 82
#define OP_BGTR 83
#define OP_BGTR_EQ 84
#define OP_BLESS 85
#define OP_BLESS_EQ 86
#define OP_BEQ 87
#define OP_BNOTEQ 88
#define OP_BCOMPLEMENT 89

#define OP_LDCSTR 90
#define OP_BSEQ 91
#define OP_BSNOTEQ 92
#define OP_BSADD 93
#define OP_BSMULT 94
#define OP_SMULT 95

#define OP_INVOKE_METHOD 96
#define OP_LDCBOOL 98
#define OP_BLEQ 99
#define OP_BLNOTEQ 100
#define OP_LDCBYTE 101
#define OP_LDCNULL 102
#define OP_POP_N_WITHOUT_TOP 103
#define OP_CALL_PARAM_INITIALIZER 104
#define OP_NEW_RANGE 105
#define OP_INVOKE_VIRTUAL_CLONE_METHOD 106
#define OP_FOLD_PARAMS_TO_ARRAY 107
#define OP_NEW_TUPLE 108
#define OP_NEW_REGEX 109
#define OP_DUP 110
#define OP_SWAP 111

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
#define METHOD_PATH2(klass, method) ((klass)->mConstPool.mConst + (method)->mPathOffset)

// limits:
#define CL_LOCAL_VARIABLE_MAX 64                    // max number of local variables
#define CL_METHODS_MAX 128
#define CL_BLOCKS_MAX 96
#define CL_FIELDS_MAX 0xefffffff
#define CL_METHOD_PARAM_MAX 16                      // max number of param
#define CL_ARRAY_ELEMENTS_MAX 32                    // max number of array elements(constant array value)
#define CL_TUPLE_ELEMENTS_MAX CL_GENERICS_CLASS_PARAM_MAX // max number of tuple elements(constant tuple value)
#define CL_GENERICS_CLASS_PARAM_MAX 8              // max number of generics class param
#define CL_BLOCK_NEST_MAX 50
#define SCRIPT_STATMENT_MAX 1024

#define CL_NAMESPACE_NAME_MAX 32 // max length of namespace
#define CL_CLASS_NAME_MAX 32    // max length of class name
#define CL_VARIABLE_NAME_MAX 32   
#define CL_REAL_CLASS_NAME_MAX (CL_NAMESPACE_NAME_MAX + CL_CLASS_NAME_MAX + 16 + 1)
#define CL_METHOD_NAME_MAX 64   // max length of method or field name
#define CL_METHOD_NAME_WITH_PARAMS_MAX (CL_METHOD_NAME_MAX + CL_REAL_CLASS_NAME_MAX * CL_METHOD_PARAM_MAX)
#define CL_CLASS_TYPE_VARIABLE_MAX CL_CLASS_NAME_MAX
#define CL_ELSE_IF_MAX 32
#define CL_BREAK_MAX 32
#define METHOD_BLOCK_NEST_MAX 0x00ff
#define CL_METHOD_EXCEPTION_MAX 8
#define CL_ALIAS_MAX 4096

#define CL_CATCH_BLOCK_NUMBER_MAX 32

#define CL_FIELD_INITIALIZER_STACK_SIZE 255
#define CL_PARAM_INITIALIZER_STACK_SIZE 255

#define WORDSIZ 256

#define CLASS_HASH_SIZE 512

#define CL_MODULE_HASH_SIZE 256

#define CL_STACK_SIZE 1024

#define CL_GENERICS_CLASS_DEPTH_MAX 7

#define CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX 8

#define CL_MODULE_PARAM_MAX 16

#define CL_ENUM_NUM_MAX 128

#define REGEX_LENGTH_MAX 1024
#define MULTIPLE_ASSIGNMENT_NUM_MAX 16

struct sConstStruct {
    char* mConst;
    int mSize;
    int mLen;
};

typedef struct sConstStruct sConst;

#define CONS_str(constant, offset) (char*)((constant)->mConst + offset)

typedef unsigned int CLObject;

struct sCLClassStruct;
typedef struct sCLClassStruct sCLClass;

union MVALUE_UNION {
    struct {
        CLObject mValue;
    } mObjectValue;
};

typedef union MVALUE_UNION MVALUE;

struct sVMType {
    sCLClass* generics_param_types[CL_GENERICS_CLASS_PARAM_MAX];
    int num_generics_param_types;

    struct sVMType* parent;
};

#define CL_VM_TYPES_MAX 2048

struct sVMInfoStruct {
    MVALUE* stack;
    MVALUE* stack_ptr;
    int stack_size;
    CLObject vm_types[CL_VM_TYPES_MAX];
    int num_vm_types;
    CLObject vm_type_context;
#ifdef VM_DEBUG
    FILE* debug_log;
#endif
    CLObject thread_obj;
    CLObject thread_block_obj;

    char mRunningClassName[CL_CLASS_NAME_MAX+1];
    char mRunningMethodName[CL_METHOD_NAME_MAX+1];

    struct sVMInfoStruct* next_info;

    sBuf* print_buffer;
};

typedef struct sVMInfoStruct sVMInfo;

struct sCLTypeStruct {
    int mClassNameOffset;                                  // real class name(offset of constant pool)

    BOOL mStar;

    char mGenericsTypesNum;
    struct sCLTypeStruct* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX]; // real class name(offset of constant pool)

    struct sCLTypeStruct* mNext;
};

typedef struct sCLTypeStruct sCLType;

/// field flags ///
#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02
#define CL_PROTECTED_FIELD 0x04

struct sCLFieldStruct {
    int mFlags;
    int mNameOffset;   // offset of constant pool

    union {
        MVALUE mStaticField;
        int mOffset;
    } uValue;

    sByteCode mInitializer;
    int mInitializerLVNum;
    int mInitializerMaxStack;

    sCLType mType;

    int mFieldIndex;
};

typedef struct sCLFieldStruct sCLField;

typedef BOOL (*fNativeMethod)(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

struct sVarTableStruct;

/// block type ///
struct sCLBlockTypeStruct {
    sCLType mResultType;

    int mNameOffset;

    int mNumParams;
    sCLType* mParamTypes;
};

typedef struct sCLBlockTypeStruct sCLBlockType;

struct sCLParamInitializerStruct {
    sByteCode mInitializer;
    int mMaxStack;
    int mLVNum;
};

typedef struct sCLParamInitializerStruct sCLParamInitializer;

struct sCLGenericsParamTypesStruct {
    int mNameOffset;

    sCLType mExtendsType;

    char mNumImplementsTypes;
    sCLType mImplementsTypes[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];
};

typedef struct sCLGenericsParamTypesStruct sCLGenericsParamTypes;

/// method flags ///
#define CL_NATIVE_METHOD 0x01
#define CL_CLASS_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04
#define CL_CONSTRUCTOR 0x08
#define CL_SYNCHRONIZED_METHOD 0x10
#define CL_ALIAS_METHOD 0x20
#define CL_VIRTUAL_METHOD 0x40
#define CL_ABSTRACT_METHOD 0x80
#define CL_GENERICS_NEWABLE_CONSTRUCTOR 0x100
#define CL_PROTECTED_METHOD 0x200
#define CL_METHOD_PARAM_VARABILE_ARGUMENTS 0x400

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
    sCLParamInitializer* mParamInitializers;

    int mNumBlockType;
    sCLBlockType mBlockType;

    int mExceptionClassNameOffset[CL_METHOD_EXCEPTION_MAX];   // real class name(offset of constant pool)
    int mNumException;

    int mNumLocals;      // number of local variables

    int mMaxStack;

    char mNumParamInitializer;

    char mGenericsTypesNum;
    sCLGenericsParamTypes mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
};

typedef struct sCLMethodStruct sCLMethod;

/// class flags ///
#define CLASS_FLAGS_INTERFACE 0x1000000
#define CLASS_FLAGS_PRIVATE 0x2000000
#define CLASS_FLAGS_ABSTRACT 0x4000000
#define CLASS_FLAGS_MODIFIED 0x10000000
#define CLASS_FLAGS_SPECIAL_CLASS 0x20000000
#define CLASS_FLAGS_DYNAMIC_TYPING 0x40000000
#define CLASS_FLAGS_FINAL 0x80000000
#define CLASS_FLAGS_STRUCT 0x100000000
#define CLASS_FLAGS_ENUM 0x200000000
#define CLASS_FLAGS_VERSION 0x00ff
#define CLASS_VERSION(klass) ((klass)->mFlags & CLASS_FLAGS_VERSION)
#define CLASS_VERSION_MAX 255
#define CLASS_FLAGS_KIND 0xff00
#define CLASS_KIND(klass) ((klass)->mFlags & CLASS_FLAGS_KIND)
#define CLASS_FLAGS_BASE_KIND 0xff0000
#define CLASS_BASE_KIND(klass) ((klass)->mFlags & CLASS_FLAGS_BASE_KIND)

#define CLASS_KIND_VOID 0x0100
#define CLASS_KIND_INT 0x0200
#define CLASS_KIND_BYTE 0x0300
#define CLASS_KIND_FLOAT 0x0400
#define CLASS_KIND_BOOL 0x0500
#define CLASS_KIND_NULL 0x0600
#define CLASS_KIND_OBJECT 0x0700
#define CLASS_KIND_ARRAY 0x0800
#define CLASS_KIND_BYTES 0x0900
#define CLASS_KIND_HASH 0x0a00
#define CLASS_KIND_BLOCK 0x0b00
#define CLASS_KIND_CLASSNAME 0x0c00
#define CLASS_KIND_STRING 0x0d00
#define CLASS_KIND_THREAD 0x0e00
#define CLASS_KIND_MUTEX 0x0f00
#define CLASS_KIND_FILE 0x1000
#define CLASS_KIND_REGULAR_FILE 0x1100
#define CLASS_KIND_GENERICS_PARAM 0x1200
#define CLASS_KIND_ANONYMOUS 0x1300
#define CLASS_KIND_TYPE 0x1400
#define CLASS_KIND_RANGE 0x1500
#define CLASS_KIND_ONIGURUMA_REGEX 0x1600
#define CLASS_KIND_ENUM 0x1700
#define CLASS_KIND_TUPLE 0x1800
#define CLASS_KIND_CLASS 0x1900
#define CLASS_KIND_FIELD 0x1a00
#define CLASS_KIND_METHOD 0x1b00

#define CLASS_KIND_EXCEPTION 0x5000
#define CLASS_KIND_NULL_POINTER_EXCEPTION 0x5100
#define CLASS_KIND_RANGE_EXCEPTION 0x5200
#define CLASS_KIND_CONVERTING_STRING_CODE_EXCEPTION 0x5300
#define CLASS_KIND_CLASS_NOT_FOUND_EXCEPTION 0x5400
#define CLASS_KIND_IO_EXCEPTION 0x5500
#define CLASS_KIND_OVERFLOW_EXCEPTION 0x5600
#define CLASS_KIND_CANT_SOLVE_GENERICS_TYPE 0x5700
#define CLASS_KIND_TYPE_ERROR 0x5800
#define CLASS_KIND_METHOD_MISSING_EXCEPTION 0x5900
#define CLASS_KIND_DIVISION_BY_ZERO 0x5A00
#define CLASS_KIND_OVERFLOW_STACK_SIZE 0x5B00
#define CLASS_KIND_INVALID_REGEX_EXCEPTION 0x5C00

#define CLASS_KIND_BASE_VOID 0x10000
#define CLASS_KIND_BASE_NULL 0x20000
#define CLASS_KIND_BASE_INT 0x30000
#define CLASS_KIND_BASE_BYTE 0x40000
#define CLASS_KIND_BASE_FLOAT 0x50000
#define CLASS_KIND_BASE_BOOL 0x60000
#define CLASS_KIND_BASE_OBJECT 0x70000
#define CLASS_KIND_BASE_ARRAY 0x80000
#define CLASS_KIND_BASE_BYTES 0x90000
#define CLASS_KIND_BASE_HASH 0xA0000
#define CLASS_KIND_BASE_BLOCK 0xB0000
#define CLASS_KIND_BASE_STRING 0xC0000
#define CLASS_KIND_BASE_THREAD 0xD0000
#define CLASS_KIND_BASE_TYPE 0xE0000
#define CLASS_KIND_BASE_EXCEPTION 0xF0000
#define CLASS_KIND_BASE_NULL_POINTER_EXCEPTION 0x100000
#define CLASS_KIND_BASE_RANGE_EXCEPTION 0x110000
#define CLASS_KIND_BASE_CONVERTING_STRING_CODE_EXCEPTION 0x120000
#define CLASS_KIND_BASE_CLASS_NOT_FOUND_EXCEPTION 0x130000
#define CLASS_KIND_BASE_IO_EXCEPTION 0x140000
#define CLASS_KIND_BASE_OVERFLOW_EXCEPTION 0x150000
#define CLASS_KIND_BASE_METHOD_MISSING_EXCEPTION 0x160000
#define CLASS_KIND_BASE_CANT_SOLVE_GENERICS_TYPE 0x170000
#define CLASS_KIND_BASE_TYPE_ERROR 0x180000
#define CLASS_KIND_BASE_OVERFLOW_STACK_SIZE 0x190000
#define CLASS_KIND_BASE_DIVISION_BY_ZERO 0x1A0000
#define CLASS_KIND_BASE_GENERICS_PARAM 0x1B0000
#define CLASS_KIND_BASE_ANONYMOUS 0x1C0000
#define CLASS_KIND_BASE_RANGE 0x1D0000
#define CLASS_KIND_BASE_INVALID_REGEX_EXCEPTION 0x1E0000
#define CLASS_KIND_BASE_ONIGURUMA_REGEX 0x1F0000
#define CLASS_KIND_BASE_ENCODING 0x200000
#define CLASS_KIND_BASE_ENUM 0x210000
#define CLASS_KIND_BASE_TUPLE 0x220000
#define CLASS_KIND_BASE_CLASS 0x230000
#define CLASS_KIND_BASE_FIELD 0x240000
#define CLASS_KIND_BASE_METHOD 0x250000

#define SUPER_CLASS_MAX 8
#define IMPLEMENTED_INTERFACE_MAX 32

#define NUM_DEFINITION_MAX 128

#define CL_VMT_NAME_MAX (CL_METHOD_NAME_MAX + 2)

struct sVMethodMapStruct {
    char mMethodName[CL_VMT_NAME_MAX];
    char mMethodIndex;
};

typedef struct sVMethodMapStruct sVMethodMap;

typedef CLObject (*fCreateFun)(CLObject type_object, sVMInfo*);
typedef void (*fMarkFun)(CLObject self, unsigned char* mark_flg);
typedef void (*fFreeFun)(CLObject self);
typedef void (*fShowFun)(sVMInfo* info, CLObject self);

struct sCLClassStruct {
    long long mFlags;
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

    sCLType mSuperClasses[SUPER_CLASS_MAX];
    char mNumSuperClasses;

    sCLType mImplementedInterfaces[IMPLEMENTED_INTERFACE_MAX];
    char mNumImplementedInterfaces;

    int* mDependencesOffset;
    int mNumDependences;
    int mSizeDependences;

    fFreeFun mFreeFun;
    fShowFun mShowFun;
    fMarkFun mMarkFun;
    fCreateFun mCreateFun;

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list

    char mGenericsTypesNum;
    sCLGenericsParamTypes mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];

    sVMethodMap* mVirtualMethodMap;
    int mNumVirtualMethodMap;
    int mSizeVirtualMethodMap;

    int mCloneMethodIndex;
};

#define CL_MODULE_NAME_MAX (CL_CLASS_NAME_MAX+CL_NAMESPACE_NAME_MAX+2)

struct sCLModuleStruct {
    BOOL mModified;
    char mName[CL_MODULE_NAME_MAX+1];
    sBuf mBody;
};

typedef struct sCLModuleStruct sCLModule;

struct sCLNodeTypeStruct {
    sCLClass* mClass;
    char mGenericsTypesNum;
    BOOL mStar;
    struct sCLNodeTypeStruct* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
};

typedef struct sCLNodeTypeStruct sCLNodeType;

#define NAMESPACE_NAME_MAX 32

struct sVarStruct {
    char mName[CL_METHOD_NAME_MAX];
    int mIndex;
    sCLNodeType* mType;

    int mBlockLevel;
};

typedef struct sVarStruct sVar;

struct sVarTableStruct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    int mVarNum;
    int mMaxBlockVarNum;

    int mBlockLevel;

    struct sVarTableStruct* mParent;            // make linked list
    struct sVarTableStruct* mNext;              // for free var table
};

typedef struct sVarTableStruct sVarTable;

struct sCLNameSpaceStruct {
    char mName[NAMESPACE_NAME_MAX];
    sCLClass* mClassHashList[CLASS_HASH_SIZE];

    struct sCLNameSpaceStruct* mNextNameSpace;  // next namespace in hash table linked list
};

typedef struct sCLNameSpaceStruct sCLNameSpace;

/// object header ///
struct sCLObjectHeaderStruct {
    int mExistence;                      // for gabage collection
    int mHeapMemSize;
    CLObject mType;
    sCLClass* mClass;
};

typedef struct sCLObjectHeaderStruct sCLObjectHeader;

#define CLOBJECT_HEADER(obj) ((sCLObjectHeader*)object_to_ptr((obj)))

struct sCLBoolStruct {
    sCLObjectHeader mHeader;
    int mValue;
};

typedef struct sCLBoolStruct sCLBool;

#define CLBOOL(obj) ((sCLBool*)object_to_ptr((obj)))

struct sCLNullStruct {
    sCLObjectHeader mHeader;
    int mValue;
};

typedef struct sCLNullStruct sCLNull;

#define CLNULL(obj) ((sCLNull*)object_to_ptr((obj)))

struct sCLIntStruct {
    sCLObjectHeader mHeader;
    int mValue;
};

typedef struct sCLIntStruct sCLInt;

#define CLINT(obj) ((sCLInt*)object_to_ptr((obj)))

struct sCLFloatStruct {
    sCLObjectHeader mHeader;
    float mValue;
};

typedef struct sCLFloatStruct sCLFloat;

#define CLFLOAT(obj) ((sCLFloat*)object_to_ptr((obj)))

struct sCLByteStruct {
    sCLObjectHeader mHeader;
    unsigned char mValue;
};

typedef struct sCLByteStruct sCLByte;

#define CLBYTE(obj) ((sCLByte*)object_to_ptr((obj)))

#define DUMMY_ARRAY_SIZE 32

struct sCLUserObjectStruct {
    sCLObjectHeader mHeader;
    MVALUE mFields[DUMMY_ARRAY_SIZE];
};

typedef struct sCLUserObjectStruct sCLUserObject;

#define CLUSEROBJECT(obj) ((sCLUserObject*)object_to_ptr((obj)))

struct sCLArrayItemsStruct {
    sCLObjectHeader mHeader;
    MVALUE mItems[DUMMY_ARRAY_SIZE];
};

typedef struct sCLArrayItemsStruct sCLArrayItems;

#define CLARRAY_DATA(obj) (((sCLArrayItems*)object_to_ptr((obj))))

struct sCLArrayStruct {
    sCLObjectHeader mHeader;

    CLObject mData;
    int mSize;
    int mLen;
};

typedef struct sCLArrayStruct sCLArray;

#define CLARRAY(obj) ((sCLArray*)object_to_ptr((obj)))
#define CLARRAY_ITEMS(obj) (CLARRAY_DATA(CLARRAY((obj))->mData))
#define CLARRAY_ITEMS2(obj, i) (CLARRAY_ITEMS((obj))->mItems[(i)])

struct sCLOnigurumaRegexStruct {
    sCLObjectHeader mHeader;

    regex_t* mRegex;

    OnigUChar* mSource;
    BOOL mIgnoreCase;
    BOOL mMultiLine;
    BOOL mGlobal;
    CLObject mEncoding;
};

typedef struct sCLOnigurumaRegexStruct sCLOnigurumaRegex;

#define CLONIGURUMAREGEX(obj) ((sCLOnigurumaRegex*)object_to_ptr(obj))

struct sCLStringDataStruct {
    sCLObjectHeader mHeader;

    wchar_t mChars[DUMMY_ARRAY_SIZE];
};

typedef struct sCLStringDataStruct sCLStringData;

#define CLSTRING_DATA(obj) ((sCLStringData*)object_to_ptr(CLSTRING((obj))->mData))

struct sCLStringStruct {
    sCLObjectHeader mHeader;
    int mLen;
    CLObject mData;
};

typedef struct sCLStringStruct sCLString;

#define CLSTRING(obj) ((sCLString*)object_to_ptr((obj)))

struct sCLHashDataItemStruct {
    unsigned int mHashValue;
    CLObject mKey;
    CLObject mItem;
};

typedef struct sCLHashDataItemStruct sCLHashDataItem;

struct sCLHashDataStruct {
    sCLObjectHeader mHeader;
    sCLHashDataItem mItems[DUMMY_ARRAY_SIZE];
};

typedef struct sCLHashDataStruct sCLHashData;

#define CLHASH_DATA(obj) ((sCLHashData*)object_to_ptr((obj)))

struct sCLHashStruct {
    sCLObjectHeader mHeader;

    CLObject mData;
    int mSize;
    int mLen;
};

typedef struct sCLHashStruct sCLHash;

#define CLHASH(obj) ((sCLHash*)object_to_ptr((obj)))

#define CLHASH_ITEM(obj, index) (CLHASH_DATA(CLHASH(obj)->mData)->mItems[(index)].mItem)
#define CLHASH_KEY(obj, index) (CLHASH_DATA(CLHASH(obj)->mData)->mItems[(index)].mKey)

struct sCLRangeStruct {
    sCLObjectHeader mHeader;
    int mHead;
    int mTail;
};

typedef struct sCLRangeStruct sCLRange;

#define CLRANGE(obj) ((sCLRange*)object_to_ptr((obj)))

struct sCLBlockStruct {
    sCLObjectHeader mHeader;

    sByteCode* mCode;
    sConst* mConstant;

    BOOL mStaticMethodBlock;
    int mMaxStack;
    int mNumLocals;
    int mNumParams;

    MVALUE* mParentLocalVar;
    int mNumParentVar;

    CLObject mResultType;

    CLObject mParams[CL_METHOD_PARAM_MAX];
};

typedef struct sCLBlockStruct sCLBlock;

#define CLBLOCK(obj) ((sCLBlock*)object_to_ptr((obj)))

struct sCLThreadStruct {
    sCLObjectHeader mHeader;

    pthread_t mThread;
};

typedef struct sCLThreadStruct sCLThread;

#define CLTHREAD(obj) ((sCLThread*)object_to_ptr((obj)))

struct sCLMutexStruct {
    sCLObjectHeader mHeader;

    pthread_mutex_t mMutex;
};

typedef struct sCLMutexStruct sCLMutex;

#define CLMUTEX(obj) ((sCLMutex*)object_to_ptr((obj)))

struct sCLFileStruct {
    sCLObjectHeader mHeader;

    int mFD;
};

typedef struct sCLFileStruct sCLFile;

#define CLFILE(obj) ((sCLFile*)object_to_ptr((obj)))

struct sCLBytesDataStruct {
    sCLObjectHeader mHeader;

    char mChars[DUMMY_ARRAY_SIZE];
};

typedef struct sCLBytesDataStruct sCLBytesData;

#define CLBYTES_DATA(obj) ((sCLBytesData*)object_to_ptr(CLBYTES((obj))->mData))

struct sCLBytesStruct {
    sCLObjectHeader mHeader;
    int mLen;
    CLObject mData;
};

typedef struct sCLBytesStruct sCLBytes;

#define CLBYTES(obj) ((sCLBytes*)object_to_ptr((obj)))

struct sCLTypeObjectStruct {
    sCLObjectHeader mHeader;

    sCLClass* mClass;
    int mGenericsTypesNum;
    CLObject mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
};

typedef struct sCLTypeObjectStruct sCLTypeObject;

#define CLTYPEOBJECT(obj) ((sCLTypeObject*)object_to_ptr((obj)))

struct sCLClassObjectStruct {
    sCLObjectHeader mHeader;
    CLObject mClass;
};

typedef struct sCLClassObjectStruct sCLClassObject;

#define CLCLASSOBJECT(obj) ((sCLClassObject*)object_to_ptr((obj)))

struct sCLFieldObjectStruct {
    sCLObjectHeader mHeader;

    sCLClass* mClass;
    sCLField* mField;
};

typedef struct sCLFieldObjectStruct sCLFieldObject;

#define CLFIELD(obj) ((sCLFieldObject*)object_to_ptr((obj)))

struct sCLMethodObjectStruct {
    sCLObjectHeader mHeader;

    sCLClass* mClass;
    sCLMethod* mMethod;
};

typedef struct sCLMethodObjectStruct sCLMethodObject;

#define CLMETHOD(obj) ((sCLMethodObject*)object_to_ptr((obj)))

/// clover functions ///

// result: (TRUE) success (FALSE) failed. should exit from process
BOOL cl_init(int heap_size, int handle_size);
void cl_final();
BOOL cl_load_fundamental_classes();

BOOL cl_eval_file(char* file_name);

void cl_create_clc_file();

// result (TRUE): success (FALSE): threw exception
BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack, int stack_size);
BOOL cl_excute_block_with_new_stack(MVALUE* result, CLObject block, BOOL result_existance, sVMInfo* new_info, CLObject vm_type);


int cl_print(sVMInfo* info, char* msg, ...);
void cl_puts(char* str);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_generics(char* real_class_name, sCLNodeType* type_);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name, int parametor_num);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name, int parametor_num);

int cl_get_method_index(sCLClass* klass, char* method_name);
    // result: (-1) --> not found (non -1) --> method index

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_class_field(sCLClass* klass, char* field_name, sCLClass* field_class, MVALUE* result);

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_array_element(CLObject array, int index, sCLClass* element_class, MVALUE* result);

#endif

