#ifndef CLOVER_H
#define CLOVER_H

#include "debug.h"
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

#define OP_IADD 0x01
#define OP_LDC 0x02
#define OP_ASTORE 0x03
#define OP_ISTORE 0x04
#define OP_FSTORE 0x05
#define OP_POP 0x06

/// virtual machine side data ///
typedef int CLObject;
#define SENIOR_OBJECT_BIT 0x80000000

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

BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, int* local_var_num, BOOL flg_main);
BOOL cl_eval(char* cmdline, char* sname, int* sline);
BOOL cl_main(sByteCode* code, sConst* constant);
BOOL cl_excute_method(sByteCode* code, sConst* constant, uint local_var_num);

void cl_editline_init();
void cl_editline_final();
ALLOC char* editline(char* prompt, char* rprompt);

#endif

