#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>

MVALUE* gCLStack;
int gCLStackSize;
MVALUE* gCLStackPtr;

#ifdef VM_DEBUG
FILE* gDebugLog;
#endif

void cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    heap_init(heap_size, handle_size);

    class_init(load_foundamental_class);
    parser_init(load_foundamental_class);

#ifdef VM_DEBUG
    gDebugLog = fopen("debug.log", "w");
#endif
}

void cl_final()
{
#ifdef VM_DEBUG
    fclose(gDebugLog);
#endif

    parser_final();
    class_final();

    heap_final();

    FREE(gCLStack);
}

// get under shelter the object
void push_object(CLObject object)
{
    gCLStackPtr->mObjectValue = object;
    gCLStackPtr++;
}

// remove the object from stack
CLObject pop_object()
{
    gCLStackPtr--;
    return gCLStackPtr->mObjectValue;
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


static unsigned char visible_control_character(unsigned char c)
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
    int i;

    puts("show_constants -+-");
    for(i=0; i<constant->mLen; i++) {
        printf("[%d].%d(%c) ", i, constant->mConst[i], visible_control_character(constant->mConst[i]));
    }
    puts("");
}

#ifdef VM_DEBUG
void vm_debug(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(gDebugLog, "%s", msg2);
}

static void show_stack(MVALUE* stack, MVALUE* stack_ptr, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_debug("stack_ptr %d top_of_stack %d var %d\n", (int)(stack_ptr - stack), (int)(top_of_stack - stack), (int)(var - stack));

    for(i=0; i<10; i++) {
        if(stack + i == var) {
            if(stack + i == stack_ptr) {
                vm_debug("->v-- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
            else {
                vm_debug("  v-- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
        }
        else if(stack + i == top_of_stack) {
            if(stack + i == stack_ptr) {
                vm_debug("->--- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
            else {
                vm_debug("  --- stack[%d] value %d\n", i, stack[i].mIntValue);
            }
        }
        else if(stack + i == stack_ptr) {
            vm_debug("->    stack[%d] value %d\n", i, stack[i].mIntValue);
        }
        else {
            vm_debug("      stack[%d] value %d\n", i, stack[i].mIntValue);
        }
    }
}
#endif

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var, sCLNodeType* type_)
{
    int ivalue1, ivalue2, ivalue3, ivalue4;
    char cvalue1, cvalue2, cvalue3, cvalue4;
    float fvalue1, fvalue2, fvalue3;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr;

    sCLClass* klass1, *klass2, *klass3;
    sCLMethod* method;
    sCLField* field;
    char* p;
    wchar_t* str;
    char* real_class_name;

    sCLNodeType params[CL_METHOD_PARAM_MAX];
    unsigned int num_params;

    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];

    int i, j;

    sCLNodeType generics_type;

    MVALUE* top_of_stack = gCLStackPtr;
    unsigned char* pc = code->mCode;

    while(pc - code->mCode < code->mLen) {
//printf("pc - code->mCode %d code->mLen %d\n", (int)(pc - code->mCode), code->mLen);
        switch(*pc) {
            case OP_LDC:
#ifdef VM_DEBUG
vm_debug("OP_LDC\n");
#endif
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

                    case CONSTANT_FLOAT:
                        gCLStackPtr->mFloatValue = *(float*)p;
#ifdef VM_DEBUG
vm_debug("OP_LDC float value %f\n", gCLStackPtr->mFloatValue);
#endif
                        break;

                    case CONSTANT_WSTRING: {
                        unsigned int len = *(unsigned int*)p;
                        p+=sizeof(unsigned int);
                        gCLStackPtr->mObjectValue = create_string_object(gStringType.mClass, (wchar_t*)p, len);
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
#ifdef VM_DEBUG
vm_debug("OP_LOAD\n");
#endif
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
#ifdef VM_DEBUG
vm_debug("OP_STORE\n");
#endif
                pc++;
                ivalue1 = *pc;
                pc += sizeof(int);

#ifdef VM_DEBUG
vm_debug("OP_STORE %d\n", ivalue1);
#endif
                var[ivalue1] = *(gCLStackPtr-1);
                break;

            case OP_LDFIELD:
#ifdef VM_DEBUG
vm_debug("OP_LDFIELD\n");
#endif
                pc++;

                ivalue1 = *(unsigned int*)pc;                   // field index
                pc += sizeof(int);

                ovalue1 = (gCLStackPtr-1)->mObjectValue;
                gCLStackPtr--;

                *gCLStackPtr = CLOBJECT(ovalue1)->mFields[ivalue1];
#ifdef VM_DEBUG
vm_debug("LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);
#endif
                gCLStackPtr++;
                break;

            case OP_SRFIELD:
#ifdef VM_DEBUG
vm_debug("OP_SRFIELD\n");
#endif
                pc++;

                ivalue1 = *(int*)pc;                        // field index
                pc += sizeof(int);

                ovalue1 = (gCLStackPtr-2)->mObjectValue;    // target object
                mvalue1 = gCLStackPtr-1;                    // right value

                CLOBJECT(ovalue1)->mFields[ivalue1] = *mvalue1;
#ifdef VM_DEBUG
vm_debug("SRFIELD object %d field num %d value %d\n", (int)ovalue1, ivalue1, mvalue1->mIntValue);
#endif
                gCLStackPtr-=2;

                *gCLStackPtr = *mvalue1;
                gCLStackPtr++;
                break;

            case OP_LD_STATIC_FIELD: 
#ifdef VM_DEBUG
vm_debug("OP_LD_STATIC_FIELD\n");
#endif
                pc++;

                ivalue1 = *(unsigned int*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(unsigned int*)pc;  // field index
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get class named %s\n", real_class_name);
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                *gCLStackPtr = field->uValue.mStaticField;
#ifdef VM_DEBUG
vm_debug("LD_STATIC_FIELD %d\n", field->uValue.mStaticField.mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_SR_STATIC_FIELD:
#ifdef VM_DEBUG
vm_debug("OP_SR_STATIC_FIELD\n");
#endif
                pc++;

                ivalue1 = *(unsigned int*)pc;   // class name
                pc += sizeof(int);
                
                ivalue2 = *(unsigned int*)pc;  // field index
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                field->uValue.mStaticField = *(gCLStackPtr-1);
#ifdef VM_DEBUG
vm_debug("OP_SR_STATIC_FIELD value %d\n", (gCLStackPtr-1)->mIntValue);
#endif
                break;

            case OP_NEW_OBJECT:
#ifdef VM_DEBUG
vm_debug("NEW_OBJECT\n");
#endif
                pc++;

                ivalue1 = *((int*)pc);
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ovalue1 = create_object(klass1);

                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                break;

            case OP_NEW_STRING:
#ifdef VM_DEBUG
vm_debug("OP_NEW_STRING\n");
#endif
                pc++;

                ivalue1 = *((int*)pc);    // real class name
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                gCLStackPtr->mObjectValue = create_string_object(klass1, L"", 0);
                gCLStackPtr++;
                break;

            case OP_NEW_ARRAY:
#ifdef VM_DEBUG
vm_debug("OP_NEW_ARRAY\n");
#endif
                pc++;

                ivalue1 = *((int*)pc);
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *(unsigned int*)pc;   // number of elements
                pc += sizeof(int);

                stack_ptr = gCLStackPtr - ivalue2;
                for(i=0; i<ivalue2; i++) {
                    objects[i] = *stack_ptr++;
                }

                ovalue1 = create_array_object(klass1, objects, ivalue2);
#ifdef VM_DEBUG
vm_debug("new array %d\n", ovalue1);
#endif

                gCLStackPtr -= ivalue2;
                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                break;

            case OP_NEW_HASH:
#ifdef VM_DEBUG
vm_debug("OP_NEW_HASH\n");
#endif
                pc++;

                ivalue1 = *((int*)pc);
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *(unsigned int*)pc;   // number of elements
                pc += sizeof(int);

                gCLStackPtr->mObjectValue = create_hash_object(klass1, NULL, NULL, 0);
                gCLStackPtr++;
                break;

            case OP_INVOKE_METHOD:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_METHOD\n");
#endif
                pc++;

                /// type data ///
                memset(&generics_type, 0, sizeof(generics_type));

                cvalue1 = *(unsigned char*)pc;      // generics type num
                pc += sizeof(unsigned char);

                generics_type.mGenericsTypesNum = cvalue1;

                for(i=0; i<cvalue1; i++) {
                    ivalue1 = *(unsigned int*)pc;    // generics type offset
                    pc += sizeof(unsigned int);

                    p = constant->mConst + ivalue1;
                    p++; // throw constant type away
                    p+=sizeof(unsigned int);  // throw length of string away
                    real_class_name = p;    // real class name of a param

                    generics_type.mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, type_);

                    if(generics_type.mGenericsTypes[i] == NULL) {
                        vm_error("can't get a class named %s\n", real_class_name);
                        return FALSE;
                    }
                }

                /// method data ///
                ivalue1 = *(unsigned int*)pc;       // method name offset
                pc += sizeof(unsigned int);

                ivalue2 = *(unsigned int*)pc;       // method num params
                pc += sizeof(unsigned int);
                
                memset(params, 0, sizeof(params));

                for(i=0; i<ivalue2; i++) {
                    ivalue3 = *(unsigned int*)pc;
                    pc += sizeof(unsigned int);

                    p = constant->mConst + ivalue3;
                    p++; // throw constant type away
                    p+=sizeof(unsigned int);  // throw length of string away
                    real_class_name = p;    // real class name of a param

                    params[i].mClass = cl_get_class_with_generics(real_class_name, &generics_type);

                    if(params[i].mClass == NULL) {
                        vm_error("can't get a class named %s\n", real_class_name);
                        return FALSE;
                    }

                    cvalue1 = *(char*)pc;
                    pc+=sizeof(char);

                    params[i].mGenericsTypesNum = cvalue1;

                    for(j=0; j<cvalue1; j++) {
                        ivalue4 = *(unsigned int*)pc;
                        pc += sizeof(unsigned int);

                        p = constant->mConst + ivalue4;
                        p++; // throw constant type away
                        p+=sizeof(unsigned int);  // throw length of string away
                        real_class_name = p;    // real class name of a param

                        params[i].mGenericsTypes[j] = cl_get_class_with_generics(real_class_name, &generics_type);

                        if(params[i].mGenericsTypes[j] == NULL) {
                            vm_error("can't get a class named %s\n", real_class_name);
                            return FALSE;
                        }
                    }
                }

                cvalue1 = *(unsigned char*)pc;  // existance of result
                pc += sizeof(unsigned char);

                cvalue2 = *(unsigned char*)pc;  // super
                pc += sizeof(unsigned char);

                cvalue3 = *(unsigned char*)pc;   // class method
                pc += sizeof(unsigned char);

                cvalue4 = *(unsigned char*)pc;   // object kind
                pc += sizeof(unsigned char);

                if(cvalue4 == INVOKE_METHOD_KIND_OBJECT) {
                    ovalue1 = (gCLStackPtr-ivalue2-1)->mObjectValue;   // get self

                    if(cvalue2) { // super
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;

                        if(klass1 == NULL) {
                            vm_error("can't get a class from object #%lu\n", ovalue1);
                            return FALSE;
                        }

                        klass1 = get_super(klass1);

                        if(klass1 == NULL) {
                            vm_error("can't get a super class from object #%lu\n", ovalue1);
                            return FALSE;
                        }
                    }
                    else {
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;

                        if(klass1 == NULL) {
                            vm_error("can't get a class from object #%lu\n", ovalue1);
                            return FALSE;
                        }
                    }
                }
                else {
                    ivalue3 = *(unsigned int*)pc;       // class name
                    pc += sizeof(int);

                    real_class_name = CONS_str((*constant), ivalue3);
                    klass1 = cl_get_class_with_generics(real_class_name, &generics_type);

                    if(klass1 == NULL) {
                        vm_error("can't get a class named %s\n", real_class_name);
                        return FALSE;
                    }
                }

#ifdef VM_DEBUG
vm_debug("klass1 %s\n", REAL_CLASS_NAME(klass1));
vm_debug("method name1 %s\n", CONS_str((*constant), ivalue1));
vm_debug("method num params %d\n", ivalue2);
#endif
                method = get_virtual_method_with_params(klass1, CONS_str((*constant), ivalue1), params, ivalue2, &klass2, cvalue3, &generics_type);
                if(method == NULL) {
                    vm_error("can't get a method named %s.%s\n", REAL_CLASS_NAME(klass1), CONS_str((*constant), ivalue1));
                    return FALSE;
                }
#ifdef VM_DEBUG
vm_debug("do call (%s.%s)\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
for(i=0; i<generics_type.mGenericsTypesNum; i++) {
vm_debug("with %s\n", REAL_CLASS_NAME(generics_type.mGenericsTypes[i]));
}
#endif

                if(!cl_excute_method(method, &klass2->mConstPool, cvalue1, &generics_type)) {
                    return FALSE;
                }
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_METHOD(%s.%s) end\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
#endif
                break;

            case OP_INVOKE_INHERIT:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_INHERIT\n");
#endif
                pc++;

                ivalue1 = *(unsigned int*)pc;           // real class name offset
                pc += sizeof(int);

                real_class_name = CONS_str((*constant), ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *(unsigned int*)pc;             // method index
                pc += sizeof(int);

                cvalue2 = *(unsigned char*)pc;          // existance of result
                pc += sizeof(unsigned char);

                method = klass1->mMethods + ivalue2;
#ifdef VM_DEBUG
vm_debug("klass1 %s\n", REAL_CLASS_NAME(klass1));
vm_debug("method name (%s)\n", METHOD_NAME(klass1, ivalue2));
#endif
                if(!cl_excute_method(method, &klass1->mConstPool, cvalue2, NULL)) {
                    return FALSE;
                }
                break;

            case OP_RETURN:
                return TRUE;

            case OP_IADD:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_IADD\n");
#endif
                ivalue1 = (gCLStackPtr-2)->mIntValue + (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IADD %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FADD:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_FADD\n");
#endif
                fvalue1 = (gCLStackPtr-2)->mFloatValue + (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FADD %d\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_SADD:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_SADD\n");
#endif
                ovalue1 = (gCLStackPtr-2)->mObjectValue;
                ovalue2 = (gCLStackPtr-1)->mObjectValue;

                gCLStackPtr-=2;

#ifdef VM_DEBUG
vm_debug("CLALEN(ovalue1) %d CLALEN(ovalue2) %d\n", CLSTRING(ovalue1)->mLen, CLSTRING(ovalue2)->mLen);
#endif
                ivalue1 = CLSTRING(ovalue1)->mLen -1;  // string length of ovalue1
                ivalue2 = CLSTRING(ovalue2)->mLen -1;  // string length of ovalue2

                str = MALLOC(sizeof(wchar_t)*(ivalue1 + ivalue2 + 1));


                wcscpy(str, CLSTRING(ovalue1)->mChars);
                wcscat(str, CLSTRING(ovalue2)->mChars);

                ovalue3 = create_string_object(CLOBJECT_HEADER(ovalue1)->mClass, str, ivalue1 + ivalue2);

                gCLStackPtr->mObjectValue = ovalue3;
#ifdef VM_DEBUG
vm_debug("OP_SADD %ld\n", ovalue3);
#endif
                gCLStackPtr++;

                FREE(str);
                break;

            case OP_ISUB:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_ISUB\n");
#endif
                ivalue1 = (gCLStackPtr-2)->mIntValue - (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_ISUB %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FSUB:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_FSUB\n");
#endif
                fvalue1 = (gCLStackPtr-2)->mFloatValue - (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FSUB %d\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IMULT:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_IMULT\n");
#endif
                ivalue1 = (gCLStackPtr-2)->mIntValue * (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IMULT %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FMULT:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_FMULT\n");
#endif
                fvalue1 = (gCLStackPtr-2)->mFloatValue * (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FMULT %d\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IDIV:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_IDIV\n");
#endif
                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("division by zero");
                    return FALSE;
                }

                ivalue1 = (gCLStackPtr-2)->mIntValue / (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IDIV %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FDIV:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_FDIV\n");
#endif

                if((gCLStackPtr-2)->mFloatValue == 0.0) {
                    vm_error("division by zero");
                    return FALSE;
                }

                fvalue1 = (gCLStackPtr-2)->mFloatValue / (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FDIV %d\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IMOD:
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_IMOD\n");
#endif
                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("remainder by zero");
                    return FALSE;
                }

                ivalue1 = (gCLStackPtr-2)->mIntValue % (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IMOD %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
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

            case OP_LOGICAL_DENIAL:
                pc++;

                ivalue1 = (gCLStackPtr-1)->mIntValue;
                ivalue1 = !ivalue1;

                (gCLStackPtr-1)->mIntValue = ivalue1;
                break;

            case OP_COMPLEMENT:
                pc++;

                ivalue1 = (gCLStackPtr-1)->mIntValue;
                ivalue1 = ~ivalue1;

                (gCLStackPtr-1)->mIntValue = ivalue1;
                break;

            default:
                vm_error("invalid op code(%d)\n", *pc);
                vm_error("unexpected error at cl_vm\n");
                exit(1);
        }
#ifdef VM_DEBUG
show_stack(gCLStack, gCLStackPtr, top_of_stack, var);
show_heap();
#endif
    }

    return TRUE;
}

BOOL cl_main(sByteCode* code, sConst* constant, unsigned int lv_num, unsigned int max_stack)
{
    MVALUE* lvar;
    BOOL result;

    gCLStackPtr = gCLStack;
    lvar = gCLStack;
    gCLStackPtr += lv_num;

    if(gCLStackPtr + max_stack > gCLStack + gCLStackSize) {
        vm_error("overflow stack size\n");
        return FALSE;
    }

    result = cl_vm(code, constant, lvar, NULL);

    return result;
}

BOOL cl_excute_method(sCLMethod* method, sConst* constant, BOOL result_existance, sCLNodeType* type_)
{
    int real_param_num;
    
    real_param_num = method->mNumParams + (method->mFlags & CL_CLASS_METHOD ? 0:1);

    if(method->mFlags & CL_NATIVE_METHOD) {
        MVALUE* lvar;
        BOOL result;

        lvar = gCLStackPtr - real_param_num;

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        result = method->uCode.mNativeMethod(&gCLStackPtr, lvar);

        if(result_existance) {
            MVALUE* mvalue;

            mvalue = gCLStackPtr-1;
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
        MVALUE* lvar;
        BOOL result;

        lvar = gCLStackPtr - real_param_num;
        if(method->mNumLocals - real_param_num > 0) {
            gCLStackPtr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
        }

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        result = cl_vm(&method->uCode.mByteCodes, constant, lvar, type_);

        if(result_existance) {
            MVALUE* mvalue;

            mvalue = gCLStackPtr-1;
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

