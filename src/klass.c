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
// get class
//////////////////////////////////////////////////

uint get_hash(uchar* name)
{
    uint hash = 0;
    uchar* p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

sCLClass* gClassHashList[CLASS_HASH_SIZE];

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

void create_real_class_name(uchar* result, int result_size, uchar* namespace, uchar* class_name)
{
    xstrncpy(result, namespace, result_size);
    xstrncat(result, "$", result_size);
    xstrncat(result, class_name, result_size);
}

sCLClass* cl_get_class_with_namespace(uchar* namespace, uchar* class_name)
{
    const int real_name_max = CL_NAMESPACE_NAME_MAX + CL_CLASS_NAME_MAX + 2;

    uchar real_name[real_name_max];
    create_real_class_name(real_name, real_name_max, namespace, class_name);

    const int hash = get_hash(real_name) % CLASS_HASH_SIZE;

    sCLClass* klass = gClassHashList[hash];

    while(klass) {
        if(strcmp(CLASS_NAME(klass), real_name) == 0) {
            return klass;
        }
        else {
            klass = klass->mNextClass;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////
// alloc class
//////////////////////////////////////////////////
static void add_class_to_class_table(uchar* class_name, sCLClass* klass)
{
    /// added this to class table ///
    const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;
}

// result must be not NULL; this is for compiler.c
sCLClass* alloc_class(uchar* class_name)
{
    sCLClass* klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mSizeMethods = 4;
    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);
    klass->mSizeFields = 4;
    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    klass->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, class_name);  // class name

    add_class_to_class_table(class_name, klass);

    return klass;
}

static void freed_class(sCLClass* klass)
{
    sConst_free(&klass->mConstPool);

    if(klass->mMethods) {
        int i;
        for(i=0; i<klass->mNumMethods; i++) {
            sCLMethod* method = klass->mMethods + i;
            if(method->mParamTypes) FREE(method->mParamTypes);

            if(!(method->mHeader & CL_NATIVE_METHOD) && method->mByteCodes.mCode != NULL) {
                sByteCode_free(&method->mByteCodes);
            }
        }
        FREE(klass->mMethods);
    }

    if(klass->mFields) {
        FREE(klass->mFields);
    }

    FREE(klass);
}

// expected result_size == CL_METHOD_NAME_REAL_MAX
void make_real_method_name(char* result, uint result_size, char* name, sCLClass* class_params[], uint num_params)
{
    xstrncpy(result, name, result_size);
    xstrncat(result, "$", result_size);

    int i;
    for(i=0; i<num_params; i++) {
        xstrncat(result, CLASS_NAME(class_params[i]));
        if(i+1 < num_params) xstrncat(result, "$");
    }
}

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, uchar* name, sCLClass* result_type, sCLClass* class_params[], uint num_params)
{
    if(klass->mNumMethods >= CL_METHODS_MAX) {
        return FALSE;
    }
    if(klass->mNumMethods >= klass->mSizeMethods) {
        const int new_size = klass->mSizeMethods * 2;
        klass->mMethods = REALLOC(klass->mMethods, sizeof(sCLMethod)*new_size);
        memset(klass->mMethods + klass->mSizeMethods, 0, sizeof(sCLMethod)*(new_size-klass->mSizeMethods));
        klass->mSizeMethods = new_size;
    }

    sCLMethod* method = klass->mMethods + klass->mNumMethods;
    method->mHeader = (static_ ? CL_STATIC_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0) | (native_ ? CL_NATIVE_METHOD:0);

    method->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);

    method->mPathOffset = klass->mConstPool.mLen;

    const int path_max = CL_METHOD_NAME_REAL_MAX + CL_CLASS_NAME_MAX + 2;
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

    method->mNumLocals = 0;

    return TRUE;
}

void alloc_bytecode(sCLMethod* method)
{
    sByteCode_init(&method->mByteCodes);
}

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, uchar* name, sCLClass* type_)
{
    if(klass->mNumFields >= CL_FIELDS_MAX) {
        return FALSE;
    }
    if(klass->mNumFields >= klass->mSizeFields) {
        const int new_size = klass->mSizeFields * 2;
        klass->mFields = REALLOC(klass->mFields, sizeof(sCLField)*new_size);
        memset(klass->mFields + klass->mSizeFields, 0, sizeof(sCLField)*(new_size-klass->mSizeFields));
        klass->mSizeFields = new_size;
    }

    sCLField* field = klass->mFields + klass->mNumFields;

    field->mHeader = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

    field->mNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, name);    // field name

    field->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, CLASS_NAME(type_));  // class name

    klass->mNumFields++;
    
    return TRUE;
}

//////////////////////////////////////////////////
// native method
//////////////////////////////////////////////////
static BOOL Clover_compaction(MVALUE* stack, MVALUE* stack_ptr)
{
    puts("running compaciton...");
    cl_gc();

    return TRUE;
}

static BOOL Clover_load(MVALUE* stack, MVALUE* stack_ptr)
{
    MVALUE* value = stack_ptr-1;

    const int size = (CLALEN(value->mObjectValue) + 1) * MB_LEN_MAX;
    uchar* str = MALLOC(size);
    wcstombs(str, CLASTART(value->mObjectValue), size);

    load_class(str);

    FREE(str);

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

static BOOL int_toString(MVALUE* stack, MVALUE* stack_ptr)
{
puts("Hello int_toString");

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
    { 1081, Clover_load },
    { 1126, float_floor },
    { 1222, Clover_print },
    { 1235, int_toString },
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

//////////////////////////////////////////////////
// load and save class
//////////////////////////////////////////////////
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

    return ALLOC buf.mBuf;
}

// result: (null) --> file not found (class pointer) --> success
sCLClass* load_class(uchar* file_name)
{
    int i;

    uchar* klass_data = ALLOC native_load_class(file_name);
    uchar* p = klass_data;

    if(p == NULL) {
        FREE(klass_data);
        return NULL;
    }

    sCLClass* klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    /// load constant pool ///
    const int const_pool_len = *(uint*)p;
    p += sizeof(int);

    sConst_append(&klass->mConstPool, p, const_pool_len);
    p += const_pool_len;

    /// load class name offset ///
    klass->mClassNameOffset = *(uchar*)p;
    p += sizeof(uchar);

    /// load fields ///
    klass->mSizeFields = klass->mNumFields = *(uchar*)p;
    p += sizeof(uchar);

    if(klass->mNumFields > 0) {
        klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

        for(i=0; i<klass->mNumFields; i++) {
            klass->mFields[i].mHeader = *(int*)p;
            p += sizeof(int);
            klass->mFields[i].mNameOffset = *(int*)p;
            p += sizeof(int);
            klass->mFields[i].mStaticField = *(MVALUE*)p;
            p += sizeof(int);
            klass->mFields[i].mClassNameOffset = *(int*)p;
            p += sizeof(int);
        }
    }
    else {
        klass->mFields = NULL;
    }

    /// load methods ///
    klass->mSizeMethods = klass->mNumMethods = *(uchar*)p;
    p += sizeof(uchar);

    if(klass->mNumMethods > 0) {
        klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);

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
                sByteCode_init(&klass->mMethods[i].mByteCodes);

                const int len_bytecodes = *(uint*)p;
                p += sizeof(uint);
                sByteCode_append(&klass->mMethods[i].mByteCodes, p, len_bytecodes);
                p += len_bytecodes;
            }

            klass->mMethods[i].mResultType = *(uint*)p;
            p += sizeof(int);

            const int param_num = *(uint*)p;
            klass->mMethods[i].mNumParams = param_num;
            p += sizeof(int);

            if(param_num == 0) 
                klass->mMethods[i].mParamTypes = NULL;
            else 
                klass->mMethods[i].mParamTypes = CALLOC(1, sizeof(uint)*param_num);

            int j;
            for(j=0; j<param_num; j++) {
                klass->mMethods[i].mParamTypes[j] = *(uint*)p;
                p += sizeof(int);
            }

            const int local_num = *(uint*)p;
            klass->mMethods[i].mNumLocals = local_num;
            p += sizeof(int);
        }
    }
    else {
        klass->mMethods = NULL;
    }

    add_class_to_class_table(CLASS_NAME(klass), klass);

show_class(klass);

    FREE(klass_data);
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
    sBuf_append(&buf, &klass->mClassNameOffset, sizeof(uchar));

    /// save fields
    sBuf_append(&buf, &klass->mNumFields, sizeof(uchar));
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sBuf_append(&buf, &klass->mFields[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mNameOffset, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mStaticField, sizeof(uint));
        //sBuf_append(&buf, &klass->mFields[i].mSize, sizeof(uint));
        sBuf_append(&buf, &klass->mFields[i].mClassNameOffset, sizeof(uint));
    }

    /// save methods
    sBuf_append(&buf, &klass->mNumMethods, sizeof(uchar));
    for(i=0; i<klass->mNumMethods; i++) {
        sBuf_append(&buf, &klass->mMethods[i].mHeader, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mNameOffset, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mPathOffset, sizeof(uint));

        if(klass->mMethods[i].mHeader & CL_NATIVE_METHOD) {
        }
        else {
            sBuf_append(&buf, &klass->mMethods[i].mByteCodes.mLen, sizeof(uint));
            sBuf_append(&buf, klass->mMethods[i].mByteCodes.mCode, klass->mMethods[i].mByteCodes.mLen);
        }

        sBuf_append(&buf, &klass->mMethods[i].mResultType, sizeof(uint));
        sBuf_append(&buf, &klass->mMethods[i].mNumParams, sizeof(uint));

        int j;
        for(j=0; j<klass->mMethods[i].mNumParams; j++) {
            sBuf_append(&buf, &klass->mMethods[i].mParamTypes[j], sizeof(uint));
        }

        sBuf_append(&buf, &klass->mMethods[i].mNumLocals, sizeof(uint));
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
            printf("field number %d --> %s static field %d\n", i, FIELD_NAME(klass, i), klass->mFields[i].mStaticField.mIntValue);
        }
        else {
            printf("field number %d --> %s\n", i, FIELD_NAME(klass, i));
        }
    }

    printf("num methods %d\n", klass->mNumMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        printf("--- method number %d ---\n", i);
        printf("name index %d (%s)\n", klass->mMethods[i].mNameOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mNameOffset));
        printf("path index %d (%s)\n", klass->mMethods[i].mPathOffset, CONS_str(klass->mConstPool, klass->mMethods[i].mPathOffset));
        printf("result type %s\n", CONS_str(klass->mConstPool, klass->mMethods[i].mResultType));
        printf("num params %d\n", klass->mMethods[i].mNumParams);
        printf("num locals %d\n", klass->mMethods[i].mNumLocals);
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
int j;
for(j=0; j<klass->mMethods[i].mByteCodes.mLen; j++) {
    printf("(%d) ", klass->mMethods[i].mByteCodes.mCode[j]);
}
puts("");
        }
    }
}

sCLClass* create_dummy_class(uchar* class_name)
{
    sCLClass* klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mClassNameOffset = klass->mConstPool.mLen;
    sConst_append_str(&klass->mConstPool, class_name);  // class name

    add_class_to_class_table(class_name, klass);

    return klass;
}

//////////////////////////////////////////////////
// accesser function
//////////////////////////////////////////////////

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
int get_field_index(sCLClass* klass, uchar* field_name)
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

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_with_params(sCLClass* klass, uchar* method_name, sCLClass** class_params, uint num_params)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method = klass->mMethods + i;

            /// type checking ///
            if(num_params == method->mNumParams) {
                int j;
                for(j=0; j<num_params; j++ ) {
                    uchar* class_name = CONS_str(klass->mConstPool, method->mParamTypes[j]);
                    sCLClass* class_param2 = cl_get_class(class_name);

                    if(class_param2 == NULL || class_params[j] != class_param2) {
                        break;
                    }
                }

                if(j == num_params) {
                    return method;
                }
            }
        }
    }

    return NULL;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, uchar* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> method index
int get_method_index_with_params(sCLClass* klass, uchar* method_name, sCLClass** class_params, uint num_params)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method = klass->mMethods + i;

            /// type checking ///
            if(num_params == method->mNumParams) {
                int j;
                for(j=0; j<num_params; j++) {
                    uchar* class_name = CONS_str(klass->mConstPool, method->mParamTypes[j]);
                    sCLClass* class_param2 = cl_get_class(class_name);

                    if(class_param2 == NULL || class_params[j] != class_param2) {
                        break;
                    }
                }

                if(j == num_params) {
                    return i;
                }
            }
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> index
int get_method_num_params(sCLClass* klass, uint method_index)
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

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
sCLClass* gIntClass;      // foudamental classes
sCLClass* gStringClass;
sCLClass* gFloatClass;
sCLClass* gVoidClass;
sCLClass* gCloverClass;

void class_init(BOOL load_foundamental_class)
{
    //memset(gClassHashList, 0, sizeof(sCLClass*)*CLASS_HASH_SIZE);

    if(load_foundamental_class) {
        gVoidClass = load_class(DATAROOTDIR "/void.clc");
        gIntClass = load_class(DATAROOTDIR "/int.clc");
        gFloatClass = load_class(DATAROOTDIR "/float.clc");
        gStringClass = load_class(DATAROOTDIR "/String.clc");
        gCloverClass  = load_class(DATAROOTDIR "/Clover.clc");
    }
    else {
        gVoidClass = create_dummy_class("void");
        gIntClass = create_dummy_class("int");
        gFloatClass = create_dummy_class("float");
        gStringClass = create_dummy_class("String");
        gCloverClass = create_dummy_class("Clover");
    }
}

void class_final()
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass = klass->mNextClass;
                freed_class(klass);
                klass = next_klass;
            }
        }
    }
}
