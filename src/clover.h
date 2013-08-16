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
typedef unsigned int uint;
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

typedef BOOL (*fNativeMethod)(MVALUE* stack);

#define METHOD_FLAGS_BYTE_CODE 0x01
#define METHOD_FLAGS_NATIVE_METHOD 0x02

typedef struct {
    uchar mFlags;

    union {
        uchar* mByteCodes;
        fNativeMethod mNativeMethod;
    };
} sMMethod;

#define CONSTANT_INT 1
#define CONSTANT_STRING 2

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

#endif

