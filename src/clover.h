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

/// parser side data ///
typedef struct {
    uchar* mCode;
    uint mSize;
    uint mLen;
} sByteCode;

typedef struct {
    uchar* mConst;
    uint mSize;
    uint mLen;
} sConst;

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

/// virtual machine side data ///
typedef uint CLObject;

typedef union {
    uint mIntValue;
    uchar mCharValue;
    CLObject mObjectValue;
    CLObject mClassRef;
} MVALUE;

#define CONSTANT_INT 1
#define CONSTANT_STRING 2

#define CL_STATIC_FIELD 0x01

typedef struct {
    uint mHeader;

    union {
        MVALUE mStaticField;
        uint mOffset;
    };
} sCLField;

typedef BOOL (*fNativeMethod)(MVALUE* stack);

#define CL_NATIVE_METHOD 0x01

typedef struct {
    uint mHeader;
    uint mNameIndex;
    uint mPathIndex;

    union {
        struct {
            uchar* mByteCodes;
            uint mLenByteCodes;
        };
        fNativeMethod mNativeMethod;
    };

    uint mNumParams;
} sCLMethod;

#define CONSTAT_POOL(klass) klass->mByteRepresentation
#define CONS_ptr(klass, n) (klass->mByteRepresentation + klass->mConstantOffsets[n])
#define CONS_type(klass, n) *(klass->mByteRepresentation + klass->mConstantOffsets[n])
#define CONS_int(klass, n) *(int*)(klass->mByteRepresentation + klass->mConstantOffsets[n] + 1)
#define CONS_str_len(klass, n) *(int*)(klass->mByteRepresentation + klass->mConstantOffsets[n] + 1)
#define CONS_str(klass, n) (klass->mByteRepresentation + klass->mConstantOffsets[n] + 1 + sizeof(int))

typedef struct sCLClassStruct {
    uchar* mByteRepresentation;

    uint* mConstantOffsets;
    uint mNumConstants;

    uint mClassNameIndex;   // index of mConstatOffset

    sCLField* mFields;
    uint mNumFields;

    sCLMethod* mMethods;
    uint mNumMethods;

    struct sCLClassStruct* mNextClass;   // next class in hash table linked list
} sCLClass;

#define CLASS_NAME(klass) (klass->mByteRepresentation + klass->mConstantOffsets[klass->mClassNameIndex] + 1 + sizeof(int))
#define METHOD_NAME(klass, n) (klass->mByteRepresentation + klass->mMethods[n].mNameIndex + 1 + sizeof(int))
#define METHOD_PATH(klass, n) (klass->mByteRepresentation + klass->mMethods[n].mPathIndex + 1 + sizeof(int))

void cl_init(int global_size, int stack_size, int heap_size, int handle_size);
void cl_final();

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

