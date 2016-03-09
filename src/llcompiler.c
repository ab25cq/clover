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

static BOOL declare_clover_to_llvm_functions(LLVMModuleRef module)
{
    /// get_int_value_from_stack ///
    int num_params = 2;
    char* func_name = (char*)"get_int_value_from_stack";
    LLVMTypeRef func_result_type = LLVMInt32Type();

    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMPointerType(LLVMVoidType(), 0)};

    LLVMTypeRef ret_type = LLVMFunctionType(func_result_type, param_types, num_params, 0);

    LLVMValueRef fun = LLVMAddFunction(module, func_name, ret_type);
    LLVMSetLinkage(fun, LLVMExternalLinkage);

    LLVMValueRef llvm_param1 = LLVMGetParam(fun ,0);
    LLVMSetValueName(llvm_param1, "stack_num");

    LLVMValueRef llvm_param2 = LLVMGetParam(fun, 1);
    LLVMSetValueName(llvm_param2, "info");

    /// vm_mutex_lock ///
    num_params = 0;
    func_name = (char*)"vm_mutex_lock";
    func_result_type = LLVMVoidType();

    ret_type = LLVMFunctionType(func_result_type, NULL, num_params, 0);

    fun = LLVMAddFunction(module, func_name, ret_type);
    LLVMSetLinkage(fun, LLVMExternalLinkage);

    return TRUE;
}

static LLVMValueRef call_function(LLVMModuleRef module, LLVMBuilderRef builder, char* fun_name, LLVMValueRef* params, int num_params)
{
    LLVMValueRef fun = LLVMGetNamedFunction(module, fun_name);

    if(fun == NULL) {
        fprintf(stderr, "invalid function call\n");
        exit(2);
    }

    return LLVMBuildCall(builder, fun, params, num_params, "calltmp");
}

static void add_vm_mutex_lock(LLVMModuleRef module, LLVMBuilderRef builder)
{
    (void)call_function(module, builder, (char*)"vm_mutex_lock", NULL, 0);
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
    LLVMValueRef value;

    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11;

    while(pc - code->mCode < code->mLen) {
        switch(*pc) {
            case OP_IADD: 
                {
                    pc++;

                    add_vm_mutex_lock(module, builder);

                    LLVMValueRef params[2];

                    params[0] = LLVMConstInt(LLVMInt32Type(), -1, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef left = call_function(module, builder, (char*)"get_int_value_from_stack", params, 2);

                    params[0] = LLVMConstInt(LLVMInt32Type(), -2, 0);
                    params[1] = LLVMGetParam(fun, 2);
                    LLVMValueRef right = call_function(module, builder, (char*)"get_int_value_from_stack", params, 2);

                    ASSERT(left && right);

                    value = LLVMBuildAdd(builder, left, right, "addtmp");
                }
                break;

            case OP_BADD:
                pc++;
                break;

            case OP_SHADD:
                pc++;
                break;

            case OP_UIADD:
                pc++;
                break;

            case OP_LOADD:
                pc++;
                break;

            case OP_FADD:
                pc++;
                break;

            case OP_DADD:
                pc++;
                break;

            case OP_SADD:
                pc++;
                break;

            case OP_BSADD:
                pc++;
                break;

            case OP_ISUB:
                pc++;
                break;

            case OP_BSUB:
                pc++;
                break;

            case OP_SHSUB:
                pc++;
                break;

            case OP_UISUB:
                pc++;
                break;

            case OP_LOSUB:
                pc++;
                break;

            case OP_FSUB:
                pc++;
                break;

            case OP_DSUB:
                pc++;
                break;

            case OP_IMULT:
                pc++;
                break;

            case OP_BMULT:
                pc++;
                break;

            case OP_SHMULT:
                pc++;
                break;

            case OP_UIMULT:
                pc++;
                break;

            case OP_LOMULT:
                pc++;
                break;

            case OP_FMULT:
                pc++;
                break;

            case OP_DMULT:
                pc++;
                break;

            case OP_BSMULT:
                pc++;
                break;

            case OP_SMULT:
                pc++;
                break;

            case OP_IDIV:
                pc++;
                break;

            case OP_BDIV:
                pc++;
                break;

            case OP_SHDIV:
                pc++;
                break;

            case OP_UIDIV:
                pc++;
                break;

            case OP_LODIV:
                pc++;
                break;

            case OP_FDIV:
                pc++;
                break;

            case OP_DDIV:
                pc++;
                break;

            case OP_IMOD:
                pc++;
                break;

            case OP_BMOD:
                pc++;
                break;

            case OP_SHMOD:
                pc++;
                break;

            case OP_UIMOD:
                pc++;
                break;

            case OP_LOMOD:
                pc++;
                break;

            case OP_ILSHIFT:
                pc++;
                break;

            case OP_BLSHIFT:
                pc++;
                break;

            case OP_SHLSHIFT:
                pc++;
                break;

            case OP_UILSHIFT:
                pc++;
                break;

            case OP_LOLSHIFT:
                pc++;
                break;

            case OP_IRSHIFT:
                pc++;
                break;

            case OP_BRSHIFT:
                pc++;
                break;

            case OP_SHRSHIFT:
                pc++;
                break;

            case OP_UIRSHIFT:
                pc++;
                break;

            case OP_LORSHIFT:
                pc++;
                break;

            case OP_IAND:
                pc++;
                break;

            case OP_BAND:
                pc++;
                break;

            case OP_SHAND:
                pc++;
                break;

            case OP_UIAND:
                pc++;
                break;

            case OP_LOAND:
                pc++;
                break;

            case OP_IXOR:
                pc++;
                break;

            case OP_BXOR:
                pc++;
                break;

            case OP_SHXOR:
                pc++;
                break;

            case OP_UIXOR:
                pc++;
                break;

            case OP_LOXOR:
                pc++;
                break;

            case OP_IOR:
                pc++;
                break;

            case OP_BOR:
                pc++;
                break;

            case OP_SHOR:
                pc++;
                break;

            case OP_UIOR:
                pc++;
                break;

            case OP_LOOR:
                pc++;
                break;

            case OP_IGTR:
                pc++;
                break;

            case OP_BGTR:
                pc++;
                break;

            case OP_SHGTR:
                pc++;
                break;

            case OP_UIGTR:
                pc++;
                break;

            case OP_LOGTR:
                pc++;
                break;

            case OP_FGTR:
                pc++;
                break;

            case OP_DGTR:
                pc++;
                break;

            case OP_IGTR_EQ:
                pc++;
                break;

            case OP_BGTR_EQ:
                pc++;
                break;

            case OP_SHGTR_EQ:
                pc++;
                break;

            case OP_UIGTR_EQ:
                pc++;
                break;

            case OP_LOGTR_EQ:
                pc++;
                break;

            case OP_FGTR_EQ:
                pc++;
                break;

            case OP_DGTR_EQ:
                pc++;
                break;

            case OP_ILESS:
                pc++;
                break;

            case OP_BLESS:
                pc++;
                break;

            case OP_SHLESS:
                pc++;
                break;

            case OP_UILESS:
                pc++;
                break;

            case OP_LOLESS:
                pc++;
                break;

            case OP_FLESS:
                pc++;
                break;

            case OP_DLESS:
                pc++;
                break;

            case OP_ILESS_EQ:
                pc++;
                break;

            case OP_BLESS_EQ:
                pc++;
                break;

            case OP_SHLESS_EQ:
                pc++;
                break;

            case OP_UILESS_EQ:
                pc++;
                break;

            case OP_LOLESS_EQ:
                pc++;
                break;

            case OP_FLESS_EQ:
                pc++;
                break;

            case OP_DLESS_EQ:
                pc++;
                break;

            case OP_IEQ:
                pc++;
                break;

            case OP_BEQ:
                pc++;
                break;

            case OP_SHEQ:
                pc++;
                break;

            case OP_UIEQ:
                pc++;
                break;

            case OP_LOEQ:
                pc++;
                break;

            case OP_FEQ:
                pc++;
                break;

            case OP_DEQ:
                pc++;
                break;

            case OP_SEQ:
                pc++;
                break;

            case OP_BSEQ:
                pc++;
                break;

            case OP_INOTEQ:
                pc++;
                break;

            case OP_BNOTEQ:
                pc++;
                break;

            case OP_SHNOTEQ:
                pc++;
                break;

            case OP_UINOTEQ:
                pc++;
                break;

            case OP_LONOTEQ:
                pc++;
                break;

            case OP_FNOTEQ:
                pc++;
                break;

            case OP_DNOTEQ:
                pc++;
                break;

            case OP_SNOTEQ:
                pc++;
                break;

            case OP_BSNOTEQ:
                pc++;
                break;

            case OP_COMPLEMENT:
                pc++;
                break;

            case OP_BCOMPLEMENT:
                pc++;
                break;

            case OP_SHCOMPLEMENT:
                pc++;
                break;

            case OP_UICOMPLEMENT:
                pc++;
                break;

            case OP_LOCOMPLEMENT:
                pc++;
                break;

            case OP_BLEQ:
                pc++;
                break;

            case OP_BLNOTEQ:
                pc++;
                break;

            case OP_BLANDAND:
                pc++;
                break;

            case OP_BLOROR:
                pc++;
                break;

            case OP_LOGICAL_DENIAL:
                pc++;
                break;

            case OP_LDCINT:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCBYTE:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCFLOAT:
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

            case OP_LDCWSTR: {
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;
                }
                break;

            case OP_LDCBOOL:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;
                break;

            case OP_LDCNULL:
                pc++;
                break;

            case OP_LDTYPE: {
                pc++;

                //pc_in_stack = pc;
                //obj = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                //pc = pc_in_stack;

                ivalue1 = *pc;      // nest of method from definition point
                pc++;

                }
                break;

            case OP_LDCSTR: {
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;
                }
                break;

            case OP_LDCSHORT:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCUINT:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCLONG:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                ivalue2 = *pc;
                pc++;
                break;

            case OP_LDCCHAR:
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                break;

            case OP_LDCDOUBLE:
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;
                break;


            case OP_LDCPATH: {
                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                }
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
            case OP_OSTORE:
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_SRFIELD:
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
VMLOG(info, "OP_LD_STATIC_FIELD\n");
                pc++;

                ivalue1 = *pc;             // class name
                pc++;

                ivalue2 = *pc;             // field index
                pc++;
                break;

            case OP_SR_STATIC_FIELD:
                pc++;

                ivalue1 = *pc;                    // class name
                pc++;

                ivalue2 = *pc;                    // field index
                pc++;
                break;

            case OP_NEW_OBJECT:
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */
                break;

            case OP_NEW_ARRAY:
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
                pc++;

                /*
                pc_in_stack = pc;
                type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                pc = pc_in_stack;
                push_object(type1, info);
                */
                break;

            case OP_NEW_TUPLE:
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
                break;

            case OP_INVOKE_METHOD:
                pc++;

                /// method data ///
                ivalue1 = *pc;                  // real class name offset
                pc++;

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
/*
                    pc_in_stack = pc;
                    type1 = create_type_object_from_bytecodes(&pc_in_stack, code, constant, info);
                    pc = pc_in_stack;
                    push_object(type1, info);
*/
/*
                    pc_in_stack = pc;
                    if(!get_class_info_from_bytecode(&klass2, &pc_in_stack, constant, info))
                    {
                        vm_mutex_unlock();
                        return FALSE;
                    }
                    pc = pc_in_stack;
*/
                }

                break;

            case OP_INVOKE_VIRTUAL_METHOD: {
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
                pc++;

                ivalue1 = *pc;      // star
                pc++;

                break;

            case OP_CALL_PARAM_INITIALIZER: {
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
                pc++;

                ivalue2 = *pc;
                pc++;
                }
                break;

            case OP_NOTIF: {
                pc++;
                ivalue2 = *pc;
                pc++;
                }
                break;

            case OP_GOTO:
                pc++;
                break;

            case OP_RETURN:
                pc++;
                return TRUE;

            case OP_THROW:
                pc++;
                return FALSE;

            case OP_TRY:
                pc++;

                ivalue1 = *pc;                  // catch block number
                pc++;

                ivalue2 = *pc;                  // finally block existance
                pc++;

                ivalue3 = *pc;                  // finally block result existance
                pc++;
                break;

            case OP_INVOKE_BLOCK:
                pc++;

                ivalue1 = *pc;          // block index
                pc++;

                ivalue2 = *pc;         // result existance
                pc++;
                break;

            case OP_BREAK_IN_METHOD_BLOCK:
                pc++;

                return TRUE;

            case OP_OUTPUT_RESULT: 
                pc++;
                break;

            case OP_POP:
                pc++;
                break;

            case OP_POP_N:
                pc++;

                ivalue1 = *pc;
                pc++;

                break;

            case OP_POP_N_WITHOUT_TOP:
                pc++;

                ivalue1 = *pc;
                pc++;
                
                break;

            case OP_DUP:
                pc++;
                break;

            case OP_SWAP:
                pc++;
                break;

            case OP_INC_VALUE:
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            case OP_DEC_VALUE:
                pc++;

                ivalue1 = *pc;
                pc++;
                break;

            default:
                fprintf(stderr, "unexpected error\n");
                pc++;
        }
    }

    /// compile codes ///
    LLVMValueRef result_value = LLVMConstInt(LLVMInt32Type(), 1, 0);
    LLVMBuildRet(builder, result_value);

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

