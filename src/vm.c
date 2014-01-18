#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
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

static void vm_error(char* msg, ...)
{
    char msg2[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}

static void show_stack(MVALUE* stack, MVALUE* stack_ptr, MVALUE* top_of_stack, MVALUE* var)
{
    int i;
    for(i=0; i<10; i++) {
        if(stack + i == var) {
            if(stack + i == stack_ptr) {
                printf("->v-- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
            else {
                printf("  v-- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
        }
        else if(stack + i == top_of_stack) {
            if(stack + i == stack_ptr) {
                printf("->--- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
            else {
                printf("  --- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
        }
        else if(stack + i == stack_ptr) {
            printf("->    stack[%d] value %d\n", i, stack[i].mIntValue);
        }
        else {
            printf("      stack[%d] value %d\n", i, stack[i].mIntValue);
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

#define VM_DEBUG

#ifdef VM_DEBUG
void vm_debug(char* msg, ...)
{
    char msg2[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}
#endif

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var)
{
    uint ivalue1, ivalue2, ivalue3;
    uchar cvalue1, cvalue2, cvalue3, cvalue4;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr;

    sCLClass* klass1, *klass2, *klass3;
    sCLMethod* method;
    sCLField* field;
    char* p;
    wchar_t* str;
    char* real_class_name;

    sCLClass* params[CL_METHOD_PARAM_MAX];
    uint num_params;

    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];

    int i;

    MVALUE* top_of_stack = gCLStackPtr;
    uchar* pc = code->mCode;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDC:
                pc++;

                ivalue1 = *(int*)(pc);
                pc += sizeof(int);

                p = constant->mConst + ivalue1;

                cvalue1 = *p;
                p++;

                switch(cvalue1) {
                    case CONSTANT_INT:
                        gCLStackPtr->mIntValue = *(int*)p;
#ifdef VM_DEBUG
vm_debug("OP_LDC int value %d\n", gCLStackPtr->mIntValue);
#endif
                        break;

                    case CONSTANT_WSTRING: {
                        uint len = *(uint*)p;
                        p+=sizeof(uint);
                        gCLStackPtr->mObjectValue = create_string_object((wchar_t*)p, len);
#ifdef VM_DEBUG
vm_debug("OP_LDC string object %ld\n", gCLStackPtr->mObjectValue);
#endif
                        }
                        break;
                }

                gCLStackPtr++;
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
                pc++;
                ivalue1 = *pc;
#ifdef VM_DEBUG
vm_debug("OP_LOAD %d\n", ivalue1);
#endif
                pc += sizeof(int);
                *gCLStackPtr = var[ivalue1];
                gCLStackPtr++;
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
            case OP_OSTORE:
                pc++;
                ivalue1 = *pc;
                pc += sizeof(int);

#ifdef VM_DEBUG
vm_debug("OP_STORE %d\n", ivalue1);
#endif
                var[ivalue1] = *(gCLStackPtr-1);
                break;

            case OP_LDFIELD:
                pc++;

                ivalue1 = *(uint*)pc;                   // field index
                pc += sizeof(int);

                ovalue1 = (gCLStackPtr-1)->mObjectValue;
                gCLStackPtr--;

                *gCLStackPtr = CLFIELD(ovalue1, ivalue1);
#ifdef VM_DEBUG
vm_debug("LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);
#endif
                gCLStackPtr++;
                break;

            case OP_SRFIELD:
                pc++;

                ivalue1 = *(int*)pc;                        // field index
                pc += sizeof(int);

                ovalue1 = (gCLStackPtr-2)->mObjectValue;    // target object
                mvalue1 = gCLStackPtr-1;                    // right value

                CLFIELD(ovalue1, ivalue1) = *mvalue1;
#ifdef VM_DEBUG
vm_debug("SRFIELD object %d field num %d value %d\n", (int)ovalue1, ivalue1, mvalue1->mIntValue);
#endif
                gCLStackPtr-=2;

                *gCLStackPtr = *mvalue1;
                gCLStackPtr++;
                break;

            case OP_LD_STATIC_FIELD: 
                pc++;

                ivalue1 = *(uint*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(uint*)pc;  // field index
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    vm_error("can't get this class(%s)\n", real_class_name);
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                *gCLStackPtr = field->mStaticField;
#ifdef VM_DEBUG
vm_debug("LD_STATIC_FIELD %d\n", field->mStaticField.mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_SR_STATIC_FIELD:
                pc++;

                ivalue1 = *(uint*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(uint*)pc;  // field index
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    vm_error("can't get this class(%s)\n", real_class_name);
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                field->mStaticField = *(gCLStackPtr-1);
#ifdef VM_DEBUG
vm_debug("OP_SR_STATIC_FIELD value %d\n", (gCLStackPtr-1)->mIntValue);
#endif
                break;

            case OP_NEW_OBJECT:
#ifdef VM_DEBUG
vm_debug("NEW_OBJECT");
#endif
                pc++;

                ivalue1 = *((int*)pc);
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    vm_error("can't get this class(%s)\n", real_class_name);
                    return FALSE;
                }

                ovalue1 = create_object(klass1);

                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                break;

            case OP_NEW_ARRAY:
#ifdef VM_DEBUG
vm_debug("OP_NEW_ARRAY\n");
#endif
                pc++;

                ivalue1 = *(uint*)pc;   // number of elements
                pc += sizeof(int);

                stack_ptr = gCLStackPtr - ivalue1;
                for(i=0; i<ivalue1; i++) {
                    objects[i] = *stack_ptr++;
                }

                ovalue1 = create_array_object(objects, ivalue1);
#ifdef VM_DEBUG
vm_debug("new array %d\n", ovalue1);
#endif

                gCLStackPtr -= ivalue1;
                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                break;

            case OP_INVOKE_METHOD:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_METHOD\n");
#endif
                pc++;

                ivalue1 = *(uint*)pc;       // method name offset
                pc += sizeof(uint);

                ivalue2 = *(uint*)pc;       // method num params
                pc += sizeof(uint);

                for(i=0; i<ivalue2; i++) {
                    ivalue3 = *(uint*)pc;
                    pc += sizeof(uint);

                    p = constant->mConst + ivalue3;
                    p++; // throw constant type away
                    p+=sizeof(uint);  // throw length of string away
                    real_class_name = p;    // real class name of a param

                    params[i] = cl_get_class(real_class_name);

                    if(params[i] == NULL) {
                        vm_error("can't get this class(%s)\n", real_class_name);
                        return FALSE;
                    }
                }

                cvalue1 = *(uchar*)pc;  // existance of result
                pc += sizeof(uchar);

                cvalue2 = *(uchar*)pc;  // super
                pc += sizeof(uchar);

                cvalue3 = *(uchar*)pc;   // class method
                pc += sizeof(uchar);

                cvalue4 = *(uchar*)pc;   // object kind
                pc += sizeof(uchar);

                if(cvalue4 == INVOKE_METHOD_KIND_OBJECT) {
                    ovalue1 = (gCLStackPtr-ivalue2-1)->mObjectValue;   // get self

                    if(cvalue2) { // super
                        klass1 = get_super(CLCLASS(ovalue1));
                    }
                    else {
                        klass1 = CLCLASS(ovalue1);
                    }
                }
                else {
                    ivalue3 = *(uint*)pc;       // class name
                    pc += sizeof(int);

                    real_class_name = CONS_str((*constant), ivalue3);
                    klass1 = cl_get_class(real_class_name);

                    if(klass1 == NULL) {
                        vm_error("can't get this class(%s)\n", real_class_name);
                        return FALSE;
                    }
                }

#ifdef VM_DEBUG
vm_debug("klass1 %s\n", REAL_CLASS_NAME(klass1));
vm_debug("method name1 %s\n", CONS_str((*constant), ivalue1));
vm_debug("method num params %d\n", ivalue2);
#endif

                method = get_virtual_method_with_params(klass1, CONS_str((*constant), ivalue1), params, ivalue2, &klass2, cvalue3);
                if(method == NULL) {
                    vm_error("can't get this method which name is %s\n", CONS_str((*constant), ivalue1));
                    return FALSE;
                }

                if(!cl_excute_method(method, &klass2->mConstPool, cvalue1)) {
                    return FALSE;
                }
                break;

            case OP_INVOKE_INHERIT:
                pc++;

                ivalue1 = *(uint*)pc;           // real class name offset
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    vm_error("can't get this class(%s)\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *(uint*)pc;             // method index
                pc += sizeof(int);

                cvalue2 = *(uchar*)pc;          // existance of result
                pc += sizeof(uchar);

                method = klass1->mMethods + ivalue2;
#ifdef VM_DEBUG
vm_debug("klass1 %s\n", REAL_CLASS_NAME(klass1));
vm_debug("method name (%s)\n", METHOD_NAME(klass1, ivalue2));
#endif
                if(!cl_excute_method(method, &klass1->mConstPool, cvalue2)) {
                    return FALSE;
                }
                break;

            case OP_IADD:
                ivalue1 = (gCLStackPtr-1)->mIntValue + (gCLStackPtr-2)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IADD %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                pc++;
                break;

            case OP_SADD:
                ovalue1 = (gCLStackPtr-2)->mObjectValue;
                ovalue2 = (gCLStackPtr-1)->mObjectValue;

                gCLStackPtr-=2;

#ifdef VM_DEBUG
printf("CLALEN(ovalue1) %d CLALEN(ovalue2) %d\n", CLSTRING_LEN(ovalue1), CLSTRING_LEN(ovalue2));
#endif
                ivalue1 = CLSTRING_LEN(ovalue1) -1;  // string length of ovalue1
                ivalue2 = CLSTRING_LEN(ovalue2) -1;  // string length of ovalue2

                str = MALLOC(sizeof(wchar_t)*(ivalue1 + ivalue2 + 1));

#ifdef VM_DEBUG
wprintf(L"ovalue1 (%ls)\n", CLSTRING_START(ovalue1));
wprintf(L"ovalue2 (%ls)\n", CLSTRING_START(ovalue2));
#endif

                wcscpy(str, CLSTRING_START(ovalue1));
                wcscat(str, CLSTRING_START(ovalue2));

                ovalue3 = create_string_object(str, ivalue1 + ivalue2);

#ifdef VM_DEBUG
wprintf(L"str (%ls)\n", str);
#endif

                gCLStackPtr->mObjectValue = ovalue3;
#ifdef VM_DEBUG
vm_debug("OP_SADD %ld\n", ovalue3);
#endif
                gCLStackPtr++;
                pc++;

                FREE(str);
                break;

            case OP_POP:
#ifdef VM_DEBUG
vm_debug("OP_POP\n");
#endif
                gCLStackPtr--;
                pc++;
                break;

            case OP_POP_N:
                pc++;
                ivalue1 = *pc;
                pc += sizeof(int);
#ifdef VM_DEBUG
vm_debug("OP_POP %d\n", ivalue1);
#endif

                gCLStackPtr -= ivalue1;
                break;


            default:
                vm_error("unexpected error at cl_vm\n");
                exit(1);
        }
#ifdef VM_DEBUG
show_stack(gCLStack, gCLStackPtr, top_of_stack, var);
show_heap();
#endif
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
        vm_error("overflow stack size\n");
        return FALSE;
    }

    return cl_vm(code, constant, lvar);
}

BOOL cl_excute_method(sCLMethod* method, sConst* constant, BOOL result_existance)
{
    const int real_param_num = method->mNumParams + (method->mFlags & CL_CLASS_METHOD ? 0:1);

    if(method->mFlags & CL_NATIVE_METHOD) {
        MVALUE* lvar = gCLStackPtr - real_param_num;

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        BOOL result = method->mNativeMethod(gCLStackPtr, lvar);

        if(result_existance) {
            MVALUE* mvalue = gCLStackPtr-1;
            gCLStackPtr = lvar;
            *gCLStackPtr = *mvalue;
            gCLStackPtr++;
        }
        else {
            gCLStackPtr = lvar;
        }

        return result;
    }
    else {
        MVALUE* lvar = gCLStackPtr - real_param_num;
        gCLStackPtr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        BOOL result = cl_vm(&method->mByteCodes, constant, lvar);

        if(result_existance) {
            MVALUE* mvalue = gCLStackPtr-1;
            gCLStackPtr = lvar;
            *gCLStackPtr = *mvalue;
            gCLStackPtr++;
        }
        else {
            gCLStackPtr = lvar;
        }

        return result;
    }
}

