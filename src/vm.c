#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

MVALUE* gCLStack;
int gCLStackSize;
MVALUE* gCLStackPtr;

#ifdef VM_DEBUG
FILE* gDebugLog;
#endif

struct sStackFrameStruct {
    MVALUE* mBaseOfBlockStack;
    MVALUE* mLocalVar;
    int mNumVars;
};

typedef struct sStackFrameStruct sStackFrame;

sStackFrame gStackFrame[CL_BLOCK_NEST_MAX];
int gNumStackFrame;

BOOL push_stack_frame(MVALUE* base_of_block_stack, MVALUE* lvar, int num_vars)
{
    gStackFrame[gNumStackFrame].mBaseOfBlockStack = base_of_block_stack;
    gStackFrame[gNumStackFrame].mLocalVar = lvar;
    gStackFrame[gNumStackFrame].mNumVars = num_vars;
    gNumStackFrame++;
    if(gNumStackFrame >= CL_BLOCK_NEST_MAX) {
        vm_error("overflow stackframe nest\n");
        return FALSE;
    }

    return TRUE;
}

void pop_stack_frame()
{
    gNumStackFrame--;
}

BOOL cl_init(int global_size, int stack_size, int heap_size, int handle_size, BOOL load_foundamental_class)
{
    gCLStack = MALLOC(sizeof(MVALUE)* stack_size);
    gCLStackSize = stack_size;
    gCLStackPtr = gCLStack;

    memset(gCLStack, 0, sizeof(MVALUE) * stack_size);

    gNumStackFrame = 0;
    memset(gStackFrame, 0, sizeof(gStackFrame));

    heap_init(heap_size, handle_size);

    if(!class_init(load_foundamental_class)) {
        return FALSE;
    }
    parser_init(load_foundamental_class);

#ifdef VM_DEBUG
    gDebugLog = fopen("debug.log", "w");
#endif

    return TRUE;
}

void cl_final()
{
#ifdef VM_DEBUG
    fclose(gDebugLog);
#endif

    heap_final();
    parser_final();
    class_final();

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

void vm_error(char* msg, ...)
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

    cl_print("show_constants -+-\n");
    for(i=0; i<constant->mLen; i++) {
        cl_print("[%d].%d(%c) ", i, constant->mConst[i], visible_control_character(constant->mConst[i]));
    }
    cl_print("\n");
}

#ifdef VM_DEBUG
void vm_debug(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    //fprintf(gDebugLog, "%s", msg2);
    fprintf(stderr, "%s", msg2);
}

static void show_stack(MVALUE* stack, MVALUE* stack_ptr, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_debug("stack_ptr %d top_of_stack %d var %d\n", (int)(stack_ptr - stack), (int)(top_of_stack - stack), (int)(var - stack));

    for(i=0; i<20; i++) {
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

static BOOL get_node_type_from_bytecode(int** pc, sConst* constant, sCLNodeType* type, sCLNodeType* generics_type)
{
    unsigned int ivalue1, ivalue2;
    char* p;
    char* real_class_name;
    char cvalue1;
    int i;
    int size;

    ivalue1 = **pc;                                                      // real class name param offset
    (*pc)++;

    real_class_name = CONS_str(constant, ivalue1);

    type->mClass = cl_get_class_with_generics(real_class_name, generics_type);

    if(type->mClass == NULL) {
        vm_error("can't get a class named %s\n", real_class_name);
        return FALSE;
    }

    type->mGenericsTypesNum = **pc;
    (*pc)++;

    for(i=0; i<type->mGenericsTypesNum; i++) {
        ivalue2 = **pc;                     // real class name offset
        (*pc)++;

        real_class_name = CONS_str(constant, ivalue2);
        type->mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, generics_type);

        if(type->mGenericsTypes[i] == NULL) {
            vm_error("can't get a class named %s\n", real_class_name);
            return FALSE;
        }
    }

    return TRUE;
}

/// virtual machine result ///
typedef int VMResult;

#define VMR_SUCCESS 0x0100
#define VMR_ERROR 0x0200
#define VMR_RETURN 0x0300
#define VMR_REVERT 0x0400
#define VMR_RETURN_COUNT 0x00ff
#define VMR_TYPE 0xff00
#define VMR_EXISTANCE_RESULT 0x10000

static VMResult excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, BOOL static_method_block);
static VMResult excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params);

static VMResult cl_vm(sByteCode* code, sConst* constant, MVALUE* var, sCLNodeType* type_)
{
    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11, ivalue12;
    char cvalue1;
    float fvalue1;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr;

    sCLClass* klass1, *klass2, *klass3;
    sCLMethod* method;
    sCLField* field;
    wchar_t* str;
    char* real_class_name;
    sCLNodeType params[CL_METHOD_PARAM_MAX];
    sCLNodeType params2[CL_METHOD_PARAM_MAX];
    sCLNodeType type2;
    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];
    int num_vars;
    int i, j;
    sCLNodeType generics_type;
    MVALUE* top_of_stack;
    int* pc;

    VMResult result;
    int return_count;

    pc = code->mCode;
    top_of_stack = gCLStackPtr;
    num_vars = gCLStackPtr - var;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_LDCINT:
                pc++;

                ivalue1 = *pc;                      // constant pool offset
                pc++;

                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_LDC int value %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_LDCFLOAT:
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

                gCLStackPtr->mFloatValue = *(float*)(constant->mConst + ivalue1);
#ifdef VM_DEBUG
vm_debug("OP_LDC float value %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_LDCWSTR: {
                int size;
                wchar_t* wcs;

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                wcs = (wchar_t*)(constant->mConst + ivalue1);
                size = wcslen(wcs);

                gCLStackPtr->mObjectValue = create_string_object(gStringType.mClass, wcs, size);
#ifdef VM_DEBUG
vm_debug("OP_LDC string object %ld\n", gCLStackPtr->mObjectValue);
#endif
                gCLStackPtr++;
                }
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
                pc++;

                ivalue1 = *pc;
                pc++;
#ifdef VM_DEBUG
vm_debug("OP_LOAD %d\n", ivalue1);
#endif

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
                pc++;

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

                ivalue1 = *pc;                                      // field index
                pc++;

                ovalue1 = (gCLStackPtr-1)->mObjectValue;
                gCLStackPtr--;

#ifdef VM_DEBUG
vm_debug("LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);
#endif

                *gCLStackPtr = CLUSEROBJECT(ovalue1)->mFields[ivalue1];
                gCLStackPtr++;
                break;

            case OP_SRFIELD:
                pc++;

                ivalue1 = *pc;                              // field index
                pc++;

                ovalue1 = (gCLStackPtr-2)->mObjectValue;    // target object
                mvalue1 = gCLStackPtr-1;                    // right value

                CLUSEROBJECT(ovalue1)->mFields[ivalue1] = *mvalue1;
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

                ivalue1 = *pc;                                                  // class name
                pc++;

                ivalue2 = *pc;                                                  // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get class named %s\n", real_class_name);
                    return VMR_ERROR;
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

                ivalue1 = *pc;                                                  // class name
                pc++;

                ivalue2 = *pc;                                                  // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
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

                ivalue1 = *pc;                      // class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                if(!create_user_object(klass1, &ovalue1)) {
                    return VMR_ERROR;
                }

                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                break;

            case OP_NEW_STRING:
#ifdef VM_DEBUG
vm_debug("OP_NEW_STRING\n");
#endif
                pc++;

                ivalue1 = *pc;                              // real class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                gCLStackPtr->mObjectValue = create_string_object(klass1, L"", 0);
                gCLStackPtr++;
                break;

            case OP_NEW_ARRAY:
#ifdef VM_DEBUG
vm_debug("OP_NEW_ARRAY\n");
#endif
                pc++;

                ivalue1 = *pc;
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                ivalue2 = *pc;                              // number of elements
                pc++;

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

                ivalue1 = *pc;                                  // class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                ivalue1 = *pc;                                  // number of elements
                pc++;

                gCLStackPtr->mObjectValue = create_hash_object(klass1, NULL, NULL, 0);
                gCLStackPtr++;
                break;

            case OP_NEW_BLOCK: {
                char* const_buf;
                int* code_buf;
                int constant2_len;
                int code2_len;

#ifdef VM_DEBUG
vm_debug("OP_NEW_BLOCK\n");
#endif

                pc++;

                ivalue1 = *pc;                          // block type class
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                ivalue2 = *pc;                      // max stack
                pc++;

                ivalue3 = *pc;                      // num locals
                pc++;

                ivalue4 = *pc;                      // num params
                pc++;

#ifdef VM_DEBUG
vm_debug("OP_NEW_BLOCK max stack %d num locals %d num params %d\n", ivalue2, ivalue3, ivalue4);
#endif

                constant2_len = *pc;                // constant pool len
                pc++;
                
                ivalue5 = *pc;                      // constant pool offset
                pc++;

                const_buf = CONS_str(constant, ivalue5);

                code2_len = *pc;                    // byte code len
                pc++;

                ivalue6 = *pc;                      // byte code offset
                pc++;

                code_buf = (int*)CONS_str(constant, ivalue6);

                ovalue1 = create_block(klass1, const_buf, constant2_len, code_buf, code2_len, ivalue2, ivalue3, ivalue4, var, num_vars);

                gCLStackPtr->mObjectValue = ovalue1;
                gCLStackPtr++;
                }
                break;

            case OP_INVOKE_METHOD:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_METHOD\n");
#endif
                pc++;

                /// type data ///
                memset(&generics_type, 0, sizeof(generics_type));

                cvalue1 = (char)*pc;      // generics type num
                pc++;

                generics_type.mGenericsTypesNum = cvalue1;

                for(i=0; i<cvalue1; i++) {
                    ivalue1 = *pc;                              // generics type offset
                    pc++;

                    real_class_name = CONS_str(constant, ivalue1);    // real class name of a param
                    generics_type.mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, type_);

                    if(generics_type.mGenericsTypes[i] == NULL) {
                        vm_error("can't get a class named %s\n", real_class_name);
                        return VMR_ERROR;
                    }
                }

                /// method data ///
                ivalue1 = *pc;                                                      // method name offset
                pc++;

                ivalue2 = *pc;                                                      // method num params
                pc++;
                
                memset(params, 0, sizeof(params));

                for(i=0; i<ivalue2; i++) {
                    if(!get_node_type_from_bytecode(&pc, constant, &params[i], &generics_type)) {
                        return VMR_ERROR;
                    }
                }

                memset(params2, 0, sizeof(params2));

                ivalue11 = *pc;                         // existance of block
                pc++;

                ivalue3 = *pc;                           // method block num params
                pc++; 

                for(i=0; i<ivalue3; i++) {          // block params
                    if(!get_node_type_from_bytecode(&pc, constant, &params2[i], &generics_type)) {
                        return VMR_ERROR;
                    }
                }

                ivalue4 = *pc;                          // the existance of block result
                pc++;

                if(ivalue4) {
                    memset(&type2, 0, sizeof(type2));   // block result type
                    if(!get_node_type_from_bytecode(&pc, constant, &type2, &generics_type)) {
                        return VMR_ERROR;
                    }
                }
                else {
                    memset(&type2, 0, sizeof(type2));   // block result type
                }

                ivalue5 = *pc;  // existance of result
                pc++;

                ivalue6 = *pc;  // super
                pc++;

                ivalue7 = *pc;   // class method
                pc++;

                ivalue8 = *pc;                           // method num block
                pc++;

                ivalue12 = *pc;                          // num params
                pc++;

ASSERT(ivalue11 == 1 && type2.mClass != NULL || ivalue11 == 0);

                ivalue9 = *pc;   // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_OBJECT) {
                    ovalue1 = (gCLStackPtr-ivalue2-ivalue8-1)->mObjectValue;   // get self

                    if(ivalue6) { // super
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;

                        if(klass1 == NULL) {
                            vm_error("can't get a class from object #%lu\n", ovalue1);
                            return VMR_ERROR;
                        }

                        klass1 = get_super(klass1);

                        if(klass1 == NULL) {
                            vm_error("can't get a super class from object #%lu\n", ovalue1);
                            return VMR_ERROR;
                        }
                    }
                    else {
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;

                        if(klass1 == NULL) {
                            vm_error("can't get a class from object #%lu\n", ovalue1);
                            return VMR_ERROR;
                        }
                    }
                }
                else {
                    ivalue10 = *pc;                  // class name
                    pc++;

                    real_class_name = CONS_str(constant, ivalue10);
                    klass1 = cl_get_class_with_generics(real_class_name, &generics_type);

                    if(klass1 == NULL) {
                        vm_error("can't get a class named %s\n", real_class_name);
                        return VMR_ERROR;
                    }
                }

                method = get_virtual_method_with_params(klass1, CONS_str(constant, ivalue1), params, ivalue2, &klass2, ivalue7, &generics_type, ivalue11, ivalue3, params2, &type2);
                if(method == NULL) {
                    vm_error("can't get a method named %s.%s\n", REAL_CLASS_NAME(klass1), CONS_str(constant, ivalue1));
                    return VMR_ERROR;
                }
#ifdef VM_DEBUG
vm_debug("do call (%s.%s)\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
for(i=0; i<generics_type.mGenericsTypesNum; i++) {
vm_debug("with %s\n", REAL_CLASS_NAME(generics_type.mGenericsTypes[i]));
}
#endif

                result = excute_method(method, klass2, &klass2->mConstPool, ivalue5, &generics_type, ivalue12);

                switch(result & VMR_TYPE) {
                    case VMR_SUCCESS:
                        break;

                    case VMR_ERROR:
                        return VMR_ERROR;

                    case VMR_RETURN:
                        return_count = result & VMR_RETURN_COUNT;
                        if(return_count == 1) {
                            return_count--;
                            return VMR_RETURN | return_count | (result & VMR_EXISTANCE_RESULT);
                        }
                        else {
                            return_count--;
                            if(return_count > 0) {
                                return VMR_RETURN | return_count | (result & VMR_EXISTANCE_RESULT);
                            }
                        }
                        break;

                    case VMR_REVERT:
                        fprintf(stderr, "unexpected error on excute_method result\n");
                        exit(0);
                }

#ifdef VM_DEBUG
vm_debug("OP_INVOKE_METHOD(%s.%s) end\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
#endif
#ifdef VM_DEBUG
vm_debug("after pc-code->mCode %d code->mLen %d\n", pc - code->mCode, code->mLen);
#endif
                break;

            case OP_INVOKE_INHERIT:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_INHERIT\n");
#endif
                pc++;

                ivalue1 = *pc;                  // real class name offset
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    vm_error("can't get a class named %s\n", real_class_name);
                    return VMR_ERROR;
                }

                ivalue2 = *pc;                  // method index
                pc++;

                ivalue3 = *pc;                  // existance of result
                pc++;

                ivalue4 = *pc;                  // num params
                pc++;

                method = klass1->mMethods + ivalue2;
#ifdef VM_DEBUG
vm_debug("klass1 %s\n", REAL_CLASS_NAME(klass1));
vm_debug("method name (%s)\n", METHOD_NAME(klass1, ivalue2));
#endif
                result = excute_method(method, klass1, &klass1->mConstPool, ivalue3, NULL, ivalue4);

                switch(result & VMR_TYPE) {
                    case VMR_SUCCESS:
                        break;

                    case VMR_ERROR:
                        return VMR_ERROR;

                    case VMR_RETURN:
                        return_count = result & VMR_RETURN_COUNT;
                        if(return_count == 1) {
                            return_count--;
                            return VMR_RETURN | return_count | (result & VMR_EXISTANCE_RESULT);
                        }
                        else {
                            return_count--;
                            if(return_count > 0) {
                                return VMR_RETURN | return_count | (result & VMR_EXISTANCE_RESULT);
                            }
                        }
                        break;

                    case VMR_REVERT:
                        fprintf(stderr, "unexpected error on excute_method result\n");
                        exit(0);
                }
                break;

            case OP_INVOKE_BLOCK:
#ifdef VM_DEBUG
vm_debug("OP_INVOKE_BLOCK\n");
#endif
                pc++;

                ivalue1 = *pc;          // bloc index
                pc++;
#ifdef VM_DEBUG
vm_debug("block index %d\n", ivalue1);
#endif

                ivalue2 = *pc;         // result existance
                pc++;

                ivalue3 = *pc;         // static method block
                pc++;

                ovalue1 = var[ivalue1].mObjectValue;

                result = excute_block(ovalue1, type_, ivalue2, ivalue3);

                switch(result & VMR_TYPE) {
                    case VMR_SUCCESS:
                        break;

                    case VMR_ERROR:
                        return FALSE;

                    case VMR_RETURN:
                        return result;

                    case VMR_REVERT:
                        break;
                }
                break;

            case OP_RETURN:
                pc++;

                ivalue1 = *pc;
                pc++;

                ivalue2 = *pc;
                pc++;

                return VMR_RETURN | (ivalue1 & VMR_RETURN_COUNT) | (ivalue2 ? VMR_EXISTANCE_RESULT:0);

            case OP_REVERT:
                return VMR_REVERT;

            case OP_IADD:
                pc++;

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

                ovalue1 = (gCLStackPtr-2)->mObjectValue;
                ovalue2 = (gCLStackPtr-1)->mObjectValue;

                gCLStackPtr-=2;

                ivalue1 = CLSTRING(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLSTRING(ovalue2)->mLen;  // string length of ovalue2

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

                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("division by zero");
                    return VMR_ERROR;
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

                if((gCLStackPtr-2)->mFloatValue == 0.0) {
                    vm_error("division by zero");
                    return VMR_ERROR;
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

                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("remainder by zero");
                    return VMR_ERROR;
                }

                ivalue1 = (gCLStackPtr-2)->mIntValue % (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IMOD %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_ILSHIFT:
                pc++;

                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("division by zero");
                    return VMR_ERROR;
                }

                ivalue1 = (gCLStackPtr-2)->mIntValue << (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_ILSHIFT %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IRSHIFT:
                pc++;
                if((gCLStackPtr-2)->mIntValue == 0) {
                    vm_error("division by zero");
                    return VMR_ERROR;
                }

                ivalue1 = (gCLStackPtr-2)->mIntValue >> (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IRSHIFT %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IGTR:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue > (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IGTR %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FGTR:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue > (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FGTR %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IGTR_EQ:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue >= (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IGTR_EQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FGTR_EQ:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue >= (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FGTR_EQ %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_ILESS:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue < (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_ILESS %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FLESS:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue < (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FLESS %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_ILESS_EQ:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue <= (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_ILESS_EQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FLESS_EQ:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue <= (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FLESS_EQ %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IEQ:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue == (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IEQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FEQ:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue == (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FEQ %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_SEQ:
                pc++;

                ovalue1 = (gCLStackPtr-2)->mObjectValue;
                ovalue2 = (gCLStackPtr-1)->mObjectValue;
                
                ivalue1 = (wcscmp(CLSTRING(ovalue1)->mChars, CLSTRING(ovalue2)->mChars) == 0);
                
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_SEQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_INOTEQ:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue != (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_INOTEQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FNOTEQ:
                pc++;

                fvalue1 = (gCLStackPtr-2)->mFloatValue != (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FNOTEQ %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_SNOTEQ:
                pc++;

                ovalue1 = (gCLStackPtr-2)->mObjectValue;
                ovalue2 = (gCLStackPtr-1)->mObjectValue;
                
                ivalue1 = (wcscmp(CLSTRING(ovalue1)->mChars, CLSTRING(ovalue2)->mChars) != 0);
                
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_SNOTEQ %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IAND:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue & (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IAND %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IXOR:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue ^ (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IXOR %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IOR:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue | (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IOR %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IOROR:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue || (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IOROR %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_IANDAND:
                pc++;
                ivalue1 = (gCLStackPtr-2)->mIntValue && (gCLStackPtr-1)->mIntValue;
                gCLStackPtr-=2;
                gCLStackPtr->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_IANDAND %d\n", gCLStackPtr->mIntValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FOROR:
                pc++;
                fvalue1 = (gCLStackPtr-2)->mFloatValue || (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FOROR %f\n", gCLStackPtr->mFloatValue);
#endif
                gCLStackPtr++;
                break;

            case OP_FANDAND:
                pc++;
                fvalue1 = (gCLStackPtr-2)->mFloatValue && (gCLStackPtr-1)->mFloatValue;
                gCLStackPtr-=2;
                gCLStackPtr->mFloatValue = fvalue1;
#ifdef VM_DEBUG
vm_debug("OP_FANDAND %d\n", gCLStackPtr->mFloatValue);
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
                pc++;
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
#ifdef VM_DEBUG
vm_debug("OP_LOGICAL_DENIAL %d\n", ivalue1);
#endif
                break;

            case OP_COMPLEMENT:
                pc++;

                ivalue1 = (gCLStackPtr-1)->mIntValue;
                ivalue1 = ~ivalue1;

                (gCLStackPtr-1)->mIntValue = ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_COMPLEMENT %d\n", ivalue1);
#endif
                break;

            case OP_DEC_VALUE:
                pc++;

                ivalue1 = *pc;
                pc++;

                (gCLStackPtr-1)->mIntValue -= ivalue1;
                break;

            case OP_INC_VALUE:
                pc++;

                ivalue1 = *pc;
                pc++;

                (gCLStackPtr-1)->mIntValue += ivalue1;
#ifdef VM_DEBUG
vm_debug("OP_INC_VALUE %d\n", ivalue1);
#endif
                break;

            case OP_IF: {
                pc++;

                ivalue2 = *pc;
                pc++;

                /// get result of conditional ///
                ivalue1 = (gCLStackPtr-1)->mIntValue;
                gCLStackPtr--;

                if(ivalue1) {
                    pc += ivalue2;
                }
                }
                break;

            case OP_NOTIF: {
                pc++;

                ivalue2 = *pc;
                pc++;

                /// get result of conditional ///
                ivalue1 = (gCLStackPtr-1)->mIntValue;
                gCLStackPtr--;

                if(!ivalue1) {
                    pc += ivalue2;
                }
                }
                break;

            case OP_GOTO:
                pc++;

                ivalue1 = *pc;
                pc++;

                pc = code->mCode + ivalue1;
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

#ifdef VM_DEBUG
vm_debug("pc-code->mCode %d code->mLen %d\n", pc - code->mCode, code->mLen);
#endif

    return VMR_SUCCESS;
}

BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack)
{
    MVALUE* lvar;
    VMResult result;
    int nest_of_method_block;

    gCLStackPtr = gCLStack;
    lvar = gCLStack;
    gCLStackPtr += lv_num;

    if(gCLStackPtr + max_stack > gCLStack + gCLStackSize) {
        vm_error("overflow stack size\n");
        return FALSE;
    }

    result = cl_vm(code, constant, lvar, NULL);

    switch(result & VMR_TYPE) {
        case VMR_SUCCESS:
            break;

        case VMR_ERROR:
            return FALSE;

        case VMR_RETURN:
            fprintf(stderr, "unexpected error on cl_main result\n");
            exit(0);

        case VMR_REVERT:
            fprintf(stderr, "unexpected error on cl_main result\n");
            exit(0);
    }

    return TRUE;
}

BOOL run_initializar(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack)
{
    MVALUE* lvar;
    VMResult vm_result;
    int nest_of_method_block;

    gCLStackPtr = gCLStack;
    lvar = gCLStack;
    gCLStackPtr += lv_num;

    if(gCLStackPtr + max_stack > gCLStack + gCLStackSize) {
        vm_error("overflow stack size\n");
        return FALSE;
    }

    vm_result = cl_vm(code, constant, lvar, NULL);
    *result = *(gCLStackPtr-1);

    switch(vm_result & VMR_TYPE) {
        case VMR_SUCCESS:
            break;

        case VMR_ERROR:
            return FALSE;

        case VMR_RETURN:
            fprintf(stderr, "unexpected error on run_initializar result\n");
            exit(0);

        case VMR_REVERT:
            fprintf(stderr, "unexpected error on run_initializar result\n");
            exit(0);
    }

    return TRUE;
}

static void sigchld_block(int block)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);

    if(sigprocmask(block?SIG_BLOCK:SIG_UNBLOCK, &sigset, NULL) != 0)
    {
        fprintf(stderr, "error\n");
        exit(1);
    }
}

static void sigttou_block(int block)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTTOU);

    if(sigprocmask(block?SIG_BLOCK:SIG_UNBLOCK, &sigset, NULL) != 0)
    {
        fprintf(stderr, "error\n");
        exit(1);
    }
}

static BOOL excute_external_method_on_terminal(sCLMethod* method, sConst* constant, int num_params, MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* params;
    pid_t pid;

    params = lvar;

    /// fork ///
    pid = fork();
    if(pid < 0) {
        perror("fork");
        return FALSE;
    }

    /// a child process ///
    if(pid == 0) {
        char** argv;
        int i;
        int n;
        char buf[128];

        // a child process has a own process group
        pid = getpid();
        if(setpgid(pid,pid) < 0) {
            perror("setpgid(child)");
            return FALSE;
        }

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(child)");
            return FALSE;
        }
        sigttou_block(0);

        /// set environment variables ////
        snprintf(buf, 128, "%d", xgetmaxx());
        setenv("COLUMNS", buf, 1);

        snprintf(buf, 128, "%d", xgetmaxy());
        setenv("LINES", buf, 1);

        argv = malloc(sizeof(char*)*(num_params+1));

        argv[0] = strdup(CONS_str(constant, method->mNameOffset));

        n = 1;
        for(i=0; i<num_params; i++) {
            CLObject object;

            object = params[i].mObjectValue;

            if(substition_posibility_of_class(gStringType.mClass, CLOBJECT_HEADER(object)->mClass)) {
                char* mbstring;
                int size;

                size = sizeof(char) * (CLSTRING(object)->mLen + 1) * MB_LEN_MAX;
                mbstring = malloc(size);
                wcstombs(mbstring, CLSTRING(object)->mChars, size);
                argv[n++] = mbstring;
            }
        }

        argv[n] = NULL;

        execvp(CONS_str(constant, method->mNameOffset), argv);
        fprintf(stderr, "exec('%s') error\n", argv[0]);
        exit(127);
    }
    /// the parent process 
    else {
        int status;

        (void)setpgid(pid, pid);

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(parent)");
            return FALSE;
        }
        sigttou_block(0);

        // wait everytime
        if(waitpid(pid, &status, WUNTRACED) < 0) {
            perror("waitpid");
            return FALSE;
        }

        sigttou_block(1);
        if(tcsetpgrp(0, getpgid(0)) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp");

            return FALSE;
        }
        sigttou_block(0);
    }

    return TRUE;
}

static BOOL excute_external_method(sCLMethod* method, sConst* constant, int num_params, MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* params;
    pid_t pid;
    int nextout, nexterr;
    int pipeoutfds[2] = { -1, -1 };
    int pipeerrfds[2] = { -1 , -1};

    params = lvar;

    if(pipe(pipeoutfds) < 0) {
        perror("pipe");
        return FALSE;
    }
    nextout = pipeoutfds[1];
    if(pipe(pipeerrfds) < 0) {
        perror("pipe");
        return FALSE;
    }
    nexterr = pipeerrfds[1];

    /// fork ///
    pid = fork();
    if(pid < 0) {
        perror("fork");
        return FALSE;
    }

    /// a child process ///
    if(pid == 0) {
        char** argv;
        int i;
        int n;
        char buf[128];

        // a child process has a own process group
        pid = getpid();
        if(setpgid(pid,pid) < 0) {
            perror("setpgid(child)");
            return FALSE;
        }

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(child)");
            return FALSE;
        }
        sigttou_block(0);

        /// set environment variables ////
        snprintf(buf, 128, "%d", xgetmaxx());
        setenv("COLUMNS", buf, 1);

        snprintf(buf, 128, "%d", xgetmaxy());
        setenv("LINES", buf, 1);

        if(dup2(nextout, 1) < 0) {
            perror("dup2");
            return FALSE;
        }
        if(close(nextout) < 0) { return FALSE; }
        if(close(pipeoutfds[0]) < 0) { return FALSE; }

        if(dup2(nexterr, 2) < 0) {
            perror("dup2");
            return FALSE;
        }
        if(close(nexterr) < 0) { return FALSE; }
        if(close(pipeerrfds[0]) < 0) { return FALSE; }

        argv = malloc(sizeof(char*)*(num_params+1));

        argv[0] = strdup(CONS_str(constant, method->mNameOffset));

        n = 1;
        for(i=0; i<num_params; i++) {
            CLObject object;

            object = params[i].mObjectValue;

            if(substition_posibility_of_class(gStringType.mClass, CLOBJECT_HEADER(object)->mClass)) {
                char* mbstring;
                int size;

                size = sizeof(char) * (CLSTRING(object)->mLen + 1) * MB_LEN_MAX;
                mbstring = malloc(size);
                wcstombs(mbstring, CLSTRING(object)->mChars, size);
                argv[n++] = mbstring;
            }
        }

        argv[n] = NULL;

        execvp(CONS_str(constant, method->mNameOffset), argv);
        fprintf(stderr, "exec('%s') error\n", argv[0]);
        exit(127);
    }
    /// the parent process 
    else {
        int status;
        fd_set mask, read_ok;
        sBuf output, err_output;
        wchar_t* wstr;
        wchar_t* wstr2;
        wchar_t* wstr3;

        close(pipeoutfds[1]);
        close(pipeerrfds[1]);

        (void)setpgid(pid, pid);

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(parent)");
            return FALSE;
        }
        sigttou_block(0);

        /// read output and error output ///
        sBuf_init(&output);
        sBuf_init(&err_output);

        FD_ZERO(&mask);
        FD_SET(pipeoutfds[0], &mask);
        FD_SET(pipeerrfds[0], &mask);

        while(1) {
            char buf[1024];
            int size;
            int size2;
            struct timeval tv = { 0, 1000 * 1000 / 100 };

            read_ok = mask;
                    
            if(select((pipeoutfds[0] > pipeerrfds[0] ? pipeoutfds[0] + 1:pipeerrfds[0] + 1), &read_ok, NULL, NULL, &tv) > 0) {
                if(FD_ISSET(pipeoutfds[0], &read_ok)) {
                    size = read(pipeoutfds[0], buf, 1024);
                    
                    if(size < 0 || size == 0) {
                        close(pipeoutfds[0]);
                    }

                    sBuf_append(&output, buf, size);
                }
                else if(FD_ISSET(pipeerrfds[0], &read_ok)) {
                    size = read(pipeerrfds[0], buf, 1024);
                    
                    if(size < 0 || size == 0) {
                        close(pipeoutfds[0]);
                    }

                    sBuf_append(&err_output, buf, size);
                }
            }
            else {
                pid_t pid2;

                // wait everytime
                pid2 = waitpid(pid, &status, WUNTRACED|WNOHANG);

                if(pid2 == pid) {
                    break;
                }
            }
        }

        (void)close(pipeoutfds[0]);
        (void)close(pipeerrfds[0]);

/*
        if(WIFSTOPPED(status)) {
            cl_print("signal interrupt");

            kill(pid, SIGKILL);
            pid = waitpid(pid, &status, WUNTRACED);
            if(tcsetpgrp(0, getpgid(0)) < 0) {
                perror("tcsetpgrp");
                return FALSE;
            }

            return FALSE;
        }
        /// exited normally ///
        else if(WIFEXITED(status)) {
            /// command not found ///
            if(WEXITSTATUS(status) == 127) {
                if(tcsetpgrp(0 , getpgid(0)) < 0) {
                    perror("tcsetpgrp");
                    return FALSE;
                }

                cl_print("command not found(%s)", CONS_str(constant, method->mNameOffset));
                return FALSE;
            }
        }
        else if(WIFSIGNALED(status)) {
            cl_print("signal interrupt");

            if(tcsetpgrp(0, getpgid(0)) < 0) {
                perror("tcsetpgrp");
                return FALSE;
            }
            return FALSE;
        }
*/

        sigttou_block(1);
        if(tcsetpgrp(0, getpgid(0)) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp");
            FREE(output.mBuf);
            FREE(err_output.mBuf);

            return FALSE;
        }
        sigttou_block(0);

        wstr = MALLOC(sizeof(wchar_t)*(output.mLen + 1));
        mbstowcs(wstr, output.mBuf, output.mLen+1);

        wstr2 = MALLOC(sizeof(wchar_t)*(err_output.mLen + 1));
        mbstowcs(wstr2, err_output.mBuf, err_output.mLen + 1);

        wstr3 = MALLOC(sizeof(wchar_t)*(output.mLen + err_output.mLen + 1));
        wcscpy(wstr3, wstr);
        wcscat(wstr3, wstr2);

        (*stack_ptr)->mObjectValue = create_string_object(gStringType.mClass, wstr3, wcslen(wstr3));
        (*stack_ptr)++;

        FREE(wstr);
        FREE(wstr2);
        FREE(wstr3);
        FREE(output.mBuf);
        FREE(err_output.mBuf);
    }

    return TRUE;
}

static VMResult excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params)
{
#ifdef VM_DEBUG
vm_debug("excute_method start");
#endif
    int real_param_num;
    VMResult result;
    BOOL native_result;
    BOOL external_result;
    int return_count;
    
    real_param_num = method->mNumParams + (method->mFlags & CL_CLASS_METHOD ? 0:1) + method->mNumBlockType;

    if(method->mFlags & CL_EXTERNAL_METHOD) {
        MVALUE* lvar;
        BOOL run_on_terminal;
        sCLField* field;

        lvar = gCLStackPtr - num_params;

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        run_on_terminal = FALSE;

        field = get_field(klass, "terminal_programs");

        if(field == NULL || (field->mFlags & CL_STATIC_FIELD) == 0) {
            run_on_terminal = FALSE;
        }
        else {
            char* program_name;
            wchar_t program_name_wstr[CL_METHOD_NAME_MAX+1];
            CLObject array;


            program_name = CONS_str(constant, method->mNameOffset);
            if((int)mbstowcs(program_name_wstr, program_name, CL_METHOD_NAME_MAX+1) == -1) {
                vm_error("mbstowcs error");
                return VMR_ERROR;
            }

            array = get_object_from_mvalue(field->uValue.mStaticField);

            if(array && substition_posibility_of_class(CLOBJECT_HEADER(array)->mClass, gArrayType.mClass)) {
                int i;

                for(i=0; i<CLARRAY(array)->mLen; i++) {
                    CLObject item;

                    item = get_object_from_mvalue(CLARRAY_ITEMS(array, i));

                    if(substition_posibility_of_class(CLOBJECT_HEADER(item)->mClass, gStringType.mClass)) {
                        wchar_t* wstr;
                        wstr = CLSTRING(item)->mChars;

                        if(wcscmp(wstr, program_name_wstr) == 0) {
                            run_on_terminal = TRUE;
                        }
                    }
                }
            }
        }

        if(run_on_terminal) {
            external_result = excute_external_method_on_terminal(method, constant, num_params, &gCLStackPtr, lvar);
        }
        else {
            external_result = excute_external_method(method, constant, num_params, &gCLStackPtr, lvar);
        }

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

        if(external_result) {
            return VMR_SUCCESS;
        }
        else {
            return VMR_ERROR;
        }
    }
    else if(method->mFlags & CL_NATIVE_METHOD) {
        MVALUE* lvar;

        lvar = gCLStackPtr - real_param_num;

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        native_result = method->uCode.mNativeMethod(&gCLStackPtr, lvar);

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

        if(native_result) {
            return VMR_SUCCESS;
        }
        else {
            return VMR_ERROR;
        }
    }
    else {
        MVALUE* lvar;

        lvar = gCLStackPtr - real_param_num;
        if(method->mNumLocals - real_param_num > 0) {
            gCLStackPtr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
        }

        if(gCLStackPtr + method->mMaxStack > gCLStack + gCLStackSize) {
            vm_error("overflow stack size\n");
            return FALSE;
        }

        result = cl_vm(&method->uCode.mByteCodes, constant, lvar, type_);

        if(((result & VMR_TYPE) == VMR_RETURN && (result & VMR_EXISTANCE_RESULT)) || result_existance) {
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

BOOL cl_excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params)
{
    VMResult result;

    result = excute_method(method, klass, constant, result_existance, type_, num_params);

    if(result == VMR_ERROR) {
        return FALSE;
    }

    return TRUE;
}

static VMResult excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, BOOL static_method_block)
{
    int real_param_num;
    MVALUE* lvar;
    int i, j;
    VMResult result;

    real_param_num = CLBLOCK(block)->mNumParams + (static_method_block ? 0: 1);

    lvar = gCLStackPtr - real_param_num;

    /// copy caller local vars to current local vars ///
    memmove(lvar + CLBLOCK(block)->mNumVars, lvar, sizeof(MVALUE)*real_param_num);
    memmove(lvar, CLBLOCK(block)->mLocalVar, sizeof(MVALUE)*CLBLOCK(block)->mNumVars);

    gCLStackPtr += CLBLOCK(block)->mNumVars;

    if(CLBLOCK(block)->mNumLocals - real_param_num > 0) {
        gCLStackPtr += (CLBLOCK(block)->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
    }

    if(gCLStackPtr + CLBLOCK(block)->mMaxStack > gCLStack + gCLStackSize) {
        vm_error("overflow stack size\n");
        //pop_stack_frame();
        return FALSE;
    }

    result = cl_vm(CLBLOCK(block)->mCode, CLBLOCK(block)->mConstant, lvar, type_);

    /// restore caller local vars, and restore base of all block stack  ///
    memmove(CLBLOCK(block)->mLocalVar, lvar, sizeof(MVALUE)*CLBLOCK(block)->mNumVars);

    if(((result & VMR_TYPE) == VMR_RETURN && (result & VMR_EXISTANCE_RESULT)) || result_existance) {
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

BOOL cl_excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, BOOL static_method_block)
{
    VMResult result;

    result = excute_block(block, type_, result_existance, static_method_block);

    if(result == VMR_ERROR) {
        return FALSE;
    }

    return TRUE;
}

