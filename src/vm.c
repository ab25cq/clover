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
    void** mPtr;
    uint*  mOrder;
    uint mSizeHandles;
    uint mNumHandles;
    uint mNumFreeHandles;
} sCLHeapManager;

static sCLHeapManager gCLHeap;

void cl_heap_init(int heap_size, int num_handles)
{
    gCLHeap.mMem = MALLOC(sizeof(uchar)*heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;
    gCLHeap.mPtr = MALLOC(sizeof(void*)*num_handles);
    gCLHeap.mOrder = MALLOC(sizeof(uint)*num_handles);
    gCLHeap.mSizeHandles = num_handles;
    gCLHeap.mNumHandles = 0;
    gCLHeap.mNumFreeHandles = 0;
}

void cl_heap_final()
{
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mPtr);
    FREE(gCLHeap.mOrder);
}

#define FIRST_OBJ 1234

static MVALUE* cl_object_to_ptr(CLObject obj) 
{
    return gCLHeap.mPtr[obj - FIRST_OBJ];
}

#define CLCLASS(obj) (cl_object_to_ptr(obj))[0].mClassRef
#define CLATYPE(obj) (cl_object_to_ptr(obj))[1].mIntValue
#define CLALEN(obj) (cl_object_to_ptr(obj))[2].mIntValue
#define CLASTART(obj) (void*)(cl_object_to_ptr(obj) + 3)

CLObject cl_alloc_object(uint size)
{
    int num;
    if(gCLHeap.mNumFreeHandles) {
        num = gCLHeap.mOrder[gCLHeap.mNumHandles - gCLHeap.mNumFreeHandles];
    }
    else {
        num = gCLHeap.mNumHandles;
        gCLHeap.mOrder[num] = num;
        gCLHeap.mNumHandles++;
    }
    gCLHeap.mPtr[num] = gCLHeap.mMem + gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

    return num + FIRST_OBJ;
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
    size += sizeof(MVALUE) * 3;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject cl_alloc_array(enum eArrayType type, uint len)
{
    CLObject obj;

    uint size = cl_array_size(type, len);

    obj = cl_alloc_object(size);

    CLCLASS(obj) = 0;  // pointer to class is NULL for array.
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

#define MARK_BIT 0x80000000
#define CLEAR_MARK_BIT 0x7FFFFFFF

static void sweep()
{
    uchar* temp = MALLOC(sizeof(uchar)*gCLHeap.mNumHandles);
    memset(temp, 0, sizeof(uchar)*gCLHeap.mNumHandles);

    /// clear mark bit ///
    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(gCLHeap.mOrder[i] & MARK_BIT) {
            gCLHeap.mOrder[i] &= CLEAR_MARK_BIT;
            temp[i] = 1;
        }
    }

    int prev_mem_size = gCLHeap.mMemLen;
    gCLHeap.mMemLen = 0;
    int used_handle = 0;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        int index = gCLHeap.mOrder[i];
        CLObject obj = index + FIRST_OBJ;
        CLObject klass = CLCLASS(obj);

        /// this is not a marking object ///
        if(!temp[index]) {
            if(cl_object_to_ptr(obj) != NULL) {
                /// destroy object ///
                // freeObject(obj);
                gCLHeap.mPtr[index] = NULL;
            }
        }
        /// this is a marking object ///
        else {
            int obj_size;

            /// pointer to class is NULL for array
            if(klass == 0) {
                obj_size = cl_array_size(CLATYPE(obj), CLALEN(obj));
            }
            else {
                /// obj_size = cl_klass_size(klass);
            }

            /// copy object to new heap
            void* src = gCLHeap.mPtr[index];
            void* dst = gCLHeap.mMem + gCLHeap.mMemLen;

            if(src != dst) { memmove(dst, src, obj_size); }

            gCLHeap.mPtr[index] = dst;
            gCLHeap.mOrder[used_handle] = index;
            gCLHeap.mMemLen += obj_size;
            used_handle++;
        }
    }

    gCLHeap.mNumFreeHandles = gCLHeap.mNumHandles - used_handle;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(!temp[i]) {
            gCLHeap.mOrder[used_handle] = i;
            used_handle++;
        }
    }

    memset(gCLHeap.mMem + gCLHeap.mMemLen, 0, prev_mem_size - gCLHeap.mMemLen);

    FREE(temp);
}

static BOOL is_valid_object(CLObject obj)
{
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;
}

static void mark_object(CLObject obj)
{
    const int index = obj - FIRST_OBJ;
    gCLHeap.mOrder[index] |= MARK_BIT;
}

static void mark()
{
    /// mark stack ///
    int i;
    const int len = gCLStackPtr - gCLStack;
    for(i=0; i<len; i++) {
        if(is_valid_object(gCLStack[i].mObjectValue)) {
            mark_object(gCLStack[i].mObjectValue);
        }
    }

    /// mark global vars ///
    for(i=0; i<gCLNumGlobalVars; i++) {
        if(is_valid_object(gCLGlobalVars[i].mObjectValue)) {
            mark_object(gCLGlobalVars[i].mObjectValue);
        }
    }
}

void cl_gc()
{
    mark();
    sweep();
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
    puts("handles");
    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        int order = gCLHeap.mOrder[i];
        void* ptr = gCLHeap.mPtr[i];

        printf("%d. (order %d) (ptr %p)\n", i, order, ptr);
    }

    puts("data");
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        int order = gCLHeap.mOrder[i];
        MVALUE* data = gCLHeap.mPtr[order];

        printf("%d order %d data %p\n", i, order, data);

        if(data) {
            CLObject klass = data[0].mClassRef;

            /// array ///
            if(klass == 0) {
                enum eArrayType type = data[1].mIntValue;
                int len = data[2].mIntValue;
                
                switch(type) {
                    case kATObject:
                        break;

                    case kATWChar: {
                        wchar_t* data2 = MALLOC(sizeof(wchar_t)*len + 1);
                        memcpy(data2, data + 3, sizeof(wchar_t)*len);
                        data2[len] = 0;

                        const int size = (len + 1) * MB_LEN_MAX;
                        uchar* str = MALLOC(size);
                        wcstombs(str, data2, size);

                        printf("%d: type %d len %d (%s)\n", order, type, len, str);

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

