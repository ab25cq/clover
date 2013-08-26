#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

static MVALUE* gCLStack;
static uint gCLStackSize;
static MVALUE* gCLStackPtr;

typedef struct _sFreedMemory {
    uint mOffset;
    uint mSize;

    struct _sFreedMemory* mNext;
} sFreedMemory;

typedef struct {
    uchar* mMem;
    uint mMemLen;
    uint mMemSize;

    sFreedMemory* mFreedMemory;

    uint* mOffset;      // -1 for freed memory
    uint mOffsetSize;
    uint mOffsetNum;

    uint* mFreedOffset;
    uint mFreedOffsetSize;
    uint mFreedOffsetNum;
} sCLHeapManager;

static sCLHeapManager gCLHeap;

static void heap_init(int heap_size, int num_handles)
{
    gCLHeap.mMem = CALLOC(1, heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;

    gCLHeap.mOffset = CALLOC(1, sizeof(uint)*num_handles);
    gCLHeap.mOffsetSize = num_handles;
    gCLHeap.mOffsetNum = 0;
    
    gCLHeap.mFreedOffset = CALLOC(1, sizeof(uint)*num_handles);
    gCLHeap.mFreedOffsetSize = num_handles;
    gCLHeap.mFreedOffsetNum = 0;

    gCLHeap.mFreedMemory = NULL;
}

static void heap_final()
{
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mOffset);
    FREE(gCLHeap.mFreedOffset);

    sFreedMemory* freed_memory = gCLHeap.mFreedMemory;
    while(freed_memory) {
        sFreedMemory* next = freed_memory->mNext;
        FREE(freed_memory);
        freed_memory = next;
    }
}

#define FIRST_OBJ 1234

MVALUE* cl_object_to_ptr(CLObject obj) 
{
    const uint index = obj - FIRST_OBJ;
    return (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[index];
}

CLObject cl_alloc_object(uint size)
{
    if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
        cl_gc();

        /// create new space of object ///
        if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
            const int new_heap_size = (gCLHeap.mMemSize + size) * 2;

            gCLHeap.mMem = REALLOC(gCLHeap.mMem, new_heap_size);
            memset(gCLHeap.mMem + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);
            gCLHeap.mMemSize = new_heap_size;
        }
    }

    uint offset;
    if(gCLHeap.mFreedOffsetNum > 0) {
        offset = gCLHeap.mFreedOffset[gCLHeap.mFreedOffsetNum-1];
        gCLHeap.mFreedOffsetNum--;
    }
    else {
        if(gCLHeap.mOffsetNum == gCLHeap.mOffsetSize) {
            const int new_offset_size = (gCLHeap.mOffsetSize + 1) * 2;

            gCLHeap.mOffset = REALLOC(gCLHeap.mOffset, sizeof(uint)*new_offset_size);
            memset(gCLHeap.mOffset + gCLHeap.mOffsetSize, 0, new_offset_size - gCLHeap.mOffsetSize);
            gCLHeap.mOffsetSize = new_offset_size;
        }

        offset = gCLHeap.mOffsetNum;
        gCLHeap.mOffsetNum++;
    }

    const uint obj = offset + FIRST_OBJ;

    gCLHeap.mOffset[offset] = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

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

    CLCLASS(obj) = 0;  // pointer to class is 0 for array.
    CLATYPE(obj) = type;
    CLALEN(obj) = len;

    return obj;
}

CLObject cl_create_string_object(wchar_t* str, uint len)
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
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mOffsetNum;
}

static void mark(uchar* mark_flg)
{
    /// mark stack ///
    int i;
    const int len = gCLStackPtr - gCLStack;
    for(i=0; i<len; i++) {
        CLObject obj = gCLStack[i].mObjectValue;
        if(is_valid_object(obj)) {
            mark_flg[obj - FIRST_OBJ] = TRUE;
        }
    }
}

static void compaction(uchar* mark_flg)
{
    int prev_mem_len = gCLHeap.mMemLen;
    gCLHeap.mMemLen = 0;

    int i;
    for(i=0; i<gCLHeap.mOffsetNum; i++) {
        if(gCLHeap.mOffset[i] != -1) {
            MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
            CLObject klass = CLPCLASS(data);
            CLObject obj = i + FIRST_OBJ;

            /// this is not a marking object ///
            if(!mark_flg[i]) {
                /// destroy object ///
                /// freeObject(obj);

                gCLHeap.mOffset[i] = -1;

                if(gCLHeap.mFreedOffsetNum == gCLHeap.mFreedOffsetSize) {
                    const int new_offset_size = (gCLHeap.mFreedOffsetSize + 1) * 2;

                    gCLHeap.mFreedOffset = REALLOC(gCLHeap.mFreedOffset, sizeof(uint)*new_offset_size);
                    memset(gCLHeap.mFreedOffset + gCLHeap.mFreedOffsetSize, 0, new_offset_size - gCLHeap.mFreedOffsetSize);
                    gCLHeap.mFreedOffsetSize = new_offset_size;
                }
            
                gCLHeap.mFreedOffset[gCLHeap.mFreedOffsetNum] = i;
                gCLHeap.mFreedOffsetNum++;
            }
            /// this is a marking object ///
            else {
                CLPEXISTENCE(data)++;

                int obj_size;

                /// 0 value for klass is array
                if(klass == 0) {
                    obj_size = cl_array_size(CLPATYPE(data), CLPALEN(data));
                }
                else {
                    /// obj_size = cl_klass_size(klass);
                }
                
                /// copy object to new heap
                void* src = gCLHeap.mMem + gCLHeap.mOffset[i];
                void* dst = gCLHeap.mMem + gCLHeap.mMemLen;

                if(src != dst) { memmove(dst, src, obj_size); }

                gCLHeap.mOffset[i] = gCLHeap.mMemLen;
                gCLHeap.mMemLen += obj_size;
            }
        }
    }

    memset(gCLHeap.mMem + gCLHeap.mMemLen, 0, prev_mem_len - gCLHeap.mMemLen);
}

void cl_gc()
{
    uchar* mark_flg = MALLOC(gCLHeap.mOffsetNum);
    memset(mark_flg, 0, gCLHeap.mOffsetNum);

    mark(mark_flg);
    compaction(mark_flg);

    FREE(mark_flg);
}

void cl_init(int global_size, int stack_size, int heap_size, int handle_size)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    heap_init(heap_size, handle_size);

    class_init();
    parser_init();
}

void cl_final()
{
    parser_final();
    class_final();

    heap_final();

    FREE(gCLStack);
}

static void show_stack(MVALUE* mstack, MVALUE* stack, MVALUE* top_of_stack)
{
    int i;
    for(i=0; i<10; i++) {
        if(mstack + i == top_of_stack) {
            printf("-- stack[%d] value %d\n", i, mstack[i].mIntValue);
        }
        else if(mstack + i == stack) {
            printf("-> stack[%d] value %d\n", i, mstack[i].mIntValue);
        }
        else {
            printf("   stack[%d] value %d\n", i, mstack[i].mIntValue);
        }
    }
}

static void show_heap()
{
    printf("offsetnum %d freed_offset_num %d\n", gCLHeap.mOffsetNum, gCLHeap.mFreedOffsetNum);
    int i;
    for(i=0; i<gCLHeap.mOffsetNum; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mOffset[i] == -1) {
            printf("*** %d --> (ptr null)\n", obj);
        }
        else {
            MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
            CLObject klass = CLPCLASS(data);
            uint existance_count = CLPEXISTENCE(data);

            printf("*** %d --> (ptr %p) (class %d) (existance count %d) ", obj, data, klass, existance_count);

            /// array ///
            if(klass == 0) {
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

                        printf("(type %d) (len %d) (%s)\n", type, len, str);

                        FREE(data2);
                        FREE(str);
                        }
                        break;
                }
            }
        }
    }

}

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var, MVALUE* gvar)
{
    MVALUE* top_of_stack = gCLStackPtr;
    uchar* pc = code->mCode;

    uint ivalue1, ivalue2, ivalue3;
    uchar cvalue1, cvalue2, cvalue3;
    CLObject ovalue1, ovalue2, ovalue3;
    sCLMethod* method;
    uchar* p;

    sCLClass* klass1, klass2, klass3;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDC: {
                ivalue1 = *(int*)(pc + 1);
                pc += sizeof(int) + 1;

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
                        gCLStackPtr->mObjectValue = cl_create_string_object((wchar_t*)p, len);
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

            case OP_SADD: {
                CLObject robject = (gCLStackPtr-1)->mObjectValue;
                CLObject lobject = (gCLStackPtr-2)->mObjectValue;

                gCLStackPtr-=2;

                wchar_t* str = MALLOC(sizeof(wchar_t)*(CLALEN(lobject) + CLALEN(robject)));

                memcpy(str, CLASTART(lobject), sizeof(wchar_t)*CLALEN(lobject));
                memcpy(str + CLALEN(lobject), CLASTART(robject), sizeof(wchar_t)*CLALEN(robject));

                ovalue3 = cl_create_string_object(str, CLALEN(lobject) + CLALEN(robject));

                gCLStackPtr->mObjectValue = ovalue3;
printf("OP_SADD %d\n", ovalue3);
                gCLStackPtr++;
                pc++;

                FREE(str);
                }
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
                pc++;
                ivalue1 = *pc;
printf("OP_STORE %d\n", ivalue1);
                pc += sizeof(int);
                gCLStackPtr--;
                var[ivalue1] = *gCLStackPtr;
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
                pc++;
                ivalue1 = *pc;
printf("OP_LOAD %d\n", ivalue1);
                pc += sizeof(int);
                *gCLStackPtr = var[ivalue1];
                gCLStackPtr++;
                break;

            case OP_POP:
printf("OP_POP\n");
                gCLStackPtr--;
                pc++;
                break;

            case OP_INVOKE_STATIC_METHOD:
                pc++;

                ivalue1 = *(uint*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(uint*)pc;  // method index
                pc += sizeof(int);

                klass1 = cl_get_class(CONS_str((*constant), ivalue1));
                method = klass1->mMethods + ivalue2;

                if(method->mHeader & CL_NATIVE_METHOD) {
                    method->mNativeMethod(gCLStack, gCLStackPtr);
                }
                else {
                }

                break;

            default:
                fprintf(stderr, "unexpected error at cl_vm\n");
                exit(1);
        }
show_stack(gCLStack, gCLStackPtr, top_of_stack);
show_heap();
    }
/*
puts("GC...");
cl_gc();
show_heap();
*/

    return TRUE;
}

BOOL cl_main(sByteCode* code, sConst* constant, uint global_var_num)
{
    gCLStackPtr = gCLStack;
    MVALUE* gvar = gCLStackPtr;
    gCLStackPtr += global_var_num;

    return cl_vm(code, constant, gvar, gvar);
}

BOOL cl_excute_method(sByteCode* code, sConst* constant, uint global_var_num, uint local_var_num)
{
    gCLStackPtr = gCLStack;
    MVALUE* gvar = gCLStackPtr;
    gCLStackPtr += global_var_num;
    MVALUE* lvar = gCLStackPtr;
    gCLStackPtr += local_var_num;

    return cl_vm(code, constant, lvar, gvar);
}

