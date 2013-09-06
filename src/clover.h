#ifndef CLOVER_H
#define CLOVER_H

#include "debug.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// clover definition
///////////////////////////////////////////////////////////////////////////////
typedef long int64;
typedef unsigned long uint64;

typedef unsigned int uint;

typedef short int16;
typedef unsigned short uint16;

typedef float float32;

typedef unsigned char uchar;

/// virtual machine side data ///
#define OP_IADD 1
#define OP_LDC 2
#define OP_ASTORE 3
#define OP_ISTORE 4
#define OP_FSTORE 5
#define OP_ALOAD 6
#define OP_ILOAD 7
#define OP_FLOAD 8
#define OP_POP 9
#define OP_SADD 10
#define OP_FADD 11
#define OP_INVOKE_STATIC_METHOD 12

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
#define CONS_str(constants, offset) (constants.mConst + offset + 1 + sizeof(int))
#define CONS_int(constants, offset) *(int*)(constants.mConst + offset + 1)

#define CONS_str_raw(constant, offset) (constant + offset + 1 + sizeof(int))

typedef struct {
    uchar* mConst;
    uint mSize;
    uint mLen;
} sConst;

typedef uint CLObject;

typedef union {
    uint mIntValue;
    uchar mCharValue;
    CLObject mObjectValue;
    CLObject mClassRef;
} MVALUE;

#define CL_STATIC_FIELD 0x01
#define CL_PRIVATE_FIELD 0x02

typedef struct {
    uint mHeader;
    uint mNameOffset;

    union {
        MVALUE mStaticField;
        uint mOffset;
    };

    uint mClassNameOffset; // offset of constant pool
} sCLField;

typedef BOOL (*fNativeMethod)(MVALUE* stack, MVALUE* stack_ptr);

#define CL_NATIVE_METHOD 0x01
#define CL_STATIC_METHOD 0x02
#define CL_PRIVATE_METHOD 0x04

typedef struct {
    uint mHeader;
    uint mNameOffset;
    uint mPathOffset;

    union {
        sByteCode mByteCodes;
        fNativeMethod mNativeMethod;
    };

    uint mResultType;     // offset of constant pool

    uint mNumParams;
    uint* mParamTypes;    // offset of constant pool
} sCLMethod;

#define CL_METHODS_MAX 64  // max number of methods
#define CL_FIELDS_MAX 64   // max number of fields
#define CL_CLASS_NAME_MAX 32    // max length of class name
#define CL_METHOD_NAME_MAX 32   // max length of method or field name
#define CL_METHOD_PARAM_MAX 16   // max number of param
#define WORDSIZ 128

#define CLASS_INTERFACE 0x01

typedef struct sCLClassStruct {
    uint mFlags;
    sConst mConstPool;

    uint mClassNameOffset;   // Offset of mConstatns

    sCLField* mFields;
    uint mNumFields;

    sCLMethod* mMethods;
    uint mNumMethods;

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list
} sCLClass;

#define CLASS_NAME(klass) (klass->mConstPool.mConst + klass->mClassNameOffset + 1 + sizeof(int))
#define METHOD_NAME(klass, n) (klass->mConstPool.mConst + klass->mMethods[n].mNameOffset + 1 + sizeof(int))
#define METHOD_PATH(klass, n) (klass->mConstPool.mConst + klass->mMethods[n].mPathOffset + 1 + sizeof(int))

void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class);
void cl_final();

void cl_create_clc_file();
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, int* global_var_num, BOOL flg_main);
BOOL cl_eval(char* cmdline, char* sname, int* sline);
BOOL cl_main(sByteCode* code, sConst* constant, uint global_var_num);
BOOL cl_excute_method(sByteCode* code, sConst* constant, uint global_var_num, uint local_var_num);
void cl_gc();

void cl_editline_init();
void cl_editline_final();
ALLOC char* editline(char* prompt, char* rprompt);

sCLClass* cl_get_class(uchar* class_name);
uint cl_get_method_index(sCLClass* klass, uchar* method_name);
    // result: (-1) --> not found (non -1) --> method index

#endif

