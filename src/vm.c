#include "clover.h"
#include <stdlib.h>
#include <stdio.h>

static MVALUE* gCLStack;
static uint gCLStackSize;
static MVALUE* gCLStackPtr;

typedef struct {
    uchar* mMem;
    uint mMemSize;
    void** mPtr;
    int*  mOrder;
    uint mSizeHandles;
    uint mNumHandles;
    uint mNumFreeHandles;
} sCLHeapManager;

static sCLHeapManager gCLHeap;

void cl_heap_init(int heap_size, int num_handles)
{
    gCLHeap.mMem = MALLOC(sizeof(uchar)*heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mPtr = MALLOC(sizeof(void*)*num_handles);
    gCLHeap.mOrder = MALLOC(sizeof(int)*num_handles);
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
    gCLHeap.mPtr[num] = gCLHeap.mMem + gCLHeap.mMemSize;
    gCLHeap.mMemSize += size;

    return num;
}

void cl_sweep()
{
}

void cl_gc()
{
}

void cl_init(int stack_size, int heap_size, int handle_size)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    cl_heap_init(heap_size, handle_size);
}

void cl_final()
{
    cl_heap_final();

    FREE(gCLStack);
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

BOOL cl_vm(sByteCode* code, sConst* constant)
{
    uchar* pc = code->mCode;
    MVALUE* stack = gCLStack;

    uint ivalue1, ivalue2, ivalue3;
    uchar cvalue1, cvalue2, cvalue3;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDC:
                ivalue1 = *(int*)(pc + 1);
                stack->mIntValue = constant->mConst[ivalue1];
printf("OP_LDC %d\n", stack->mIntValue);
                stack++;
                pc += sizeof(int) / sizeof(char) + 1;
                break;

            case OP_IADD:
                ivalue1 = (stack-1)->mIntValue + (stack-2)->mIntValue;
                stack-=2;
                stack->mIntValue = ivalue1;
printf("OP_IADD %d\n", stack->mIntValue);
                stack++;
                pc++;
                break;

            default:
                fprintf(stderr, "unexpected error at cl_vm\n");
                exit(1);
        }
        
        
show_stack(gCLStack, stack);


        
    }

    return TRUE;
}
