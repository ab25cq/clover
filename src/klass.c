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
// class heap
//////////////////////////////////////////////////
#define PAGE_SIZE 4096

uchar* gClassHeap;
uint gUsedClassHeap;

static void class_heap_init()
{
    gClassHeap = CALLOC(1, PAGE_SIZE);  // don't free after 
    gUsedClassHeap = 0;
}

static void class_heap_final()
{
}

void* alloc_class_part(uint size)
{
    size = (size + 3) & ~3;             // align to 4 byte boundry

    if(size >= PAGE_SIZE) {
        return CALLOC(1, size); // don't free after
    }
    if(gUsedClassHeap + size >= PAGE_SIZE) {
        gClassHeap = calloc(1, PAGE_SIZE);  // don't free after
        gUsedClassHeap = 0;
    }

    uchar* result = gClassHeap + gUsedClassHeap;
    gUsedClassHeap += size;

    return result;
}

//////////////////////////////////////////////////
// class
//////////////////////////////////////////////////
sCLClass* gClassHashList[CLASS_HASH_SIZE];

uint get_hash(uchar* name)
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

static BOOL Clover_compaction(MVALUE* stack, MVALUE* stack_ptr)
{
    puts("running compaciton...");
    cl_gc();

    return TRUE;
}

static BOOL Clover_print(MVALUE* stack, MVALUE* stack_ptr)
{
    MVALUE* value = stack_ptr-1;

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    uchar* str = MALLOC(size);
    wcstombs(str, CLASTART(value->mObjectValue), size);

    printf("%s\n", str);

    FREE(str);

    return TRUE;
}

static BOOL Clover_compile(MVALUE* stack, MVALUE* stack_ptr)
{
/*
    MVALUE* value = stack_ptr-1;

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    uchar* fname = MALLOC(size);
    wcstombs(fname, CLASTART(value->mObjectValue), size);

    int fd = open(fname, O_RDONLY);

    if(fd < 0) {
        fprintf(stderr, "can't open %s.\n", fname);
        FREE(fname);
        return FALSE;
    }

    sBuf buf;
    sBuf_init(&buf);

    while(1) {
        char buf2[BUFSIZ];
        int size = read(fd, buf2, BUFSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&buf, buf2, size);
    }

    close(fd);

    int sline = 1;
    if(!cl_parse_class(buf.mBuf, fname, &sline)) {
        FREE(buf.mBuf);
        FREE(fname);
        return FALSE;
    }

    FREE(buf.mBuf);
    FREE(fname);
*/

    return TRUE;
}

static BOOL Clover_gc(MVALUE* stack, MVALUE* stack_ptr)
{
puts("Hello Clover_gc");

    return TRUE;
}

static BOOL String_length(MVALUE* stack, MVALUE* stack_ptr)
{
puts("Hello String_length");

    return TRUE;
}

static BOOL int_times(MVALUE* stack, MVALUE* stack_ptr)
{
puts("Hello int_times");

    return TRUE;
}

static BOOL float_floor(MVALUE* stack, MVALUE* stack_ptr)
{
puts("float_floor");

    return TRUE;
}

typedef struct {
    uint mHash;
    fNativeMethod mFun;
} sNativeMethod;

// manually sort is needed
sNativeMethod gNativeMethods[] = {
    { 867, Clover_gc },
    { 923, int_times },
    { 1126, float_floor },
    { 1222, Clover_print },
    { 1319, String_length },
    { 1410, Clover_compile },
    { 1734, Clover_compaction },
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

// result: (null) --> file not found (uchar* pointer) --> success
ALLOC uchar* native_load_class(uchar* file_name)
{
    int f = open(file_name, O_RDONLY);

    if(f < 0) {
        return NULL;
    }

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

    return buf.mBuf;       // don't free after
}

// result must be not NULL
sCLClass* alloc_class(uchar* class_name)
{
    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*CL_METHODS_MAX);
    klass->mFields = CALLOC(1, sizeof(sCLField)*CL_FIELDS_MAX);

    klass->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, class_name);  // class name

    /// added to class table ///
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    return klass;
}

// result (TRUE) --> success (FALSE) --> overflow number methods or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, uchar* name, sCLClass* result_type, sCLClass* class_params[], uint num_params)
{
    sCLMethod* method = klass->mMethods + klass->mNumMethods;
    method->mHeader = (static_ ? CL_STATIC_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0);

    method->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);

    method->mPathOffset = klass->mConstPool.mLen;

    const int path_max = CL_METHOD_NAME_MAX + CL_CLASS_NAME_MAX;
    char buf[path_max];
    snprintf(buf, path_max, "%s.%s", CLASS_NAME(klass), name);
    sConst_append_str(&klass->mConstPool, buf);

    method->mResultType = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, CLASS_NAME(result_type));

    method->mParamTypes = CALLOC(1, sizeof(uint)*num_params);

    if(num_params >= CL_METHOD_PARAM_MAX) {
        return FALSE;
    }

    int i;
    for(i=0; i<num_params; i++) {
        method->mParamTypes[i] = klass->mConstPool.mLen;
        sConst_append_str(&klass->mConstPool, CLASS_NAME(class_params[i]));
    }
    method->mNumParams = num_params;

    klass->mNumMethods++;
    
    if(klass->mNumMethods >= CL_METHODS_MAX) {
        return FALSE;
    }

    return TRUE;
}

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, uchar* name, sCLClass* type_)
{
    sCLField* field = klass->mFields + klass->mNumFields;

    field->mHeader = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

    field->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);    // field name

    field->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, CLASS_NAME(type_));  // class name

    klass->mNumFields++;
    
    if(klass->mNumFields >= CL_FIELDS_MAX) {
        return FALSE;
    }

    return TRUE;
}

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, uchar* field_name)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0) {
            return klass->mFields + i;
        }
    }

    return NULL;
}

// result: (-1) --> not found (non -1) --> field index
uint get_field_index(sCLClass* klass, uchar* field_name)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, uchar* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return klass->mMethods + i;
        }
    }

    return NULL;
}

// result: (-1) --> not found (non -1) --> method index
uint get_method_index(sCLClass* klass, uchar* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> index
uint get_method_num_params(sCLClass* klass, uint method_index)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        return klass->mMethods[method_index].mNumParams;
    }

    return -1;
}

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* get_method_param_types(sCLClass* klass, uint method_index, uint param_num)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        sCLMethod* method = klass->mMethods + method_index;

        if(param_num >= 0 && param_num < method->mNumParams && method->mParamTypes != NULL) {
            uchar* class_name = CONS_str(klass->mConstPool, method->mParamTypes[param_num]);
            return cl_get_class(class_name);
        }
    }

    return NULL;
}

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* get_method_result_type(sCLClass* klass, uint method_index)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        sCLMethod* method = klass->mMethods + method_index;
        uchar* class_name = CONS_str(klass->mConstPool, method->mResultType);
        return cl_get_class(class_name);
    }

    return NULL;
}

// result: (null) --> file not found (class pointer) --> success
sCLClass* load_class(uchar* file_name)
{
    uchar* p = native_load_class(file_name);

    if(p == NULL) {
        return NULL;
    }

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    /// load constant pool ///
    klass->mConstPool.mLen = *(uint*)p;
    p += sizeof(int);
    klass->mConstPool.mConst = p;
    p += klass->mConstPool.mLen;

    /// load class name offset ///
    klass->mClassNameOffset = *(uint*)p;
    p += sizeof(uint);

    /// load fields ///
    klass->mNumFields = *(uint*)p;
    p += sizeof(uint);

    if(klass->mNumFields > 0) {
        klass->mFields = alloc_class_part(sizeof(sCLField)*klass->mNumFields);
    }
    else {
        klass->mFields = NULL;
    }

    int i;
    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = *(int*)p;
        p += sizeof(int);
        klass->mFields[i].mStaticField = *(MVALUE*)p;
        p += sizeof(int);
        //klass->mFields[i].mSize = *(uint*)p;
        //p += sizeof(int);
        klass->mFields[i].mClassNameOffset = *(int*)p;
        p += sizeof(int);
    }

    /// load methods ///
    klass->mNumMethods = *(int*)p;
    p += sizeof(int);

    if(klass->mNumMethods > 0) {
        klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);
    }
    else {
        klass->mMethods = NULL;
    }

    for(i=0; i<klass->mNumMethods; i++) {
        klass->mMethods[i].mHeader = *(uint*)p;
        p += sizeof(int);

        klass->mMethods[i].mNameOffset = *(uint*)p;
        p += sizeof(int);

        klass->mMethods[i].mPathOffset = *(uint*)p;
        p += sizeof(int);

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
            uchar* method_path = CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset);

            klass->mMethods[i].mNativeMethod = get_native_method(method_path);
        }
        else {
            klass->mMethods[i].mByteCodes.mLen = *(uint*)p;
            p += sizeof(uint);
            klass->mMethods[i].mByteCodes.mCode = p;
            p += klass->mMethods[i].mByteCodes.mLen;
        }

        klass->mMethods[i].mResultType = *(uint*)p;
        p += sizeof(int);

        const int param_num = *(uint*)p;
        klass->mMethods[i].mNumParams = param_num;
        p += sizeof(int);

        if(param_num == 0) 
            klass->mMethods[i].mParamTypes = NULL;
        else 
            klass->mMethods[i].mParamTypes = alloc_class_part(sizeof(uint)*param_num);

        int j;
        for(j=0; j<param_num; j++) {
            klass->mMethods[i].mParamTypes[j] = *(uint*)p;
            p += sizeof(int);
        }
    };

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    return klass;
}

/// result: true --> success, false --> failed to write
BOOL save_class(sCLClass* klass, uchar* file_name)
{
    sBuf buf;
    sBuf_init(&buf);

    /// save constant pool ///
    sBuf_append(&buf, &klass->mConstPool.mLen, sizeof(uint));
    sBuf_append(&buf, klass->mConstPool.mConst, klass->mConstPool.mLen);

    /// save class name offset
    sBuf_append(&buf, &klass->mClassNameOffset, sizeof(uint));

    /// save fields
    sBuf_append(&buf, &klass->mNumFields, sizeof(uint));
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sBuf_append(&buf, &klass->mFields[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mStaticField, sizeof(uint));
        //sBuf_append(&buf, &klass->mFields[i].mSize, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mClassNameOffset, sizeof(uint));
    }

    /// save methods
    sBuf_append(&buf, &klass->mNumMethods, sizeof(uint));
    for(i=0; i<klass->mNumMethods; i++) {
        sBuf_append(&buf, &klass->mMethods[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mNameOffset, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mPathOffset, sizeof(uint));

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
        }
        else {
            sBuf_append(&buf, &klass->mMethods[i].mByteCodes.mLen, sizeof(uint));
            sBuf_append(&buf, &klass->mMethods[i].mByteCodes.mCode, klass->mMethods[i].mByteCodes.mLen);
        }

        sBuf_append(&buf, &klass->mMethods[i].mResultType, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mNumParams, sizeof(uint));

        int j;
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            sBuf_append(&buf, &klass->mMethods[i].mParamTypes[j], sizeof(uint));
        }
    }

    /// write ///
    int f = open(file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    uint total_size = 0;
    while(total_size < buf.mLen) {
        size_t size;
        if(buf.mLen - total_size < BUFSIZ) {
            size = write(f, buf.mBuf + total_size, buf.mLen - total_size);
        }
        else {
            size = write(f, buf.mBuf + total_size, BUFSIZ);
        }
        if(size < 0) {
            FREE(buf.mBuf);
            return FALSE;
        }

        total_size += size;
    }
    close(f);

    FREE(buf.mBuf);

    return TRUE;
}

void show_class(sCLClass* klass)
{
    printf("-+- %s -+-\n", CLASS_NAME(klass));

    /// show constant pool ///
    printf("constant len %d\n", klass->mConstPool.mLen);
    show_constants(&klass->mConstPool);

    uchar* p = klass->mConstPool.mConst;

    while(p - klass->mConstPool.mConst < klass->mConstPool.mLen) {
        uchar type = *p;
        p++;

        switch(type) {
            case CONSTANT_STRING: {
                uint len = *(uint*)p;
                p+=sizeof(int);
                uchar* str = p;
                p+=len;
                printf("len (%d) str (%s)\n", len, str);
                }
                break;

            case CONSTANT_INT: {
                uint num = *(uint*)p;
                p+=sizeof(int);

                printf("value (%d)\n", num);
                }
                break;
        }
    }

    printf("ClassNameOffset %d (%s)\n", klass->mClassNameOffset, CONS_str(klass->mConstPool, klass->mClassNameOffset));

    printf("num fields %d\n", klass->mNumFields);

    int i;
    for(i=0; i<klass->mNumFields; i++) {
        if(klass->mFields[i].mHeader & CL_STATIC_FIELD) {
            printf("field number %d --> static field %d\n", i, klass->mFields[i].mStaticField.mIntValue);
        }
        else {
            printf("field number %d\n", i);
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        printf("--- method number %d ---\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mNameOffset));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset));
        printf("result type %s\n", CONS_str(klass->mConstPool, klass->mMethods[i].mResultType));
        printf("num params %d\n", klass->mMethods[i].mNumParams);
        int j;
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            printf("%d. %s\n", j, CONS_str(klass->mConstPool, klass->mMethods[i].mParamTypes[j]));
        }

        if(klass->mMethods[i].mHeader & CL_STATIC_METHOD) {
            printf("static method\n");
        }

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
            printf("native methods %p\n", klass->mMethods[i].mNativeMethod);
        }
        else {
            printf("length of bytecodes %d\n", klass->mMethods[i].mByteCodes.mLen);
        }
    }
}

//////////////////////////////////////////////////
// write foudamental classes
//////////////////////////////////////////////////
static void write_clover_class()
{
    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    //// constant pool ///
    uint offset0 = constant.mLen;
    sConst_append_str(&constant, "Clover");
    uint offset1 = constant.mLen;
    sConst_append_str(&constant, "compaction");
    uint offset2 = constant.mLen;
    sConst_append_str(&constant, "Clover.compaction");
    uint offset3 = constant.mLen;
    sConst_append_str(&constant, "void");

    uint offset4 = constant.mLen;
    sConst_append_str(&constant, "print");
    uint offset5 = constant.mLen;
    sConst_append_str(&constant, "Clover.print");
    uint offset6 = constant.mLen;
    sConst_append_str(&constant, "String");
    uint offset7 = constant.mLen;
    sConst_append_str(&constant, "void");

    uint offset8 = constant.mLen;
    sConst_append_str(&constant, "compile");
    uint offset9 = constant.mLen;
    sConst_append_str(&constant, "Clover.compile");
    uint offset10 = constant.mLen;
    sConst_append_str(&constant, "String");
    uint offset11 = constant.mLen;
    sConst_append_str(&constant, "void");

    klass->mConstPool.mConst = alloc_class_part(constant.mLen);
    memcpy(klass->mConstPool.mConst, constant.mConst, constant.mLen);
    klass->mConstPool.mLen = constant.mLen;

    FREE(constant.mConst);

    /// class name offset ///
    klass->mClassNameOffset = offset0;

    /// fields ///
    klass->mNumFields = 0;
    klass->mFields = NULL;

    /// methods ///
    klass->mNumMethods = 3;
    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    klass->mMethods[0].mHeader = CL_NATIVE_METHOD|CL_STATIC_METHOD;
    klass->mMethods[0].mNameOffset = offset1;
    klass->mMethods[0].mPathOffset = offset2;
    klass->mMethods[0].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[0].mPathOffset));
    klass->mMethods[0].mNumParams = 0;
    klass->mMethods[0].mParamTypes = NULL;
    klass->mMethods[0].mResultType = offset3;

    klass->mMethods[1].mHeader = CL_NATIVE_METHOD|CL_STATIC_METHOD;
    klass->mMethods[1].mNameOffset = offset4;
    klass->mMethods[1].mPathOffset = offset5;
    klass->mMethods[1].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[1].mPathOffset));
    klass->mMethods[1].mNumParams = 1;
    klass->mMethods[1].mParamTypes = alloc_class_part(sizeof(uint)*1);
    klass->mMethods[1].mParamTypes[0] = offset6;
    klass->mMethods[1].mResultType = offset7;

    klass->mMethods[2].mHeader = CL_NATIVE_METHOD|CL_STATIC_METHOD;
    klass->mMethods[2].mNameOffset = offset8;
    klass->mMethods[2].mPathOffset = offset9;
    klass->mMethods[2].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[1].mPathOffset));
    klass->mMethods[2].mNumParams = 1;
    klass->mMethods[2].mParamTypes = alloc_class_part(sizeof(uint)*1);
    klass->mMethods[2].mParamTypes[0] = offset10;
    klass->mMethods[2].mResultType = offset11;

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    /// show ///
//show_class(klass);

    /// save ///
    if(!save_class(klass, "Clover.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

static void write_string_class()
{
    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    //// constant pool ///
    uint offset0 = constant.mLen;
    sConst_append_str(&constant, "String");
    uint offset1 = constant.mLen;
    sConst_append_str(&constant, "length");
    uint offset2 = constant.mLen;
    sConst_append_str(&constant, "String.length");
    uint offset3 = constant.mLen;
    sConst_append_str(&constant, "int");

    klass->mConstPool.mConst = alloc_class_part(constant.mLen);
    memcpy(klass->mConstPool.mConst, constant.mConst, constant.mLen);
    klass->mConstPool.mLen = constant.mLen;

    FREE(constant.mConst);

    /// class name offset ///
    klass->mClassNameOffset = offset0;

    /// fields ///
    klass->mNumFields = 0;
    klass->mFields = NULL;

    /// methods ///
    klass->mNumMethods = 1;
    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    klass->mMethods[0].mHeader = CL_NATIVE_METHOD;
    klass->mMethods[0].mNameOffset = offset1;
    klass->mMethods[0].mPathOffset = offset2;
    klass->mMethods[0].mNumParams = 0;
    klass->mMethods[0].mParamTypes = NULL;
    klass->mMethods[0].mResultType = offset3;
    klass->mMethods[0].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[0].mPathOffset));

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    /// show ///
//show_class(klass);

    /// save ///
    if(!save_class(klass, "String.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

static void write_float_class()
{
    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    //// constant pool ///
    uint offset0 = constant.mLen;
    sConst_append_str(&constant, "float");
    uint offset1 = constant.mLen;
    sConst_append_str(&constant, "floor");
    uint offset2 = constant.mLen;
    sConst_append_str(&constant, "float.floor");
    uint offset3 = constant.mLen;
    sConst_append_str(&constant, "int");

    klass->mConstPool.mConst = alloc_class_part(constant.mLen);
    memcpy(klass->mConstPool.mConst, constant.mConst, constant.mLen);
    klass->mConstPool.mLen = constant.mLen;

    FREE(constant.mConst);

    /// class name offset ///
    klass->mClassNameOffset = offset0;

    /// fields ///
    klass->mNumFields = 0;
    klass->mFields = NULL; //alloc_class_part(sizeof(sCLField)*klass->mNumFields);;

/*
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = CL_STATIC_FIELD;
        klass->mFields[i].mStaticField.mIntValue = 333;
    }
*/

    /// methods ///
    klass->mNumMethods = 1;
    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    klass->mMethods[0].mHeader = CL_NATIVE_METHOD;
    klass->mMethods[0].mNameOffset = offset1;
    klass->mMethods[0].mPathOffset = offset2;
    klass->mMethods[0].mNumParams = 0;
    klass->mMethods[0].mParamTypes = NULL;
    klass->mMethods[0].mResultType = offset3;
    klass->mMethods[0].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[0].mPathOffset));

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    /// show ///
//show_class(klass);

    /// save ///
    if(!save_class(klass, "float.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

static void write_void_class()
{
    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    //// constant pool ///
    uint offset0 = constant.mLen;
    sConst_append_str(&constant, "void");

    klass->mConstPool.mConst = alloc_class_part(constant.mLen);
    memcpy(klass->mConstPool.mConst, constant.mConst, constant.mLen);
    klass->mConstPool.mLen = constant.mLen;

    FREE(constant.mConst);

    /// class name offset ///
    klass->mClassNameOffset = offset0;

    /// fields ///
    klass->mNumFields = 0;
    klass->mFields = NULL; //alloc_class_part(sizeof(sCLField)*klass->mNumFields);;

/*
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = CL_STATIC_FIELD;
        klass->mFields[i].mStaticField.mIntValue = 333;
    }
*/

    /// methods ///
    klass->mNumMethods = 0;
    klass->mMethods = NULL;

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    /// show ///
//show_class(klass);

    /// save ///
    if(!save_class(klass, "void.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

static void write_int_class()
{
    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    sCLClass* klass = alloc_class_part(sizeof(sCLClass));

    //// constant pool ///
    uint offset0 = constant.mLen;
    sConst_append_str(&constant, "int");
    uint offset1 = constant.mLen;
    sConst_append_str(&constant, "to_s");
    uint offset2 = constant.mLen;
    sConst_append_str(&constant, "int.to_s");
    uint offset3 = constant.mLen;
    sConst_append_str(&constant, "String");

    klass->mConstPool.mConst = alloc_class_part(constant.mLen);
    memcpy(klass->mConstPool.mConst, constant.mConst, constant.mLen);
    klass->mConstPool.mLen = constant.mLen;

    FREE(constant.mConst);

    /// class name offset ///
    klass->mClassNameOffset = offset0;

    /// fields ///
    klass->mNumFields = 0;
    klass->mFields = NULL; //alloc_class_part(sizeof(sCLField)*klass->mNumFields);;

/*
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        klass->mFields[i].mHeader = CL_STATIC_FIELD;
        klass->mFields[i].mStaticField.mIntValue = 333;
    }
*/

    /// methods ///
    klass->mNumMethods = 1;
    klass->mMethods = alloc_class_part(sizeof(sCLMethod)*klass->mNumMethods);

    klass->mMethods[0].mHeader = CL_NATIVE_METHOD;
    klass->mMethods[0].mNameOffset = offset1;
    klass->mMethods[0].mPathOffset = offset2;
    klass->mMethods[0].mNumParams = 0;
    klass->mMethods[0].mParamTypes = NULL;
    klass->mMethods[0].mResultType = offset3;
    klass->mMethods[0].mNativeMethod = get_native_method(CONS_str(klass->mConstPool, klass->mMethods[0].mPathOffset));

    /// added to class table ///
    uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;

    /// show ///
//show_class(klass);

    /// save ///
    if(!save_class(klass, "int.clc")) {
        fprintf(stderr, "Saving a class is failed");
    }
}

void cl_create_clc_file()
{
    write_clover_class();
    write_string_class();
    write_int_class();
    write_float_class();
    write_void_class();
}

void class_init(BOOL load_foundamental_class)
{
    class_heap_init();

    memset(gClassHashList, 0, sizeof(sCLClass*)*CLASS_HASH_SIZE);

    if(load_foundamental_class) {
        sCLClass* klass = load_class(DATAROOTDIR "/void.clc");
        if(klass) { show_class(klass); }

        klass = load_class(DATAROOTDIR "/int.clc");
        if(klass) { show_class(klass); }

        klass = load_class(DATAROOTDIR "/float.clc");
        if(klass) { show_class(klass); }

        klass = load_class(DATAROOTDIR "/String.clc");
        if(klass) { show_class(klass); }

        klass = load_class(DATAROOTDIR "/Clover.clc");
        if(klass) { show_class(klass); }
    }
}

void class_final()
{
    class_heap_final();
}
