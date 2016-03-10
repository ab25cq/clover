#include "clover.h"
#include "common.h"
#include <wchar.h>
#include <ctype.h>

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
    self->mBuf = MMALLOC(sizeof(char)*64);
    self->mSize = 64;
    self->mLen = 0;
    *(self->mBuf) = 0;
}

void sBuf_append(sBuf* self, void* str, size_t size)
{
    void* str2;

    str2 = MCALLOC(1, size);        // prevent deleting from bellow REALLOC
    memcpy(str2, str, size);

    if(self->mSize <= self->mLen + size + 1) {
        int old_data_size = self->mSize;

        self->mSize = (self->mSize + size + 1) * 2;
        self->mBuf = MREALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    memcpy(self->mBuf + self->mLen, str2, size);

    self->mLen += size;
    self->mBuf[self->mLen] = 0;

    MFREE(str2);
}

void sBuf_append_char(sBuf* self, char c)
{
    if(self->mSize <= self->mLen + 1 + 1) {
        int old_data_size = self->mSize;

        self->mSize = (self->mSize + 1 + 1) * 2;
        self->mBuf = MREALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    self->mBuf[self->mLen] = c;
    self->mLen++;
    self->mBuf[self->mLen] = 0;
}

void sBuf_append_str(sBuf* self, char* str)
{
    sBuf_append(self, str, strlen(str));
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
    self->mCode = MCALLOC(1, sizeof(int)*self->mSize);
}

void sByteCode_free(sByteCode* self)
{
    MFREE(self->mCode);
}

static void sByteCode_append(sByteCode* self, int value, BOOL no_output_to_bytecodes)
{
    if(!no_output_to_bytecodes) {
        if(self->mSize == self->mLen) {
            int old_data_size = self->mSize;

            self->mSize = (self->mSize +1) * 2;
            self->mCode = MREALLOC(self->mCode, sizeof(int) * self->mSize);
        }

        self->mCode[self->mLen] = value;
        self->mLen++;
    }
}

void append_int_value_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes)
{
    sByteCode_append(self, value, no_output_to_bytecodes);
}

void append_ulong_value_to_bytecodes(sByteCode* self, unsigned long value, BOOL no_output_to_bytecodes)
{
    int n1, n2;
    memcpy(&n1, &value, sizeof(int));
    memcpy(&n2, (char*)&value + sizeof(int), sizeof(int));
    sByteCode_append(self, n1, no_output_to_bytecodes);
    sByteCode_append(self, n2, no_output_to_bytecodes);
}

void append_opecode_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes)
{
    sByteCode_append(self, value, no_output_to_bytecodes);
}

//////////////////////////////////////////////////
// Constant Pool operation. Make it resizable and get alignment
//////////////////////////////////////////////////
void sConst_init(sConst* self)
{
    self->mSize = 1024;
    self->mLen = 0;
    self->mConst = MCALLOC(1, sizeof(char)*self->mSize);
}

void sConst_free(sConst* self)
{
    MFREE(self->mConst);
}

static void arrange_alignment_of_const_core(sConst* self, int alignment)
{
    int new_len;
     
    new_len = (self->mLen + (alignment-1)) & ~(alignment-1);

    if(self->mSize <= new_len) {
        int old_data_size = self->mSize;

        self->mSize = new_len * 2;
        self->mConst = MREALLOC(self->mConst, sizeof(char)*self->mSize);
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

static int sConst_append(sConst* self, void* data, int size, BOOL no_output_to_bytecodes)
{
    if(!no_output_to_bytecodes) {
        int result; 
        void* data2;

        data2 = MCALLOC(1, size);        // prevent deleting from below free
        memcpy(data2, data, size);

        arrange_alignment_of_const(self, size);

        if(self->mSize <= self->mLen + size + 1) {
            char* old_data;
            int old_size;

            old_data = self->mConst;
            old_size = self->mSize;

            self->mSize = (self->mSize + size) * 2;
            self->mConst = MCALLOC(1, sizeof(char) * self->mSize);

            memcpy(self->mConst, old_data, self->mLen);

            MFREE(old_data);
        }

        result = self->mLen;

        memcpy(self->mConst + self->mLen, data2, size);

        self->mLen += size;

        MFREE(data2);

        return result;
    }
    else {
        return 0;
    }
}

int append_int_value_to_constant_pool(sConst* constant, int n, BOOL no_output_to_bytecodes)
{
    return sConst_append(constant, &n, sizeof(int), no_output_to_bytecodes);
}

int append_float_value_to_constant_pool(sConst* constant, float n, BOOL no_output_to_bytecodes)
{
    return sConst_append(constant, &n, sizeof(float), no_output_to_bytecodes);
}

int append_double_value_to_constant_pool(sConst* constant, double n, BOOL no_output_to_bytecodes)
{
    return sConst_append(constant, &n, sizeof(double), no_output_to_bytecodes);
}

int append_str_to_constant_pool(sConst* constant, char* str, BOOL no_output_to_bytecodes)
{
    int len;
    
    len = strlen(str);
    return sConst_append(constant, str, len+1, no_output_to_bytecodes);
}

int append_wstr_to_constant_pool(sConst* constant, char* str, BOOL no_output_to_bytecodes)
{
    int len;
    wchar_t* wcs;
    int result;

    len = strlen(str);
    wcs = MMALLOC(sizeof(wchar_t)*(len+1));
    (void)mbstowcs(wcs, str, len+1);

    result = sConst_append(constant, wcs, sizeof(wchar_t)*(len+1), no_output_to_bytecodes);

    MFREE(wcs);

    return result;
}

void append_str_to_bytecodes(sByteCode* code, sConst* constant, char* str, BOOL no_output_to_bytecodes)
{
    int offset;

    offset = append_str_to_constant_pool(constant, str, no_output_to_bytecodes);
    append_int_value_to_bytecodes(code, offset, no_output_to_bytecodes);
}

void append_constant_pool_to_bytecodes(sByteCode* code, sConst* constant, sConst* constant2, BOOL no_output_to_bytecodes)
{
    int offset;

    offset = sConst_append(constant, constant2->mConst, constant2->mLen, no_output_to_bytecodes);
    append_int_value_to_bytecodes(code, constant2->mLen, no_output_to_bytecodes);
    append_int_value_to_bytecodes(code, offset, no_output_to_bytecodes);
}

void append_code_to_bytecodes(sByteCode* code, sConst* constant, sByteCode* code2, BOOL no_output_to_bytecodes)
{
    int offset;

    offset = sConst_append(constant, code2->mCode, sizeof(int)*code2->mLen, no_output_to_bytecodes);
    append_int_value_to_bytecodes(code, code2->mLen, no_output_to_bytecodes);
    append_int_value_to_bytecodes(code, offset, no_output_to_bytecodes);
}

void append_buf_to_constant_pool(sConst* self, char* src, int src_len, BOOL no_output_to_bytecodes)
{
    sConst_append(self, src, src_len, no_output_to_bytecodes);
}

void append_buf_to_bytecodes(sByteCode* self, int* code, int len, BOOL no_output_to_bytecodes)
{
    if(!no_output_to_bytecodes) {
        if(self->mSize <= self->mLen + len) {
            int old_data_size = self->mSize;

            self->mSize = (self->mSize +len) * 2;
            self->mCode = MREALLOC(self->mCode, sizeof(int) * self->mSize);
        }

        memcpy(self->mCode + self->mLen, code, sizeof(int)*len);
        self->mLen+=len;
    }
}

void append_generics_type_to_bytecode(sByteCode* self, sConst* constant, sCLNodeType* type_, BOOL no_output_to_bytecodes)
{
    int i;

    MASSERT(type_ != NULL);

    append_str_to_bytecodes(self, constant, REAL_CLASS_NAME(type_->mClass), no_output_to_bytecodes);

    append_int_value_to_bytecodes(self, type_->mGenericsTypesNum, no_output_to_bytecodes);

    for(i=0; i<type_->mGenericsTypesNum; i++) {
        append_generics_type_to_bytecode(self, constant, type_->mGenericsTypes[i], no_output_to_bytecodes);
    }
}

void show_constants(sConst* constant)
{
    int i;
    int line;

    line = 0;

    for(i=0; i<constant->mLen; i++) {
        char c;

        if((i % 10) == 0) {
            printf("%4d: ", line);
        }

        c = constant->mConst[i];
        if(isalpha(c)) 
            printf("%c ", c);
        else
            printf("%d ", c);

        if((i % 10) == 9) {
            printf("\n");
            line++;
        }
    }
}
