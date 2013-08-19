#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#define CL_STATIC_FIELD 0x01

typedef struct {
    uint mHeader;

    union {
        MVALUE mStaticField;
        uint mOffset;
    };
} sCLField;

typedef BOOL (*fNativeMethod)(MVALUE* stack);

typedef struct {
    uint mHeader;

    union {
        uchar* mByteCodes;
        fNativeMethod mNativeMethod;
    };

    uint mNumParams;
} sCLMethod;

typedef struct sCLClassStruct {
    sCLField* mFields;
    uint mNumFields;

    sCLMethod* mMethods;
    uint mNumMethods;
} sCLClass;

void* gClassHeap;
uint gSizeClassHeap;
uint gUsedClassHeap;

void class_heap_init(uint alloc_size)
{
    gClassHeap = CALLOC(1, alloc_size);
    gSizeClassHeap = alloc_size;
    gUsedClassHeap = 0;
}

void class_heap_final()
{
    FREE(gClassHeap);
}

void* alloc_class_part(uint size)
{
    size = (size + 3) & ~3;             // align to 4 byte boundry

    if(gUsedClassHeap + size >= gSizeClassHeap) {
        const int new_size = (gSizeClassHeap + size) * 2;
        gClassHeap = REALLOC(gClassHeap, new_size);
        memset(gClassHeap + gSizeClassHeap, 0, new_size - gSizeClassHeap);
        gSizeClassHeap = new_size;
    }

    void* result = gClassHeap + gUsedClassHeap;
    gUsedClassHeap += size;

    return result;
}

sCLClass* alloc_class(uint size_fields, uint size_methods)
{
    sCLClass self = alloc_class_part(sizeof(sCLClass));

    self->mSizeFields = size_fields;
    self->mFields = alloc_class_part(sizeof(sCLField)*size_fields);
    self->mNumFields = 0;

    self->mSizeMethods = size_methods;
    self->mMethods = alloc_class_part(1, sizeof(sCLMethod)*size_methods);
    self->mMethods = 0;

    return self;
}

void add_static_field_to_class(sCLClass* self)
{
    if(self->mSizeFields == self->mNumFields) {
        const int new_size = (self->mSizeFields + 1) * 2;
        self->mFields = REALLOC(self->mFields, sizeof(sCLField) * new_size);
        memset(self->mFields + self->mSizeFields, 0, sizeof(sCLField)* (new_size - self->mSizeFields));
        self->mSizeFields = new_size;
    }

    sCLField* new_one = self->mFields + self->mNumFields;
    new_one->mHeader = CL_STATIC_FIELD;
    new_one->mStaticField = 0;
    self->mNumFields++;
}

void add_field_to_class(sCLClass* self)
{
    if(self->mSizeFields == self->mNumFields) {
        const int new_size = (self->mSizeFields + 1) * 2;
        self->mFields = REALLOC(self->mFields, sizeof(sCLField) * new_size);
        memset(self->mFields + self->mSizeFields, 0, sizeof(sCLField)* (new_size - self->mSizeFields));
        self->mSizeFields = new_size;
    }

    sCLField* new_one = self->mFields + self->mNumFields;
    new_one->mHeader = 0;
    new_one->mOffset = self->mNumFields;
    self->mNumFields++;
}

void add_native_method_to_class(sCLClass* self, )
{
}

