#include "clover.h"
#include "common.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#define MAX_N_PARAMETERS 64

static void llvmcompiler_init()
{
}

static void llvmcompiler_final()
{
}

static void proto_type_of_function(LLVMModuleRef module, const char* func_name, LLVMTypeRef func_result_type, LLVMTypeRef param_types[], char* param_names[], int num_params)
{
    int i;
    LLVMTypeRef ret_type = LLVMFunctionType(func_result_type, param_types, num_params, 0);

    LLVMValueRef fun = LLVMAddFunction(module, func_name, ret_type);
    LLVMSetLinkage(fun, LLVMExternalLinkage);

    for(i=0; i<num_params; i++) {
        LLVMValueRef llvm_param = LLVMGetParam(fun , i);
        LLVMSetValueName(llvm_param, param_names[i]);
    }
}

static BOOL declare_clover_to_llvm_functions(LLVMModuleRef module)
{
    /// get_int_value_from_stack ///
    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};
    char* param_names[] = { (char*)"stack_num", (char*)"info" };

    proto_type_of_function(module, "llget_int_value_from_stack", LLVMInt32Type(), param_types, param_names, 2);

    /// get_byte_value_from_stack ///
    LLVMTypeRef param_types2[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};
    char* param_names2[] = { (char*)"stack_num", (char*)"info" };

    proto_type_of_function(module, "llget_byte_value_from_stack", LLVMInt8Type(), param_types2, param_names2, 2);

    /// llget_type_object_from_stack ///
    LLVMTypeRef param_types3[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};
    char* param_names3[] = { (char*)"stack_num", (char*)"info" };

    proto_type_of_function(module, "llget_type_object_from_stack", LLVMInt32Type(), param_types3, param_names3, 2);

    /// llinc_stack_pointer ///
    LLVMTypeRef param_types4[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};
    char* param_names4[] = { (char*)"stack_num", (char*)"info" };

    proto_type_of_function(module, "llinc_stack_pointer", LLVMVoidType(), param_types4, param_names4, 2);

    /// llcreate_int_object ///
    LLVMTypeRef param_types5[] = { LLVMInt32Type(), LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};
    char* param_names5[] = { (char*)"value", (char*)"type_object", (char*)"info" };

    proto_type_of_function(module, "llcreate_int_object", LLVMVoidType(), param_types5, param_names5, 3);

    /// vm_mutex_lock ///
    proto_type_of_function(module, "vm_mutex_lock", LLVMVoidType(), NULL, NULL, 0);

    /// vm_mutex_unlock ///
    proto_type_of_function(module, "vm_mutex_unlock", LLVMVoidType(), NULL, NULL, 0);

    /// llentry_exception_object ///
    LLVMTypeRef param_types6[] = { LLVMPointerType(LLVMVoidType(), 0), LLVMPointerType(LLVMInt8Type(), 0), LLVMPointerType(LLVMInt8Type(), 0) };
    char* param_names6[] = { (char*)"info", (char*)"class_name", (char*)"msg" };

    proto_type_of_function(module, "llentry_exception_object", LLVMVoidType(), param_types6, param_names6, 3);

    /// llcreate_type_object_from_string ///
    LLVMTypeRef param_types7[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    char* param_names7[] = { (char*)"class_name" };

    proto_type_of_function(module, "llcreate_type_object_from_string", LLVMInt32Type(), param_types7, param_names7, 1);

    /// llsolve_generics_types ///
    LLVMTypeRef param_types8[] = { LLVMInt32Type(), LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0) };
    char* param_names8[] = { (char*)"type_object", (char*)"vm_type", (char*)"info" };

    proto_type_of_function(module, "llsolve_generics_types", LLVMInt32Type(), param_types8, param_names8, 1);

    /// llpop_object_except_top ///
    LLVMTypeRef param_types9[] = { LLVMPointerType(LLVMVoidType(), 0) };
    char* param_names9[] = { (char*)"info" };

    proto_type_of_function(module, "llpop_object_except_top", LLVMVoidType(), param_types9, param_names9, 1);

    /// llpop_object ///
    LLVMTypeRef param_types10[] = { LLVMPointerType(LLVMVoidType(), 0) };
    char* param_names10[] = { (char*)"info" };

    proto_type_of_function(module, "llpop_object", LLVMVoidType(), param_types10, param_names10, 1);

    /// llpush_object ///
    LLVMTypeRef param_types11[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0) };
    char* param_names11[] = { (char*)"object", (char*)"info" };

    proto_type_of_function(module, "llpush_object", LLVMVoidType(), param_types11, param_names11, 2);

    return TRUE;
}

static LLVMValueRef call_function(LLVMModuleRef module, LLVMBuilderRef builder, char* fun_name, LLVMValueRef* params, int num_params)
{
    LLVMValueRef fun = LLVMGetNamedFunction(module, fun_name);

    MASSERT(fun != NULL);

    return LLVMBuildCall(builder, fun, params, num_params, "calltmp");
}

static BOOL create_type_name_from_bytecodes(char* type_name, int type_name_size, int** pc, sCLClass* klass)
{
    int ivalue1, ivalue2;
    char* real_class_name;
    sCLClass* klass2;
    int i;
    sConst* constant;

    constant = &klass->mConstPool;

    ivalue1 = **pc;          // offset
    (*pc)++;

    real_class_name = CONS_str(constant, ivalue1);
    klass2 = cl_get_class(real_class_name);

    if(klass2 == NULL) {
        return FALSE;
    }

    xstrncat(type_name, real_class_name, type_name_size);

    ivalue2 = **pc;          // num generics types
    (*pc)++;

    if(ivalue2 > 0) {
        xstrncat(type_name, (char*)"<", type_name_size);

        for(i=0; i<ivalue2; i++) {
            if(!create_type_name_from_bytecodes(type_name, type_name_size, pc, klass))
            {
                return FALSE;
            }

            if(i < ivalue2 -1) {
                xstrncat(type_name, (char*)",", type_name_size);
            }
        }

        xstrncat(type_name, (char*)">", type_name_size);
    }

    return TRUE;
}

static BOOL compile_method_to_llvm(sCLClass* klass, sCLMethod* method, LLVMBuilderRef builder, LLVMModuleRef module)
{
    const int num_params = 5;
    const char* func_name = CONS_str(&klass->mConstPool, method->mPathOffset);
    const LLVMTypeRef func_result_type = LLVMInt32Type();

    LLVMTypeRef param_types[5] = {LLVMPointerType(LLVMPointerType(LLVMVoidType(),0),0), LLVMPointerType(LLVMVoidType(), 0), LLVMPointerType(LLVMVoidType(), 0), LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};

    LLVMTypeRef ret_type = LLVMFunctionType(func_result_type, param_types, num_params, 0);
    LLVMValueRef fun = LLVMAddFunction(module, func_name, ret_type);
    LLVMSetLinkage(fun, LLVMExternalLinkage);

    LLVMValueRef llvm_param = LLVMGetParam(fun, 0);
    LLVMSetValueName(llvm_param, "stack_ptr");

    llvm_param = LLVMGetParam(fun, 1);
    LLVMSetValueName(llvm_param, "lvar");

    llvm_param = LLVMGetParam(fun, 2);
    LLVMSetValueName(llvm_param, "info");

    llvm_param = LLVMGetParam(fun, 3);
    LLVMSetValueName(llvm_param, "vm_type");

    llvm_param = LLVMGetParam(fun, 4);
    LLVMSetValueName(llvm_param, "klass");

    // Create basic block.
    LLVMBasicBlockRef new_basic_block = LLVMAppendBasicBlock(fun, "entry");
    LLVMPositionBuilderAtEnd(builder, new_basic_block);

    sByteCode* code = &method->uCode.mByteCodes;

    int* pc = code->mCode;

    LLVMValueRef params[128];
    LLVMValueRef value = LLVMConstInt(LLVMInt32Type(), 0, 0);

    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_IADD: 
puts("OP_IADD");
                {
                    pc++;

                    /// vm mutex lock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);

                    /// get left value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// get right value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// go ///
                    value = LLVMBuildAdd(builder, left, right, "addtmp");

                    /// get int object type ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef ovalue1_type = call_function(module, builder, (char*)"llget_type_object_from_stack", params, 2);


                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// llcreate_int_object ///
                    params[0] = value;
                    params[1] = ovalue1_type;
                    params[2] = LLVMGetParam(fun, 2);
                    value = call_function(module, builder, (char*)"llcreate_int_object", params, 3);

                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), 1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// call vm_mutex_unlock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);
                }
                break;

            case OP_BADD:
puts("OP_BADD");
                pc++;
                break;

            case OP_SHADD:
puts("OP_SHADD");
                pc++;
                break;

            case OP_UIADD:
puts("OP_UIADD");
                pc++;
                break;

            case OP_LOADD:
puts("OP_LOADD");
                pc++;
                break;

            case OP_FADD:
puts("OP_FADD");
                pc++;
                break;

            case OP_DADD:
puts("OP_DADD");
                pc++;
                break;

            case OP_SADD:
puts("OP_SADD");
                pc++;
                break;

            case OP_BSADD:
puts("OP_BSADD");
                pc++;
                break;

            case OP_ISUB:
puts("OP_ISUB");
                {
                    pc++;

                    /// vm mutex lock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);

                    /// get left value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// get right value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// go ///
                    value = LLVMBuildSub(builder, left, right, "addtmp");

                    /// get int object type ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef ovalue1_type = call_function(module, builder, (char*)"llget_type_object_from_stack", params, 2);


                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// llcreate_int_object ///
                    params[0] = value;
                    params[1] = ovalue1_type;
                    params[2] = LLVMGetParam(fun, 2);
                    value = call_function(module, builder, (char*)"llcreate_int_object", params, 3);

                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), 1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// call vm_mutex_unlock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);
                }
                break;

            case OP_BSUB:
puts("OP_BSUB");
                pc++;
                break;

            case OP_SHSUB:
puts("OP_SHSUB");
                pc++;
                break;

            case OP_UISUB:
puts("OP_UISUB");
                pc++;
                break;

            case OP_LOSUB:
puts("OP_LOSUB");
                pc++;
                break;

            case OP_FSUB:
puts("OP_FSUB");
                pc++;
                break;

            case OP_DSUB:
puts("OP_DSUB");
                pc++;
                break;

            case OP_IMULT:
puts("OP_IMULT");
                {
                    pc++;

                    /// vm mutex lock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);

                    /// get left value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// get right value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// go ///
                    value = LLVMBuildMul(builder, left, right, "addtmp");

                    /// get int object type ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef ovalue1_type = call_function(module, builder, (char*)"llget_type_object_from_stack", params, 2);


                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// llcreate_int_object ///
                    params[0] = value;
                    params[1] = ovalue1_type;
                    params[2] = LLVMGetParam(fun, 2);
                    value = call_function(module, builder, (char*)"llcreate_int_object", params, 3);

                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), 1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// call vm_mutex_unlock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);
                }
                break;

            case OP_BMULT:
puts("OP_BMULT");
                pc++;
                break;

            case OP_SHMULT:
puts("OP_SHMULT");
                pc++;
                break;

            case OP_UIMULT:
puts("OP_UIMULT");
                pc++;
                break;

            case OP_LOMULT:
puts("OP_LOMULT");
                pc++;
                break;

            case OP_FMULT:
puts("OP_FMULT");
                pc++;
                break;

            case OP_DMULT:
puts("OP_DMULT");
                pc++;
                break;

            case OP_BSMULT:
puts("OP_BSMULT");
                pc++;
                break;

            case OP_SMULT:
puts("OP_SMULT");
                pc++;
                break;

            case OP_IDIV:
puts("OP_IDIV");
                {
                    pc++;

                    /// vm mutex lock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);

                    /// get left value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// get right value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// check 0 div ///
                    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(fun, "then");
                    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(fun, "ifcont");
                    
                    LLVMBuildCondBr(builder, right, merge_block, then_block);

                    LLVMPositionBuilderAtEnd(builder, then_block);

                    params[0] = LLVMGetParam(fun, 2);
                    params[1] = LLVMBuildGlobalStringPtr(builder, "DivisionByZeroException", "DivisionByZeroException");
                    params[2] = LLVMBuildGlobalStringPtr(builder, "division by zero", "division by zero");

                    (void)call_function(module, builder, (char*)"llentry_exception_object", params, 3);
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);

                    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));

                    /*
                    LLVMBuildBr(builder, merge_block);
                    */

                    LLVMPositionBuilderAtEnd(builder, merge_block);

                    /// go ///
                    value = LLVMBuildSDiv(builder, left, right, "addtmp");

                    /// get int object type ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef ovalue1_type = call_function(module, builder, (char*)"llget_type_object_from_stack", params, 2);


                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// llcreate_int_object ///
                    params[0] = value;
                    params[1] = ovalue1_type;
                    params[2] = LLVMGetParam(fun, 2);
                    value = call_function(module, builder, (char*)"llcreate_int_object", params, 3);

                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), 1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// call vm_mutex_unlock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);
                }
                break;

            case OP_BDIV:
puts("OP_BDIV");
                pc++;
                break;

            case OP_SHDIV:
puts("OP_SHDIV");
                pc++;
                break;

            case OP_UIDIV:
puts("OP_UIDIV");
                pc++;
                break;

            case OP_LODIV:
puts("OP_LODIV");
                pc++;
                break;

            case OP_FDIV:
puts("OP_FDIV");
                pc++;
                break;

            case OP_DDIV:
puts("OP_DDIV");
                pc++;
                break;

            case OP_IMOD:
puts("OP_IMOD");
                {
                    pc++;

                    /// vm mutex lock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);

                    /// get left value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// get right value ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"llget_int_value_from_stack", params, 2);

                    /// check 0 div ///
                    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(fun, "then");
                    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(fun, "ifcont");
                    
                    LLVMBuildCondBr(builder, right, merge_block, then_block);

                    LLVMPositionBuilderAtEnd(builder, then_block);

                    params[0] = LLVMGetParam(fun, 2);
                    params[1] = LLVMBuildGlobalStringPtr(builder, "DivisionByZeroException", "DivisionByZeroException");
                    params[2] = LLVMBuildGlobalStringPtr(builder, "division by zero", "division by zero");

                    (void)call_function(module, builder, (char*)"llentry_exception_object", params, 3);
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);

                    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));

                    /*
                    LLVMBuildBr(builder, merge_block);
                    */

                    LLVMPositionBuilderAtEnd(builder, merge_block);

                    /// go ///
                    value = LLVMBuildSRem(builder, left, right, "addtmp");

                    /// get int object type ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef ovalue1_type = call_function(module, builder, (char*)"llget_type_object_from_stack", params, 2);


                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// llcreate_int_object ///
                    params[0] = value;
                    params[1] = ovalue1_type;
                    params[2] = LLVMGetParam(fun, 2);
                    value = call_function(module, builder, (char*)"llcreate_int_object", params, 3);

                    /// inc stack pointer ///
                    params[0] = LLVMConstInt(LLVMInt32Type(), 1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    call_function(module, builder, (char*)"llinc_stack_pointer", params, 2);

                    /// call vm_mutex_unlock ///
                    (void)call_function(module, builder, (char*)"vm_mutex_unlock", NULL, 0);
                }
                break;

            case OP_BMOD:
puts("OP_BMOD");
                pc++;
                break;

            case OP_SHMOD:
puts("OP_SHMOD");
                pc++;
                break;

            case OP_UIMOD:
puts("OP_UIMOD");
                pc++;
                break;

            case OP_LOMOD:
puts("OP_LOMOD");
                pc++;
                break;

            case OP_ILSHIFT:
puts("OP_ILSHIFT");
                pc++;
                break;

            case OP_BLSHIFT:
puts("OP_BLSHIFT");
                pc++;
                break;

            case OP_SHLSHIFT:
puts("OP_SHLSHIFT");
                pc++;
                break;

            case OP_UILSHIFT:
puts("OP_UILSHIFT");
                pc++;
                break;

            case OP_LOLSHIFT:
puts("OP_LOLSHIFT");
                pc++;
                break;

            case OP_IRSHIFT:
puts("OP_IRSHIFT");
                pc++;
                break;

            case OP_BRSHIFT:
puts("OP_BRSHIFT");
                pc++;
                break;

            case OP_SHRSHIFT:
puts("OP_SHRSHIFT");
                pc++;
                break;

            case OP_UIRSHIFT:
puts("OP_UIRSHIFT");
                pc++;
                break;

            case OP_LORSHIFT:
puts("OP_LORSHIFT");
                pc++;
                break;

            case OP_IAND:
puts("OP_IAND");
                pc++;
                break;

            case OP_BAND:
puts("OP_BAND");
                pc++;
                break;

            case OP_SHAND:
puts("OP_SHAND");
                pc++;
                break;

            case OP_UIAND:
puts("OP_UIAND");
                pc++;
                break;

            case OP_LOAND:
puts("OP_LOAND");
                pc++;
                break;

            case OP_IXOR:
puts("OP_IXOR");
                pc++;
                break;

            case OP_BXOR:
puts("OP_BXOR");
                pc++;
                break;

            case OP_SHXOR:
puts("OP_SHXOR");
                pc++;
                break;

            case OP_UIXOR:
puts("OP_UIXOR");
                pc++;
                break;

            case OP_LOXOR:
puts("OP_LOXOR");
                pc++;
                break;

            case OP_IOR:
puts("OP_IOR");
                pc++;
                break;

            case OP_BOR:
puts("OP_BOR");
                pc++;
                break;

            case OP_SHOR:
puts("OP_SHOR");
                pc++;
                break;

            case OP_UIOR:
puts("OP_UIOR");
                pc++;
                break;

            case OP_LOOR:
puts("OP_LOOR");
                pc++;
                break;

            case OP_IGTR:
puts("OP_IGTR");
                pc++;
                break;

            case OP_BGTR:
puts("OP_BGTR");
                pc++;
                break;

            case OP_SHGTR:
puts("OP_SHGTR");
                pc++;
                break;

            case OP_UIGTR:
puts("OP_UIGTR");
                pc++;
                break;

            case OP_LOGTR:
puts("OP_LOGTR");
                pc++;
                break;

            case OP_FGTR:
puts("OP_FGTR");
                pc++;
                break;

            case OP_DGTR:
puts("OP_DGTR");
                pc++;
                break;

            case OP_IGTR_EQ:
puts("OP_IGTR_EQ");
                pc++;
                break;

            case OP_BGTR_EQ:
puts("OP_BGTR_EQ");
                pc++;
                break;

            case OP_SHGTR_EQ:
puts("OP_SHGTR_EQ");
                pc++;
                break;

            case OP_UIGTR_EQ:
puts("OP_UIGTR_EQ");
                pc++;
                break;

            case OP_LOGTR_EQ:
puts("OP_LOGTR_EQ");
                pc++;
                break;

            case OP_FGTR_EQ:
puts("OP_FGTR_EQ");
                pc++;
                break;

            case OP_DGTR_EQ:
puts("OP_DGTR_EQ");
                pc++;
                break;

            case OP_ILESS:
puts("OP_ILESS");
                pc++;
                break;

            case OP_BLESS:
puts("OP_BLESS");
                pc++;
                break;

            case OP_SHLESS:
puts("OP_SHLESS");
                pc++;
                break;

            case OP_UILESS:
puts("OP_UILESS");
                pc++;
                break;

            case OP_LOLESS:
puts("OP_LOLESS");
                pc++;
                break;

            case OP_FLESS:
puts("OP_FLESS");
                pc++;
                break;

            case OP_DLESS:
puts("OP_DLESS");
                pc++;
                break;

            case OP_ILESS_EQ:
puts("OP_ILESS_EQ");
                pc++;
                break;

            case OP_BLESS_EQ:
puts("OP_BLESS_EQ");
                pc++;
                break;

            case OP_SHLESS_EQ:
puts("OP_SHLESS_EQ");
                pc++;
                break;

            case OP_UILESS_EQ:
puts("OP_UILESS_EQ");
                pc++;
                break;

            case OP_LOLESS_EQ:
puts("OP_LOLESS_EQ");
                pc++;
                break;

            case OP_FLESS_EQ:
puts("OP_FLESS_EQ");
                pc++;
                break;

            case OP_DLESS_EQ:
puts("OP_DLESS_EQ");
                pc++;
                break;

            case OP_IEQ:
puts("OP_IEQ");
                pc++;
                break;

            case OP_BEQ:
puts("OP_BEQ");
                pc++;
                break;

            case OP_SHEQ:
puts("OP_SHEQ");
                pc++;
                break;

            case OP_UIEQ:
puts("OP_UIEQ");
                pc++;
                break;

            case OP_LOEQ:
puts("OP_LOEQ");
                pc++;
                break;

            case OP_FEQ:
puts("OP_FEQ");
                pc++;
                break;

            case OP_DEQ:
puts("OP_DEQ");
                pc++;
                break;

            case OP_SEQ:
puts("OP_SEQ");
                pc++;
                break;

            case OP_BSEQ:
puts("OP_BSEQ");
                pc++;
                break;

            case OP_INOTEQ:
puts("OP_INOTEQ");
                pc++;
                break;

            case OP_BNOTEQ:
puts("OP_BNOTEQ");
                pc++;
                break;

            case OP_SHNOTEQ:
puts("OP_SHNOTEQ");
                pc++;
                break;

            case OP_UINOTEQ:
puts("OP_UINOTEQ");
                pc++;
                break;

            case OP_LONOTEQ:
puts("OP_LONOTEQ");
                pc++;
                break;

            case OP_FNOTEQ:
puts("OP_FNOTEQ");
                pc++;
                break;

            case OP_DNOTEQ:
puts("OP_DNOTEQ");
                pc++;
                break;

            case OP_SNOTEQ:
puts("OP_SNOTEQ");
                pc++;
                break;

            case OP_BSNOTEQ:
puts("OP_BSNOTEQ");
                pc++;
                break;

            case OP_COMPLEMENT:
puts("OP_COMPLEMENT");
                pc++;
                break;

            case OP_BCOMPLEMENT:
puts("OP_BCOMPLEMENT");
                pc++;
                break;

            case OP_SHCOMPLEMENT:
puts("OP_SHCOMPLEMENT");
                pc++;
                break;

            case OP_UICOMPLEMENT:
puts("OP_UICOMPLEMENT");
                pc++;
                break;

            case OP_LOCOMPLEMENT:
puts("OP_LOCOMPLEMENT");
                pc++;
                break;

            case OP_BLEQ:
puts("OP_BLEQ");
                pc++;
                break;

            case OP_BLNOTEQ:
puts("OP_BLNOTEQ");
                pc++;
                break;

            case OP_BLANDAND:
puts("OP_BLANDAND");
                pc++;
                break;

            case OP_BLOROR:
puts("OP_BLOROR");
                pc++;
                break;

            case OP_LOGICAL_DENIAL:
puts("OP_LOGICAL_DENIAL");
                pc++;
                break;

            case OP_LDCINT:
puts("OP_LDCINT");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCBYTE:
puts("OP_LDCBYTE");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCFLOAT:
puts("OP_LDCFLOAT");
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

            case OP_LDCWSTR: {
puts("OP_LDCWSTR");
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;
                }
                break;

            case OP_LDCBOOL:
puts("OP_LDCBOOL");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;
                break;

            case OP_LDCNULL:
puts("OP_LDCNULL");
                pc++;
                break;

            case OP_LDTYPE: {
puts("OP_LDTYPE");
                pc++;

                //pc_in_stack = pc;
                //obj = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                //pc = pc_in_stack;

                ivalue1 = *pc;      // nest of method from definition point
                pc++;

                }
                break;

            case OP_LDCSTR: 
puts("OP_LDCSTR");
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;
                break;

            case OP_LDCSHORT:
puts("OP_LDCSHORT");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCUINT:
puts("OP_LDCUINT");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCLONG:
puts("OP_LDCLONG");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                ivalue2 = *pc;
                pc++;
                break;

            case OP_LDCCHAR:
puts("OP_LDCCHAR");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCDOUBLE:
puts("OP_LDCDOUBLE");
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;
                break;


            case OP_LDCPATH:
puts("OP_LDCPATH");
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
            case OP_OSTORE:
puts("OP_OSTORE");
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
puts("OP_OLOAD");
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_SRFIELD:
puts("OP_SRFIELD");
                pc++;

                ivalue1 = *pc;                              // field index
                pc++;

                ivalue2 = *pc;                // class name of field index
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */
                break;

            case OP_LDFIELD:
puts("OP_LDFIELD");
                pc++;

                ivalue1 = *pc;                // field index
                pc++;

                ivalue2 = *pc;                // class name of field index
                pc++;

                /*
                /// type checking ///
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */
                break;

            case OP_LD_STATIC_FIELD: 
puts("OP_LD_STATIC_FIELD");
                pc++;

                ivalue1 = *pc;             // class name
                pc++;

                ivalue2 = *pc;             // field index
                pc++;
                break;

            case OP_SR_STATIC_FIELD:
puts("OP_SR_STATIC_FIELD");
                pc++;

                ivalue1 = *pc;                    // class name
                pc++;

                ivalue2 = *pc;                    // field index
                pc++;
                break;

            case OP_NEW_OBJECT: {
puts("OP_NEW_OBJECT");
                pc++;

                char type_name[CL_TYPE_NAME_MAX];

                if(!create_type_name_from_bytecodes(type_name, CL_TYPE_NAME_MAX, &pc, klass))
                {
                    fprintf(stderr, "invalid class name in bytecodes\n");
                    return FALSE;
                }
                }
                break;

            case OP_NEW_ARRAY:
puts("OP_NEW_ARRAY");
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */

                ivalue2 = *pc;                              // number of elements
                pc++;
                break;

            case OP_NEW_HASH:
puts("OP_NEW_HASH");
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */

                ivalue2 = *pc;                              // number of elements
                pc++;
                break;

            case OP_NEW_RANGE:
puts("OP_NEW_RANGE");
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */
                break;

            case OP_NEW_TUPLE:
puts("OP_NEW_TUPLE");
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */

                ivalue2 = *pc;                              // number of elements
                pc++;
                break;


            case OP_NEW_REGEX:
puts("OP_NEW_REGEX");
                pc++;

                ivalue1 = *pc;          // regex string
                pc++;

                ivalue2 = *pc;          // global
                pc++;

                ivalue3 = *pc;          // mutiline
                pc++;

                ivalue4 = *pc;          // ignore case
                pc++;
                break;

            case OP_NEW_BLOCK: {
puts("OP_NEW_BLOCK");
                pc++;

                ivalue1 = *pc;                      // max stack
                pc++;

                ivalue2 = *pc;                      // num locals
                pc++;

                ivalue3 = *pc;                      // num params
                pc++;

                ivalue4 = *pc;                      // max_block_var_num
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);           // block result type

                memset(params, 0, sizeof(params));
                for(j=0; j<ivalue4; j++) {           // block param types
                    pc_in_stack = pc;
                    params[j] = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                    pc = pc_in_stack;
                    push_object(params[j], info);
                }
                */

                ivalue5 = *pc;                // constant pool len
                pc++;
                
                ivalue6 = *pc;                      // constant pool offset
                pc++;

                ivalue7 = *pc;                    // byte code len
                pc++;

                ivalue8 = *pc;                      // byte code offset
                pc++;

                ivalue9 = *pc;                      // breakable
                pc++;

                ivalue10 = *pc;                     // caller existance
                pc++;
                }

            case OP_INVOKE_METHOD: {
puts("OP_INVOKE_METHOD");
                pc++;

                /// method data ///
                ivalue1 = *pc;                  // real class name offset
                pc++;

                char* real_class_name = CONS_str(&klass->mConstPool, ivalue1);
                sCLClass* klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    fprintf(stderr, "klass not found(%s)\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *pc;                  // method index
                pc++;

                ivalue3 = *pc;                  // existance of result
                pc++;

                ivalue4 = *pc;                  // num params
                pc++;

                ivalue6 = *pc;                  // method num block type
                pc++;

                ivalue10 = *pc;                 // class method
                pc++;

                ivalue11 = *pc;                 // method name
                pc++;

                ivalue9 = *pc;                  // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_CLASS) {
                    char type_name[CL_TYPE_NAME_MAX];

                    type_name[0] = 0;

                    if(!create_type_name_from_bytecodes(type_name, CL_TYPE_NAME_MAX, &pc, klass)) {
                        fprintf(stderr, "invalid class name in byte codes\n");
                        return FALSE;
                    }
                    
                    LLVMValueRef class_name = LLVMBuildGlobalStringPtr(builder, type_name, "class_name");

                    params[0] = class_name;

                    LLVMValueRef type_object = call_function(module, builder, (char*)"llcreate_type_object_from_string", params, 1);

                    params[0] = type_object;
                    params[1] = LLVMGetParam(fun, 2);

                    (void)call_function(module, builder, (char*)"llpush_object", params, 2);

                    params[0] = type_object;
                    params[1] = LLVMGetParam(fun, 3);
                    params[2] = LLVMGetParam(fun, 2);

                    call_function(module, builder, (char*)"llsolve_generics_types", params, 3);
                    //LLVMValueRef solve_result = call_function(module, builder, (char*)"llsolve_generics_types", params, 3);
                }
                else {
                }

                }
                break;

            case OP_INVOKE_VIRTUAL_METHOD: {
puts("OP_INVOKE_VIRTUAL_METHOD");
                pc++;

                /// method data ///
                ivalue1 = *pc;           // method name offset
                pc++;

                ivalue2 = *pc;           // method num params
                pc++;

                ivalue11 = *pc;                         // existance of block
                pc++;

                ivalue5 = *pc;  // existance of result
                pc++;

                ivalue6 = *pc;  // super
                pc++;

                ivalue7 = *pc;   // class method
                pc++;

                ivalue8 = *pc;   // method num block
                pc++;

                ivalue9 = *pc;   // object kind
                pc++;


                if(ivalue9 == INVOKE_METHOD_KIND_OBJECT) {
                }
                else {
/*
                    pc_in_stack = pc;
                    type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                    pc = pc_in_stack;
*/

/*
                    pc_in_stack = pc;
                    if(!get_class_info_from_bytecode(&klass3, &pc_in_stack, constant, info))
                    {
                        vm_mutex_unlock();
                        return FALSE;
                    }
                    pc = pc_in_stack;
*/

                }

                }
                break;

            case OP_INVOKE_VIRTUAL_CLONE_METHOD:
puts("OP_INVOKE_VIRTUAL_CLONE_METHOD");
                pc++;

                ivalue1 = *pc;      // star
                pc++;

                break;

            case OP_CALL_PARAM_INITIALIZER: {
puts("OP_CALL_PARAM_INITIALIZER");
                pc++;

                ivalue1 = *pc;
                pc++;

                ivalue3 = *pc;              // code len
                pc++;

                ivalue4 = *pc;              // code offset
                pc++;

                ivalue5 = *pc;              // max_stack
                pc++;

                ivalue6 = *pc;              // lv_num
                pc++;

                }
                break;

            case OP_FOLD_PARAMS_TO_ARRAY: // for variable argument
puts("OP_FOLD_PARAMS_TO_ARRAY");
                pc++;

                ivalue1 = *pc;          // num variable argument
                pc++;

                ivalue2 = *pc;          // existance of block object
                pc++;

/*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);  // Array<anonymous>
                pc = pc_in_stack;
*/
                break;

            case OP_IF: {
puts("OP_IF");
                pc++;

                ivalue2 = *pc;
                pc++;
                }
                break;

            case OP_NOTIF: {
puts("OP_NOTIF");
                pc++;
                ivalue2 = *pc;
                pc++;
                }
                break;

            case OP_GOTO:
puts("OP_GOTO");
                pc++;
                break;

            case OP_RETURN:
puts("OP_RETURN");
                pc++;
                LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 1, 0));
                /*
                if(value == NULL || func_result_type == LLVMVoidType()) {
                    LLVMBuildRetVoid(builder);
                }
                else {
                    // Insert body as return vale.
                    LLVMBuildRet(builder, value);
                }
                */
                break;

            case OP_THROW:
puts("OP_THROW");
                pc++;
                return FALSE;

            case OP_TRY:
puts("OP_TRY");
                pc++;

                ivalue1 = *pc;                  // catch block number
                pc++;

                ivalue2 = *pc;                  // finally block existance
                pc++;

                ivalue3 = *pc;                  // finally block result existance
                pc++;
                break;

            case OP_INVOKE_BLOCK:
puts("OP_INVOKE_BLOCK");
                pc++;

                ivalue1 = *pc;          // block index
                pc++;

                ivalue2 = *pc;         // result existance
                pc++;
                break;

            case OP_BREAK_IN_METHOD_BLOCK:
puts("OP_BREAK_IN_METHOD_BLOCK");
                pc++;

                return TRUE;

            case OP_OUTPUT_RESULT: 
puts("OP_OUTPUT_RESULT");
                pc++;
                break;

            case OP_POP:
puts("OP_POP");
                pc++;
                break;

            case OP_POP_N:
puts("OP_POP_N");
                pc++;

                ivalue1 = *pc;
                pc++;

                break;

            case OP_POP_N_WITHOUT_TOP:
puts("OP_POP_N_WITHOUT_TOP");
                pc++;

                ivalue1 = *pc;
                pc++;
                
                break;

            case OP_DUP:
puts("OP_DUP");
                pc++;
                break;

            case OP_SWAP:
puts("OP_SWAP");
                pc++;
                break;

            case OP_INC_VALUE:
puts("OP_INC_VALUE");
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_DEC_VALUE:
puts("OP_DEC_VALUE");
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            default:
                fprintf(stderr, "unexpected error\n");
                pc++;
        }
    }

    return TRUE;
}

static BOOL compile_script(char* file_name)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("llclover");
    LLVMBuilderRef builder = LLVMCreateBuilder();

    /// declare the clover to llvm functions ///
    if(!declare_clover_to_llvm_functions(module)) {
        return FALSE;
    }

    /// load class and compile method ///
    sCLClass* klass = load_class(file_name, TRUE);

    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method = klass->mMethods + i;

        if(!compile_method_to_llvm(klass, method, builder, module)) {
            return FALSE;
        }
    }

    /// write the output ///
    char output_fname[PATH_MAX];
    char* p2 = file_name + strlen(file_name);
    while(p2 >= file_name) {
        if(*p2 == '.') {
            break;
        }
        else {
            p2--;
        }
    }
    memcpy(output_fname, file_name, p2 - file_name);
    output_fname[p2-file_name] = 0;
    xstrncat(output_fname, (char*)".ll", PATH_MAX);

    LLVMPrintModuleToFile(module, output_fname, NULL);
    //LLVMDumpModule(module);

    return TRUE;
}

int main(int argc, char* argv[]) 
{
    llvmcompiler_init();

    //CHECKML_BEGIN

    int i;
    for(i=1; i<argc; i++) {
        if(!compile_script(argv[i])) {
            llvmcompiler_final();
            //CHECKML_END
            exit(2);
        }
    }
    llvmcompiler_final();

    //CHECKML_END

    exit(0);
}

