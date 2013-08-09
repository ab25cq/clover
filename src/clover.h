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

/// virtual machine side data ///
typedef union {
    uint mIntValue;
    uchar mCharValue;
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

BOOL cl_init(int stack_size);
BOOL cl_final();

BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant);
BOOL cl_vm(sByteCode* code, sConst* constant);
BOOL cl_eval(char* cmdline, char* sname, int* sline);

void cl_editline_init();
void cl_editline_final();
ALLOC char* editline(char* prompt, char* rprompt);

#endif

