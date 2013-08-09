#include "clover.h"
#include <stdlib.h>
#include <stdio.h>

static MVALUE* gMStack;
static uint gMStackSize;
static MVALUE* gMStackPtr;

BOOL cl_init(int stack_size)
{
    gMStack = MALLOC(sizeof(MVALUE)* stack_size);
    gMStackSize = stack_size;
    gMStackPtr = gMStack;

    memset(gMStack, 0, sizeof(MVALUE) * stack_size);
}

BOOL cl_final()
{
    FREE(gMStack);
}

BOOL cl_vm(sByteCode* code, sConst* constant)
{
    uchar* pc = code->mCode;
    MVALUE* stack = gMStack;

    uint ivalue1, ivalue2, ivalue3;
    uchar cvalue1, cvalue2, cvalue3;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDC:
                ivalue1 = *(int*)(pc + 1);
                stack->mIntValue = constant->mConst[ivalue1];
                stack++;
                pc += sizeof(int) / sizeof(char) + 1;
                break;

            case OP_IADD:
                ivalue1 = (stack-1)->mIntValue + (stack-2)->mIntValue;
                stack-=2;
                stack->mIntValue = ivalue1;
                pc++;
                break;

            default:
                fprintf(stderr, "unexpected error at cl_vm\n");
                exit(1);
        }
    }

printf("value %d\n", stack->mIntValue);

    return TRUE;
}
