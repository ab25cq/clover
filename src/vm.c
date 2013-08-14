#include "clover.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

static MVALUE* gCLStack;
static uint gCLStackSize;
static MVALUE* gCLStackPtr;
static MVALUE* gCLGlobalVars;
static uint gCLSizeGlobalVars;
static uint gCLNumGlobalVars;

typedef struct {
    uchar* mMem;
    uint mMemLen;
    uint mMemSize;

    uchar* mSeniorMem;
    uint mSeniorMemLen;
    uint mSeniorMemSize;

    int* mOffset;
    uint mOffsetSize;
    uint mOffsetNum;

    int* mSeniorOffset;
    uint mSeniorOffsetSize;
    uint mSeniorOffsetNum;
} sCLHeapManager;

static sCLHeapManager gCLHeap;

void cl_heap_init(int heap_size, int num_handles)
{
    gCLHeap.mMem = MALLOC(sizeof(uchar)*heap_size);
    memset(gCLHeap.mMem, 0, sizeof(uchar)*heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;

    gCLHeap.mOffset = MALLOC(sizeof(int)*num_handles);
    memset(gCLHeap.mOffset, 0, sizeof(int)*num_handles);
    gCLHeap.mOffsetSize = num_handles;
    gCLHeap.mOffsetNum = 0;

    gCLHeap.mSeniorMem = MALLOC(sizeof(uchar)*heap_size);
    memset(gCLHeap.mSeniorMem, 0, sizeof(uchar)*heap_size);
    gCLHeap.mSeniorMemSize = heap_size;
    gCLHeap.mSeniorMemLen = 0;

    gCLHeap.mSeniorOffset = MALLOC(sizeof(int)*num_handles);
    memset(gCLHeap.mSeniorOffset, 0, sizeof(int)*num_handles);
    gCLHeap.mSeniorOffsetSize = num_handles;
    gCLHeap.mSeniorOffsetNum = 0;
}

void cl_heap_final()
{
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mSeniorMem);
    FREE(gCLHeap.mOffset);
    FREE(gCLHeap.mSeniorOffset);
}

#define FIRST_OBJ 1234

static MVALUE* cl_object_to_ptr(CLObject obj) 
{
    if(obj & SENIOR_OBJECT_BIT) {
        const int index = (obj&~SENIOR_OBJECT_BIT) - FIRST_OBJ;
        return (MVALUE*)gCLHeap.mSeniorMem + gCLHeap.mSeniorOffset[index];
    }
    else {
        const int index = obj - FIRST_OBJ;
        return (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[index];
    }
}

#define OBJECT_HEADER_NUM 4

#define CLEXISTENCE(obj) (cl_object_to_ptr(obj))[0].mIntValue
#define CLCLASS(obj) (cl_object_to_ptr(obj))[1].mClassRef
#define CLATYPE(obj) (cl_object_to_ptr(obj))[2].mIntValue
#define CLALEN(obj) (cl_object_to_ptr(obj))[3].mIntValue
#define CLASTART(obj) (void*)(cl_object_to_ptr(obj) + 4)

#define CLPEXISTENCE(data) ((MVALUE*)data)[0].mIntValue
#define CLPCLASS(data) ((MVALUE*)data)[1].mClassRef
#define CLPATYPE(data) ((MVALUE*)data)[2].mIntValue
#define CLPALEN(data) ((MVALUE*)data)[3].mIntValue
#define CLPASTART(data) (void*)(((MVALUE*)data) + 4)

CLObject cl_alloc_object(uint size)
{
    if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
        cl_gc();

        /// create new space of object ///
        if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
            const int new_heap_size = (gCLHeap.mMemSize + size) * 2;

            gCLHeap.mMem = REALLOC(gCLHeap.mMem, sizeof(uchar)*new_heap_size);
            memset(gCLHeap.mMem + gCLHeap.mMemSize, 0, sizeof(uchar)*(new_heap_size - gCLHeap.mMemSize));
            gCLHeap.mMemSize = new_heap_size;
        }
    }
    if(gCLHeap.mOffsetNum == gCLHeap.mOffsetSize) {
        const int new_offset_size = (gCLHeap.mOffsetSize + 1) * 2;

        gCLHeap.mOffset = REALLOC(gCLHeap.mOffset, sizeof(int)*new_offset_size);
        memset(gCLHeap.mOffset + gCLHeap.mOffsetSize, 0, sizeof(int)*(new_offset_size - gCLHeap.mOffsetSize));
        gCLHeap.mOffsetSize = new_offset_size;
    }

    const int obj = gCLHeap.mOffsetNum + FIRST_OBJ;

    gCLHeap.mOffset[gCLHeap.mOffsetNum] = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;
    gCLHeap.mOffsetNum++;

    return obj;
}

enum eArrayType { kATObject, kATWChar };

uint cl_array_size(enum eArrayType type, uint len)
{
    uint size;
    switch(type) {
        case kATObject:
            size = 4 * len;
            break;

        case kATWChar:
            size = sizeof(wchar_t) * len;
            break;

        default:
            fprintf(stderr, "unexpected error on cl_array_size\n");
            exit(1);
    }
    size += sizeof(MVALUE) * OBJECT_HEADER_NUM;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject cl_alloc_array(enum eArrayType type, uint len)
{
    CLObject obj;

    uint size = cl_array_size(type, len);

    obj = cl_alloc_object(size);

    CLCLASS(obj) = -1;  // pointer to class is -1 for array.
    CLATYPE(obj) = type;
    CLALEN(obj) = len;

    return obj;
}

CLObject cl_create_string_object(uchar* str, uint len)
{
    CLObject obj;

    obj = cl_alloc_array(kATWChar, len);

    wchar_t* data = CLASTART(obj);

    int i;
    for(i=0; i<len; i++) {
        data[i] = str[i];
    }

    return obj;
}

static BOOL is_valid_object(CLObject obj)
{
    if(obj & SENIOR_OBJECT_BIT) {
        const uint index = (obj & ~SENIOR_OBJECT_BIT);
        return index >= FIRST_OBJ && index < FIRST_OBJ + gCLHeap.mSeniorOffsetNum;
    }
    else {
        return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mOffsetNum;
    }
}

static void mark(uchar* mark_flg, uchar* senior_mark_flg)
{
    /// mark stack ///
    int i;
    const int len = gCLStackPtr - gCLStack;
    for(i=0; i<len; i++) {
        CLObject obj = gCLStack[i].mObjectValue;
        if(is_valid_object(obj)) {
            if(obj & SENIOR_OBJECT_BIT) {
                const uint index = (obj & ~SENIOR_OBJECT_BIT) - FIRST_OBJ;
                senior_mark_flg[index] = TRUE;
            }
            else {
                mark_flg[obj - FIRST_OBJ] = TRUE;
            }
        }
    }

    /// mark global vars ///
    for(i=0; i<gCLNumGlobalVars; i++) {
        CLObject obj = gCLGlobalVars[i].mObjectValue;

        if(is_valid_object(obj)) {
            if(obj & SENIOR_OBJECT_BIT) {
                const uint index = (obj & ~SENIOR_OBJECT_BIT) - FIRST_OBJ;
                senior_mark_flg[index] = TRUE;
            }
            else {
                mark_flg[obj - FIRST_OBJ] = TRUE;
            }
        }
    }
}

static void sweep(uchar* mark_flg, uchar* senior_mark_flg)
{
    int prev_mem_len = gCLHeap.mMemLen;
    gCLHeap.mMemLen = 0;

    const int offset_num = gCLHeap.mOffsetNum;
    gCLHeap.mOffsetNum = 0;

    int i;
    for(i=0; i<offset_num; i++) {
        MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
        CLObject klass = CLPCLASS(data);
        CLObject obj = i + FIRST_OBJ;

        /// this is not a marking object ///
        if(!mark_flg[i]) {
            /// destroy object ///
            /// freeObject(obj);
        }
        /// this is a marking object ///
        else {
            CLPEXISTENCE(data)++;

            int obj_size;

            /// -1 value for klass is array
            if(klass == -1) {
                obj_size = cl_array_size(CLPATYPE(data), CLPALEN(data));
            }
            else {
                /// obj_size = cl_klass_size(klass);
            }
            
            /// copy object to new heap
            void* src = gCLHeap.mMem + gCLHeap.mOffset[i];
            void* dst = gCLHeap.mMem + gCLHeap.mMemLen;

            if(src != dst) { memmove(dst, src, obj_size); }

            gCLHeap.mOffset[gCLHeap.mOffsetNum] = gCLHeap.mMemLen;
            gCLHeap.mMemLen += obj_size;
            gCLHeap.mOffsetNum++;
        }
    }

    memset(gCLHeap.mMem + gCLHeap.mMemLen, 0, prev_mem_len - gCLHeap.mMemLen);
}

void cl_gc()
{
    uchar* mark_flg = MALLOC(sizeof(uchar)*gCLHeap.mOffsetNum);
    memset(mark_flg, 0, sizeof(uchar)*gCLHeap.mOffsetNum);

    uchar* senior_mark_flg = MALLOC(sizeof(uchar)*gCLHeap.mSeniorOffsetNum);
    memset(mark_flg, 0, sizeof(uchar)*gCLHeap.mSeniorOffsetNum);

    mark(mark_flg, senior_mark_flg);
    sweep(mark_flg, senior_mark_flg);

    FREE(mark_flg);
    FREE(senior_mark_flg);
}

void cl_init(int global_size, int stack_size, int heap_size, int handle_size)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    gCLGlobalVars = MALLOC(sizeof(MVALUE) * global_size);
    gCLSizeGlobalVars = 0;
    gCLNumGlobalVars = 0;

    memset(gCLGlobalVars, 0, sizeof(MVALUE) * global_size);

    cl_heap_init(heap_size, handle_size);
}

void cl_final()
{
    cl_heap_final();

    FREE(gCLStack);
    FREE(gCLGlobalVars);
}

static void show_stack(MVALUE* mstack, MVALUE* stack)
{
    int i;
    for(i=0; i<10; i++) {
        if(mstack + i == stack) {
            printf("-> stack[%d] value %d\n", i, mstack[i].mIntValue);
        }
        else {
            printf("   stack[%d] value %d\n", i, mstack[i].mIntValue);
        }
    }
}

static void show_heap()
{
    int i;
    for(i=0; i<gCLHeap.mOffsetNum; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mOffset[i] < 0) {
            printf("%d. (null)\n", obj);
        }
        else {
            MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
            CLObject klass = CLPCLASS(data);
            uint existance_count = CLPEXISTENCE(data);

            printf("*** %d --> (ptr %p) (class %d) (existance count %d) ***\n", obj, data, klass, existance_count);

            /// array ///
            if(klass == -1) {
                enum eArrayType type = CLPATYPE(data);
                int len = CLPALEN(data);
                
                switch(type) {
                    case kATObject:
                        break;

                    case kATWChar: {
                        wchar_t* data2 = MALLOC(sizeof(wchar_t)*len + 1);
                        memcpy(data2, CLPASTART(data), sizeof(wchar_t)*len);
                        data2[len] = 0;

                        const int size = (len + 1) * MB_LEN_MAX;
                        uchar* str = MALLOC(size);
                        wcstombs(str, data2, size);

                        printf("type %d len %d (%s)\n", type, len, str);

                        FREE(data2);
                        FREE(str);
                        }
                        break;
                }
            }
        }
    }

}

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var)
{
    uchar* pc = code->mCode;

    uint ivalue1, ivalue2, ivalue3;
    uchar cvalue1, cvalue2, cvalue3;
    uchar* p;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDC: {
                ivalue1 = *(int*)(pc + 1);
                pc += sizeof(int) / sizeof(char) + 1;

                p = constant->mConst + ivalue1;

                uchar const_type = *p;
                p++;

                switch(const_type) {
                    case CONSTANT_INT:
                        gCLStackPtr->mIntValue = *(int*)p;
printf("OP_LDC int value %d\n", gCLStackPtr->mIntValue);
                        break;

                    case CONSTANT_STRING: {
                        uint len = *(uint*)p;
                        ((uint*)p)++;
                        gCLStackPtr->mObjectValue = cl_create_string_object(p, len);
printf("OP_LDC string object %d\n", gCLStackPtr->mObjectValue);
                        }
                        break;
                }

                gCLStackPtr++;
                }
                break;

            case OP_IADD:
                ivalue1 = (gCLStackPtr-1)->mIntValue + (gCLStackPtr-2)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
printf("OP_IADD %d\n", gCLStackPtr->mIntValue);
                gCLStackPtr++;
                pc++;
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
                cvalue1 = *(pc + 1);
printf("OP_STORE %d\n", cvalue1);
                pc += 2;
                gCLStackPtr--;
                var[cvalue1] = *gCLStackPtr;
                break;

            case OP_POP:
                gCLStackPtr--;
                pc++;
                break;

            default:
                fprintf(stderr, "unexpected error at cl_vm\n");
                exit(1);
        }
show_stack(gCLStack, gCLStackPtr);
show_heap();
    }
puts("GC...");
cl_gc();
printf("offset_num %d\n", gCLHeap.mOffsetNum);
show_heap();

    return TRUE;
}

BOOL cl_main(sByteCode* code, sConst* constant)
{
    return cl_vm(code, constant, gCLGlobalVars);
}

BOOL cl_excute_method(sByteCode* code, sConst* constant, uint local_var_num)
{
    MVALUE* lvar = gCLStackPtr;
    gCLStackPtr += local_var_num;

    return cl_vm(code, constant, lvar);
}

