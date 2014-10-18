#include "clover.h"
#include "common.h"
#include <wchar.h>

void show_buffer(char* buf, int len)
{
    int i;

    for(i=0; i<len; i++) {
        char c;

        c = buf[i];
        printf("%d ", c);

        if(i % 50 == 49) printf("\n");
    }
    puts("");
}

//////////////////////////////////////////////////
// resizable buf
//////////////////////////////////////////////////
void sBuf_init(sBuf* self)
{
    self->mBuf = MALLOC(sizeof(char)*64);
    self->mSize = 64;
    self->mLen = 0;
    *(self->mBuf) = 0;
}

void sBuf_append(sBuf* self, void* str, size_t size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    memcpy(self->mBuf + self->mLen, str, size);

    self->mLen += size;
    self->mBuf[self->mLen] = 0;
}

void sBuf_append_char(sBuf* self, char c)
{
    if(self->mSize <= self->mLen + 1 + 1) {
        self->mSize = (self->mSize + 1 + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    self->mBuf[self->mLen] = c;
    self->mLen++;
    self->mBuf[self->mLen] = 0;
}

void sBuf_show(sBuf* self)
{
    show_buffer(self->mBuf, self->mLen);
}

//////////////////////////////////////////////////
// Byte Code operation. Make it resizable
//////////////////////////////////////////////////
void sByteCode_init(sByteCode* self)
{
    self->mSize = 128;
    self->mLen = 0;
    self->mCode = CALLOC(1, sizeof(int)*self->mSize);
}

void sByteCode_free(sByteCode* self)
{
    FREE(self->mCode);
}

void sByteCode_append(sByteCode* self, int value)
{
    if(self->mSize == self->mLen) {
        self->mSize = (self->mSize +1) * 2;
        self->mCode = REALLOC(self->mCode, sizeof(int) * self->mSize);
    }

    self->mCode[self->mLen] = value;
    self->mLen++;
}

void append_int_value_to_bytecodes(sByteCode* self, int value)
{
    sByteCode_append(self, value);
}

void append_opecode_to_bytecodes(sByteCode* self, int value)
{
    sByteCode_append(self, value);
}

//////////////////////////////////////////////////
// Constant Pool operation. Make it resizable and get alignment
//////////////////////////////////////////////////
void sConst_init(sConst* self)
{
    self->mSize = 1024;
    self->mLen = 0;
    self->mConst = CALLOC(1, sizeof(char)*self->mSize);
}

void sConst_free(sConst* self)
{
    FREE(self->mConst);
}

static void arrange_alignment_of_const_core(sConst* self, int alignment)
{
    int new_len;
     
    new_len = (self->mLen + (alignment-1)) & ~(alignment-1);

    if(self->mSize <= new_len) {
        self->mSize = new_len * 2;
        self->mConst = REALLOC(self->mConst, sizeof(char)*self->mSize);
    }

    if(new_len > self->mLen) {
        memset(self->mConst + self->mLen, 0, new_len - self->mLen);
    }

    self->mLen = new_len;
}

static void arrange_alignment_of_const(sConst* self, int size)
{
    if(self->mLen == 0) {
    }
    else if(size == 1) {
    }
    else if(size == 2) {
        arrange_alignment_of_const_core(self, 2);
    }
    else if(size == 4) {
        arrange_alignment_of_const_core(self, 4);
    }
    else if(size == 8) {
        arrange_alignment_of_const_core(self, 8);
    }
    else {
        arrange_alignment_of_const_core(self, 4);
    }
}

static int sConst_append(sConst* self, void* data, int size)
{
    int result; 

    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mConst = REALLOC(self->mConst, sizeof(char) * self->mSize);
    }

    arrange_alignment_of_const(self, size);

    result = self->mLen;

    memcpy(self->mConst + self->mLen, data, size);

    self->mLen += size;

    return result;
}

int append_int_value_to_constant_pool(sConst* constant, int n)
{
    return sConst_append(constant, &n, sizeof(int));
}

int append_float_value_to_constant_pool(sConst* constant, float n)
{
    return sConst_append(constant, &n, sizeof(float));
}

int append_str_to_constant_pool(sConst* constant, char* str)
{
    int len;
    
    len = strlen(str);
    return sConst_append(constant, str, len+1);
}

int append_wstr_to_constant_pool(sConst* constant, char* str)
{
    int len;
    wchar_t* wcs;
    int result;

    len = strlen(str);
    wcs = MALLOC(sizeof(wchar_t)*(len+1));
    (void)mbstowcs(wcs, str, len+1);

    result = sConst_append(constant, wcs, sizeof(wchar_t)*(len+1));

    FREE(wcs);

    return result;
}

void append_str_to_bytecodes(sByteCode* code, sConst* constant, char* str)
{
    int offset;

    offset = append_str_to_constant_pool(constant, str);
    append_int_value_to_bytecodes(code, offset);
}

void append_constant_pool_to_bytecodes(sByteCode* code, sConst* constant, sConst* constant2)
{
    int offset;

    offset = sConst_append(constant, constant2->mConst, constant2->mLen);
    append_int_value_to_bytecodes(code, constant2->mLen);
    append_int_value_to_bytecodes(code, offset);
}

void append_code_to_bytecodes(sByteCode* code, sConst* constant, sByteCode* code2)
{
    int offset;

    offset = sConst_append(constant, code2->mCode, sizeof(int)*code2->mLen);
    append_int_value_to_bytecodes(code, code2->mLen);
    append_int_value_to_bytecodes(code, offset);
}

void append_buf_to_constant_pool(sConst* self, char* src, int src_len)
{
    sConst_append(self, src, src_len);
}

void append_buf_to_bytecodes(sByteCode* self, int* code, int len)
{
    if(self->mSize <= self->mLen + len) {
        self->mSize = (self->mSize +len) * 2;
        self->mCode = REALLOC(self->mCode, sizeof(int) * self->mSize);
    }

    memcpy(self->mCode + self->mLen, code, sizeof(int)*len);
    self->mLen+=len;
}

void append_generics_type_to_bytecode(sByteCode* self, sConst* constant, sCLNodeType* type_)
{
    int i;

    ASSERT(type_ != NULL);

    append_str_to_bytecodes(self, constant, REAL_CLASS_NAME(type_->mClass));

    append_int_value_to_bytecodes(self, type_->mGenericsTypesNum);

    for(i=0; i<type_->mGenericsTypesNum; i++) {
        append_generics_type_to_bytecode(self, constant, type_->mGenericsTypes[i]);
    }
}
