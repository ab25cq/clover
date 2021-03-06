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
#define OP_BADD 2
#define OP_SHADD 3
#define OP_UIADD 4
#define OP_LOADD 5
#define OP_FADD 6
#define OP_DADD 7
#define OP_SADD 8
#define OP_BSADD 9

#define OP_ISUB 20
#define OP_BSUB 21
#define OP_SHSUB 22
#define OP_UISUB 23
#define OP_LOSUB 24
#define OP_FSUB 25
#define OP_DSUB 26

#define OP_IMULT 40
#define OP_BMULT 41
#define OP_SHMULT 42
#define OP_UIMULT 43
#define OP_LOMULT 44
#define OP_FMULT 45
#define OP_DMULT 46
#define OP_BSMULT 47
#define OP_SMULT 48

#define OP_IDIV 60
#define OP_BDIV 61
#define OP_SHDIV 62
#define OP_UIDIV 63
#define OP_LODIV 64
#define OP_FDIV 65
#define OP_DDIV 66

#define OP_IMOD 80
#define OP_BMOD 81
#define OP_SHMOD 82
#define OP_UIMOD 83
#define OP_LOMOD 84

#define OP_ILSHIFT 100
#define OP_BLSHIFT 101
#define OP_SHLSHIFT 102
#define OP_UILSHIFT 103
#define OP_LOLSHIFT 104

#define OP_IRSHIFT 120
#define OP_BRSHIFT 121
#define OP_SHRSHIFT 122
#define OP_UIRSHIFT 123
#define OP_LORSHIFT 124

#define OP_IAND 140
#define OP_BAND 141
#define OP_SHAND 142
#define OP_UIAND 143
#define OP_LOAND 144

#define OP_IXOR 160
#define OP_BXOR 161
#define OP_SHXOR 162
#define OP_UIXOR 163
#define OP_LOXOR 164

#define OP_IOR 180
#define OP_BOR 181
#define OP_SHOR 182
#define OP_UIOR 183
#define OP_LOOR 184

#define OP_IGTR 200
#define OP_BGTR 201
#define OP_SHGTR 202
#define OP_UIGTR 203
#define OP_LOGTR 204
#define OP_FGTR 205
#define OP_DGTR 206

#define OP_IGTR_EQ 220
#define OP_BGTR_EQ 221
#define OP_SHGTR_EQ 222
#define OP_UIGTR_EQ 223
#define OP_LOGTR_EQ 224
#define OP_FGTR_EQ 225
#define OP_DGTR_EQ 226

#define OP_ILESS 240
#define OP_BLESS 241
#define OP_SHLESS 242
#define OP_UILESS 243
#define OP_LOLESS 244
#define OP_FLESS 245
#define OP_DLESS 246

#define OP_ILESS_EQ 260
#define OP_BLESS_EQ 261
#define OP_SHLESS_EQ 262
#define OP_UILESS_EQ 263
#define OP_LOLESS_EQ 264
#define OP_FLESS_EQ 265
#define OP_DLESS_EQ 266

#define OP_IEQ 280
#define OP_BEQ 281
#define OP_SHEQ 282
#define OP_UIEQ 283
#define OP_LOEQ 284
#define OP_FEQ 285
#define OP_DEQ 286
#define OP_SEQ 287
#define OP_BSEQ 288

#define OP_INOTEQ 300
#define OP_BNOTEQ 301
#define OP_SHNOTEQ 302
#define OP_UINOTEQ 303
#define OP_LONOTEQ 304
#define OP_FNOTEQ 305
#define OP_DNOTEQ 306
#define OP_SNOTEQ 307
#define OP_BSNOTEQ 308

#define OP_COMPLEMENT 320
#define OP_BCOMPLEMENT 321
#define OP_SHCOMPLEMENT 322
#define OP_UICOMPLEMENT 323
#define OP_LOCOMPLEMENT 324

#define OP_BLEQ 340
#define OP_BLNOTEQ 341
#define OP_BLANDAND 342
#define OP_BLOROR 343
#define OP_LOGICAL_DENIAL 344

#define OP_LDCINT 360
#define OP_LDCBYTE 361
#define OP_LDCFLOAT 362
#define OP_LDCWSTR 363
#define OP_LDCBOOL 364
#define OP_LDCNULL 365
#define OP_LDTYPE 366
#define OP_LDCSTR 367
#define OP_LDCSHORT 368
#define OP_LDCUINT 369
#define OP_LDCLONG 370
#define OP_LDCCHAR 371
#define OP_LDCDOUBLE 372
#define OP_LDCPATH 373

#define OP_ASTORE 380
#define OP_ISTORE 381
#define OP_FSTORE 382
#define OP_OSTORE 383

#define OP_ALOAD 400
#define OP_ILOAD 401
#define OP_FLOAD 402
#define OP_OLOAD 403

#define OP_SRFIELD 420
#define OP_LDFIELD 421
#define OP_LD_STATIC_FIELD 422
#define OP_SR_STATIC_FIELD 423

#define OP_NEW_OBJECT 450
#define OP_NEW_ARRAY 451
#define OP_NEW_HASH 452
#define OP_NEW_RANGE 453
#define OP_NEW_TUPLE 454
#define OP_NEW_REGEX 455
#define OP_NEW_BLOCK 456

#define OP_INVOKE_METHOD 500
#define OP_INVOKE_VIRTUAL_METHOD 501
#define OP_INVOKE_VIRTUAL_CLONE_METHOD 502
#define OP_CALL_PARAM_INITIALIZER 503
#define OP_FOLD_PARAMS_TO_ARRAY 504

#define OP_IF 600
#define OP_NOTIF 601
#define OP_GOTO 602
#define OP_RETURN 603
#define OP_THROW 604
#define OP_TRY 605
#define OP_INVOKE_BLOCK 606
#define OP_BREAK_IN_METHOD_BLOCK 607

#define OP_OUTPUT_RESULT 700

#define OP_POP 800
#define OP_POP_N 801
#define OP_POP_N_WITHOUT_TOP 802
#define OP_DUP 803
#define OP_SWAP 804
#define OP_INC_VALUE 805
#define OP_DEC_VALUE 806

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

#define EXTRA_STACK_SIZE_FOR_PUSHING_OBJECT 128

#define WORDSIZ 256

#define CLASS_HASH_SIZE 512

#define CL_MODULE_HASH_SIZE 256

#define CL_TYPE_NAME_MAX 1024

#define CL_STACK_SIZE 4096
//2048
//#define CL_STACK_SIZE 1024

#define CL_GENERICS_CLASS_DEPTH_MAX 7

#define CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX 8

#define CL_MODULE_PARAM_MAX 16

#define CL_ENUM_NUM_MAX 128

#define REGEX_LENGTH_MAX 1024
#define MULTIPLE_ASSIGNMENT_NUM_MAX 16

#define CL_PREPROCESSOR_FUN_MAX 512
#define CL_PREPROCESSOR_FUN_NAME_MAX 64
#define CL_PREPROCESSOR_FUN_ARGUMENTS_NUM 9
#define CL_PREPROCESSOR_FUN_ARGMENT_LENGTH_MAX 64

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
    int log_number;
    FILE* debug_log;
#endif
    CLObject thread_obj;
    CLObject thread_block_obj;

    sCLClass* mRunningClass;
    struct sCLMethodStruct* mRunningMethod;

    sCLClass* mRunningClassOnException;
    struct sCLMethodStruct* mRunningMethodOnException;

    struct sVMInfoStruct* next_info;

    sBuf* print_buffer;
    BOOL existance_of_break;
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

typedef BOOL (*fNativeMethod)(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type);

typedef void (*fNativeClassInitializar)(sCLClass* klass);

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
#define CLASS_FLAGS_VERSION 0x00ff
#define CLASS_VERSION(klass) ((klass)->mFlags & CLASS_FLAGS_VERSION)
#define CLASS_VERSION_MAX 127
#define CLASS_FLAGS_INTERFACE 0x100
#define CLASS_FLAGS_PRIVATE 0x200
#define CLASS_FLAGS_ABSTRACT 0x400
#define CLASS_FLAGS_MODIFIED 0x800
#define CLASS_FLAGS_SPECIAL_CLASS 0x1000
#define CLASS_FLAGS_DYNAMIC_TYPING 0x2000
#define CLASS_FLAGS_FINAL 0x4000
#define CLASS_FLAGS_STRUCT 0x8000
#define CLASS_FLAGS_ENUM 0x10000
#define CLASS_FLAGS_NATIVE 0x20000
#define CLASS_FLAGS_NATIVE_BOSS 0x40000
#define CLASS_FLAGS_GENERICS_PARAM 0x80000
#define CLASS_FLAGS_INCLUDED_MODULE 0x100000

#define SUPER_CLASS_MAX 8
#define IMPLEMENTED_INTERFACE_MAX 32
#define INCLUDED_MODULE_MAX 32

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
    BOOL mFieldsInitialized;
    long long mFlags;
    sConst mConstPool;

    char mNameSpaceOffset;   // Offset of constant pool
    char mClassNameOffset;   // Offset of constant pool
    char mRealClassNameOffset;   // Offset of constant pool

    sCLField* mFields;
    int mNumFields;
    int mSizeFields;
    int mNumFieldsIncludingSuperClasses;

    sCLMethod* mMethods;
    int mNumMethods;
    int mSizeMethods;

    sCLType mSuperClass;

    sCLType mSuperClasses[SUPER_CLASS_MAX];
    char mNumSuperClasses;

    sCLType mImplementedInterfaces[IMPLEMENTED_INTERFACE_MAX];
    char mNumImplementedInterfaces;

    sCLType mIncludedModules[INCLUDED_MODULE_MAX];
    char mNumIncludedModules;

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
    int mMethodMissingMethodIndex;
    int mMethodMissingMethodIndexOfClassMethod;
    int mInitializeMethodIndex;
    int mPreInitializeMethodIndex;
    int mCompletionMethodIndex;
    int mCompletionMethodIndexOfClassMethod;

    int mMethodIndexOfCompileTime;  // compile time data
    int mNumLoadedMethods;          // compile time data

    int mSumOfNoneClassFieldsOnlySuperClasses;  // runtime only data
    int mSumOfNoneClassFields;                  // runtime only data
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

struct sCLPointerStruct {
    sCLObjectHeader mHeader;

    void* mValue;
    int mSize;

    char* mPointer;
    CLObject mPointedObject;
};

typedef struct sCLPointerStruct sCLPointer;

#define CLPOINTER(obj) ((sCLPointer*)object_to_ptr((obj)))

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

struct sCLUIntStruct {
    sCLObjectHeader mHeader;
    unsigned int mValue;
};

typedef struct sCLUIntStruct sCLUInt;

#define CLUINT(obj) ((sCLUInt*)object_to_ptr((obj)))

struct sCLShortStruct {
    sCLObjectHeader mHeader;
    unsigned short mValue;
};

typedef struct sCLShortStruct sCLShort;

#define CLSHORT(obj) ((sCLShort*)object_to_ptr((obj)))

struct sCLLongStruct {
    sCLObjectHeader mHeader;
    unsigned long mValue;
};

typedef struct sCLLongStruct sCLLong;

#define CLLONG(obj) ((sCLLong*)object_to_ptr((obj)))

struct sCLFloatStruct {
    sCLObjectHeader mHeader;
    float mValue;
};

typedef struct sCLFloatStruct sCLFloat;

#define CLFLOAT(obj) ((sCLFloat*)object_to_ptr((obj)))

struct sCLDoubleStruct {
    sCLObjectHeader mHeader;
    double mValue;
};

typedef struct sCLDoubleStruct sCLDouble;

#define CLDOUBLE(obj) ((sCLDouble*)object_to_ptr((obj)))

struct sCLByteStruct {
    sCLObjectHeader mHeader;
    unsigned char mValue;
};

typedef struct sCLByteStruct sCLByte;

#define CLBYTE(obj) ((sCLByte*)object_to_ptr((obj)))

struct sCLCharStruct {
    sCLObjectHeader mHeader;
    wchar_t mValue;
};

typedef struct sCLCharStruct sCLChar;

#define CLCHAR(obj) ((sCLChar*)object_to_ptr((obj)))

#define DUMMY_ARRAY_SIZE 32

struct sCLUserObjectStruct {
    sCLObjectHeader mHeader;
    int mNumFields;
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

struct sCLStringBufferStruct {
    sCLObjectHeader mHeader;
    int mLen;
    int mSize;
    wchar_t* mChars;
};

typedef struct sCLStringBufferStruct sCLStringBuffer;

#define CLSTRINGBUFFER(obj) ((sCLStringBuffer*)object_to_ptr((obj)))

struct sCLParserStruct {
    sCLObjectHeader mHeader;
    long mSize;
    long mPoint;
    CLObject mPointedObject;
};

typedef struct sCLParserStruct sCLParser;

#define CLPARSER(obj) ((sCLParser*)object_to_ptr((obj)))

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
    CLObject mHead;
    CLObject mTail;
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

    int mBreakable;             // 0:none 1:tuple 2:bool 3:dynamic typing
    BOOL mCallerExistance; 
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

struct sCLBytesStruct {
    sCLObjectHeader mHeader;
    int mLen;
    char* mChars;
    int mSize;

    long mPoint;
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
BOOL cl_init(int heap_size, int handle_size, int argc, char** argv);
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
sCLClass* cl_get_class_with_namespace(char* namespace_, char* class_name, int parametor_num);

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace_, char* class_name, int parametor_num);

int cl_get_method_index(sCLClass* klass, char* method_name);
    // result: (-1) --> not found (non -1) --> method index

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_class_field(sCLClass* klass, char* field_name, sCLClass* field_class, MVALUE* result);

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_array_element(CLObject array, int index, sCLClass* element_class, MVALUE* result);

#endif

