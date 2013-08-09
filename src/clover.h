#ifndef MCLOVER_H
#define MCLOVER_H

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

typedef BOOL (*fMNativeMethod)(MVALUE* stack);

#define METHOD_FLAGS_BYTE_CODE 0x01
#define METHOD_FLAGS_NATIVE_METHOD 0x02

typedef struct {
    uchar mFlags;

    union {
        uchar* mByteCodes;
        fMNativeMethod mNativeMethod;
    };
} sMMethod;

extern BOOL cl_parse(char* source, char* sname, int* sline);
extern BOOL cl_vm(sByteCode* code, sConst* constant);

#endif

