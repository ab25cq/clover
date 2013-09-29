#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

MVALUE* gCLStack;
uint gCLStackSize;
MVALUE* gCLStackPtr;

void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    heap_init(heap_size, handle_size);

    class_init(load_foundamental_class);
    parser_init(load_foundamental_class);
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
    for(i=0; i<25; i++) {
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

static uchar visible_control_character(uchar c)
{
    if(c < ' ') {
        return '^';
    }
    else {
        return c;
    }
}

void show_constants(sConst* constant)
{
    puts("show_constants -+-");
    int i;
    for(i=0; i<constant->mLen; i++) {
        printf("[%d].%d(%c) ", i, constant->mConst[i], visible_control_character(constant->mConst[i]));
    }
    puts("");
}

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var)
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

                    case CONSTANT_WSTRING: {
                        uint len = *(uint*)p;
                        p+=sizeof(uint);
                        gCLStackPtr->mObjectValue = create_string_object((wchar_t*)p, len);
printf("OP_LDC string object %ld\n", gCLStackPtr->mObjectValue);
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

                ovalue3 = create_string_object(str, CLALEN(lobject) + CLALEN(robject));

                gCLStackPtr->mObjectValue = ovalue3;
printf("OP_SADD %ld\n", ovalue3);
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
                //gCLStackPtr--;
                var[ivalue1] = *(gCLStackPtr-1);
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

            case OP_POP_N:
printf("OP_POP\n");
                pc++;
                ivalue1 = *pc;
                pc += sizeof(int);

                gCLStackPtr -= ivalue1;
                break;

            case OP_INVOKE_STATIC_METHOD:
printf("OP_INVOKE_STATIC_METHOD\n");
                pc++;

                ivalue1 = *(uint*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(uint*)pc;  // method index
                pc += sizeof(int);
                
                cvalue1 = *(uchar*)pc;  // existance of result
                pc += sizeof(uchar);

                klass1 = cl_get_class(CONS_str((*constant), ivalue1));
                method = klass1->mMethods + ivalue2;

printf("class name (%s)\n", CLASS_NAME(klass1));
printf("method name (%s)\n", METHOD_NAME(klass1, ivalue2));

                if(method->mHeader & CL_NATIVE_METHOD) {
                    method->mNativeMethod(gCLStack, gCLStackPtr);
                }
                else {
                    if(!cl_excute_method(&method->mByteCodes, &klass1->mConstPool, method->mNumLocals, top_of_stack, method->mMaxStack)) {
                        return FALSE;
                    }
                }

                if(cvalue1) {
                    MVALUE* result_value = gCLStackPtr-1;
                    gCLStackPtr = top_of_stack;
                    *gCLStackPtr = *result_value;
                    gCLStackPtr++;
                }
                else {
                    gCLStackPtr = top_of_stack;
                }
                break;

            case OP_RETURN:
printf("OP_RETURN\n");
                pc++;
                return TRUE;

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

BOOL cl_main(sByteCode* code, sConst* constant, uint lv_num, uint max_stack)
{
    gCLStackPtr = gCLStack;
    MVALUE* lvar = gCLStack;
    gCLStackPtr += lv_num;

    if(gCLStackPtr + max_stack > gCLStack + gCLStackSize) {
        fprintf(stderr, "overflow stack size\n");
        return FALSE;
    }

    return cl_vm(code, constant, lvar);
}

BOOL cl_excute_method(sByteCode* code, sConst* constant, uint lv_num, MVALUE* top_of_stack, uint max_stack)
{
    gCLStackPtr = top_of_stack;
    MVALUE* lvar = top_of_stack;
    gCLStackPtr += lv_num;

    if(gCLStackPtr + max_stack > gCLStack + gCLStackSize) {
        fprintf(stderr, "overflow stack size\n");
        return FALSE;
    }

    return cl_vm(code, constant, lvar);
}

