#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>

//////////////////////////////////////////////////
// resizable buf
//////////////////////////////////////////////////
typedef struct {
    char* mBuf;
    uint mSize;
    uint mLen;
} sBuf;

static void sBuf_init(sBuf* self)
{
    self->mBuf = MALLOC(sizeof(char)*64);
    self->mSize = 64;
    self->mLen = 0;
    *(self->mBuf) = 0;
}

static void sBuf_append_char(sBuf* self, char c)
{
    if(self->mSize <= self->mLen + 1 + 1) {
        self->mSize = (self->mSize + 1 + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    self->mBuf[self->mLen] = c;
    self->mBuf[self->mLen+1] = 0;
    self->mLen++;
}

static void sBuf_append(sBuf* self, void* str, size_t size)
{
    const int len = strlen(str);

    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    memcpy(self->mBuf + self->mLen, str, size);

    self->mLen += size;
    self->mBuf[self->mLen] = 0;
}

//////////////////////////////////////////////////
// class 
//////////////////////////////////////////////////
#define PAGE_SIZE 4096

uchar* gClassHeap;
uint gUsedClassHeap;

static void class_heap_init()
{
    gClassHeap = calloc(1, PAGE_SIZE);  // don't free after 
    gUsedClassHeap = 0;
}

static void class_heap_final()
{
}

static void* alloc_class_part(uint size)
{
    size = (size + 3) & ~3;             // align to 4 byte boundry

    if(size >= PAGE_SIZE) {
        return calloc(1, size); // don't free after
    }
    if(gUsedClassHeap + size >= PAGE_SIZE) {
        gClassHeap = calloc(1, PAGE_SIZE);  // don't free after
        gUsedClassHeap = 0;
    }

    uchar* result = gClassHeap + gUsedClassHeap;
    gUsedClassHeap += size;

    return result;
}


#define CLASS_HASH_SIZE 128
sCLClass* gClassHashList[CLASS_HASH_SIZE];

static uint get_hash(uchar* name)
{
    uint hash = 0;
    uchar* p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

sCLClass* cl_get_class(uchar* class_name)
{
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;

    sCLClass* klass = gClassHashList[hash];

    while(klass) {
        if(strcmp(CLASS_NAME(klass), class_name) == 0) {
            return klass;
        }
        else {
            klass = klass->mNextClass;
        }
    }

    return NULL;
}

static BOOL Clover_Compaction(MVALUE* stack)
{
puts("Clover_Compaction");
}

static BOOL Clover_GC(MVALUE* stack)
{
puts("Clover_GC");
}

typedef struct {
    uint mHash;
    fNativeMethod mFun;
} sNativeMethod;

sNativeMethod gNativeMethods[2] = {
    { 803, Clover_GC },                       // Clover.GC
    { 1702, Clover_Compaction }               // Clover.Compaction
};

static fNativeMethod get_native_method(uchar* name)
{
    uint hash = get_hash(name);

    uint top = 0;
    uint bot = sizeof(gNativeMethods) / sizeof(sNativeMethod);

    while(1) {
        uint mid = (top + bot) / 2;
        
        if(gNativeMethods[mid].mHash == hash) {
            return gNativeMethods[mid].mFun;
        }

        if(mid == top) break;
        if(hash < gNativeMethods[mid].mHash) {
            bot = mid;
        }
        else {
            top = mid;
        }
    }

    return NULL;
}

static uchar* native_load_class(uchar* file_name)
{
    int f = open(file_name, O_RDONLY);

    sBuf buf;
    sBuf_init(&buf);

    while(1) {
        char buf2[BUFSIZ];
        int size = read(f, buf2, BUFSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&buf, buf2, size);
    }

    uchar* result = alloc_class_part(buf.mLen);
    memcpy(result, buf.mBuf, buf.mLen);
    FREE(buf.mBuf);

    return result;
}

sCLClass* load_class(uchar* file_name)
{
    uchar* p = native_load_class(file_name);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    klass->mByteRepresentation = p;

    klass->mNumConstants = *(int*)p;
    p += sizeof(int);

    klass->mConstantOffsets = alloc_class_part(sizeof(uint)*klass->mNumConstants);

    int i;
    for(i=0; i<klass->mNumConstants; i++) {
        klass->mConstantOffsets[i] = p - klass->mByteRepresentation;

        uchar type = *p;
        p++;

        switch(type) {
            case CONSTANT_STRING: {
                const int len = *(int*)p;
                p += sizeof(int);
                p += len;
                }
                break;

            case CONSTANT_INT: {
                p += sizeof(int);
                }
                break;
        }
    }

    klass->mClassNameIndex = *(int*)p;
    p += sizeof(int);

    klass->mNumFields = *(int*)p;
    p += sizeof(int);

    klass->mFields = alloc_class_part(sizeof(sCLField)*klass->mNumFields);

    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = *(int*)p;
        p += sizeof(int);
        klass->mFields[i].mStaticField = *(MVALUE*)p;
        p += sizeof(int);
    }

    klass->mNumMethods = *(int*)p;
    p += sizeof(int);

    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        klass->mMethods[i].mHeader = *(uint*)p;
        p += sizeof(int);

        klass->mMethods[i].mNameIndex = *(uint*)p;
        p += sizeof(int);

        klass->mMethods[i].mPathIndex = *(uint*)p;
        p += sizeof(int);

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
            uchar* method_path = CONS_ptr(klass, klass->mMethods[i].mPathIndex) + 1;
            method_path += sizeof(int);

            klass->mMethods[i].mNativeMethod = get_native_method(method_path);
        }
        else {
            klass->mMethods[i].mLenByteCodes = *(int*)p;
            p += sizeof(int);
            klass->mMethods[i].mByteCodes = p;
            p += klass->mMethods[i].mLenByteCodes;
        }

        klass->mMethods[i].mNumParams = *(uint*)p;
        p += sizeof(int);
    };

    uchar* class_name  = CONS_str(klass, klass->mClassNameIndex);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;
}

/// result: true --> success, false --> failed to write
BOOL save_class(sCLClass* klass, uchar* file_name)
{
    sBuf buf;
    sBuf_init(&buf);

    /// save constant number
    sBuf_append(&buf, &klass->mNumConstants, sizeof(int));
    
    /// save constant pool 
    uchar* p = CONSTAT_POOL(klass);
    int i;
    for(i=0; i<klass->mNumConstants; i++) {
        uchar type = *p;
        sBuf_append(&buf, p, sizeof(uchar));
        p++;

        switch(type) {
            case CONSTANT_STRING: {
                int len = *(int*)p;
                sBuf_append(&buf, p, sizeof(int));  // len

                p += sizeof(int);

                sBuf_append(&buf, p, len);

                p += len;
                }
                break;

            case CONSTANT_INT: {
                sBuf_append(&buf, p, sizeof(int));

                p += sizeof(int);
                }
                break;
        }
    }

    /// save class name index
    sBuf_append(&buf, &klass->mClassNameIndex, sizeof(uint));

    /// save fields
    sBuf_append(&buf, &klass->mNumFields, sizeof(uint));
    for(i=0; i<klass->mNumFields; i++) {
        sBuf_append(&buf, &klass->mFields[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mStaticField, sizeof(uint));
    }

    /// save methods
    sBuf_append(&buf, &klass->mNumMethods, sizeof(uint));
    for(i=0; i<klass->mNumMethods; i++) {
        sBuf_append(&buf, &klass->mMethods[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mNameIndex, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mPathIndex, sizeof(uint));

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
        }
        else {
            sBuf_append(&buf, &klass->mMethods[i].mLenByteCodes, sizeof(uint));
            sBuf_append(&buf, &klass->mMethods[i].mByteCodes, klass->mMethods[i].mLenByteCodes);
        }

        sBuf_append(&buf, &klass->mMethods[i].mNumParams, sizeof(uint));
    }

    int f = open(file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if(write(f, buf.mBuf, buf.mLen) < 0) {
        FREE(buf.mBuf);
        return FALSE;
    }
    close(f);

    FREE(buf.mBuf);

    return TRUE;
}

static void show_class(sCLClass* klass)
{
    printf("-+- %s -+-\n", CLASS_NAME(klass));

    /// show constant pool ///
    printf("num constatnt %d\n", klass->mNumConstants);

    int i;
    for(i=0; i<klass->mNumConstants; i++) {
        uchar type = CONS_type(klass, i);

        switch(type) {
            case CONSTANT_STRING:
                printf("len (%d) str (%s)\n", CONS_str_len(klass, i), CONS_str(klass, i));
                break;

            case CONSTANT_INT:
                printf("value (%d)\n", CONS_int(klass, i));
                break;
        }
    }

    printf("ClassNameIndex %d (%s)\n", klass->mClassNameIndex, CONS_str(klass, klass->mClassNameIndex));

    printf("num fields %d\n", klass->mNumFields);

    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mHeader & CL_STATIC_FIELD) {
            printf("%d. static field %d\n", i, klass->mFields[i].mStaticField.mIntValue);
        }
        else {
            printf("%d. field\n", i);
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        printf("number %d\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameIndex, CONS_str(klass, klass->mMethods[i].mNameIndex));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathIndex, CONS_str(klass, klass->mMethods[i].mPathIndex));
        printf("num params %d\n", klass->mMethods[i].mNumParams);

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
            printf("native methods %p\n", klass->mMethods[i].mNativeMethod);
        }
        else {
            printf("length of bytecodes %d\n", klass->mMethods[i].mLenByteCodes);
        }
    }
}

static void add_int_to_constant_pool(uchar** p, int number)
{
    uchar type = CONSTANT_INT;

    **p = type;
    (*p)++;

    *(int*)*p = number;
    (*p)+=sizeof(int);
}

static void add_string_to_constatnt_pool(uchar** p, char* str)
{
    uchar type = CONSTANT_STRING;
    const int len = strlen(str);

    **p = type;
    (*p)++;

    *(int*)*p = len + 1;
    (*p)+=sizeof(int);

    memcpy(*p, str, len + 1);
    (*p)+=len+1;
}

static void write_initial_class()
{
    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    klass->mByteRepresentation = alloc_class_part(1024);

    uchar* p = klass->mByteRepresentation;

    klass->mNumConstants = 3;
    klass->mConstantOffsets = alloc_class_part(sizeof(uint*)*klass->mNumConstants);

    klass->mConstantOffsets[0] = p - klass->mByteRepresentation;
    add_string_to_constatnt_pool(&p, "Clover");                 // 0
    klass->mConstantOffsets[1] = p - klass->mByteRepresentation;
    add_string_to_constatnt_pool(&p, "Compaction");            // 1
    klass->mConstantOffsets[2] = p - klass->mByteRepresentation;
    add_string_to_constatnt_pool(&p, "Clover.Compaction");     // 2

    klass->mClassNameIndex = 0;

    klass->mNumFields = 1;
    klass->mFields = alloc_class_part(sizeof(sCLField)*klass->mNumFields);;

    int i;
    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = CL_STATIC_FIELD;
        klass->mFields[i].mStaticField.mIntValue = 333;
    }

    klass->mNumMethods = 1;
    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    klass->mMethods[0].mHeader = CL_NATIVE_METHOD;
    klass->mMethods[0].mNameIndex = 1;
    klass->mMethods[0].mPathIndex = 2;

    uchar* method_path = CONS_str(klass, klass->mMethods[0].mPathIndex);
    klass->mMethods[0].mNativeMethod = get_native_method(method_path);

    uchar* class_name  = CONS_str(klass, klass->mClassNameIndex);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    show_class(klass);

    if(!save_class(klass, "Clover.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

// result: (-1) --> not found (non -1) --> method index
uint cl_get_method_index(sCLClass* klass, uchar* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return i;
        }
    }

    return -1;
}

void class_init(uint alloc_size)
{
    class_heap_init();

    memset(gClassHashList, 0, sizeof(sCLClass*)*CLASS_HASH_SIZE);

    load_class(DATAROOTDIR "/Clover.clc");

    sCLClass* klass = cl_get_class("Clover");

    if(klass) {
        show_class(klass);
    }
//write_initial_class();
}

void class_final()
{
    class_heap_final();
}
