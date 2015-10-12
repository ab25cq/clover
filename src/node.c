#include "clover.h"
#include "common.h"
#include <ctype.h>

static BOOL load_local_varialbe_from_var_index(int index, sCLNodeType** type_, sCompileInfo* info);

//////////////////////////////////////////////////
// Compile nodes to byte codes
//////////////////////////////////////////////////
static void inc_stack_num(int* stack_num, int* max_stack, int value)
{
    (*stack_num)+=value;
    if(*stack_num > *max_stack)  {
        *max_stack = *stack_num;
    }
}

static void dec_stack_num(int* stack_num, int value)
{
    (*stack_num)-=value;
}

static BOOL check_private_access(sCLClass* klass, sCLClass* access_class)
{
    if(klass == access_class) {
        return TRUE;
    }

    return FALSE;
}

static BOOL is_called_from_inside(sCLClass* real_caller_class, sCLClass* klass)
{
    return real_caller_class && klass && (real_caller_class == klass || is_parent_class(real_caller_class, klass));
}

static void show_caller_method(char* method_name, sCLNodeType** class_params, int num_params, BOOL existance_of_block, sCLNodeType** block_class_params, int block_num_params, sCLNodeType* block_type, sCompileInfo* info)
{
    int i;

    parser_err_msg_without_line("called method type --> %s(", method_name);

    for(i=0; i<num_params; i++) {
        show_node_type_for_errmsg(class_params[i]);

        if(i != num_params-1) parser_err_msg_without_line(",");
    }

    if(!existance_of_block) {
        parser_err_msg_without_line(")\n");
    }
    else {
        parser_err_msg_without_line(") with ");
        show_node_type_for_errmsg(block_type);
        parser_err_msg_without_line(" block{|");
        for(i=0; i<block_num_params; i++) {
            show_node_type_for_errmsg(block_class_params[i]);

            if(i != block_num_params-1) parser_err_msg_without_line(",");
        }
        parser_err_msg_without_line("|}\n");
    }
}

static int get_parent_var_num(sNodeBlock* block)
{
    int result;
    sVarTable* it;

    result = 0;

    it = block->mLVTable->mParent;

    while(it) {
        result += it->mVarNum;

        it = it->mParent;
    }

    return result;
}

static BOOL do_call_method_with_duck_typing(sCLClass* klass, sCLMethod* method, char* method_name,  BOOL class_method,  sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, sCLNodeType* result_type)
{
    int i;
    int n;
    BOOL method_num_block_type;
    int method_num_params;

    ASSERT(method == NULL);

    method_num_params = *num_params;

    method_num_block_type = block_exist ? 1:0;
    
    /// make block ///
    if(block_id) {
        sConst constant;
        sByteCode code;
        sCLNodeType* dummy;
        sCLNodeType caller_class;
        int j;

        sConst_init(&constant);
        sByteCode_init(&code);

        /// compile block ///
        dummy = clone_node_type(*type_);
        caller_class.mClass = klass;
        caller_class.mGenericsTypesNum = 0;
        if(!compile_block_object(&gNodeBlocks[block_id], &constant, &code, &dummy, info, &caller_class, method, kBKMethodBlock)) {
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return TRUE;
        }

        append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mMaxStack, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumLocals, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumParams, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, get_parent_var_num(&gNodeBlocks[block_id]), info->no_output_to_bytecodes);

        append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mBlockType, info->no_output_to_bytecodes);
        for(j=0; j<gNodeBlocks[block_id].mNumParams; j++) {
            append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mClassParams[j], info->no_output_to_bytecodes);
        }

        append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
        append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, 3, info->no_output_to_bytecodes);          // breakable

        inc_stack_num(info->stack_num, info->max_stack, 1);

        FREE(constant.mConst);
        FREE(code.mCode);
    }

    /// make code ///
    append_opecode_to_bytecodes(info->code, OP_INVOKE_VIRTUAL_METHOD, info->no_output_to_bytecodes);

    append_str_to_bytecodes(info->code, info->constant, method_name, info->no_output_to_bytecodes);   // method name

    append_int_value_to_bytecodes(info->code, method_num_params, info->no_output_to_bytecodes); // method num params

    append_int_value_to_bytecodes(info->code, block_exist, info->no_output_to_bytecodes);           // existance of block

    append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);               // an existance of result flag
    append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);               // a flag of calling super
    append_int_value_to_bytecodes(info->code, class_method, info->no_output_to_bytecodes);                // a flag of class method kind
    append_int_value_to_bytecodes(info->code, method_num_block_type, info->no_output_to_bytecodes);       // method num block type

    if(class_method)
    {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS, info->no_output_to_bytecodes);

        append_generics_type_to_bytecode(info->code, info->constant, (*type_), info->no_output_to_bytecodes);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
    }
    else {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT, info->no_output_to_bytecodes);
    }

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params+(block_exist?1:0));
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1+(block_exist?1:0));
    }

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = result_type;

    return TRUE;
}

static void fold_variable_arguments_to_array(sCLMethod* method, int num_params, sCompileInfo* info)
{
    int num_variable_argument;
    sCLNodeType* anonymous_array;

    num_variable_argument = num_params - (method->mNumParams-1);

    anonymous_array = alloc_node_type();
    anonymous_array->mClass = cl_get_class("Array$1");
    ASSERT(anonymous_array->mClass != NULL);
    anonymous_array->mGenericsTypesNum = 1;
    anonymous_array->mGenericsTypes[0] = alloc_node_type();
    anonymous_array->mGenericsTypes[0]->mClass = gAnonymousClass;
    anonymous_array->mGenericsTypes[0]->mGenericsTypesNum = 0;

    append_opecode_to_bytecodes(info->code, OP_FOLD_PARAMS_TO_ARRAY, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, num_variable_argument, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, method->mNumBlockType, info->no_output_to_bytecodes);       // method num block type
    append_generics_type_to_bytecode(info->code, info->constant, anonymous_array, info->no_output_to_bytecodes);

    dec_stack_num(info->stack_num, num_variable_argument-1);
}

static BOOL do_call_method(sCLClass* klass, sCLMethod* method, char* method_name,  BOOL class_method, BOOL calling_super, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, int used_param_num_with_initializer, sCLNodeType* result_type, unsigned int block_node, sCLClass* class_of_class_method_call)
{
    int method_num_params;
    int i;
    int n;

    /// check of private method ///
    if(method->mFlags & CL_PRIVATE_METHOD && !check_private_access(klass, info->real_caller_class ? info->real_caller_class->mClass:NULL)) {
        parser_err_msg_format(info->sname, *info->sline, "this is private method(%s).", METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(method->mFlags & CL_PROTECTED_METHOD && !is_called_from_inside(info->real_caller_class ? info->real_caller_class->mClass:NULL, klass)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "this is protected method(%s).", METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// check of method kind ///
    if(class_method) {
        if((method->mFlags & CL_CLASS_METHOD) == 0) {
            parser_err_msg_format(info->sname, *info->sline, "This is not class method(%s)", METHOD_NAME2(klass, method));
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    else {
        if(method->mFlags & CL_CLASS_METHOD) {
            parser_err_msg_format(info->sname, *info->sline, "This is class method(%s)", METHOD_NAME2(klass, method));
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    
    /// append param info to class_params tail ///
    for(i=method->mNumParams-used_param_num_with_initializer; 
        i < method->mNumParams; 
        i++) 
    {
        sCLParamInitializer* param_initializer;
        sByteCode* code;
        sConst* constant;
        int max_stack;
        int lv_num;

        class_params[i] = ALLOC create_node_type_from_cl_type(method->mParamTypes + i, klass);

        param_initializer = method->mParamInitializers + i;
        code = &param_initializer->mInitializer;
        max_stack = param_initializer->mMaxStack;
        lv_num = param_initializer->mLVNum;

        append_opecode_to_bytecodes(info->code, OP_CALL_PARAM_INITIALIZER, info->no_output_to_bytecodes);

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
        append_code_to_bytecodes(info->code, info->constant, code, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, max_stack, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, lv_num, info->no_output_to_bytecodes);

        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    /// make block ///
    if(block_id) {
        int j;
        sConst constant;
        sByteCode code;
        sCLNodeType* dummy;
        sCLNodeType caller_class;

        sConst_init(&constant);
        sByteCode_init(&code);

        /// compile block ///
        dummy = clone_node_type(*type_);
        caller_class.mClass = klass;
        caller_class.mGenericsTypesNum = 0;
        if(!compile_block_object(&gNodeBlocks[block_id], &constant, &code, &dummy, info, &caller_class, method, kBKMethodBlock)) {
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return TRUE;
        }

        append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mMaxStack, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumLocals, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumParams, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, get_parent_var_num(&gNodeBlocks[block_id]), info->no_output_to_bytecodes);

        append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mBlockType, info->no_output_to_bytecodes);
        for(j=0; j<gNodeBlocks[block_id].mNumParams; j++) {
            append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mClassParams[j], info->no_output_to_bytecodes);
        }

        append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
        append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
        if(type_identity(gNodeBlocks[block_id].mBlockType,gBoolType)) {
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);          // breakable
        }
        else {
            append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);          // breakable
        }

        inc_stack_num(info->stack_num, info->max_stack, 1);

        FREE(constant.mConst);
        FREE(code.mCode);
    }

    if(method->mNumBlockType == 1 && block_node == 0 && block_id == 0) {
        parser_err_msg_format(info->sname, *info->sline, "this method(%s::%s) should be call with block", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// make code ///
    method_num_params = get_method_num_params(method);

    if(method->mFlags & CL_VIRTUAL_METHOD || klass->mFlags & CLASS_FLAGS_INTERFACE || method->mFlags & CL_ABSTRACT_METHOD || klass->mFlags & CLASS_FLAGS_DYNAMIC_TYPING) 
    {
        append_opecode_to_bytecodes(info->code, OP_INVOKE_VIRTUAL_METHOD, info->no_output_to_bytecodes);

        append_str_to_bytecodes(info->code, info->constant, method_name, info->no_output_to_bytecodes);   // method name

        append_int_value_to_bytecodes(info->code, method_num_params, info->no_output_to_bytecodes); // method num params

        append_int_value_to_bytecodes(info->code, block_exist, info->no_output_to_bytecodes); // existance of block

        append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes); // an existance of result flag
        append_int_value_to_bytecodes(info->code, calling_super, info->no_output_to_bytecodes);               // a flag of calling super
        append_int_value_to_bytecodes(info->code, class_method, info->no_output_to_bytecodes);                // a flag of class method kind
        append_int_value_to_bytecodes(info->code, method->mNumBlockType, info->no_output_to_bytecodes);       // method num block type

        if(class_method)
        {
            append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, (*type_), info->no_output_to_bytecodes);
            if(class_of_class_method_call) {
                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(class_of_class_method_call), info->no_output_to_bytecodes);
            }
            else {
                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
            }
        }
        else {
            append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT, info->no_output_to_bytecodes);
        }
    }
    else {
        int method_index;

        if(method->mFlags & CL_METHOD_PARAM_VARABILE_ARGUMENTS) {
            fold_variable_arguments_to_array(method, *num_params, info);
        }

        append_opecode_to_bytecodes(info->code, OP_INVOKE_METHOD, info->no_output_to_bytecodes);

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);

        method_index = get_method_index(klass, method);

        append_int_value_to_bytecodes(info->code, method_index, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes); 
        append_int_value_to_bytecodes(info->code, method->mNumParams, info->no_output_to_bytecodes);          // num params
        append_int_value_to_bytecodes(info->code, method->mNumBlockType, info->no_output_to_bytecodes);       // method num block type
        append_int_value_to_bytecodes(info->code, (method->mFlags & CL_CLASS_METHOD) ? 1:0, info->no_output_to_bytecodes);       // class method
        append_str_to_bytecodes(info->code, info->constant, METHOD_NAME2(klass, method), info->no_output_to_bytecodes);         // method name

        if(class_method)
        {
            append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, (*type_), info->no_output_to_bytecodes);
            if(class_of_class_method_call) {
                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(class_of_class_method_call), info->no_output_to_bytecodes);
            }
            else {
                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
            }
        }
        else {
            append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT, info->no_output_to_bytecodes);
        }
    }

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params+(block_exist?1:0));
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1+(block_exist?1:0));
    }

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = result_type;

    return TRUE;
}

static BOOL do_call_mixin(sCLMethod* method, int method_index, BOOL class_method, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, int used_param_num_with_initializer, sCLNodeType* result_type, BOOL block_exist, unsigned int block_id, unsigned int block_node)
{
    int method_num_params;
    int offset;
    int i;
    sCLClass* klass;

    klass = (*type_)->mClass;

    /// check of private method ///
    if(method->mFlags & CL_PRIVATE_METHOD && !check_private_access(klass, info->real_caller_class ? info->real_caller_class->mClass:NULL)) {
        parser_err_msg_format(info->sname, *info->sline, "this is private method(%s).", METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(method->mFlags & CL_PROTECTED_METHOD && !is_called_from_inside(info->real_caller_class ? info->real_caller_class->mClass:NULL, klass)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "this is protected method(%s).", METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// check of method kind ///
    if(class_method) {
        if((method->mFlags & CL_CLASS_METHOD) == 0) {
            parser_err_msg_format(info->sname, *info->sline, "This is not class method(%s)", METHOD_NAME2(klass, method));
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    else {
        if(method->mFlags & CL_CLASS_METHOD) {
            parser_err_msg_format(info->sname, *info->sline, "This is class method(%s)", METHOD_NAME2(klass, method));
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    /// append param info to class_params tail ///
    for(i=method->mNumParams-used_param_num_with_initializer; 
        i < method->mNumParams;
        i++) 
    {
        sCLParamInitializer* param_initializer;
        sByteCode* code;
        sConst* constant;
        int max_stack;
        int lv_num;

        class_params[i] = ALLOC create_node_type_from_cl_type(method->mParamTypes + i, klass);

        param_initializer = method->mParamInitializers + i;
        code = &param_initializer->mInitializer;
        max_stack = param_initializer->mMaxStack;
        lv_num = param_initializer->mLVNum;

        append_opecode_to_bytecodes(info->code, OP_CALL_PARAM_INITIALIZER, info->no_output_to_bytecodes);

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
        append_code_to_bytecodes(info->code, info->constant, code, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, max_stack, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, lv_num, info->no_output_to_bytecodes);

        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    /// make block ///
    if(block_id) {
        sConst constant;
        sByteCode code;
        sCLNodeType* dummy;
        sCLNodeType caller_class;
        int j;

        sConst_init(&constant);
        sByteCode_init(&code);

        /// compile block ///
        dummy = clone_node_type(*type_);
        caller_class.mClass = klass;
        caller_class.mGenericsTypesNum = 0;
        if(!compile_block_object(&gNodeBlocks[block_id], &constant, &code, &dummy, info, &caller_class, method, kBKMethodBlock)) {
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return TRUE;
        }

        append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mMaxStack, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumLocals, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumParams, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, get_parent_var_num(&gNodeBlocks[block_id]), info->no_output_to_bytecodes);

        append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mBlockType, info->no_output_to_bytecodes);
        for(j=0; j<gNodeBlocks[block_id].mNumParams; j++) {
            append_generics_type_to_bytecode(info->code, info->constant, gNodeBlocks[block_id].mClassParams[j], info->no_output_to_bytecodes);
        }

        append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
        append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
        if(type_identity(gNodeBlocks[block_id].mBlockType, gBoolType)) {
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);          // breakable
        }
        else {
            append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);          // breakable
        }

        inc_stack_num(info->stack_num, info->max_stack, 1);

        FREE(constant.mConst);
        FREE(code.mCode);
    }

    if(method->mNumBlockType == 1 && block_node == 0 && block_id == 0) {
        parser_err_msg_format(info->sname, *info->sline, "this method(%s::%s) should be call with block", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// make code ///
    if(method->mFlags & CL_METHOD_PARAM_VARABILE_ARGUMENTS) {
        fold_variable_arguments_to_array(method, *num_params, info);
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_METHOD, info->no_output_to_bytecodes);

    append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);

    append_int_value_to_bytecodes(info->code, method_index, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);  // existance of result
    append_int_value_to_bytecodes(info->code, method->mNumParams, info->no_output_to_bytecodes);          // num params
    append_int_value_to_bytecodes(info->code, method->mNumBlockType, info->no_output_to_bytecodes);       // method num block type
    append_int_value_to_bytecodes(info->code, (method->mFlags & CL_CLASS_METHOD) ? 1:0, info->no_output_to_bytecodes);       // class method
    append_str_to_bytecodes(info->code, info->constant, METHOD_NAME2(klass, method), info->no_output_to_bytecodes);         // method name

    if(class_method)
    {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS, info->no_output_to_bytecodes);

        append_generics_type_to_bytecode(info->code, info->constant, (*type_), info->no_output_to_bytecodes);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass), info->no_output_to_bytecodes);
    }
    else {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT, info->no_output_to_bytecodes);
    }

    method_num_params = get_method_num_params(method);

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params+(block_exist?1:0));
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1+(block_exist?1:0));
    }

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = result_type;

    return TRUE;
}

static BOOL params_of_cl_type_to_params_of_node_type(ALLOC sCLNodeType** result, sCLType* params, int num_params, sCLClass* klass)
{
    int i;

    for(i=0; i<num_params; i++) {
        result[i] = ALLOC create_node_type_from_cl_type(&params[i], klass);
    }

    return TRUE;
}

static BOOL search_for_method_of_non_generics(sCLClass** klass, sCLMethod** method, sCLNodeType** type_, sCompileInfo* info, char* method_name, sCLNodeType** class_params, int* num_params, BOOL class_method, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type, unsigned int block_id, sCLNodeType** err_messsage_class_params)
{
    int i;

    for(i=0; i<*num_params; i++) {              // for error message
        err_messsage_class_params[i] = class_params[i];
    }

    *klass = (*type_)->mClass;
    *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, NULL, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

    /// next, search for a method of super classes ///
    if(*method) {
        return TRUE;
    }
    else {
        sCLNodeType* founeded_class;

        *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founeded_class, class_method, *type_, NULL, block_id != 0 ? 1:0, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

        /// found on super classes ///
        if(*method) {
            *klass = founeded_class->mClass;
            *type_ = founeded_class;

            return TRUE;
        }
    }

    return TRUE;
}

static BOOL search_for_method_of_generics_param_type(sCLClass** klass, sCLMethod** method, sCLNodeType** type_, sCompileInfo* info, char* method_name, sCLNodeType** class_params, int* num_params, BOOL class_method, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type, sCLNodeType** err_messsage_class_params)
{
    int i;
    sGenericsParamPattern* generics_type_patterns;
    int generics_type_pattern_num;

    create_generics_param_type_pattern(ALLOC &generics_type_patterns, &generics_type_pattern_num, info->real_caller_class);

    for(i=0; i<*num_params; i++) {              // for error message
        err_messsage_class_params[i] = class_params[i];
    }

    if(generics_type_pattern_num == 0) {
        sCLGenericsParamTypes* generics_param_types;

        generics_param_types = get_generics_param_types((*type_)->mClass, info->real_caller_class->mClass, info->real_caller_method);

        *method = NULL;

        if(generics_param_types == NULL) {
            *klass = (*type_)->mClass;

            *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, NULL, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                FREE(generics_type_patterns);
                return TRUE;
            }
            else {
                sCLNodeType* founded_class;

                *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    *type_ = founded_class;
                    *klass = founded_class->mClass;
                    FREE(generics_type_patterns);
                    return TRUE;
                }
            }
        }
        else if(generics_param_types->mExtendsType.mClassNameOffset != 0) 
        {
            sCLNodeType* extends_type;

            extends_type = ALLOC create_node_type_from_cl_type(&generics_param_types->mExtendsType, info->real_caller_class->mClass);

            *klass = extends_type->mClass;
            *type_ = extends_type;

            *method = get_method_with_type_params_and_param_initializer(extends_type, method_name, class_params, *num_params, class_method, *type_, NULL, extends_type->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                FREE(generics_type_patterns);
                return TRUE;
            }
            else {
                sCLNodeType* founded_class;

                *method = get_method_with_type_params_and_param_initializer_on_super_classes(extends_type, method_name, class_params, *num_params, &founded_class, class_method, *type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    *type_ = founded_class;
                    *klass = founded_class->mClass;
                    FREE(generics_type_patterns);
                    return TRUE;
                }
            }
        }
        else if(generics_param_types->mNumImplementsTypes > 0) {
            int j;
            sCLNodeType* klass2;

            klass2 = *type_;

            for(j=0; j<generics_param_types->mNumImplementsTypes; j++) 
            {
                sCLNodeType* implements_type;

                implements_type = ALLOC create_node_type_from_cl_type(&generics_param_types->mImplementsTypes[j], info->real_caller_class->mClass);

                *type_ = implements_type;
                *klass = implements_type->mClass;

                *method = get_method_with_type_params_and_param_initializer(implements_type, method_name, class_params, *num_params, class_method, *type_, NULL, implements_type->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    FREE(generics_type_patterns);
                    return TRUE;
                }
                else {
                    sCLNodeType* founded_class;

                    *method = get_method_with_type_params_and_param_initializer_on_super_classes(implements_type, method_name, class_params, *num_params, &founded_class, class_method, *type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                    if(*method) {
                        *type_ = founded_class;
                        *klass = founded_class->mClass;
                        FREE(generics_type_patterns);
                        return TRUE;
                    }
                }
            }
        }
        else {
            *klass = gObjectClass;
            *type_ = gObjectType;

            *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, NULL, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                FREE(generics_type_patterns);
                return TRUE;
            }
            else {
                sCLNodeType* founded_class;

                *method = get_method_with_type_params_and_param_initializer_on_super_classes((*type_), method_name, class_params, *num_params, &founded_class, class_method, *type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    *type_ = founded_class;
                    *klass = founded_class->mClass;
                    FREE(generics_type_patterns);
                    return TRUE;
                }
            }
        }
    }
    else {
        int k;

        for(k=0; k<generics_type_pattern_num; k++) {
            sGenericsParamPattern* pattern;
            sCLNodeType generics_solving_type;
            sCLGenericsParamTypes* generics_param_types;
            int j;

            pattern = generics_type_patterns + k;

            memset(&generics_solving_type, 0, sizeof(sCLNodeType));

            for(j=0; j<pattern->mGenericsTypesNum; j++) {
                generics_solving_type.mGenericsTypes[j] = pattern->mGenericsTypes[j];
            }
            generics_solving_type.mGenericsTypesNum = pattern->mGenericsTypesNum;

            generics_param_types = get_generics_param_types((*type_)->mClass, info->real_caller_class->mClass, info->real_caller_method);

            *method = NULL;

            if(generics_param_types == NULL) {
                *klass = (*type_)->mClass;

                *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    FREE(generics_type_patterns);
                    return TRUE;
                }
                else {
                    sCLNodeType* founded_class;

                    *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                    if(*method) {
                        *type_ = founded_class;
                        *klass = founded_class->mClass;
                        FREE(generics_type_patterns);
                        return TRUE;
                    }
                }
            }
            else {
                if(generics_param_types->mExtendsType.mClassNameOffset != 0) 
                {
                    sCLNodeType* extends_type;

                    extends_type = ALLOC create_node_type_from_cl_type(&generics_param_types->mExtendsType, info->real_caller_class->mClass);

                    *klass = extends_type->mClass;
                    *type_ = extends_type;

                    *method = get_method_with_type_params_and_param_initializer(extends_type, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, extends_type->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                    if(*method) {
                        FREE(generics_type_patterns);
                        return TRUE;
                    }
                    else {
                        sCLNodeType* founded_class;

                        *method = get_method_with_type_params_and_param_initializer_on_super_classes(extends_type, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                        if(*method) {
                            *type_ = founded_class;
                            *klass = founded_class->mClass;
                            FREE(generics_type_patterns);
                            return TRUE;
                        }
                    }
                }
                else if(generics_param_types->mNumImplementsTypes > 0) 
                {
                    int j;
                    sCLNodeType* klass2;

                    klass2 = *type_;

                    for(j=0; j<generics_param_types->mNumImplementsTypes; j++) {
                        sCLNodeType* implements_type;

                        implements_type = ALLOC create_node_type_from_cl_type(&generics_param_types->mImplementsTypes[j], info->real_caller_class->mClass);

                        *type_ = implements_type;
                        *klass = implements_type->mClass;

                        *method = get_method_with_type_params_and_param_initializer(implements_type, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, implements_type->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);
                        if(*method) {
                            FREE(generics_type_patterns);
                            return TRUE;
                        }
                        else {
                            sCLNodeType* founded_class;

                            *method = get_method_with_type_params_and_param_initializer_on_super_classes(implements_type, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                            if(*method) {
                                *type_ = founded_class;
                                *klass = founded_class->mClass;
                                FREE(generics_type_patterns);
                                return TRUE;
                            }
                        }
                    }

                    *klass = gObjectClass;
                    *type_ = gObjectType;

                    *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                    if(*method) {
                        FREE(generics_type_patterns);
                        return TRUE;
                    }
                    else {
                        sCLNodeType* founded_class;

                        *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                        if(*method) {
                            *type_ = founded_class;
                            *klass = founded_class->mClass;
                            FREE(generics_type_patterns);
                            return TRUE;
                        }
                    }
                }
                else {
                    *klass = gObjectClass;
                    *type_ = gObjectType;

                    *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                    if(*method) {
                        FREE(generics_type_patterns);
                        return TRUE;
                    }
                    else {
                        sCLNodeType* founded_class;

                        *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                        if(*method) {
                            *type_ = founded_class;
                            *klass = founded_class->mClass;
                            FREE(generics_type_patterns);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    FREE(generics_type_patterns);

    return TRUE;
}

static BOOL search_for_method_of_inner_generics_param_class(sCLClass** klass, sCLMethod** method, sCLNodeType** type_, sCompileInfo* info, char* method_name, sCLNodeType** class_params, int* num_params, BOOL class_method, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type, sCLNodeType** err_messsage_class_params)
{
    int i;
    sGenericsParamPattern* generics_type_patterns;
    int generics_type_pattern_num;

    create_generics_param_type_pattern(ALLOC &generics_type_patterns, &generics_type_pattern_num, info->real_caller_class);

    for(i=0; i<*num_params; i++) {              // for error message
        err_messsage_class_params[i] = class_params[i];
    }

    if(generics_type_pattern_num == 0) {
        *method = NULL;

        *klass = (*type_)->mClass;

        *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, NULL, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

        if(*method) {
            FREE(generics_type_patterns);
            return TRUE;
        }
        else {
            sCLNodeType* founded_class;

            *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                *type_ = founded_class;
                *klass = founded_class->mClass;
                FREE(generics_type_patterns);
                return TRUE;
            }
        }
    }
    else {
        int k;

        for(k=0; k<generics_type_pattern_num; k++) {
            sGenericsParamPattern* pattern;
            sCLNodeType generics_solving_type;
            int j;

            pattern = generics_type_patterns + k;

            memset(&generics_solving_type, 0, sizeof(sCLNodeType));

            for(j=0; j<pattern->mGenericsTypesNum; j++) {
                generics_solving_type.mGenericsTypes[j] = pattern->mGenericsTypes[j];
            }
            generics_solving_type.mGenericsTypesNum = pattern->mGenericsTypesNum;

            *method = NULL;

            *klass = (*type_)->mClass;

            *method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, class_method, *type_, &generics_solving_type, (*type_)->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                FREE(generics_type_patterns);
                return TRUE;
            }
            else {
                sCLNodeType* founded_class;

                *method = get_method_with_type_params_and_param_initializer_on_super_classes(*type_, method_name, class_params, *num_params, &founded_class, class_method, *type_, &generics_solving_type, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

                if(*method) {
                    *type_ = founded_class;
                    *klass = founded_class->mClass;
                    FREE(generics_type_patterns);
                    return TRUE;
                }
            }
        }
    }

    FREE(generics_type_patterns);

    return TRUE;
}

static BOOL search_for_method_of_outside_generics_param_class(sCLClass** klass, sCLMethod** method, sCLNodeType** type_, sCompileInfo* info, char* method_name, sCLNodeType** class_params, int* num_params, BOOL class_method, BOOL block_exist, int block_num_params, sCLNodeType** block_param_types, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type, sCLNodeType** err_messsage_class_params)
{
    int k;
    sGenericsParamPattern* generics_type_patterns;
    int generics_type_pattern_num;

    create_generics_param_type_pattern(ALLOC &generics_type_patterns, &generics_type_pattern_num, info->real_caller_class);

    for(k=0; k<*num_params; k++) {            // for error message
        err_messsage_class_params[k] = class_params[k];
    }

    for(k=0; k<generics_type_pattern_num; k++) {
        sGenericsParamPattern* pattern;
        sCLNodeType generics_solving_type;
        int j;
        sCLNodeType* solved_type_;

        pattern = generics_type_patterns + k;

        memset(&generics_solving_type, 0, sizeof(sCLNodeType));

        for(j=0; j<pattern->mGenericsTypesNum; j++) {
            generics_solving_type.mGenericsTypes[j] = pattern->mGenericsTypes[j];
        }
        generics_solving_type.mGenericsTypesNum = pattern->mGenericsTypesNum;

        if(!solve_generics_types_for_node_type(*type_, ALLOC &solved_type_, &generics_solving_type))
        {
            return FALSE;
        }

        *klass = solved_type_->mClass;

        *method = get_method_with_type_params_and_param_initializer(solved_type_, method_name, class_params, *num_params, class_method, solved_type_, NULL, solved_type_->mClass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

        if(*method) {
            return TRUE;
        }
        else {
            sCLNodeType* founded_class;

            *method = get_method_with_type_params_and_param_initializer_on_super_classes(solved_type_, method_name, class_params, *num_params, &founded_class, class_method, solved_type_, NULL, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type);

            if(*method) {
                *type_ = founded_class;
                *klass = founded_class->mClass;
                return TRUE;
            }
        }
    }

    return TRUE;
}

static BOOL method_not_found(sCLNodeType** type_, sCompileInfo* info, char* method_name, sCLNodeType** class_params, int* num_params, unsigned int block_id, BOOL no_defined_no_call, BOOL* not_found_method)
{
    sCLMethod* method;

    if(no_defined_no_call) {
        *not_found_method = TRUE;
        *type_ = gIntType; // dummy
        return FALSE;
    }
    else {
        method = get_method((*type_)->mClass, method_name);

        if(method) {
            parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types of this method(%s::%s)", CLASS_NAME((*type_)->mClass), method_name);
            show_all_method_for_errmsg((*type_)->mClass, method_name);
            if(block_id) {
                show_caller_method(method_name, class_params, *num_params, block_id, gNodeBlocks[block_id].mClassParams, gNodeBlocks[block_id].mNumParams, gNodeBlocks[block_id].mBlockType, info);
            }
            else {
                show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, 0, info);
            }
            (*info->err_num)++;
        }
        else {
            sCLClass* new_class;

            method = get_method_on_super_classes((*type_)->mClass, method_name, &new_class);

            if(method) {
                parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types of this method(%s::%s)", CLASS_NAME(new_class), method_name);
                show_all_method_for_errmsg(new_class, method_name);

                if(block_id) {
                    show_caller_method(method_name, class_params, *num_params, block_id, gNodeBlocks[block_id].mClassParams, gNodeBlocks[block_id].mNumParams, gNodeBlocks[block_id].mBlockType, info);
                }
                else {
                    show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, 0, info);
                }

                (*info->err_num)++;
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "There is not this method(%s) on this class(%s)", method_name, REAL_CLASS_NAME((*type_)->mClass));
                if(block_id) {
                    show_caller_method(method_name, class_params, *num_params, block_id, gNodeBlocks[block_id].mClassParams, gNodeBlocks[block_id].mNumParams, gNodeBlocks[block_id].mBlockType, info);
                }
                else {
                    show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, 0, info);
                }
                (*info->err_num)++;
            }
        }

        *type_ = gIntType; // dummy
        return FALSE;
    }

    return TRUE;
}

static BOOL determine_the_calling_method(sCLClass** klass, sCLMethod** method, char* method_name, BOOL class_method, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, BOOL no_defined_no_call, BOOL* not_found_method, unsigned int block_node, int* block_num_params, sCLNodeType** block_type, sCLNodeType* block_param_types[CL_METHOD_PARAM_MAX], BOOL* block_exist, int* used_param_num_with_initializer, sCLNodeType** result_type, sCLNodeType* err_messsage_class_params[CL_METHOD_PARAM_MAX])
{
    *not_found_method = FALSE;

    if(*type_ == NULL || (*type_)->mClass == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid method call. there is not type or class");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// get block params ///
    if(block_id) {
        int j;
        *block_exist = TRUE;

        *block_num_params = gNodeBlocks[block_id].mNumParams;

        for(j=0; j<*block_num_params; j++) {
            block_param_types[j] = clone_node_type(gNodeBlocks[block_id].mClassParams[j]);
        }
        *block_type = gNodeBlocks[block_id].mBlockType;
    }
    else if(block_node) {
        int j;

        if(info->real_caller_class && info->real_caller_method) {
            *block_exist = TRUE;

            *block_num_params = info->real_caller_method->mBlockType.mNumParams;
            for(j=0; j<*block_num_params; j++) {
                block_param_types[j] = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mParamTypes[j], info->real_caller_class->mClass);
            }

            *block_type = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mResultType, info->real_caller_class->mClass);
        }
        else {
            *block_exist = FALSE;

            *block_num_params = 0;
            memset(block_param_types, 0, sizeof(sCLNodeType*)*CL_METHOD_PARAM_MAX);
            *block_type = NULL;
        }
    }
    else {
        *block_exist = FALSE;

        *block_num_params = 0;
        memset(block_param_types, 0, sizeof(sCLNodeType*)*CL_METHOD_PARAM_MAX);
        *block_type = NULL;
    }

    *klass = NULL;
    *method = NULL;

    ASSERT(*type_ != NULL && (*type_)->mClass != NULL);
    ASSERT((*type_)->mClass->mGenericsTypesNum == (*type_)->mGenericsTypesNum);         // checked on parser.c
    ASSERT(!is_generics_param_class((*type_)->mClass) || is_generics_param_class((*type_)->mClass) && (*type_)->mClass->mGenericsTypesNum == 0);

    /// search for a method of generics param class ///
    if(is_generics_param_type(*type_)) {
        /// T->method(T, T2, ...) pattern ///
        if(is_generics_param_class((*type_)->mClass)) {
            if(!search_for_method_of_generics_param_type(klass, method, type_, info, method_name, class_params, num_params, class_method, *block_exist, *block_num_params, block_param_types, *block_type, used_param_num_with_initializer, result_type, err_messsage_class_params)) 
            {
                return TRUE;
            }

            /// check generics newable for the constructor ///
            if(*method && strcmp(method_name, "_constructor") == 0 && !((*method)->mFlags & CL_GENERICS_NEWABLE_CONSTRUCTOR) && !((*type_)->mClass->mFlags & CLASS_FLAGS_INTERFACE))
            {
                parser_err_msg_format(info->sname, *info->sline, "require generics_newable method type for the constructor of generics class");
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                return TRUE;
            }
        }
        /// InnerClass<T, T2>->method(T, T2, ...) pattern ///
        else if(info->real_caller_class && info->real_caller_class->mClass && ((*type_)->mClass == info->real_caller_class->mClass || is_parent_class((*type_)->mClass, info->real_caller_class->mClass))) 
        {
            if(!search_for_method_of_inner_generics_param_class(klass, method, type_, info, method_name, class_params, num_params, class_method, *block_exist, *block_num_params, block_param_types, *block_type, used_param_num_with_initializer, result_type, err_messsage_class_params)) 
            {
                return TRUE;
            }
        }
        /// OuterClass<T, T2>->method(T, T2, ...) pattern ///
        else {
            if(!search_for_method_of_outside_generics_param_class(klass, method, type_, info, method_name, class_params, num_params, class_method, *block_exist, *block_num_params, block_param_types, *block_type, used_param_num_with_initializer, result_type, err_messsage_class_params)) 
            {
                return TRUE;
            }
        }
    }
    /// search for a method ///
    else {
        /// non generics pattern ///
        if(!search_for_method_of_non_generics(klass, method, type_, info, method_name, class_params, num_params, class_method, *block_exist, *block_num_params, block_param_types, *block_type, used_param_num_with_initializer, result_type, block_id, err_messsage_class_params))
        {
            return TRUE;
        }
    }

    return TRUE;
}

static BOOL call_method(char* method_name, BOOL class_method, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, BOOL no_defined_no_call, BOOL* not_found_method, unsigned int block_node, sCLClass* class_of_class_method_call)
{
    sCLClass* klass;
    sCLMethod* method;
    sCLNodeType type_before;
    int block_num_params;
    sCLNodeType* block_type;
    sCLNodeType* block_param_types[CL_METHOD_PARAM_MAX];
    int used_param_num_with_initializer;
    sCLNodeType* result_type;
    sCLNodeType* err_messsage_class_params[CL_METHOD_PARAM_MAX];
    BOOL block_exist;

    info->mNestOfMethodFromDefinitionPoint++;

    type_before = **type_;

    if(!determine_the_calling_method(&klass, &method, method_name, class_method, type_, class_params, num_params, info, block_id, no_defined_no_call, not_found_method, block_node, &block_num_params, &block_type, block_param_types, &block_exist, &used_param_num_with_initializer, &result_type, err_messsage_class_params))
    {
        info->mNestOfMethodFromDefinitionPoint--;
        return FALSE;
    }

    /// method not found ///
    if(type_before.mClass->mFlags & CLASS_FLAGS_DYNAMIC_TYPING) {
        **type_ = type_before;

        if(!do_call_method_with_duck_typing(type_before.mClass, NULL, method_name, class_method, type_, class_params, num_params, info, block_id, block_exist, block_num_params, block_param_types, block_type, gAnonymousType)) 
        {
            info->mNestOfMethodFromDefinitionPoint--;
            return FALSE;
        }
    }
    else {
        if(method == NULL) {
            if(!method_not_found(type_, info, method_name, err_messsage_class_params, num_params, block_id, no_defined_no_call, not_found_method))
            {
                info->mNestOfMethodFromDefinitionPoint--;
                return TRUE;
            }
        }

        **type_ = type_before;

        if(!do_call_method(klass, method, method_name, class_method, FALSE, type_, class_params, num_params, info, block_id, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type, block_node, class_of_class_method_call)) 
        {
            info->mNestOfMethodFromDefinitionPoint--;
            return FALSE;
        }
    }

    info->mNestOfMethodFromDefinitionPoint--;

    return TRUE;
}

static BOOL call_super(char* method_name, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, unsigned int block_node)
{
    sCLClass* klass;
    int caller_method_index;
    sCLNodeType* new_class;
    sCLClass* new_class2;
    sCLMethod* method;
    int block_num_params;
    sCLNodeType* block_param_types[CL_METHOD_PARAM_MAX];
    sCLNodeType* block_type;
    BOOL block_exist;
    int used_param_num_with_initializer;
    sCLNodeType* result_type;

    info->mNestOfMethodFromDefinitionPoint++;

    /// statically checking ///
    if(info->real_caller_class == NULL || info->real_caller_class->mClass == NULL || info->real_caller_method == NULL) {
        parser_err_msg("can't call super because there are not the caller method or the caller class.", info->sname, *info->sline);
        (*info->err_num)++;
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(info->real_caller_method->mFlags & CL_CLASS_METHOD) {
        parser_err_msg_format(info->sname, *info->sline, "can't call super because this method is class method(%s).", METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(info->real_caller_class->mClass->mNumSuperClasses == 0) {
        parser_err_msg_format(info->sname, *info->sline, "there is not a super class of this class(%s).", REAL_CLASS_NAME(info->real_caller_class->mClass));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    /// search for method ///
    *type_ = info->real_caller_class;
    klass = info->real_caller_class->mClass;

    if(info->real_caller_method->mFlags & CL_CONSTRUCTOR) {
        sCLClass* super_class;

        caller_method_index = get_method_index(klass, info->real_caller_method);
        super_class = get_super(klass);
    }
    else {
        caller_method_index = get_method_index(klass, info->real_caller_method);
        ASSERT(caller_method_index != -1);
    }

    /// get block params ///
    if(block_id) {
        int j;

        block_exist = TRUE;

        block_num_params = gNodeBlocks[block_id].mNumParams;
        for(j=0; j<block_num_params; j++) {
            block_param_types[j] = gNodeBlocks[block_id].mClassParams[j];
        }
        block_type = gNodeBlocks[block_id].mBlockType;
    }
    else if(block_node) {
        int j;

        if(info->real_caller_class && info->real_caller_method) {
            block_exist = TRUE;

            block_num_params = info->real_caller_method->mBlockType.mNumParams;
            for(j=0; j<block_num_params; j++) {
                block_param_types[j] = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mParamTypes[j], info->real_caller_class->mClass);
            }

            block_type = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mResultType, info->real_caller_class->mClass);
        }
        else {
            block_exist = FALSE;

            block_num_params = 0;
            memset(block_param_types, 0, sizeof(block_param_types));
            block_type = NULL;
        }
    }
    else {
        block_exist = FALSE;
        
        block_num_params = 0;
        memset(block_param_types, 0, sizeof(block_param_types));
        block_type = NULL;
    }

    /// search for a method of super classes ///
    if(info->real_caller_class && info->real_caller_class->mClass == klass) { // if it is true, don't solve generics type
        method = get_method_with_type_params_and_param_initializer_on_super_classes((*type_), method_name, class_params, *num_params, &new_class, FALSE, NULL, NULL, block_exist, block_num_params, block_param_types, block_type, &used_param_num_with_initializer, &result_type);
    }
    else {
        method = get_method_with_type_params_and_param_initializer_on_super_classes((*type_), method_name, class_params, *num_params, &new_class, FALSE, *type_, *type_, block_exist, block_num_params, block_param_types, block_type, &used_param_num_with_initializer, &result_type);
    }

    /// found at super classes ///
    if(method) {
        (*type_) = new_class;
        klass = new_class->mClass;
    }
    /// the method is not found ///
    else {
        method = get_method_on_super_classes(klass, method_name, &new_class2);

        if(method) {
            parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types of this method(%s::%s)", CLASS_NAME(new_class2), method_name);
            show_all_method_for_errmsg(new_class2, method_name);
            if(block_exist) {
                show_caller_method(method_name, class_params, *num_params, TRUE, block_param_types, block_num_params, block_type, info);
            }
            else {
                show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, 0, info);
            }
            (*info->err_num)++;
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "There is not this method on super classes(%s)", method_name);
            (*info->err_num)++;
        }

        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(method->mFlags & CL_CLASS_METHOD) {
        parser_err_msg_format(info->sname, *info->sline, "can't call super method because this is class method(%s).", method_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(!do_call_method(klass, method, method_name, FALSE, TRUE, type_, class_params, num_params, info, block_id, block_exist, block_num_params, block_param_types, block_type, used_param_num_with_initializer, result_type, block_node, NULL))
    {
        info->mNestOfMethodFromDefinitionPoint--;
        return FALSE;
    }

    info->mNestOfMethodFromDefinitionPoint--;

    return TRUE;
}

static BOOL call_mixin(char* method_name, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info, unsigned int block_id, unsigned int block_node)
{
    int caller_method_index;
    sCLMethod* method;
    int method_index;
    int block_num_params;
    sCLNodeType* block_param_types[CL_METHOD_PARAM_MAX];
    sCLNodeType* block_type;
    BOOL block_exist;
    int used_param_num_with_initializer;
    sCLNodeType* result_type;

    info->mNestOfMethodFromDefinitionPoint++;

    if(info->real_caller_class == NULL || info->real_caller_class->mClass == NULL || info->real_caller_method == NULL) {
        parser_err_msg("can't call mixin method because there are not the caller method or the caller class.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    caller_method_index = get_method_index(info->real_caller_class->mClass, info->real_caller_method);
    ASSERT(caller_method_index != -1);

    *type_ = info->real_caller_class;

    /// get block params ///
    if(block_id) {
        int j;

        block_exist = TRUE;

        block_num_params = gNodeBlocks[block_id].mNumParams;
        for(j=0; j<block_num_params; j++) {
            block_param_types[j] = gNodeBlocks[block_id].mClassParams[j];
        }
        block_type = gNodeBlocks[block_id].mBlockType;                      // ! struct copy
    }
    else if(block_node) {
        int j;

        if(info->real_caller_class && info->real_caller_method) {
            block_exist = TRUE;

            block_num_params = info->real_caller_method->mBlockType.mNumParams;
            for(j=0; j<block_num_params; j++) {
                block_param_types[j] = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mParamTypes[j], info->real_caller_class->mClass);
            }

            block_type = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mResultType, info->real_caller_class->mClass);
        }
        else {
            block_exist = FALSE;

            block_num_params = 0;
            memset(block_param_types, 0, sizeof(block_param_types));
            block_type = NULL;
        }
    }
    else {
        block_exist = FALSE;

        block_num_params = 0;
        memset(block_param_types, 0, sizeof(block_param_types));
        block_type = NULL;
    }

    if(*type_ && info->real_caller_class && *type_ && info->real_caller_class->mClass == (*type_)->mClass)   // if it is true, don't solve generics type
    {
        method = get_method_with_type_params_and_param_initializer(*type_, method_name, class_params, *num_params, info->real_caller_method->mFlags & CL_CLASS_METHOD, NULL, NULL, caller_method_index-1, block_exist, block_num_params, block_param_types, block_type, &used_param_num_with_initializer, &result_type);
    }
    else {
        method = get_method_with_type_params_and_param_initializer(info->real_caller_class, method_name, class_params, *num_params, info->real_caller_method->mFlags & CL_CLASS_METHOD, *type_, NULL, caller_method_index-1, block_exist, block_num_params, block_param_types, block_type, &used_param_num_with_initializer, &result_type);
    }

    method_index = get_method_index(info->real_caller_class->mClass, method);

    if(method == NULL) {
        method_index = get_method_index_from_the_parametor_point(info->real_caller_class->mClass, method_name, caller_method_index, info->real_caller_method->mFlags & CL_CLASS_METHOD);

        if(method_index != -1) {
            parser_err_msg_format(info->sname, *info->sline, "can't mixin. Invalid parametor types of this method(%s::%s)", CLASS_NAME(info->real_caller_class->mClass), method_name);
            show_all_method_for_errmsg(info->real_caller_class->mClass, method_name);
            if(block_exist) {
                show_caller_method(method_name, class_params, *num_params, TRUE, block_param_types, block_num_params, block_type, info);
            }
            else {
                show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, 0, info);
            }
            (*info->err_num)++;
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "can't mixin. There is not this method before(%s).", method_name);
            (*info->err_num)++;
        }

        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if((info->real_caller_method->mFlags & CL_CLASS_METHOD) != (method->mFlags & CL_CLASS_METHOD)) {
        parser_err_msg_format(info->sname, *info->sline, "can't mixin because caller method and mixin method is the differ type");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(!do_call_mixin(method, method_index, method->mFlags & CL_CLASS_METHOD, type_, class_params, num_params, info, used_param_num_with_initializer, result_type, block_exist, block_id, block_node))
    {
        info->mNestOfMethodFromDefinitionPoint--;
        return FALSE;
    }

    info->mNestOfMethodFromDefinitionPoint--;

    return TRUE;
}

static BOOL call_method_block(sCLClass* klass, sCLNodeType** type_, sCLMethod* method, char* block_name, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType* result_type;
    sVar* var;
    int i;
    int var_index;

//    info->mNestOfMethodFromDefinitionPoint++;

    if(klass == NULL || (*type_)->mClass == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not caller class");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }
    if(method == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not caller method");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(method->mNumBlockType == 0) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not block");
        (*info->err_num)++;

        *type_ = gIntType;
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    if(info->lv_table == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
        (*info->err_num)++;

        *type_ = gIntType;
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }
    
    var = get_variable_from_table(info->lv_table, block_name);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", block_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    /// type checking ///
    if(*num_params != method->mBlockType.mNumParams) {
        parser_err_msg_format(info->sname, *info->sline, "invalid block call because of the number of block parametors.require block parametor number is %d but it is %d", method->mBlockType.mNumParams, *num_params);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        //info->mNestOfMethodFromDefinitionPoint--;
        return TRUE;
    }

    for(i=0; i<*num_params; i++) {
        sCLNodeType* node_type;

        node_type = create_node_type_from_cl_type(&method->mBlockType.mParamTypes[i], klass);

        if(!substitution_posibility(node_type, class_params[i])) {
            parser_err_msg_format(info->sname, *info->sline, "type error of block call");
            parser_err_msg_without_line("left type is ");
            show_node_type_for_errmsg(node_type);
            parser_err_msg_without_line(". right type is ");
            show_node_type_for_errmsg(class_params[i]);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            //info->mNestOfMethodFromDefinitionPoint--;
            return TRUE;
        }
    }

    /// make code ///
    result_type = ALLOC create_node_type_from_cl_type(&method->mBlockType.mResultType, klass);
    var_index = get_variable_index_from_table(info->lv_table, block_name);

    ASSERT(var_index != -1);

    append_opecode_to_bytecodes(info->code, OP_INVOKE_BLOCK, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, var_index, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, !type_identity(result_type, gVoidType), info->no_output_to_bytecodes);

    dec_stack_num(info->stack_num, method->mBlockType.mNumParams);

    if(!type_identity(result_type, gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    //info->mNestOfMethodFromDefinitionPoint--;

    return TRUE;
}

BOOL compile_left_node(unsigned int node, sCLNodeType** left_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mLeft) {
        if(!compile_node(gNodes[node].mLeft, left_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL compile_right_node(unsigned int node, sCLNodeType** right_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mRight) {
        if(!compile_node(gNodes[node].mRight, right_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL compile_middle_node(unsigned int node, sCLNodeType** middle_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mMiddle) {
        if(!compile_node(gNodes[node].mMiddle, middle_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL load_local_varialbe(char* name, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sVar* var;
    int var_index;

    if(info->lv_table == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
        (*info->err_num)++;

        *type_ = gIntType;
        return TRUE;
    }

    var = get_variable_from_table(info->lv_table, name);

    if(var == NULL || var->mType->mClass == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(substitution_posibility(var->mType, gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ILOAD, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(var->mType, gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ALOAD, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(var->mType, gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FLOAD, info->no_output_to_bytecodes);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OLOAD, info->no_output_to_bytecodes);
    }

    var_index = get_variable_index_from_table(info->lv_table, name);

    ASSERT(var_index != -1);

    append_int_value_to_bytecodes(info->code, var_index, info->no_output_to_bytecodes);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = var->mType;

    return TRUE;
}

static BOOL load_local_varialbe_from_var_index(int index, sCLNodeType** type_, sCompileInfo* info)
{
    sVar* var;

    if(info->lv_table == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
        (*info->err_num)++;

        *type_ = gIntType;
        return TRUE;
    }

    var = get_variable_from_table_by_var_index(info->lv_table, index);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable index(%d)", index);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(substitution_posibility(var->mType, gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ILOAD, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(var->mType, gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ALOAD, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(var->mType, gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FLOAD, info->no_output_to_bytecodes);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OLOAD, info->no_output_to_bytecodes);
    }

    append_int_value_to_bytecodes(info->code, index, info->no_output_to_bytecodes);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = var->mType;

    return TRUE;
}

static BOOL binary_operator_core(sCLNodeType** type_, sCompileInfo* info, int op, sCLNodeType* result_type)
{
    append_opecode_to_bytecodes(info->code, op, info->no_output_to_bytecodes);
    *type_ = result_type;
    dec_stack_num(info->stack_num, 1);

    return TRUE;
}

// op_* can take -1 value. -1 means nothing.
static BOOL binary_operator(sCLNodeType* left_type, sCLNodeType* right_type, sCLNodeType** type_, sCompileInfo* info, int op_int, int op_byte, int op_short, int op_uint, int op_long, int op_float, int op_double, int op_string, int op_bytes, int op_bool, int op_string_mult, int op_bytes_mult, char* operand_symbol, sCLNodeType* int_result_type, sCLNodeType* byte_result_type, sCLNodeType* short_result_type, sCLNodeType* uint_result_type, sCLNodeType* long_result_type, sCLNodeType* float_result_type, sCLNodeType* double_result_type, sCLNodeType* string_result_type, sCLNodeType* bytes_result_type, sCLNodeType* bool_result_type, sCLNodeType* string_mult_result_type, sCLNodeType* bytes_mult_result_type, BOOL quote)
{
    if(left_type->mClass == NULL || right_type->mClass == NULL) {
        parser_err_msg("no class type1", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        if(quote) {
            if(op_int != -1 && operand_posibility(left_type, gIntType) && operand_posibility(right_type, gIntType)) 
        {
                if(!binary_operator_core(type_, info, op_int, int_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_byte != -1 && operand_posibility(left_type, gByteType) && operand_posibility(right_type, gByteType)) 
            {
                if(!binary_operator_core(type_, info, op_byte, byte_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_short != -1 && operand_posibility_with_class_name(left_type, "short") && operand_posibility_with_class_name(right_type, "short")) 
            {
                if(!binary_operator_core(type_, info, op_short, short_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_uint != -1 && operand_posibility_with_class_name(left_type, "uint") && operand_posibility_with_class_name(right_type, "uint")) 
            {
                if(!binary_operator_core(type_, info, op_uint, uint_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_long != -1 && operand_posibility_with_class_name(left_type, "long") && operand_posibility_with_class_name(right_type, "long")) 
            {
                if(!binary_operator_core(type_, info, op_long, long_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_float != -1 && operand_posibility(left_type, gFloatType) && operand_posibility(right_type, gFloatType)) 
            {
                if(!binary_operator_core(type_, info, op_float, float_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_double != -1 && operand_posibility(left_type, gDoubleType) && operand_posibility(right_type, gDoubleType)) 
            {
                if(!binary_operator_core(type_, info, op_double, double_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_bool != -1 && operand_posibility(left_type, gBoolType) && operand_posibility(right_type, gBoolType)) 
            {
                if(!binary_operator_core(type_, info, op_bool, bool_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_string != -1 && operand_posibility(left_type, gStringType) && operand_posibility(right_type, gStringType)) 
            {
                if(!binary_operator_core(type_, info, op_string, string_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_bytes != -1 && operand_posibility(left_type, gBytesType) && operand_posibility(right_type, gBytesType)) 
            {
                if(!binary_operator_core(type_, info, op_bytes, bytes_result_type))
                {
                    return FALSE;
                }
            }
            /// multiply "aaa" * 2 ///
            else if(op_string_mult != -1 && operand_posibility(left_type, gStringType) && operand_posibility(right_type, gIntType)) 
            {
                if(!binary_operator_core(type_, info, op_string_mult, string_mult_result_type))
                {
                    return FALSE;
                }
            }
            else if(op_bytes_mult != -1 && operand_posibility(left_type, gBytesType) && operand_posibility(right_type, gIntType)) 
            {
                if(!binary_operator_core(type_, info, op_bytes_mult, bytes_mult_result_type))
                {
                    return FALSE;
                }
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "There is not quote operator of this(%s)\n", operand_symbol);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                return TRUE;
            }
        }
        else {
            int not_found_method;
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            *type_ = left_type;

            class_params2[0] = right_type;
            num_params2 = 1;

            /// print error message ///
            if(!call_method(operand_symbol, FALSE, type_, class_params2, &num_params2, info, 0, FALSE, &not_found_method, 0, NULL)) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// op_* can take -1 value. -1 means nothing.
static BOOL monadic_operator(sCLNodeType* left_type, sCLNodeType** type_, sCompileInfo* info, char* operand_symbol)
{
    if(left_type->mClass == NULL) {
        parser_err_msg("no class type1", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
        int num_params2;

        *type_ = left_type;

        int not_found_method;
        num_params2 = 0;

        /// print error message ///
        if(!call_method(operand_symbol, FALSE, type_, class_params2, &num_params2, info, 0, FALSE, &not_found_method, 0, NULL)) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL call_clone_method_for_calling_by_value(sCompileInfo* info, sCLNodeType* left_type, sCLNodeType* right_type, sCLNodeType** type_)
{
    BOOL is_clone_method;
    BOOL is_not_refference;
    sCLNodeType* solved_left_type;
    sCLNodeType* solved_right_type;

    if(!solve_generics_types_for_node_type(left_type, &solved_left_type, *type_))
    {
        /// Clover can't solve the generics types on compile time ///
        append_opecode_to_bytecodes(info->code, OP_INVOKE_VIRTUAL_CLONE_METHOD, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, left_type->mStar, info->no_output_to_bytecodes);
        return TRUE;
    }
    if(!solve_generics_types_for_node_type(right_type, &solved_right_type, *type_))
    {
        /// Clover can't solve the generics types on compile time ///
        append_opecode_to_bytecodes(info->code, OP_INVOKE_VIRTUAL_CLONE_METHOD, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, left_type->mStar, info->no_output_to_bytecodes);
        return TRUE;
    }

    if(info->real_caller_class && info->real_caller_method) {
        sCLNodeType* node_type;
         
        node_type = create_node_type_from_cl_type(&info->real_caller_method->mResultType, info->real_caller_class->mClass);

        is_clone_method = strcmp(METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method),"clone") == 0 && info->real_caller_method->mNumParams == 0 && type_identity(node_type, info->real_caller_class);
    }
    else {
        is_clone_method = FALSE;
    }
    is_not_refference = ((solved_left_type->mClass->mFlags & CLASS_FLAGS_STRUCT) && !solved_left_type->mStar) || (!(solved_left_type->mClass->mFlags & CLASS_FLAGS_STRUCT) && (solved_left_type)->mStar);

    if(!is_clone_method && is_not_refference && type_identity_without_star(solved_left_type, solved_right_type))
    {
        sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
        int num_params;
        BOOL not_found_method;
        sCLNodeType* type2;

        num_params = 0;
        memset(class_params, 0, sizeof(class_params));

        type2 = solved_left_type;

        if(!call_method("clone", FALSE, &type2, class_params, &num_params, info, 0, TRUE, &not_found_method, 0, NULL))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL store_local_variable_core(int var_index, sCLNodeType** type_, sCLNodeType* right_type, sCompileInfo* info)
{
    /// call clone method ///
    if(!call_clone_method_for_calling_by_value(info, *type_, right_type, type_))
    {
        return FALSE;
    }

    /// append opecode to bytecodes ///
    if(substitution_posibility(*type_, gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ISTORE, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(*type_, gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ASTORE, info->no_output_to_bytecodes);
    }
    else if(substitution_posibility(*type_, gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FSTORE, info->no_output_to_bytecodes);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OSTORE, info->no_output_to_bytecodes);
    }

    append_int_value_to_bytecodes(info->code, var_index, info->no_output_to_bytecodes);

    return TRUE;
}

static BOOL store_local_variable(char* name, sVar* var, unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType* right_type;
    int index;
    sCLNodeType* dummy_type;

    /// load self ///
    if(gNodes[node].uValue.sVarName.mNodeSubstitutionType != kNSNone) {
        dummy_type = clone_node_type(*type_);
        if(!load_local_varialbe(name, &dummy_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    /// a right value goes ///
    right_type = NULL;
    if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
        return FALSE;
    }

    /// operand ///
    dummy_type = clone_node_type(*type_);
    switch((int)gNodes[node].uValue.sVarName.mNodeSubstitutionType) {
        case kNSPlus:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IADD, OP_BADD, OP_SHADD, OP_UIADD, OP_LOADD, OP_FADD, OP_DADD, OP_SADD, OP_BSADD, -1, -1, -1,  "+=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, gStringType, gBytesType, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSMinus:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_ISUB, OP_BSUB, OP_SHSUB, OP_UISUB, OP_LOSUB, OP_FSUB, OP_DSUB, -1, -1, -1, -1, -1,  "-=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSMult:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IMULT, OP_BMULT, OP_SHMULT, OP_UIMULT, OP_LOMULT, OP_FMULT, OP_DMULT, -1, -1, -1, OP_SMULT, OP_BSMULT, "*=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, gStringType, gBytesType, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSDiv:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IDIV, OP_BDIV, OP_SHDIV, OP_UIDIV, OP_LODIV, OP_FDIV, OP_DDIV, -1, -1, -1, -1, -1, "/=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSMod:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IMOD, OP_BMOD, OP_SHMOD, OP_UIMOD, OP_LOMOD, -1, -1, -1, -1, -1, -1, -1,  "%=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSLShift:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_ILSHIFT, OP_BLSHIFT, OP_SHLSHIFT, OP_UILSHIFT, OP_LOLSHIFT, -1, -1, -1, -1, -1, -1, -1, "<<=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSRShift:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IRSHIFT, OP_BRSHIFT, OP_SHRSHIFT, OP_UIRSHIFT, OP_LORSHIFT, -1, -1, -1, -1, -1, -1, -1, ">>=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSAnd:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IAND, OP_BAND, OP_SHAND, OP_UIAND, OP_LOAND, -1, -1, -1, -1, -1, -1, -1, "&=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSXor:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IXOR, OP_BXOR, OP_SHXOR, OP_UIXOR, OP_LOXOR, -1, -1, -1, -1, -1, -1, -1, "^=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSOr:
            if(!binary_operator(dummy_type, right_type, &dummy_type, info, OP_IOR, OP_BOR, OP_SHOR, OP_UIOR, OP_LOOR, -1, -1, -1, -1, -1, -1, -1,  "|=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
        
    }

    /// type checking ///
    if((*type_)->mClass == NULL) {
        parser_err_msg("no type left value", info->sname, *info->sline);
        return TRUE;
    }
    if(right_type->mClass == NULL) {
        parser_err_msg("no type right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substitution_posibility_with_solving_generics(*type_, right_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        parser_err_msg_without_line("left type is ");
        show_node_type_for_errmsg(*type_);
        parser_err_msg_without_line(". right type is ");
        show_node_type_for_errmsg(right_type);
        parser_err_msg_without_line("\n");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    index = get_variable_index_from_table(info->lv_table, name);

    ASSERT(index != -1);

    /// append opecode to bytecodes ///
    if(!store_local_variable_core(index, type_, right_type, info)) {
        return FALSE;
    }

    *type_ = var->mType;

    return TRUE;
}

static BOOL load_field(char* field_name, BOOL class_field, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType* field_type;
    sCLClass* klass;
    sCLNodeType* found_class;

    if(*type_ == NULL) {
        parser_err_msg("left value has not class. can't get field", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy

        return TRUE;
    }

    field_type = NULL;

    if(class_field) {
        field = get_field_including_super_classes(*type_, field_name, &found_class, class_field, &field_type, *type_);
        if(found_class == NULL) {
            parser_err_msg_format(info->sname, *info->sline, "This class field(%s) is not found", field_name);
            (*info->err_num)++;
            *type_ = gIntType; // dummy

            return TRUE;
        }
        else {
            field_index = get_field_index(found_class->mClass, field_name, class_field);
        }
    }
    else {
        field = get_field_including_super_classes(*type_, field_name, &found_class, class_field, &field_type, *type_);
        field_index = get_field_index_including_super_classes_without_class_field((*type_)->mClass, field_name);
    }

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this field(%s) in this class(%s)", field_name, REAL_CLASS_NAME((*type_)->mClass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy

        return TRUE;
    }

    /// check of private field ///
    if(field->mFlags & CL_PRIVATE_FIELD && !check_private_access(found_class->mClass, info->real_caller_class ? info->real_caller_class->mClass:NULL)){
        parser_err_msg_format(info->sname, *info->sline, "this is private field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy

        return TRUE;
    }
    if(field->mFlags & CL_PROTECTED_FIELD && !is_called_from_inside(info->real_caller_class ? info->real_caller_class->mClass:NULL, (*type_)->mClass)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "this is protected field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        if((field->mFlags & CL_STATIC_FIELD) == 0) {
            parser_err_msg_format(info->sname, *info->sline, "this is not static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    else {
        if(field->mFlags & CL_STATIC_FIELD) {
            parser_err_msg_format(info->sname, *info->sline, "this is static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    if(field_type == NULL || type_identity(field_type, gVoidType)) {
        parser_err_msg("This field has no type", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_LD_STATIC_FIELD, info->no_output_to_bytecodes);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(found_class->mClass), info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, field_index, info->no_output_to_bytecodes);

        inc_stack_num(info->stack_num, info->max_stack, 1);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_LDFIELD, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, field_index, info->no_output_to_bytecodes);
        append_generics_type_to_bytecode(info->code, info->constant, *type_, info->no_output_to_bytecodes);
    }

    *type_ = field_type;

    return TRUE;
}

static BOOL increase_or_decrease_local_variable(char* name, sVar* var, unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    int index;
    sCLNodeType* dummy_type;

    /// load self ///
    dummy_type = clone_node_type(*type_);
    if(!load_local_varialbe(name, &dummy_type, class_params, num_params, info)) {
        return FALSE;
    }

    /// operand ///
    index = get_variable_index_from_table(info->lv_table, name);

    ASSERT(index != -1);

    dummy_type = clone_node_type(*type_);
    switch((int)gNodes[node].uValue.sOperand.mOperand) {
        case kOpPlusPlus:
            if(!monadic_operator(dummy_type, &dummy_type, info, "++")) {
                return FALSE;
            }
            break;

        case kOpPlusPlus2:
            if(!monadic_operator(dummy_type, &dummy_type, info, "++2")) {
                return FALSE;
            }
            break;
            
        case kOpMinusMinus:
            if(!monadic_operator(dummy_type, &dummy_type, info, "--")) {
                return FALSE;
            }
            break;

        case kOpMinusMinus2:
            if(!monadic_operator(dummy_type, &dummy_type, info, "--2")) {
                return FALSE;
            }
            break;
    }

    /// type checking ///
    if((*type_)->mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }

    *type_ = var->mType;

    return TRUE;
}

static BOOL store_field_core(unsigned int node, char* field_name, BOOL class_field, sCLNodeType* right_type, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType* field_type;
    sCLNodeType* dummy_type;
    sCLNodeType* found_class;
    BOOL is_clone_method;
    BOOL is_not_refference;
    BOOL cant_solve_the_generics_type_on_compile_time;

    field_type = NULL;

    /// get field type ////
    if(class_field) {
        found_class = *type_;

        field = get_field((*type_)->mClass, field_name, class_field);
        field_index = get_field_index((*type_)->mClass, field_name, class_field);
        if(field) {
            if(info->real_caller_class && info->real_caller_class->mClass == (*type_)->mClass) { // if it is true, don't solve generics types
                (void)get_field_type((*type_)->mClass, field, ALLOC &field_type, 0);
            }
            else {
                /// check generics type  ///
                if((*type_)->mGenericsTypesNum != (*type_)->mClass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME((*type_)->mClass));
                    (*info->err_num)++;
                }

                if(!get_field_type((*type_)->mClass, field, ALLOC &field_type, *type_)) 
                {
                    parser_err_msg_format(info->sname, *info->sline, "Clover can't solve the generics types of this field(%s)", field_name);
                    (*info->err_num)++;
                }
            }
        }
    }
    else {
        field = get_field_including_super_classes((*type_), field_name, &found_class, class_field, &field_type, *type_);
        field_index = get_field_index_including_super_classes_without_class_field((*type_)->mClass, field_name);
    }

    /// operand ///
    dummy_type = ALLOC clone_node_type(*type_);
    switch((int)gNodes[node].uValue.sVarName.mNodeSubstitutionType) {
        case kNSPlus:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IADD, OP_BADD, OP_SHADD, OP_UIADD, OP_LOADD, OP_FADD, OP_DADD, OP_SADD, OP_BSADD, -1, -1, -1,  "+=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, gStringType, gBytesType, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) 
            {
                return FALSE;
            }
            break;
            
        case kNSMinus:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_ISUB, OP_BSUB, OP_SHSUB, OP_UISUB, OP_LOSUB, OP_FSUB, OP_DSUB, -1, -1, -1, -1, -1, "-=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSMult:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IMULT, OP_BMULT, OP_SHMULT, OP_UIMULT, OP_LOMULT, OP_FMULT, OP_DMULT, -1, -1, -1, OP_SMULT, OP_BSMULT, "*=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, gStringType, gBytesType, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSDiv:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IDIV, OP_BDIV, OP_SHDIV, OP_UIDIV, OP_LODIV, OP_FDIV, OP_DDIV, -1, -1, -1, -1, -1,  "/=", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSMod:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IMOD, OP_BMOD, OP_SHMOD, OP_UIMOD, OP_LOMOD, -1, -1, -1, -1, -1, -1, -1, "%=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSLShift:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_ILSHIFT, OP_BLSHIFT, OP_SHLSHIFT, OP_UILSHIFT, OP_LOLSHIFT, -1, -1, -1, -1, -1, -1, -1,  "<<=", gIntType, gByteType, gShortType, gUIntType, gLongType,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSRShift:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IRSHIFT, OP_BRSHIFT, OP_SHRSHIFT, OP_UIRSHIFT, OP_LORSHIFT, -1, -1, -1, -1, -1, -1, -1, ">>=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSAnd:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IAND, OP_BAND, OP_SHAND, OP_UIAND, OP_LOAND, -1, -1, -1, -1, -1, -1 , -1,  "&=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSXor:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IXOR, OP_BXOR, OP_SHXOR, OP_UIXOR, OP_LOXOR, -1, -1, -1, -1, -1, -1, -1,  "^=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
            
        case kNSOr:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IOR, OP_BOR, OP_SHOR, OP_UIOR, OP_LOOR, -1, -1, -1, -1, -1, -1, -1,  "|=", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sVarName.mQuote)) {
                return FALSE;
            }
            break;
        
    }

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this field(%s) in this class(%s)", field_name, REAL_CLASS_NAME((*type_)->mClass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// check of private field ///
    if(field->mFlags & CL_PRIVATE_FIELD && !check_private_access(found_class->mClass, info->real_caller_class ? info->real_caller_class->mClass:NULL)){
        parser_err_msg_format(info->sname, *info->sline, "this is private field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy

        return TRUE;
    }
    if(field->mFlags & CL_PROTECTED_FIELD && !is_called_from_inside(info->real_caller_class ? info->real_caller_class->mClass:NULL, (*type_)->mClass)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "this is protected field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        if((field->mFlags & CL_STATIC_FIELD) == 0) {
            parser_err_msg_format(info->sname, *info->sline, "this is not static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    else {
        if(field->mFlags & CL_STATIC_FIELD) {
            parser_err_msg_format(info->sname, *info->sline, "this is static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    /// type checking ///
    if(field_type == NULL || type_identity(field_type, gVoidType)) {
        parser_err_msg("This field has no type.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field_type == NULL || right_type->mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substitution_posibility_with_solving_generics(field_type, right_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        parser_err_msg_without_line("left type is ");
        show_node_type_for_errmsg(field_type);
        parser_err_msg_without_line(". right type is ");
        show_node_type_for_errmsg(right_type);
        parser_err_msg_without_line("\n");

        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// call clone method ///
    if(!call_clone_method_for_calling_by_value(info, field_type, right_type, type_))
    {
        return FALSE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_SR_STATIC_FIELD, info->no_output_to_bytecodes);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME((*type_)->mClass), info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, field_index, info->no_output_to_bytecodes);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_SRFIELD, info->no_output_to_bytecodes);
        append_int_value_to_bytecodes(info->code, field_index, info->no_output_to_bytecodes);
        append_generics_type_to_bytecode(info->code, info->constant, *type_, info->no_output_to_bytecodes);

        dec_stack_num(info->stack_num, 1);
    }

    *type_ = field_type;

    return TRUE;
}

static BOOL store_field(unsigned int node, char* field_name, BOOL class_field, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType* field_type;
    sCLNodeType* right_type;
    sCLNodeType* dummy_type;
    sCLNodeType* found_class;
    BOOL is_clone_method;
    BOOL is_not_refference;
    BOOL cant_solve_the_generics_type_on_compile_time;

    field_type = 0;

    /// load field ///
    if(gNodes[node].uValue.sVarName.mNodeSubstitutionType != kNSNone) {
        if(!class_field) {  // require compiling left node for load field
            sCLNodeType* dummy_type2;

            dummy_type2 = NULL;
            if(!compile_left_node(node, &dummy_type2, class_params, num_params, info)) {
                return FALSE;
            }
        }

        dummy_type = ALLOC clone_node_type(*type_);
        if(!load_field(field_name, class_field, &dummy_type, class_params, num_params, info))
        {
            return FALSE;
        }
    }

    /// right value ///
    right_type = NULL;
    if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
        return FALSE;
    }

    if(!store_field_core(node, field_name, class_field, right_type, type_, class_params, num_params, info))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL load_tuple_element(int element_num, int tuple_element_num, sCLNodeType* tuple_type, sCLNodeType** type_, sCompileInfo* info)
{
    /// element number check ///
    if(element_num >= tuple_element_num) {
        parser_err_msg_format(info->sname, *info->sline, "invalid tuple element number");
        (*info->err_num)++;

        *type_ = gIntType;      // dummy
        return FALSE;
    }

    /// dupe the tuple ///
    append_opecode_to_bytecodes(info->code, OP_DUP, info->no_output_to_bytecodes);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    /// load field from tuple which is at top of stack ///
    append_opecode_to_bytecodes(info->code, OP_LDFIELD, info->no_output_to_bytecodes);
    append_int_value_to_bytecodes(info->code, element_num, info->no_output_to_bytecodes);
    append_generics_type_to_bytecode(info->code, info->constant, tuple_type, info->no_output_to_bytecodes);

    return TRUE;
}

static BOOL store_tuple_core(unsigned int node, int* element_num, int tuple_element_num, sCLNodeType* right_type, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    if(node != 0) {
        int left_node;
        unsigned int right_node;

        left_node = gNodes[node].mLeft;
        if(!store_tuple_core(left_node, element_num, tuple_element_num, right_type, type_, class_params, num_params, info)) 
        {
            return FALSE;
        }

        ASSERT(gNodes[node].mNodeType == NODE_TYPE_PARAM);

        (*element_num)++;

        right_node = gNodes[node].mRight;

        /// type_checking ///
        if(*element_num >= right_type->mGenericsTypesNum) {
            parser_err_msg_format(info->sname, *info->sline, "A small number of right tuple element");
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }

        /// go ///
        switch(gNodes[right_node].mNodeType) {
            case NODE_TYPE_VARIABLE_NAME: 
            case NODE_TYPE_DEFINE_VARIABLE_NAME: {
                char* name;
                sVar* var;
                sCLNodeType* tuple_element_type;
                sCLNodeType* right_tuple_element_type;

                int index;
                sCLNodeType* dummy_type;

                /// get variable info ///
                name = gNodes[right_node].uValue.sVarName.mVarName;

                if(info->lv_table == NULL) {
                    parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
                    (*info->err_num)++;

                    *type_ = gIntType;      // dummy
                    return TRUE;
                }
                
                var = get_variable_from_table(info->lv_table, name);

                if(var == NULL) {
                    parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    return TRUE;
                }

                /// type_checking ///
                tuple_element_type = var->mType;
                right_tuple_element_type = right_type->mGenericsTypes[*element_num];

                if(!substitution_posibility(tuple_element_type, right_tuple_element_type)) 
                {
                    parser_err_msg_format(info->sname, *info->sline, "type error.");
                    parser_err_msg_without_line("Left tuple element type is ");
                    show_node_type_for_errmsg(tuple_element_type);
                    parser_err_msg_without_line(". But right tuple element type is ");
                    show_node_type_for_errmsg(right_tuple_element_type);
                    parser_err_msg_without_line("\n");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    return TRUE;
                }

                /// load an element of tuple ///
                if(!load_tuple_element(*element_num, tuple_element_num, right_type, type_, info))
                {
                    return TRUE;
                }

                /// store value to variable ///
                *type_ = var->mType;

                index = get_variable_index_from_table(info->lv_table, name);

                ASSERT(index != -1);

                /// append opecode to bytecodes ///
                if(!store_local_variable_core(index, type_, right_type, info)) {
                    return FALSE;
                }

                /// pop the field value of tuple ///
                append_opecode_to_bytecodes(info->code, OP_POP, info->no_output_to_bytecodes);
                dec_stack_num(info->stack_num, 1);

                *type_ = right_type;
                }
                break;
        
            case NODE_TYPE_FIELD: {
                sCLNodeType* field_type;
                char* field_name;
                sCLNodeType* right_tuple_element_type;

                /// load an element of tuple ///
                if(!load_tuple_element(*element_num, tuple_element_num, right_type, type_, info))
                {
                    return TRUE;
                }

                /// load target object ///
                field_type = NULL;
                if(!compile_left_node(right_node, &field_type, class_params, num_params, info)) {
                    return FALSE;
                }

                /// swap the stack value ///
                append_opecode_to_bytecodes(info->code, OP_SWAP, info->no_output_to_bytecodes);

                /// store value to the field ///
                *type_ = field_type;
                field_name = gNodes[right_node].uValue.sVarName.mVarName;
                right_tuple_element_type = right_type->mGenericsTypes[*element_num];

                if(!store_field_core(right_node, field_name, FALSE, right_tuple_element_type, type_, class_params, num_params, info)) 
                {
                    return FALSE;
                }

                /// pop the field value of tuple ///
                append_opecode_to_bytecodes(info->code, OP_POP, info->no_output_to_bytecodes);
                dec_stack_num(info->stack_num, 1);

                *type_ = right_type;
                }
                break;

            case NODE_TYPE_CLASS_FIELD: {
                char* field_name;
                sCLNodeType* right_tuple_element_type;

                /// load an element of tuple ///
                if(!load_tuple_element(*element_num, tuple_element_num, right_type, type_, info))
                {
                    return TRUE;
                }

                /// store value to the field ///
                *type_ = gNodes[right_node].mType;
                field_name = gNodes[right_node].uValue.sVarName.mVarName;
                right_tuple_element_type = right_type->mGenericsTypes[*element_num];

                if(!store_field_core(right_node, field_name, TRUE, right_tuple_element_type, type_, class_params, num_params, info)) 
                {
                    return FALSE;
                }

                /// pop the field value of tuple ///
                append_opecode_to_bytecodes(info->code, OP_POP, info->no_output_to_bytecodes);
                dec_stack_num(info->stack_num, 1);

                *type_ = right_type;
                }
                break;

            default:
                parser_err_msg_format(info->sname, *info->sline, "Invalid left node.");
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                return TRUE;
        }
    }

    return TRUE;
}

static BOOL store_tuple(unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType* right_type;
    sCLClass* tuple_class;
    char buf[128];
    unsigned int left_node;
    int element_num;

    /// right value ///
    right_type = NULL;
    if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
        return FALSE;
    }
    
    if(right_type->mGenericsTypesNum == 0) {
        parser_err_msg_format(info->sname, *info->sline, "Right type is %s. Right type requires Tuple class.", REAL_CLASS_NAME(right_type->mClass));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    snprintf(buf, 128, "Tuple$%d", right_type->mGenericsTypesNum);

    tuple_class = cl_get_class(buf);

    ASSERT(tuple_class != NULL);

    if(!substitution_posibility_of_class(tuple_class, right_type->mClass)) {
        parser_err_msg_format(info->sname, *info->sline, "Right type is %s. Right type requires Tuple class.", REAL_CLASS_NAME(right_type->mClass));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    ASSERT((int)gNodes[node].uValue.sVarName.mNodeSubstitutionType == kNSNone);

    left_node = gNodes[node].mLeft;

    ASSERT(gNodes[left_node].mNodeType == NODE_TYPE_PARAM);

    element_num = -1;
    if(!store_tuple_core(left_node, &element_num, right_type->mGenericsTypesNum, right_type, type_, class_params, num_params, info)) {
        return FALSE;
    }

    *type_ = right_type;

    return TRUE;
}

static BOOL increase_or_decrease_field(unsigned int node, unsigned int left_node, char* field_name, BOOL class_field, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType* field_type;
    sCLNodeType* dummy_type;
    sCLNodeType* found_class;

    field_type = NULL;

    /// load field ///
    if(!class_field) {  // require compiling left node for load field
        sCLNodeType* dummy_type2;

        dummy_type2 = 0;
        if(!compile_left_node(left_node, &dummy_type2, class_params, num_params, info)) {
            return FALSE;
        }
    }

    dummy_type = clone_node_type(*type_);
    if(!load_field(field_name, class_field, &dummy_type, class_params, num_params, info))
    {
        return FALSE;
    }

    /// get field type ////
    if(class_field) {
        found_class = *type_;

        field = get_field((*type_)->mClass, field_name, class_field);
        field_index = get_field_index((*type_)->mClass, field_name, class_field);
        if(field) {
            if(info->real_caller_class && info->real_caller_class->mClass == (*type_)->mClass) { // if it is true, don't solve generics types
                (void)get_field_type((*type_)->mClass, field, ALLOC &field_type, NULL);
            }
            else {
                /// check generics type  ///
                if((*type_)->mGenericsTypesNum != (*type_)->mClass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME((*type_)->mClass));
                    (*info->err_num)++;
                }

                if(!get_field_type((*type_)->mClass, field, ALLOC &field_type, *type_)) {
                    parser_err_msg_format(info->sname, *info->sline, "Clover can't solve the generics types of this field(%s)", field_name);
                    (*info->err_num)++;
                }
            }
        }
    }
    else {
        field = get_field_including_super_classes(*type_, field_name, &found_class, class_field, &field_type, *type_);
        field_index = get_field_index_including_super_classes_without_class_field((*type_)->mClass, field_name);
    }

    /// operand ///
    dummy_type = clone_node_type(*type_);
    switch((int)gNodes[node].uValue.sOperand.mOperand) {
        case kOpPlusPlus:
            if(!monadic_operator(field_type, &dummy_type, info, "++")) {
                return FALSE;
            }
            break;

        case kOpPlusPlus2:
            if(!monadic_operator(field_type, &dummy_type, info, "++2")) {
                return FALSE;
            }
            break;
            
        case kOpMinusMinus:
            if(!monadic_operator(field_type, &dummy_type, info, "--")) {
                return FALSE;
            }
            break;
            
        case kOpMinusMinus2:
            if(!monadic_operator(field_type, &dummy_type, info, "--2")) {
                return FALSE;
            }
            break;
    }

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this field(%s) in this class(%s)", field_name, REAL_CLASS_NAME((*type_)->mClass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// check of private field ///
    if(field->mFlags & CL_PRIVATE_FIELD && !check_private_access(found_class->mClass, info->real_caller_class ? info->real_caller_class->mClass:NULL)){
        parser_err_msg_format(info->sname, *info->sline, "this is private field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy

        return TRUE;
    }
    if(field->mFlags & CL_PROTECTED_FIELD && !is_called_from_inside(info->real_caller_class ? info->real_caller_class->mClass:NULL, (*type_)->mClass)) 
    {
        parser_err_msg_format(info->sname, *info->sline, "this is protected field(%s).", field_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        if((field->mFlags & CL_STATIC_FIELD) == 0) {
            parser_err_msg_format(info->sname, *info->sline, "this is not static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }
    else {
        if(field->mFlags & CL_STATIC_FIELD) {
            parser_err_msg_format(info->sname, *info->sline, "this is static field(%s)", field_name);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    /// type checking ///
    if(field_type == NULL || type_identity(field_type, gVoidType)) {
        parser_err_msg("This field has no type.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field_type == NULL) {
        parser_err_msg("no type left value", info->sname, *info->sline);
        return TRUE;
    }

    *type_ = field_type;

    return TRUE;
}

static void prepare_for_break_labels(unsigned int** break_labels_before, int** break_labels_len_before, unsigned int break_labels[], int* break_labels_len, sCompileInfo* info)
{
    *break_labels_len = 0;

    *break_labels_before = info->sLoopInfo.break_labels;
    *break_labels_len_before = info->sLoopInfo.break_labels_len;
    info->sLoopInfo.break_labels = break_labels;                               // for NODE_TYPE_BREAK to determine the goto point
    info->sLoopInfo.break_labels_len = break_labels_len;
}

static void determine_the_goto_point_of_break(unsigned int* break_labels_before, int* break_labels_len_before, sCompileInfo* info)
{
    int j;
    for(j=0; j<*info->sLoopInfo.break_labels_len; j++) {
        *(info->code->mCode + info->sLoopInfo.break_labels[j]) = info->code->mLen;   // for the label of goto when break is caleld. see NODE_TYPE_BREAK
    }

    info->sLoopInfo.break_labels = break_labels_before;               // restore the value
    info->sLoopInfo.break_labels_len = break_labels_len_before;
}

// FALSE: overflow break labels TRUE: success
static BOOL set_zero_on_goto_point_of_break(sCompileInfo* info)
{
    append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);

    info->sLoopInfo.break_labels[*info->sLoopInfo.break_labels_len] = info->code->mLen;  // after compiling while loop, this is setted on the value of loop out. see NODE_TYPE_WHILE
    (*info->sLoopInfo.break_labels_len)++;

    if(*info->sLoopInfo.break_labels_len >= CL_BREAK_MAX) {
        return FALSE;
    }
    append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

    return TRUE;
}

static void prepare_for_continue_labels(unsigned int** continue_labels_before, int** continue_labels_len_before, unsigned int continue_labels[], int* continue_labels_len, sCompileInfo* info)
{
    *continue_labels_len = 0;

    *continue_labels_before = info->sLoopInfo.continue_labels;
    *continue_labels_len_before = info->sLoopInfo.continue_labels_len;
    info->sLoopInfo.continue_labels = continue_labels;                               // for NODE_TYPE_CONTINUE to determine the jump point
    info->sLoopInfo.continue_labels_len = continue_labels_len;
}

static void determine_the_goto_point_of_continue(unsigned int* continue_labels_before, int* continue_labels_len_before, sCompileInfo* info)
{
    int j;
    for(j=0; j<*info->sLoopInfo.continue_labels_len; j++) {
        *(info->code->mCode + info->sLoopInfo.continue_labels[j]) = info->code->mLen;
    }

    info->sLoopInfo.continue_labels = continue_labels_before;               // restore the value
    info->sLoopInfo.continue_labels_len = continue_labels_len_before;
}

// FALSE: overflow continue labels TRUE: success
static BOOL set_zero_goto_point_of_continue(sCompileInfo* info)
{
    append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);

    info->sLoopInfo.continue_labels[*info->sLoopInfo.continue_labels_len] = info->code->mLen;  // after compiling while loop, this is setted on the value of loop out. see NODE_TYPE_WHILE
    (*info->sLoopInfo.continue_labels_len)++;

    if(*info->sLoopInfo.continue_labels_len >= CL_BREAK_MAX) {
        return FALSE;
    }
    append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

    return TRUE;
}

static BOOL compile_expressiont_in_loop(unsigned int conditional_node, sCLNodeType** conditional_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info, sVarTable* lv_table)
{
    sVarTable* lv_table_before;
    int* stack_num_before;
    int conditional_stack_num;

    lv_table_before = info->lv_table;
    info->lv_table = lv_table;

    if(conditional_node) {
        if(!compile_node(conditional_node, conditional_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    info->lv_table = lv_table_before;

    return TRUE;
}

static BOOL compile_conditional(unsigned int conditional_node, sCLNodeType** conditional_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info, sCLNodeType** type_, sVarTable* conditional_lv_table)
{
    sVarTable* lv_table_before;
    int* stack_num_before;
    int conditional_stack_num;

    *conditional_type = NULL;

    if(conditional_lv_table) {
        lv_table_before = info->lv_table;
        info->lv_table = conditional_lv_table;
    }

    stack_num_before = info->stack_num;
    conditional_stack_num = 0;
    info->stack_num = &conditional_stack_num;

    ASSERT(conditional_node != 0);

    if(!compile_node(conditional_node, conditional_type, class_params, num_params, info)) 
    {
        if(conditional_lv_table) {
            info->lv_table = lv_table_before;
        }
        return FALSE;
    }

    info->stack_num = stack_num_before;

    if(conditional_stack_num != 1) {
        parser_err_msg_format(info->sname, *info->sline, "stack error. conditional stack num is %d. this should be 1.\n(require a bool value.)", conditional_stack_num);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        if(conditional_lv_table) {
            info->lv_table = lv_table_before;
        }
        return FALSE;
    }

    if(!type_identity(*conditional_type, gBoolType) && !type_identity(*conditional_type, gAnonymousType)) {
        parser_err_msg_format(info->sname, *info->sline, "require the bool type for conditional");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        if(conditional_lv_table) {
            info->lv_table = lv_table_before;
        }
        return FALSE;
    }

    if(conditional_lv_table) {
        info->lv_table = lv_table_before;
    }

    return TRUE;
}

BOOL compile_block_node(unsigned int block_node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    /// a block_node goes ///
    if(block_node != 0) {
        sCLNodeType* block_type;

        block_type = gIntType;

        if(!compile_node(block_node, &block_type, class_params, num_params, info)) {
            return FALSE;
        }

        if(!type_identity(block_type, gBlockType)) {
            parser_err_msg_format(info->sname, *info->sline, "require block type after \"with\" keyword");
            show_node_type_for_errmsg(block_type);
            parser_err_msg_without_line("\n");
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }
    }

    return TRUE;
}

BOOL determine_the_calling_method_before_compling_params(sCLClass** klass, sCLMethod** method, sCompileInfo* info, unsigned int node, char* method_name, sCLNodeType** type_, unsigned int block_id, unsigned int block_node)
{
    int block_num_params;
    sCLNodeType* block_type;
    sCLNodeType* block_param_types[CL_METHOD_PARAM_MAX];
    int used_param_num_with_initializer;
    sCLNodeType* result_type;
    sCLNodeType* err_messsage_class_params[CL_METHOD_PARAM_MAX];
    BOOL block_exist;
    int stack_num_before;
    sCLNodeType* type_before;
    BOOL no_output_to_bytecodes_before;
    sCLNodeType* param_type;
    sCLMethod* calling_method_before;
    sCLClass* class_of_calling_method_before;
    sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
    int num_params;
    sCLNodeType* block_class_params[CL_METHOD_PARAM_MAX];
    int num_block_params;
    BOOL not_found_method;
    BOOL calling_array_value_before;
    BOOL calling_block_before;

    /// save the value before compiling ///
    class_of_calling_method_before = info->sParamInfo.class_of_calling_method;
    calling_method_before = info->sParamInfo.calling_method;
    calling_block_before = info->sParamInfo.calling_block;
    calling_array_value_before = info->sParamInfo.calling_array_value;

    stack_num_before = *info->stack_num;
    type_before = *type_;

    no_output_to_bytecodes_before = info->no_output_to_bytecodes;

    /// set the flags ///
    info->no_output_to_bytecodes = TRUE;
    info->sParamInfo.class_of_calling_method = FALSE;
    info->sParamInfo.calling_method = FALSE;
    info->sParamInfo.calling_block = FALSE;
    info->sParamInfo.calling_array_value = FALSE;

    /// compile the params ///
    num_params = 0;
    memset(class_params, 0, sizeof(class_params));

    param_type = NULL;
    if(!compile_right_node(node, &param_type, class_params, &num_params, info)) {
        return FALSE;
    }

    /// determine the result type of method block ///
    if(!get_result_type_of_method_block(&gNodeBlocks[block_id], info, kBKMethodBlock))
    {
        return FALSE;
    }

    /// compile the block ///
    block_num_params = 0;
    memset(block_class_params, 0, sizeof(block_class_params));

    if(!compile_block_node(block_node, type_, block_class_params, &block_num_params, info))
    {
        return FALSE;
    }

    not_found_method = FALSE;
    if(!determine_the_calling_method(klass, method, method_name, FALSE, type_, class_params, &num_params, info, block_id, FALSE, &not_found_method, block_node, &block_num_params, &block_type, block_param_types, &block_exist, &used_param_num_with_initializer, &result_type, err_messsage_class_params))
    {
        return FALSE;
    }

    /// restore the value ///
    info->no_output_to_bytecodes = no_output_to_bytecodes_before;

    *info->stack_num = stack_num_before;
    *type_ = type_before;

    info->sParamInfo.class_of_calling_method = class_of_calling_method_before;
    info->sParamInfo.calling_method = calling_method_before;
    info->sParamInfo.calling_block = calling_block_before;
    info->sParamInfo.calling_array_value = calling_array_value_before;

    return TRUE;
}

BOOL compile_params_and_block(unsigned int node, sCompileInfo* info, unsigned int block_node, sCLClass* class_of_calling_method, sCLMethod* calling_method, int* num_params, sCLNodeType** class_params, sCLNodeType* type_)
{
    sCLMethod* calling_method_before;
    sCLClass* class_of_calling_method_before;
    BOOL calling_array_value_before;
    BOOL calling_block_before;
    sCLNodeType* param_type;
    sCLNodeType* block_type;

    sCLNodeType* block_class_params[CL_METHOD_PARAM_MAX];
    int block_num_params;

    /// save the flags ///
    calling_method_before = info->sParamInfo.calling_method;
    class_of_calling_method_before = info->sParamInfo.class_of_calling_method;
    calling_block_before = info->sParamInfo.calling_block;
    calling_array_value_before = info->sParamInfo.calling_array_value;

    /// set the flags ///
    info->sParamInfo.class_of_calling_method = class_of_calling_method;
    info->sParamInfo.calling_method = calling_method;
    info->sParamInfo.calling_block = FALSE;
    info->sParamInfo.calling_array_value = FALSE;

    /// params go ///
    *num_params = 0;
    memset(class_params, 0, sizeof(sCLNodeType*)*CL_METHOD_PARAM_MAX);

    param_type = type_;
    if(!compile_right_node(node, &param_type, class_params, num_params, info)) {
        return FALSE;
    }
    
    /// restore the flags ///
    info->sParamInfo.class_of_calling_method = class_of_calling_method_before;
    info->sParamInfo.calling_method = calling_method_before;
    info->sParamInfo.calling_array_value = calling_array_value_before;
    info->sParamInfo.calling_block = calling_block_before;

    /// a block_node goes ///
    block_num_params = 0;
    memset(block_class_params, 0, sizeof(block_class_params));

    block_type = type_;
    if(!compile_block_node(block_node, &block_type, block_class_params, &block_num_params, info))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL compile_node(unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info)
{
    if(node == 0) {
        parser_err_msg("no expression", info->sname, *info->sline);
        (*info->err_num)++;
        return TRUE;
    }

    switch(gNodes[node].mNodeType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCINT, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.mValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gIntType;
            }
            break;

        /// number value ///
        case NODE_TYPE_BYTE_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCBYTE, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.mByteValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gByteType;
            }
            break;

        /// number value ///
        case NODE_TYPE_SHORT_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCSHORT, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.mShortValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gShortType;
            }
            break;

        /// number value ///
        case NODE_TYPE_UINT_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCUINT, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.mUIntValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gUIntType;
            }
            break;

        /// number value ///
        case NODE_TYPE_LONG_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCLONG, info->no_output_to_bytecodes);
            append_ulong_value_to_bytecodes(info->code, gNodes[node].uValue.mLongValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gLongType;
            }
            break;

        /// number float value ///
        case NODE_TYPE_FVALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCFLOAT, info->no_output_to_bytecodes);
            offset = append_float_value_to_constant_pool(info->constant, gNodes[node].uValue.mFValue, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gFloatType;
            }
            break;

        /// number double value ///
        case NODE_TYPE_DVALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCDOUBLE, info->no_output_to_bytecodes);
            offset = append_double_value_to_constant_pool(info->constant, gNodes[node].uValue.mDValue, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gDoubleType;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCWSTR, info->no_output_to_bytecodes);
            offset = append_wstr_to_constant_pool(info->constant, gNodes[node].uValue.mStringValue, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gStringType;
            }
            break;

        //// bytes value ///
        case NODE_TYPE_BYTES_VALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCSTR, info->no_output_to_bytecodes);
            offset = append_str_to_constant_pool(info->constant, gNodes[node].uValue.mStringValue, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gBytesType;
            }
            break;

        //// path value ///
        case NODE_TYPE_PATH_VALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCPATH, info->no_output_to_bytecodes);
            offset = append_str_to_constant_pool(info->constant, gNodes[node].uValue.mStringValue, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = create_node_type_from_class_name("Path");
            }
            break;

        //// character value ///
        case NODE_TYPE_CHARACTER_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCCHAR, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, (int)gNodes[node].uValue.mCharacterValue, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gCharType;
            }
            break;

        /// array value ///
        case NODE_TYPE_ARRAY_VALUE: {
            sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType* left_type;
            sCLClass* klass;
            sCLNodeType* first_type;
            int j;
            sCLNodeType* array_type;
            BOOL calling_array_value_before;
            sCLNodeType* interface;

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// elements go ///
            calling_array_value_before = info->sParamInfo.calling_array_value;
            info->sParamInfo.calling_array_value = TRUE;
            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }
            info->sParamInfo.calling_array_value = calling_array_value_before;
            
            /// type checking ///
            if(num_params == 0) {
                first_type = gVoidType;
            }
            else {
                first_type = class_params[0];

                for(j=1; j<num_params; j++) {
                    if(!substitution_posibility(first_type, class_params[j])) {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("first type is ");
                        show_node_type_for_errmsg(first_type);
                        parser_err_msg_without_line(". but %dth of array element type is ", j+1);
                        show_node_type_for_errmsg(class_params[j]);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                }
            }

            interface = alloc_node_type();
            interface->mClass = cl_get_class("IComparableMore");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(first_type != gVoidType && !check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An element of Array class should implement IComparableMore interface. This is %s class.", REAL_CLASS_NAME(first_type->mClass));
                (*info->err_num)++;
            }

            interface->mClass = cl_get_class("IInspectable");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An element of Array class should implement IInspectable interface");
                (*info->err_num)++;
            }

            interface->mClass = cl_get_class("ICloneable");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An element of Array class should implement ICloneable interface");
                (*info->err_num)++;
            }

            array_type = alloc_node_type();
            array_type->mClass = gArrayType->mClass;
            array_type->mGenericsTypesNum = 1;
            array_type->mGenericsTypes[0] = ALLOC clone_node_type(first_type);

            append_opecode_to_bytecodes(info->code, OP_NEW_ARRAY, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, array_type, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, num_params, info->no_output_to_bytecodes);

            dec_stack_num(info->stack_num, num_params);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = array_type;
            }
            break;

        /// hash value ///
        case NODE_TYPE_HASH_VALUE: {
            sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType* left_type;
            sCLClass* klass;
            sCLNodeType* first_type;
            sCLNodeType* first_key_type;
            int j;
            sCLNodeType* hash_type;
            BOOL calling_array_value_before;
            sCLNodeType* interface;

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// elements go ///
            calling_array_value_before = info->sParamInfo.calling_array_value;
            info->sParamInfo.calling_array_value = TRUE;
            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }
            info->sParamInfo.calling_array_value = calling_array_value_before;
            
            /// type checking ///
            if(num_params == 0) {
                first_type = gVoidType;
            }
            else {
                ASSERT((num_params % 2) == 0 || num_params < CL_ARRAY_ELEMENTS_MAX*2);

                first_key_type = class_params[0];
                first_type = class_params[1];

                for(j=2; j<num_params; j+=2) {
                    if(!substitution_posibility(first_key_type, class_params[j])) {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("first key type is ");
                        show_node_type_for_errmsg(first_key_type);
                        parser_err_msg_without_line(". but %dth of hash key type is ", j/2+1);
                        show_node_type_for_errmsg(class_params[j]);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                    if(!substitution_posibility(first_type, class_params[j+1])) {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("first item type is ");
                        show_node_type_for_errmsg(first_type);
                        parser_err_msg_without_line(". but %dth of hash item ype is ", j/2+1);
                        show_node_type_for_errmsg(class_params[j+1]);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                }
            }

            interface = alloc_node_type();
            interface->mClass = cl_get_class("IInspectable");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An item of Hash class should implement IInspectable interface");
                (*info->err_num)++;
            }

            if(!check_implemented_interface(first_key_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "A key of Hash class should implement IInspectable interface");
                (*info->err_num)++;
            }

            interface->mClass = cl_get_class("IComparableMore");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An item of Hash class should implement IComparableMore interface");
                (*info->err_num)++;
            }

            if(!check_implemented_interface(first_key_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "A key of Hash class should implement IComparable interface");
                (*info->err_num)++;
            }

            interface->mClass = cl_get_class("ICloneable");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "An item of Hash class should implement ICloneable interface");
                (*info->err_num)++;
            }

            if(!check_implemented_interface(first_key_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "A key of Hash class should implement ICloneable interface");
                (*info->err_num)++;
            }

            interface->mClass = cl_get_class("IHashKey");
            interface->mGenericsTypesNum = 0;

            ASSERT(interface->mClass != NULL);

            if(!check_implemented_interface(first_key_type, interface))
            {
                parser_err_msg_format(info->sname, *info->sline, "A key of Hash class should implement IHashKey interface");
                (*info->err_num)++;
            }

            hash_type = alloc_node_type();
            hash_type->mClass = gHashType->mClass;
            hash_type->mGenericsTypesNum = 2;
            hash_type->mGenericsTypes[0] = ALLOC clone_node_type(first_key_type);
            hash_type->mGenericsTypes[1] = ALLOC clone_node_type(first_type);

            append_opecode_to_bytecodes(info->code, OP_NEW_HASH, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, hash_type, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, num_params, info->no_output_to_bytecodes);

            dec_stack_num(info->stack_num, num_params);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = hash_type;
            }
            break;

        /// tuple value ///
        case NODE_TYPE_TUPLE_VALUE: {
            sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType* left_type;
            sCLClass* klass;
            sCLNodeType* first_type;
            int j;
            sCLNodeType* tuple_type;
            BOOL calling_array_value_before;
            sCLNodeType* interface;

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// elements go ///
            calling_array_value_before = info->sParamInfo.calling_array_value;
            info->sParamInfo.calling_array_value = TRUE;
            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }
            info->sParamInfo.calling_array_value = calling_array_value_before;
            
            for(j=0; j< num_params; j++) {
                interface = alloc_node_type();
                interface->mClass = cl_get_class("IComparableMore");
                interface->mGenericsTypesNum = 0;

                ASSERT(interface->mClass != NULL);

                if(!check_implemented_interface(class_params[j], interface))
                {
                    parser_err_msg_format(info->sname, *info->sline, "An element of Tuple class should implement IComparableMore interface");
                    (*info->err_num)++;
                }

                interface->mClass = cl_get_class("IInspectable");
                interface->mGenericsTypesNum = 0;

                ASSERT(interface->mClass != NULL);

                if(!check_implemented_interface(class_params[j], interface))
                {
                    parser_err_msg_format(info->sname, *info->sline, "An element of Tuple class should implement IInspectable interface");
                    (*info->err_num)++;
                }

                interface->mClass = cl_get_class("ICloneable");
                interface->mGenericsTypesNum = 0;

                ASSERT(interface->mClass != NULL);

                if(!check_implemented_interface(class_params[j], interface))
                {
                    parser_err_msg_format(info->sname, *info->sline, "An element of Tuple class should implement ICloneable interface");
                    (*info->err_num)++;
                }
            }

            ASSERT(num_params > 0);

            tuple_type = alloc_node_type();
            tuple_type->mClass = gTupleType[num_params-1]->mClass;
            tuple_type->mGenericsTypesNum = num_params;
            for(j=0; j<num_params; j++) {
                tuple_type->mGenericsTypes[j] = ALLOC clone_node_type(class_params[j]);
            }

            append_opecode_to_bytecodes(info->code, OP_NEW_TUPLE, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, tuple_type, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, num_params, info->no_output_to_bytecodes);

            dec_stack_num(info->stack_num, num_params);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = tuple_type;
            }
            break;

        /// regex ///
        case NODE_TYPE_REGEX_VALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_NEW_REGEX, info->no_output_to_bytecodes);

            offset = append_str_to_constant_pool(info->constant, gNodes[node].uValue.sRegex.mRegexString, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, offset, info->no_output_to_bytecodes);

            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.sRegex.mGlobal, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.sRegex.mMultiline, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.sRegex.mIgnoreCase, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gRegexType;
            }
            break;

        /// reange value ///
        case NODE_TYPE_RANGE_VALUE: {
            sCLNodeType* left_type;
            sCLNodeType* right_type;

            /// head go ///
            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            /// tail go ///
            right_type = NULL;
            if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                return FALSE;
            }

            /// type checking ///
            if(!substitution_posibility(left_type, gIntType)) {
                parser_err_msg_format(info->sname, *info->sline, "type error.");
                parser_err_msg_without_line("Head of range type is ");
                show_node_type_for_errmsg(left_type);
                parser_err_msg_without_line(". Require int type");
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                return TRUE;
            }

            if(!substitution_posibility(right_type, gIntType)) {
                parser_err_msg_format(info->sname, *info->sline, "type error.");
                parser_err_msg_without_line("Tail of range type is ");
                show_node_type_for_errmsg(right_type);
                parser_err_msg_without_line(". Require int type\n");
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                return TRUE;
            }
            
            append_opecode_to_bytecodes(info->code, OP_NEW_RANGE, info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, gRangeType, info->no_output_to_bytecodes);

            dec_stack_num(info->stack_num, 2);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gRangeType;
            }
            break;

        /// null value ///
        case NODE_TYPE_NULL: {
            append_opecode_to_bytecodes(info->code, OP_LDCNULL, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gNullType;
            }
            break;

        /// true value ///
        case NODE_TYPE_TRUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCBOOL, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gBoolType;
            }
            break;

        /// false value ///
        case NODE_TYPE_FALSE: {
            append_opecode_to_bytecodes(info->code, OP_LDCBOOL, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gBoolType;
            }
            break;

        /// class name ///
        case NODE_TYPE_CLASS_NAME: {
            int i;
            char* class_name;

            append_opecode_to_bytecodes(info->code, OP_LDTYPE, info->no_output_to_bytecodes);
            append_generics_type_to_bytecode(info->code, info->constant, gNodes[node].mType, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, info->mNestOfMethodFromDefinitionPoint, info->no_output_to_bytecodes);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gTypeType;
            }
            break;

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: 
            parser_err_msg_format(info->sname, *info->sline, "Require to initialize this local variable(%s)", gNodes[node].uValue.sVarName.mVarName);
            (*info->err_num)++;

            *type_ = gNodes[node].mType;
            break;

        /// load variable ///
        case NODE_TYPE_VARIABLE_NAME: {
            char* name;

            name = gNodes[node].uValue.sVarName.mVarName;
            if(!load_local_varialbe(name, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store value to variable ///
        case NODE_TYPE_STORE_VARIABLE_NAME: {
            char* name;
            sVar* var;

            if(info->lv_table == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
                (*info->err_num)++;

                *type_ = gIntType;
                break;
            }
            
            name = gNodes[node].uValue.sVarName.mVarName;

            var = get_variable_from_table(info->lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                return TRUE;
            }

            *type_ = var->mType;
            if(!store_local_variable(name, var, node, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// define variable and store a value ///
        case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME: {
            char* name;
            sCLNodeType* type2;
            sVar* var;

            /// define variable ///
            name = gNodes[node].uValue.sVarName.mVarName;
            type2 = gNodes[node].mType;

            /// check generics type  ///
            if(type2->mClass->mGenericsTypesNum != type2->mGenericsTypesNum) {
                parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(type2->mClass));
                (*info->err_num)++;
            }

            var = get_variable_from_table(info->lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "threre is not variable which is named (%s)", name);
                *type_ = gIntType; // dummy
                break;
            }

            /// store ///
            *type_ = var->mType;

            if(!store_local_variable(name, var, node, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;
        
        /// load field  ///
        case NODE_TYPE_FIELD: {
            /// left_value ///
            sCLNodeType* left_type;
            char* field_name;

            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            field_name = gNodes[node].uValue.sVarName.mVarName;
            *type_ = left_type;

            if(!load_field(field_name, FALSE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// load class field ///
        case NODE_TYPE_CLASS_FIELD: {
            char* field_name;
            
            field_name = gNodes[node].uValue.sVarName.mVarName;

            *type_ = gNodes[node].mType;

            if(!load_field(field_name, TRUE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store field ///
        case NODE_TYPE_STORE_FIELD: {
            /// left_value ///
            sCLNodeType* field_type;
            char* field_name;

            field_type = NULL;
            if(!compile_left_node(node, &field_type, class_params, num_params, info)) {
                return FALSE;
            }

            *type_ = field_type;
            field_name = gNodes[node].uValue.sVarName.mVarName;

            if(!store_field(node, field_name, FALSE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store class field ///
        case NODE_TYPE_STORE_CLASS_FIELD: {
            char* field_name;

            *type_ = gNodes[node].mType;
            field_name = gNodes[node].uValue.sVarName.mVarName;

            if(!store_field(node, field_name, TRUE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store class field ///
        case NODE_TYPE_STORE_TUPLE:
            if(!store_tuple(node, type_, class_params, num_params, info)) {
                return FALSE;
            }
            break;

        /// new operand ///
        case NODE_TYPE_NEW: {
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;
            sCLNodeType* klass;
            char* method_name;
            unsigned int block_id;
            BOOL not_found_method;
            unsigned int block_node;

            sCLMethod* calling_method;
            sCLClass* class_of_calling_method;
            
            klass = gNodes[node].mType;

            ASSERT(klass->mClass != NULL);

            if(klass->mClass->mFlags & CLASS_FLAGS_ABSTRACT) {
                parser_err_msg_format(info->sname, *info->sline, "This is an abstract class. An abstract class can't create object with new operator.");
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }
/*
            else if(klass->mClass->mFlags & CLASS_FLAGS_ENUM) {
                parser_err_msg_format(info->sname, *info->sline, "This is an enum class. An enum class can't create object with new operator.");
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }
*/
            else {
                append_opecode_to_bytecodes(info->code, OP_NEW_OBJECT, info->no_output_to_bytecodes);

                append_generics_type_to_bytecode(info->code, info->constant, klass, info->no_output_to_bytecodes);

                inc_stack_num(info->stack_num, info->max_stack, 1);
            }

            /// determine the calling method ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;
            block_id = gNodes[node].uValue.sMethod.mBlock;
            method_name = "_constructor";
            *type_ = klass;

            if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
            {
                return FALSE;
            }

            /// params go ///
            if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
            {
                return FALSE;
            }

            /// call constructor ///
            method_name = "_constructor";
            *type_ = klass;
            block_id = gNodes[node].uValue.sMethod.mBlock;
            not_found_method = FALSE;

            if(num_params2 == 0) {       // no call constructor if not defined
                if(!call_method(method_name, FALSE, type_, class_params2, &num_params2, info, block_id, TRUE, &not_found_method, block_node, NULL))
                {
                    return FALSE;
                }

            }
            else {
                if(!call_method(method_name, FALSE, type_, class_params2, &num_params2, info, block_id, FALSE, &not_found_method, block_node, NULL))
                {
                    return FALSE;
                }
            }

            *type_ = klass; // When no defined constructor, this is needed
            }
            break;

        /// call method ///
        case NODE_TYPE_METHOD_CALL: {
            sCLNodeType* left_type;
            sCLClass* class_of_calling_method;
            sCLMethod* calling_method;
            char* method_name;
            unsigned int block_id;
            BOOL not_found_method;
            unsigned int block_node;
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            /// left_value ///
            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            /// check ///
            if(type_identity(left_type, gVoidType)) {
                parser_err_msg("Clover can't call the method of void", info->sname, *info->sline);
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            /// determine the calling method ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = left_type;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
            {
                return FALSE;
            }

            /// params go ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;

            if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
            {
                return FALSE;
            }

            /// call method ///
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = left_type;
            block_id = gNodes[node].uValue.sMethod.mBlock;


            not_found_method = FALSE;
            if(!call_method(method_name, FALSE, type_, class_params2, &num_params2, info, block_id, FALSE, &not_found_method, block_node, NULL))
            {
                return FALSE;
            }
            }
            break;

        /// call class method ///
        case NODE_TYPE_CLASS_METHOD_CALL: {
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            unsigned int block_node;
            char* method_name;
            unsigned int block_id;
            sCLClass* class_of_class_method_call;

            sCLNodeType* right_type;
            sCLNodeType* klass;
            BOOL not_found_method;

            sCLClass* class_of_calling_method;
            sCLMethod* calling_method;

            /// determine the calling method ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = gNodes[node].mType;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
            {
                return FALSE;
            }

            /// params go ///
            if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
            {
                return FALSE;
            }

            /// call class method //
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = gNodes[node].mType;
            block_id = gNodes[node].uValue.sMethod.mBlock;
            class_of_class_method_call = gNodes[node].mType->mClass;

            not_found_method = FALSE;
            if(!call_method(method_name, TRUE, type_, class_params2, &num_params2, info, block_id, FALSE, &not_found_method, block_node, class_of_class_method_call))
            {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_SUPER: {
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;
            unsigned int block_id;
            unsigned int block_node;
            char* method_name;
            sCLMethod* calling_method;
            sCLClass* class_of_calling_method;

            if(info->real_caller_method == NULL) {
                parser_err_msg("can't call super because there are not the caller method.", info->sname, *info->sline);
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            /// load self ///
            if(!(info->real_caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, class_params, num_params, info)) {
                    return FALSE;
                }
            }

            /// determine the calling method ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            if(info->real_caller_method->mFlags & CL_CONSTRUCTOR) {
                method_name = "_constructor";
            }
            else {
                method_name = METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method);
            }

            *type_ = info->real_caller_class;

            if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
            {
                return FALSE;
            }

            /// params go ///
            if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
            {
                return FALSE;
            }

            /// call super ///
            block_id = gNodes[node].uValue.sMethod.mBlock;

            if(!call_super(method_name, type_, class_params2, &num_params2, info, block_id, block_node)) {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_INHERIT: {
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;
            unsigned int block_id;
            unsigned int block_node;
            char* method_name;
            sCLMethod* calling_method;
            sCLClass* class_of_calling_method;

            if(info->real_caller_method == NULL) {
                parser_err_msg("can't call inherit because there are not the caller method.", info->sname, *info->sline);
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            /// load self ///
            if(!(info->real_caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, class_params, num_params, info)) {
                    return FALSE;
                }
            }

            /// determine the calling method ///
            block_node = gNodes[node].uValue.sMethod.mBlockNode;
            block_id = gNodes[node].uValue.sMethod.mBlock;
            method_name = METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method);
            *type_ = info->real_caller_class;

            if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
            {
                return FALSE;
            }

            /// params go ///
            if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
            {
                return FALSE;
            }

            /// call mixin ///
            if(!call_mixin(method_name, type_, class_params2, &num_params2, info, block_id, block_node)) {
                return FALSE;
            }
            }
            break;

        /// params ///
        case NODE_TYPE_PARAM: {
            sCLNodeType* left_type;
            sCLNodeType* right_type;

            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }
            right_type = NULL;
            if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                return FALSE;
            }

            if(right_type->mClass == NULL) {
                *type_ = gIntType; // dummy
                (*info->err_num)++;
            }
            else {
                *type_ = right_type;
            }

            /// call clone method ///
            if(!info->sParamInfo.calling_array_value) {
                if(info->sParamInfo.calling_block && info->real_caller_method->mNumBlockType) 
                {
                    if(info->real_caller_method->mNumBlockType == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "There is not a method block of this method(%s)", METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method));
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }
                    else if(*num_params < info->real_caller_method->mBlockType.mNumParams) {
                        sCLNodeType* param_type;

                        param_type = create_node_type_from_cl_type(&info->real_caller_method->mBlockType.mParamTypes[*num_params], info->real_caller_class->mClass);

                        if(!call_clone_method_for_calling_by_value(info, param_type, *type_, type_))
                        {
                            return FALSE;
                        }
                    }
                    else {
                        parser_err_msg_format(info->sname, *info->sline, "Clover can't determine the block parametor type of this method(%s)", METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method));
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }
                }
                else if(info->sParamInfo.calling_method && info->sParamInfo.class_of_calling_method) {
                    if(*num_params < info->sParamInfo.calling_method->mNumParams) {
                        sCLNodeType* param_type;

                        param_type = create_node_type_from_cl_type(&info->sParamInfo.calling_method->mParamTypes[*num_params], info->sParamInfo.class_of_calling_method);

                        if(!call_clone_method_for_calling_by_value(info, param_type, *type_, type_))
                        {
                            return FALSE;
                        }
                    }
                    else {
                        /// If calling method would have variable arguments, Clover would use the type of parametor self for clone ///
                        if(info->sParamInfo.calling_method->mFlags & CL_METHOD_PARAM_VARABILE_ARGUMENTS) 
                        {
                            if(!call_clone_method_for_calling_by_value(info, *type_, *type_, type_))
                            {
                                return FALSE;
                            }
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "Clover can't determine the parametor type of this method(%s)", METHOD_NAME2(info->sParamInfo.class_of_calling_method, info->sParamInfo.calling_method));
                            (*info->err_num)++;

                            *type_ = gIntType; // dummy
                            break;
                        }
                    }
                }
            }

            class_params[*num_params] = *type_;
            (*num_params)++;
            if(*num_params >= CL_METHOD_PARAM_MAX) {
                parser_err_msg("overflow param number", info->sname, *info->sline);
                return FALSE;
            }

            }
            break;

        /// return ///
        case NODE_TYPE_RETURN:
            /// in a method block ///
            if(info->sBlockInfo.block_kind == kBKMethodBlock || info->sBlockInfo.block_kind == kBKTryBlock) 
            {
                sCLNodeType* left_type;
                sCLNodeType* result_type;

                if(info->sBlockInfo.method_block == NULL) {
                    parser_err_msg("there is not in a method block", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                if(info->sBlockInfo.method_block->mBlockType->mClass == NULL) {
                    parser_err_msg("unexpected err. no result type of method block", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                if(type_identity(info->sBlockInfo.method_block->mBlockType, gVoidType) || type_identity(info->sBlockInfo.method_block->mBlockType, gBoolType)) 
                {
                    if(gNodes[node].mLeft) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method block is void. There is a value of this return");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_RETURN, info->no_output_to_bytecodes);

                    if(info->exist_return) *(info->exist_return) = TRUE;

                    *type_ = gVoidType;
                }
                else {
                    if(gNodes[node].mLeft == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method block is not void. should return a value");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    left_type = NULL;
                    if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                        return FALSE;
                    }

                    make_block_result(&left_type);

                    if(!substitution_posibility_with_solving_generics(info->sBlockInfo.method_block->mBlockType, left_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
                    {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("require type is ");
                        show_node_type_for_errmsg(info->sBlockInfo.method_block->mBlockType);
                        parser_err_msg_without_line(". but this type is ");
                        show_node_type_for_errmsg(left_type);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_RETURN, info->no_output_to_bytecodes);

                    if(info->exist_return) *(info->exist_return) = TRUE;

                    if(*info->stack_num > 1) {
                        parser_err_msg_format(info->sname, *info->sline, "too many value of return");
                        (*info->err_num)++;
                    }
                    else if(*info->stack_num == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The value of return statment is required ");
                        (*info->err_num)++;
                    }

                    *info->stack_num = 0;      // no pop please

                    *type_ = gVoidType;
                }
            }
            /// in a method ///
            else {
                sCLNodeType* left_type;
                sCLNodeType* result_type;

                if(info->real_caller_class == NULL || info->real_caller_class->mClass == NULL || info->real_caller_method == NULL) {
                    parser_err_msg("There is not caller method. can't return", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                result_type = get_result_type_of_method(info->real_caller_class, info->real_caller_method);

                if(result_type->mClass == NULL) {
                    parser_err_msg("Unexpected err. no result type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                if(type_identity(result_type, gVoidType)) {
                    if(gNodes[node].mLeft) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method(%s::%s) is void. can't return a value", REAL_CLASS_NAME(info->real_caller_class->mClass), METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method));
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_LDCNULL, info->no_output_to_bytecodes);
                    inc_stack_num(info->stack_num, info->max_stack, 1);

                    append_opecode_to_bytecodes(info->code, OP_RETURN, info->no_output_to_bytecodes);

                    if(info->exist_return) *(info->exist_return) = TRUE;

                    if(*info->stack_num > 1) {
                        parser_err_msg_format(info->sname, *info->sline, "Too many value of return");
                        (*info->err_num)++;
                    }
                    else if(*info->stack_num == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The value of return statment is required ");
                        (*info->err_num)++;
                    }
                    *info->stack_num = 0;      // no pop please

                    *type_ = gVoidType;
                }
                else {
                    if(gNodes[node].mLeft == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method(%s::%s) is not void. should return a value", REAL_CLASS_NAME(info->real_caller_class->mClass), METHOD_NAME2(info->real_caller_class->mClass, info->real_caller_method));
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    left_type = NULL;
                    if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                        return FALSE;
                    }

                    if(!substitution_posibility_with_solving_generics(result_type, left_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
                    {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("Require type is ");
                        show_node_type_for_errmsg(result_type);
                        parser_err_msg_without_line(". but this type is ");
                        show_node_type_for_errmsg(left_type);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_RETURN, info->no_output_to_bytecodes);

                    if(info->exist_return) *(info->exist_return) = TRUE;

                    if(*info->stack_num > 1) {
                        parser_err_msg_format(info->sname, *info->sline, "Too many value of return");
                        (*info->err_num)++;
                    }
                    else if(*info->stack_num == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The value of return statment is required ");
                        (*info->err_num)++;
                    }

                    *info->stack_num = 0;      // no pop please

                    *type_ = gVoidType;
                }
            }
            break;

        case NODE_TYPE_THROW: {
            sCLNodeType* left_type;
            sCLNodeType* result_type;

            left_type = NULL;
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            /// check throws exception type ///
            if(info->real_caller_class && info->real_caller_class->mClass && info->real_caller_method) {
                if(!info->sBlockInfo.in_try_block && !is_method_exception_class(info->real_caller_class->mClass, info->real_caller_method, left_type->mClass))
                {
                    parser_err_msg_format(info->sname, *info->sline, "type error. require exception type of the method has.");
                    parser_err_msg_without_line("but this type is ");
                    show_node_type_for_errmsg(left_type);
                    parser_err_msg_without_line("\n");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }
            }
            else {
                if(!substitution_posibility_with_solving_generics(gExceptionType, left_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
                {
                    parser_err_msg_format(info->sname, *info->sline, "type error.");
                    parser_err_msg_without_line("require type is ");
                    show_node_type_for_errmsg(gExceptionType);
                    parser_err_msg_without_line(". but this type is ");
                    show_node_type_for_errmsg(left_type);
                    parser_err_msg_without_line("\n");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }
            }

            append_opecode_to_bytecodes(info->code, OP_THROW, info->no_output_to_bytecodes);

            if(*info->stack_num > 1) {
                parser_err_msg_format(info->sname, *info->sline, "too many value of throw");
                (*info->err_num)++;
            }
            else if(*info->stack_num == 0) {
                parser_err_msg_format(info->sname, *info->sline, "the value of throw statment is required ");
                (*info->err_num)++;
            }

            *info->stack_num = 0;      // no pop please

            *type_ = gVoidType;
            }
            break;

        case NODE_TYPE_TRY: {
            sConst constant;
            sByteCode code;

            sNodeBlock* try_block;
            sNodeBlock* catch_blocks[CL_CATCH_BLOCK_NUMBER_MAX];
            sNodeBlock* finally_block;
            int j;

            sVar* var;
            int var_index;

            BOOL in_try_block_before;

            int parent_max_block_var_num;

            int catch_block_number;

            try_block = gNodeBlocks + gNodes[node].uValue.sTryBlock.mTryBlock;
            for(j=0; j<gNodes[node].uValue.sTryBlock.mCatchBlockNumber; j++) {
                catch_blocks[j] = gNodeBlocks + gNodes[node].uValue.sTryBlock.mCatchBlocks[j];
            }
            if(gNodes[node].uValue.sTryBlock.mFinallyBlock) {
                finally_block = gNodeBlocks + gNodes[node].uValue.sTryBlock.mFinallyBlock;
            }
            else {
                finally_block = 0;
            }

            catch_block_number = gNodes[node].uValue.sTryBlock.mCatchBlockNumber;

            /// compile try block ///
            in_try_block_before = info->sBlockInfo.in_try_block;
            info->sBlockInfo.in_try_block = TRUE;

            sConst_init(&constant);
            sByteCode_init(&code);

            if(!compile_block_object(try_block, &constant, &code, type_, info, info->real_caller_class, info->real_caller_method, kBKTryBlock)) 
            {
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                info->sBlockInfo.in_try_block = in_try_block_before;
                return TRUE;
            }

            info->sBlockInfo.in_try_block = in_try_block_before;

            append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

            append_int_value_to_bytecodes(info->code, try_block->mMaxStack, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, try_block->mNumLocals, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, try_block->mNumParams, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, get_parent_var_num(try_block), info->no_output_to_bytecodes);

            append_generics_type_to_bytecode(info->code, info->constant, try_block->mBlockType, info->no_output_to_bytecodes);
            for(j=0; j<try_block->mNumParams; j++) {
                append_generics_type_to_bytecode(info->code, info->constant, try_block->mClassParams[j], info->no_output_to_bytecodes);
            }

            append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
            append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);          // breakable

            FREE(constant.mConst);
            FREE(code.mCode);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            /// compile catch block ///
            for(j=0; j<catch_block_number; j++) {
                sConst_init(&constant);
                sByteCode_init(&code);
                sCLNodeType* node_type;
                int k;

                if(!compile_block_object(catch_blocks[j], &constant, &code, type_, info, info->real_caller_class, info->real_caller_method, kBKTryBlock)) 
                {
                    (*info->err_num)++;
                    *type_ = gIntType; // dummy
                    return TRUE;
                }

                append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

                append_int_value_to_bytecodes(info->code, catch_blocks[j]->mMaxStack, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, catch_blocks[j]->mNumLocals, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, catch_blocks[j]->mNumParams, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, get_parent_var_num(catch_blocks[j]), info->no_output_to_bytecodes);

                append_generics_type_to_bytecode(info->code, info->constant, catch_blocks[j]->mBlockType, info->no_output_to_bytecodes);
                for(k=0; k<catch_blocks[j]->mNumParams; k++) {
                    append_generics_type_to_bytecode(info->code, info->constant, catch_blocks[j]->mClassParams[k], info->no_output_to_bytecodes);
                }

                append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
                append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);          // breakable

                FREE(constant.mConst);
                FREE(code.mCode);

                node_type = gNodes[node].uValue.sTryBlock.mExceptionType[j];

                append_opecode_to_bytecodes(info->code, OP_LDTYPE, info->no_output_to_bytecodes);
                append_generics_type_to_bytecode(info->code, info->constant, node_type, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, -1, info->no_output_to_bytecodes);

                inc_stack_num(info->stack_num, info->max_stack, 2);
            }

            /// compile finally block ///
            if(finally_block) {
                int j;
                sConst_init(&constant);
                sByteCode_init(&code);

                if(!compile_block_object(finally_block, &constant, &code, type_, info, info->real_caller_class, info->real_caller_method, kBKTryBlock)) 
                {
                    (*info->err_num)++;
                    *type_ = gIntType; // dummy
                    return TRUE;
                }

                append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK, info->no_output_to_bytecodes);

                append_int_value_to_bytecodes(info->code, finally_block->mMaxStack, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, finally_block->mNumLocals, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, finally_block->mNumParams, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, get_parent_var_num(finally_block), info->no_output_to_bytecodes);

                append_generics_type_to_bytecode(info->code, info->constant, finally_block->mBlockType, info->no_output_to_bytecodes);
                for(j=0; j<finally_block->mNumParams; j++) {
                    append_generics_type_to_bytecode(info->code, info->constant, finally_block->mClassParams[j], info->no_output_to_bytecodes);
                }

                append_constant_pool_to_bytecodes(info->code, info->constant, &constant, info->no_output_to_bytecodes);
                append_code_to_bytecodes(info->code, info->constant, &code, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);          // breakable

                FREE(constant.mConst);
                FREE(code.mCode);

                inc_stack_num(info->stack_num, info->max_stack, 1);
            }


            append_opecode_to_bytecodes(info->code, OP_TRY, info->no_output_to_bytecodes);

            append_int_value_to_bytecodes(info->code, catch_block_number, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, finally_block ? 1:0, info->no_output_to_bytecodes);

            *info->stack_num = 0;       // pop on OP_TRY running

            if(finally_block && !type_identity(finally_block->mBlockType, gVoidType)) 
            {
                append_int_value_to_bytecodes(info->code, 1, info->no_output_to_bytecodes);
                inc_stack_num(info->stack_num, info->max_stack, 1);
                *type_ = finally_block->mBlockType;
            }
            else {
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);
                inc_stack_num(info->stack_num, info->max_stack, 0);
                *type_ = gVoidType;
            }

            }
            break;

        case NODE_TYPE_BREAK: 
            if(info->sBlockInfo.block_kind == kBKWhileDoForBlock) {
                sCLNodeType* left_type;
                sCLNodeType* result_type;

                if(info->sBlockInfo.while_type == 0 || info->sLoopInfo.break_labels == NULL) {
                    parser_err_msg_format(info->sname, *info->sline, "requires while type or goto label for break. it is not in loop");
                    *type_ = gIntType; // dummy
                    (*info->err_num)++;
                    break;
                }

                result_type = info->sBlockInfo.while_type;  // this is gotten from NODE_TYPE_WHILE or NODE_TYPE_DO. this is the result type of while.

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!set_zero_on_goto_point_of_break(info)) {
                    parser_err_msg_format(info->sname, *info->sline, "too many break. overflow");
                    (*info->err_num)++;
                    *type_ = gIntType;
                    break;
                }

                if(info->exist_break) *(info->exist_break) = TRUE;

                ASSERT(result_type->mClass != NULL);
                if(type_identity(result_type, gVoidType)) {
                    *type_ = gVoidType;

                    if(*info->stack_num != 0) {
                        parser_err_msg_format(info->sname, *info->sline, "this is not require a value of this break. stack error");
                        (*info->err_num)++;
                        *type_ = gIntType;
                        break;
                    }
                }
                else {
                    *type_ = result_type;

                    if(left_type->mClass != NULL) {
                        if(!substitution_posibility_with_solving_generics(result_type, left_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
                        {
                            parser_err_msg_format(info->sname, *info->sline, "type error.");
                            parser_err_msg_without_line("left type is ");
                            show_node_type_for_errmsg(result_type);
                            parser_err_msg_without_line(". right type is ");
                            show_node_type_for_errmsg(left_type);
                            parser_err_msg_without_line("\n");
                            (*info->err_num)++;

                            *type_ = gIntType; // dummy
                            break;
                        }
                    }

                    if(*info->stack_num != 1) {
                        parser_err_msg_format(info->sname, *info->sline, "require one result value of this break");
                        (*info->err_num)++;
                        *type_ = gIntType;
                        break;
                    }
                }
            }
            else if(info->sBlockInfo.method_block && info->caller_class && info->caller_method) {
                if(type_identity(info->sBlockInfo.method_block->mBlockType, gVoidType) || type_identity(info->sBlockInfo.method_block->mBlockType, gBoolType)) 
                {
                    if(gNodes[node].mLeft) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method block is void");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_BREAK_IN_METHOD_BLOCK, info->no_output_to_bytecodes);
                    if(info->exist_return) *(info->exist_return) = TRUE;

                    *type_ = gVoidType;
                }
                else {
                    sCLNodeType* left_type;

                    if(gNodes[node].mLeft == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The result type of this method block is not void. should return a value");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    left_type = NULL;
                    if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                        return FALSE;
                    }

                    make_block_result(&left_type);

                    if(!substitution_posibility_with_solving_generics(info->sBlockInfo.method_block->mBlockType, left_type, info->real_caller_class ? info->real_caller_class->mClass : NULL, info->real_caller_method)) 
                    {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        parser_err_msg_without_line("require type is ");
                        show_node_type_for_errmsg(info->sBlockInfo.method_block->mBlockType);
                        parser_err_msg_without_line(". but this type is ");
                        show_node_type_for_errmsg(left_type);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    append_opecode_to_bytecodes(info->code, OP_BREAK_IN_METHOD_BLOCK, info->no_output_to_bytecodes);
                    if(info->exist_return) *(info->exist_return) = TRUE;

                    if(*info->stack_num > 1) {
                        parser_err_msg_format(info->sname, *info->sline, "Too many value of break");
                        (*info->err_num)++;
                    }
                    else if(*info->stack_num == 0) {
                        parser_err_msg_format(info->sname, *info->sline, "The value of break statment is required ");
                        (*info->err_num)++;
                    }

                    *info->stack_num = 0;      // no pop please

                    *type_ = gVoidType;
                }
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "it is not in loop");
                *type_ = gIntType; // dummy
                (*info->err_num)++;
            }
            break;

        case NODE_TYPE_CONTINUE: {
            sCLNodeType* result_type;

            if(info->sLoopInfo.continue_labels == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "there is not goto label for continue. it is not in loop");
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            if(!set_zero_goto_point_of_continue(info)) {
                parser_err_msg_format(info->sname, *info->sline, "too many continue. overflow");
                (*info->err_num)++;
                *type_ = gIntType;
                break;
            }

            *type_ = gVoidType;

            if(*info->stack_num != 0) {
                parser_err_msg_format(info->sname, *info->sline, "there is a value of this continue. stack error");
                (*info->err_num)++;
                *type_ = gIntType;
                break;
            }
            }
            break;

        case NODE_TYPE_BLOCK: {
            int j;
            sNodeBlock* block;

            /// block ///
            block = gNodeBlocks + gNodes[node].uValue.mBlock;

            if(!compile_block(block, type_, info)) {
                return FALSE;
            }

            ASSERT(gNodes[node].mType->mClass != NULL);
            if(type_identity(gNodes[node].mType, gVoidType)) {
                *type_ = gVoidType;
            }
            else {
                *type_ = gNodes[node].mType;
                (*info->stack_num)++;
            }
            }
            break;

        case NODE_TYPE_IF: {
            int j;
            sCLNodeType* conditional_type;
            sNodeBlock* block;
            int ivalue[CL_ELSE_IF_MAX + 1];
            int ivalue_num = 0;
            int ivalue2;
            BOOL no_pop_last_one_value;

            block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mIfBlock;

            /// conditional ///
            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, type_, block->mLVTable)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);                   // jump to if block

            /// block of if ///
            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to else or else if
            ivalue2 = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

            if(!compile_block(block, type_, info)) {
                return FALSE;
            }

            correct_stack_pointer(info->stack_num, info->sname, info->sline, info->code, info->err_num, info->no_output_to_bytecodes);
            *info->stack_num = 0;

            if(gNodes[node].uValue.sIfBlock.mElseBlock == 0) {
                *(info->code->mCode + ivalue2) = info->code->mLen + 2;
            }
            else if(gNodes[node].uValue.sIfBlock.mElseIfBlockNum == 0) {
                *(info->code->mCode + ivalue2) = info->code->mLen + 4;
            }
            else {
                *(info->code->mCode + ivalue2) = info->code->mLen + 2;
            }

            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);
            ivalue[ivalue_num++] = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

            for(j=0; j<gNodes[node].uValue.sIfBlock.mElseIfBlockNum; j++) {
                sCLNodeType* else_if_type;

                block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mElseIfBlock[j];

                /// else if conditional ///
                if(!compile_conditional(gNodes[node].uValue.sIfBlock.mElseIfConditional[j], &else_if_type, class_params, num_params, info, type_, block->mLVTable)) {
                    return FALSE;
                }

                append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);

                /// append else if block to bytecodes ///
                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                if(!compile_block(block, type_, info)) {
                    return FALSE;
                }

                correct_stack_pointer(info->stack_num, info->sname, info->sline, info->code, info->err_num, info->no_output_to_bytecodes);
                *info->stack_num = 0;

                if(gNodes[node].uValue.sIfBlock.mElseBlock == 0) {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 2;
                }
                else if(j == gNodes[node].uValue.sIfBlock.mElseIfBlockNum-1) {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 4;
                }
                else {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 2;
                }

                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);
                ivalue[ivalue_num++] = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);
            }

            if(gNodes[node].uValue.sIfBlock.mElseBlock) {
                block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mElseBlock;

                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                if(!compile_block(block, type_, info)) {
                    return FALSE;
                }

                correct_stack_pointer(info->stack_num, info->sname, info->sline, info->code, info->err_num, info->no_output_to_bytecodes);
                *info->stack_num = 0;


                *(info->code->mCode + ivalue2) = info->code->mLen;
            }

            for(j=0; j<ivalue_num; j++) {
                *(info->code->mCode + ivalue[j]) = info->code->mLen;
            }

            ASSERT(gNodes[node].mType->mClass != NULL);
            if(type_identity(gNodes[node].mType, gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "unexpected error on if statment");
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_WHILE: {
            sCLNodeType* conditional_type;
            sNodeBlock* block;
            int conditional_label;
            unsigned int while_loop_out_label;
            unsigned int break_labels[CL_BREAK_MAX];
            unsigned int* break_labels_before;
            int break_labels_len;
            int* break_labels_len_before;
            unsigned int continue_labels[CL_BREAK_MAX];
            unsigned int* continue_labels_before;
            int continue_labels_len;
            int* continue_labels_len_before;

            int j;
            sCLNodeType* while_type_before;

            block = gNodeBlocks + gNodes[node].uValue.mWhileBlock;

            /// conditional ///
            conditional_label = info->code->mLen;                           // save label for goto

            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, type_, block->mLVTable)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);                           // jump to while block

            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to while block out
            while_loop_out_label = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

            /// while block ///
            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->sBlockInfo.while_type; // save the value
            info->sBlockInfo.while_type = gNodes[node].mType; // for NODE_TYPE_BREAK to get while type

            if(!compile_loop_block(block, type_, info)) {
                return FALSE;
            }

            info->sBlockInfo.while_type = while_type_before;                   // restore the value
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);

            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to conditional_label
            append_int_value_to_bytecodes(info->code, conditional_label, info->no_output_to_bytecodes);

            // determine the jump point 
            *(info->code->mCode + while_loop_out_label) = info->code->mLen;

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            ASSERT(gNodes[node].mType->mClass != NULL);
            if(type_identity(gNodes[node].mType, gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "unexpected error on while loop");
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_DO: {
            sCLNodeType* conditional_type;
            sNodeBlock* block;
            int loop_top_label;
            unsigned int break_labels[CL_BREAK_MAX];
            unsigned int* break_labels_before;
            int break_labels_len;
            int* break_labels_len_before;
            int j;
            sCLNodeType* while_type_before;
            unsigned int continue_labels[CL_BREAK_MAX];
            unsigned int* continue_labels_before;
            int continue_labels_len;
            int* continue_labels_len_before;

            /// do block ///
            loop_top_label = info->code->mLen;                              // save label for goto

            block = gNodeBlocks + gNodes[node].uValue.mDoBlock;

            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->sBlockInfo.while_type;                            // save the value
            info->sBlockInfo.while_type = gNodes[node].mType;                          // for NODE_TYPE_BREAK to get while type

            if(!compile_loop_block(block, type_, info)) {
                return FALSE;
            }

            info->sBlockInfo.while_type = while_type_before;                           // restore

            //// conditional ///
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);

            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, type_, block->mLVTable)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_NOTIF, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);               // jump to the loop out

            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to the top of loop
            append_int_value_to_bytecodes(info->code, loop_top_label, info->no_output_to_bytecodes);

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            ASSERT(gNodes[node].mType->mClass != NULL);

            if(type_identity(gNodes[node].mType, gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "unexpected error on while loop");
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_FOR: {
            sCLNodeType* expression_type;
            sNodeBlock* block;
            int conditional_label;
            unsigned int for_loop_out_label;
            unsigned int break_labels[CL_BREAK_MAX];
            unsigned int* break_labels_before;
            int break_labels_len;
            int* break_labels_len_before;
            int j;
            sCLNodeType* while_type_before;
            unsigned int continue_labels[CL_BREAK_MAX];
            unsigned int* continue_labels_before;
            int continue_labels_len;
            int* continue_labels_len_before;
            int stack_num_at_head;

            stack_num_at_head = *info->stack_num;

            block = gNodeBlocks + gNodes[node].uValue.mForBlock;

            /// initilize expression ///
            expression_type = NULL;
            if(!compile_expressiont_in_loop(gNodes[node].mLeft, &expression_type, class_params, num_params, info, block->mLVTable)) {
                return FALSE;
            }

            correct_stack_pointer_n(info->stack_num, stack_num_at_head, info->sname, info->sline, info->code, info->err_num, info->no_output_to_bytecodes);

            /// conditional ///
            conditional_label = info->code->mLen;

            if(!compile_conditional(gNodes[node].mRight, &expression_type, class_params, num_params, info, type_, block->mLVTable)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
            append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);               // jump to for block

            /// for block ///
            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);       // jump to for block out
            for_loop_out_label = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->sBlockInfo.while_type;                   // save the value
            info->sBlockInfo.while_type = gNodes[node].mType;                 // for NODE_TYPE_BREAK to get while type

            if(!compile_loop_block(block, type_, info)) {
                return FALSE;
            }

            info->sBlockInfo.while_type = while_type_before;                   // restore the value

            /// finalization expression ///
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);
            expression_type = NULL;

            if(!compile_expressiont_in_loop(gNodes[node].mMiddle, &expression_type, class_params, num_params, info, block->mLVTable)) {
                return FALSE;
            }

            correct_stack_pointer_n(info->stack_num, stack_num_at_head, info->sname, info->sline, info->code, info->err_num, info->no_output_to_bytecodes);

            append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to the conditional label
            append_int_value_to_bytecodes(info->code, conditional_label, info->no_output_to_bytecodes);

            // determine the jump point 
            *(info->code->mCode + for_loop_out_label) = info->code->mLen;

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            /// pop vars ///
            ASSERT(gNodes[node].mType->mClass != NULL);
            if(type_identity(gNodes[node].mType, gVoidType)) {
                //append_opecode_to_bytecodes(info->code, OP_POP_N);
                //append_int_value_to_bytecodes(info->code, block->mLVTable->mVarNum);

                *type_ = gVoidType;
                *info->stack_num = 0;

                *info->stack_num = stack_num_at_head;
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "unexpected error on for loop");
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_BLOCK_CALL: {
            sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;
            sCLNodeType* right_type;
            sCLClass* klass;
            sCLMethod* method;
            char* block_name;
            BOOL static_method;
            BOOL calling_block_before;

            if(info->real_caller_method == NULL) {
                parser_err_msg("can't call block because there are not the caller method.", info->sname, *info->sline);
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            static_method = info->real_caller_method->mFlags & CL_CLASS_METHOD;

            /// initilize class params ///
            num_params2 = 0;
            memset(class_params2, 0, sizeof(class_params2));

            /// params go ///
            calling_block_before = info->sParamInfo.calling_block;
            info->sParamInfo.calling_block = TRUE;
            right_type = NULL;
            if(!compile_right_node(node, &right_type, class_params2, &num_params2, info)) {
                return FALSE;
            }

            info->sParamInfo.calling_block = calling_block_before;

            /// call method ///
            if(info->real_caller_class == NULL || info->real_caller_class->mClass == NULL) {
                parser_err_msg("can't call block because there are not the caller class.", info->sname, *info->sline);
                (*info->err_num)++;
                *type_ = gIntType; // dummy
                break;
            }

            klass = info->real_caller_class->mClass;
            *type_ = gBlockType;
            method = info->real_caller_method;
            block_name = gNodes[node].uValue.sVarName.mVarName;

            if(!call_method_block(klass, type_, method, block_name, class_params2, &num_params2, info)) {
                return FALSE;
            }

            }
            break;

        /// operand ///
        case NODE_TYPE_OPERAND:
            switch((int)gNodes[node].uValue.sOperand.mOperand) {
            case kOpAdd: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IADD, OP_BADD, OP_SHADD, OP_UIADD, OP_LOADD, OP_FADD, OP_DADD, OP_SADD, OP_BSADD, -1, -1, -1,  "+", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, gStringType, gBytesType, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpSub: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ISUB, OP_BSUB, OP_SHSUB, OP_UISUB, OP_LOSUB, OP_FSUB, OP_DSUB, -1, -1, -1, -1, -1,  "-", gIntType, gByteType, gShortType, gUIntType, gLongType,  gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpMult: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }
                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IMULT, OP_BMULT, OP_SHMULT, OP_UIMULT, OP_LOMULT, OP_FMULT, OP_DMULT, -1, -1, -1, OP_SMULT, OP_BSMULT, "*", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, gStringType, gBytesType, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpDiv: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IDIV, OP_BDIV, OP_SHDIV, OP_UIDIV, OP_LODIV, OP_FDIV, OP_DDIV, -1, -1, -1, -1, -1,  "/", gIntType, gByteType, gShortType, gUIntType, gLongType, gFloatType, gDoubleType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpMod: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IMOD, OP_BMOD, OP_SHMOD, OP_UIMOD, OP_LOMOD, -1, -1, -1, -1, -1, -1, -1,  "%", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpLeftShift: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ILSHIFT, OP_BLSHIFT, OP_SHLSHIFT, OP_UILSHIFT, OP_LOLSHIFT, -1, -1, -1, -1, -1, -1, -1,  "<<", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) 
                {
                    return FALSE;
                }
                }
                break;

            case kOpRightShift: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IRSHIFT, OP_BRSHIFT, OP_SHRSHIFT, OP_UIRSHIFT, OP_LORSHIFT, -1, -1, -1, -1, -1, -1, -1,  ">>", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonGreater: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IGTR, OP_BGTR, OP_SHGTR, OP_UIGTR, OP_LOGTR, OP_FGTR, OP_DGTR, -1, -1, -1, -1, -1,  ">", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonGreaterEqual: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IGTR_EQ, OP_BGTR_EQ, OP_SHGTR_EQ, OP_UIGTR_EQ, OP_LOGTR_EQ, OP_FGTR_EQ, OP_DGTR_EQ, -1, -1, -1, -1, -1,  ">=", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonLesser: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ILESS, OP_BLESS, OP_SHLESS, OP_UILESS, OP_LOLESS, OP_FLESS, OP_DLESS, -1, -1, -1, -1, -1,  "<", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonLesserEqual: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ILESS_EQ, OP_BLESS_EQ, OP_SHLESS_EQ, OP_UILESS_EQ, OP_LOLESS_EQ, OP_FLESS_EQ, OP_DLESS_EQ, -1, -1, -1, -1, -1,  "<=", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonEqual: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IEQ, OP_BEQ, OP_SHEQ, OP_UIEQ, OP_LOEQ, OP_FEQ, OP_DEQ, OP_SEQ, OP_BSEQ, OP_BLEQ, -1, -1, "==", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonEqualTilda: {
                sCLNodeType* left_type;
                sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
                int num_params2;
                BOOL not_found_method;
                sCLMethod* calling_method;
                sCLClass* class_of_calling_method;
                unsigned int block_node;
                unsigned int block_id;
                char* method_name;

                /// compile self ///
                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) 
                {
                    return FALSE;
                }

                /// determine the calling method ///
                method_name = "=~";
                block_node = 0;
                block_id = 0;
                *type_ = left_type;

                if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
                {
                    return FALSE;
                }

                /// params go ///
                if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
                {
                    return FALSE;
                }

                if(left_type->mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    *type_ = left_type;
                    not_found_method = FALSE;
                    if(!call_method("=~", FALSE, type_, class_params2, &num_params2, info, 0, FALSE, &not_found_method, 0, NULL)) 
                    {
                        return FALSE;
                    }
                }
                }
                break;

            case kOpComparisonNotEqual: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_INOTEQ, OP_BNOTEQ, OP_SHNOTEQ, OP_UINOTEQ, OP_LONOTEQ, OP_FNOTEQ, OP_DNOTEQ, OP_SNOTEQ, OP_BSNOTEQ, OP_BLNOTEQ, -1, -1, "!=", gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, gBoolType, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpAnd: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IAND, OP_BAND, OP_SHAND, OP_UIAND, OP_LOAND, -1, -1, -1, -1, -1, -1, -1, "&", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpXor: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IXOR, OP_BXOR, OP_SHXOR, OP_UIXOR, OP_LOXOR, -1, -1, -1, -1, -1, -1, -1, "^", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpOr: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IOR, OP_BOR, OP_SHOR, OP_UIOR, OP_LOOR, -1, -1, -1, -1, -1, -1, -1, "|", gIntType, gByteType, gShortType, gUIntType, gLongType, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }
                }
                break;

            case kOpOrOr: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;
                int ivalue2;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                /// dupe the top of stack ///
                append_opecode_to_bytecodes(info->code, OP_DUP, info->no_output_to_bytecodes);
                inc_stack_num(info->stack_num, info->max_stack, 1);

                /// if left node is true, jump the end of right node
                append_opecode_to_bytecodes(info->code, OP_NOTIF, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);                   // jump to the right node

                dec_stack_num(info->stack_num, 1);

                /// block of if ///
                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to the end of right node
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, -1, -1, -1, -1, -1, -1, -1, -1, -1, OP_BLOROR, -1, -1, "||", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gBoolType, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }

                *(info->code->mCode + ivalue2) = info->code->mLen;

                }
                break;

            case kOpAndAnd: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;
                int ivalue2;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                /// dupe the top of stack ///
                append_opecode_to_bytecodes(info->code, OP_DUP, info->no_output_to_bytecodes);
                inc_stack_num(info->stack_num, info->max_stack, 1);

                /// if left node is false, jump the end of right node
                append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);                   // jump to the right node

                dec_stack_num(info->stack_num, 1);

                /// block of if ///
                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // jump to the end of right node
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, -1, -1, -1, -1, -1, -1, -1, -1, -1, OP_BLANDAND, -1, -1, "&&", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, gBoolType, NULL, NULL, gNodes[node].uValue.sOperand.mQuote)) {
                    return FALSE;
                }

                *(info->code->mCode + ivalue2) = info->code->mLen;

                }
                break;

            case kOpPlusPlus: 
            case kOpMinusMinus:
            case kOpPlusPlus2:
            case kOpMinusMinus2: {
                unsigned int left_node;

                ASSERT(gNodes[node].mLeft != 0);

                left_node = gNodes[node].mLeft;

                if(gNodes[left_node].mNodeType == NODE_TYPE_VARIABLE_NAME) {
                    char* name;
                    sVar* var;
                    sCLNodeType* left_type;

                    if(info->lv_table == NULL) {
                        parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
                        (*info->err_num)++;

                        *type_ = gIntType;
                        return TRUE;
                    }

                    name = gNodes[left_node].uValue.sVarName.mVarName;
                    var = get_variable_from_table(info->lv_table, name);

                    if(var == NULL) {
                        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }

                    *type_ = var->mType;

                    if(!increase_or_decrease_local_variable(name, var, node, type_, class_params, num_params, info))
                    {
                        return FALSE;
                    }
                }
                else if(gNodes[left_node].mNodeType == NODE_TYPE_FIELD) {
                    /// left_value ///
                    sCLNodeType* field_type;
                    char* field_name;

                    field_type = NULL;
                    if(!compile_left_node(left_node, &field_type, class_params, num_params, info)) {
                        return FALSE;
                    }

                    *type_ = field_type;
                    field_name = gNodes[left_node].uValue.sVarName.mVarName;

                    if(!increase_or_decrease_field(node, left_node, field_name, FALSE, type_, class_params, num_params, info))
                    {
                        return FALSE;
                    }
                }
                else if(gNodes[left_node].mNodeType == NODE_TYPE_CLASS_FIELD) {
                    char* field_name;
                    
                    field_name = gNodes[left_node].uValue.sVarName.mVarName;

                    *type_ = gNodes[left_node].mType;

                    if(!increase_or_decrease_field(node, left_node, field_name, TRUE, type_, class_params, num_params, info)) {
                        return FALSE;
                    }
                }
                else {
                    parser_err_msg("unexpected error on ++ or -- operator", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                }
                break;

            case kOpIndexing: {
                sCLNodeType* left_type;
                sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
                int num_params2;
                BOOL not_found_method;
                sCLMethod* calling_method;
                sCLClass* class_of_calling_method;
                unsigned int block_node;
                unsigned int block_id;
                char* method_name;

                /// compile self ///
                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) 
                {
                    return FALSE;
                }

                /// determine the calling method ///
                method_name = "[]";
                block_node = 0;
                block_id = 0;
                *type_ = left_type;

                if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
                {
                    return FALSE;
                }

                /// params go ///
                if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
                {
                    return FALSE;
                }

                if(left_type->mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    *type_ = left_type;
                    not_found_method = FALSE;
                    if(!call_method("[]", FALSE, type_, class_params2, &num_params2, info, 0, FALSE, &not_found_method, 0, NULL)) 
                    {
                        return FALSE;
                    }
                }
                }
                break;

            case kOpSubstitutionIndexing: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;
                sCLNodeType* middle_type;
                sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
                int num_params2;
                int left_node;
                BOOL not_found_method;
                sCLMethod* calling_method;
                sCLClass* class_of_calling_method;
                char* method_name;
                unsigned int block_node;
                unsigned int block_id;

                /// left node (self) ///
                left_node = gNodes[node].mLeft;

                ASSERT(gNodes[left_node].mNodeType == NODE_TYPE_VARIABLE_NAME || gNodes[left_node].mNodeType == NODE_TYPE_FIELD || gNodes[left_node].mNodeType == NODE_TYPE_CLASS_FIELD);

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                /// determine the calling method ///
                method_name = "[]=";
                block_node = 0;
                block_id = 0;
                *type_ = left_type;

                if(!determine_the_calling_method_before_compling_params(&class_of_calling_method, &calling_method, info, node, method_name, type_, block_id, block_node))
                {
                    return FALSE;
                }

                /// params go ///
                if(!compile_params_and_block(node, info, block_node, class_of_calling_method, calling_method, &num_params2, class_params2, *type_))
                {
                    return FALSE;
                }

                /// middle node go (a[x,y] = z; This z is middle node) ///
                middle_type = NULL;
                if(!compile_middle_node(node, &middle_type, class_params, num_params, info))
                {
                    return FALSE;
                }

                /// add middle node type to the arguments ///
                class_params2[num_params2++] = middle_type;
                if(num_params2 >= CL_METHOD_PARAM_MAX) {
                    parser_err_msg("overflow param number", info->sname, *info->sline);
                    return FALSE;
                }

                if(left_type->mClass == NULL || middle_type->mClass == NULL) 
                {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    *type_ = left_type;
                    not_found_method = FALSE;
                    if(!call_method("[]=", FALSE, type_, class_params2, &num_params2, info, 0, FALSE, &not_found_method,0, NULL )) 
                    {
                        return FALSE;
                    }
                }
                }
                break;

            case kOpComplement: {
                sCLNodeType* left_type;
                BOOL not_found_method;

                /// left node ///
                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(left_type->mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    if(gNodes[node].uValue.sOperand.mQuote) {
                        if(operand_posibility(left_type, gIntType)) {
                            append_opecode_to_bytecodes(info->code, OP_COMPLEMENT, info->no_output_to_bytecodes);

                            *type_ = gIntType;
                        }
                        else if(operand_posibility(left_type, gByteType)) {
                            append_opecode_to_bytecodes(info->code, OP_BCOMPLEMENT, info->no_output_to_bytecodes);

                            *type_ = gByteType;
                        }
                        else if(operand_posibility(left_type, gShortType)) {
                            append_opecode_to_bytecodes(info->code, OP_SHCOMPLEMENT, info->no_output_to_bytecodes);

                            *type_ = gShortType;
                        }
                        else if(operand_posibility(left_type, gUIntType)) {
                            append_opecode_to_bytecodes(info->code, OP_UICOMPLEMENT, info->no_output_to_bytecodes);

                            *type_ = gUIntType;
                        }
                        else if(operand_posibility(left_type, gLongType)) {
                            append_opecode_to_bytecodes(info->code, OP_LOCOMPLEMENT, info->no_output_to_bytecodes);

                            *type_ = gLongType;
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "There is not quote operator of this(~)\n");
                            (*info->err_num)++;

                            *type_ = gIntType; // dummy
                            break;
                        }
                    }
                    else {
                        sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
                        int num_params2;

                        memset(class_params2, 0, sizeof(class_params2));
                        num_params2 = 0;

                        *type_ = left_type;

                        if(!call_method("~", FALSE, type_, class_params2, &num_params2, info, 0, TRUE, &not_found_method, 0, NULL)) 
                        {
                            return FALSE;
                        }
                    }

                }
                }
                break;
 
            case kOpLogicalDenial: {
                sCLNodeType* left_type;

                /// left node ///
                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(left_type->mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    if(gNodes[node].uValue.sOperand.mQuote) {
                        if(operand_posibility(left_type, gBoolType)) {
                            append_opecode_to_bytecodes(info->code, OP_LOGICAL_DENIAL, info->no_output_to_bytecodes);

                            *type_ = gBoolType;
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "There is not quote operator of this(!)\n");
                            (*info->err_num)++;

                            *type_ = gIntType; // dummy
                            break;
                        }
                    }
                    else {
                        sCLNodeType* class_params2[CL_METHOD_PARAM_MAX];
                        int num_params2;
                        BOOL not_found_method;

                        memset(class_params2, 0, sizeof(class_params2));
                        num_params2 = 0;

                        *type_ = left_type;

                        if(!call_method("!", FALSE, type_, class_params2, &num_params2, info, 0, TRUE, &not_found_method, 0, NULL)) 
                        {
                            return FALSE;
                        }
                    }
                }
                }
                break;

            case kOpConditional:{
                int j;
                sCLNodeType* conditional_type;
                sCLNodeType* true_value_type;
                sCLNodeType* false_value_type;
                int else_label;
                int block_out_label;
                int true_stack_num;
                int false_stack_num;
                int* stack_num_before;

                /// conditional ///
                if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, type_, NULL)) {
                    return FALSE;
                }

                append_opecode_to_bytecodes(info->code, OP_IF, info->no_output_to_bytecodes);
                append_int_value_to_bytecodes(info->code, 2, info->no_output_to_bytecodes);

                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // goto else label
                else_label = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                /// true value ///
                true_stack_num = 0;
                stack_num_before = info->stack_num;
                info->stack_num = &true_stack_num;
                true_value_type = NULL;
                if(!compile_middle_node(node, &true_value_type, class_params, num_params, info)) {
                    return FALSE;
                }
                info->stack_num = stack_num_before;

                append_opecode_to_bytecodes(info->code, OP_GOTO, info->no_output_to_bytecodes);    // goto end of ? operator
                block_out_label = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0, info->no_output_to_bytecodes);

                /// false value ///
                *(info->code->mCode + else_label) = info->code->mLen;

                false_stack_num = 0;
                stack_num_before = info->stack_num;
                info->stack_num = &false_stack_num;
                false_value_type = NULL;

                if(!compile_right_node(node, &false_value_type, class_params, num_params, info)) {
                    return FALSE;
                }
                info->stack_num = stack_num_before;

                *(info->code->mCode + block_out_label) = info->code->mLen;

                if(type_identity(true_value_type, false_value_type)) {
                    *type_ = true_value_type;

                    if(true_stack_num != 1 || false_stack_num != 1) {
                        parser_err_msg_format(info->sname, *info->sline, "stack num error. true_stack_num is %d. false_stack_num is %d", true_stack_num, false_stack_num);
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        break;
                    }

                    *(info->stack_num) = 1;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "type error.");
                    parser_err_msg_without_line("true expression type is ");
                    show_node_type_for_errmsg(true_value_type);
                    parser_err_msg_without_line(". false expression type is ");
                    show_node_type_for_errmsg(false_value_type);
                    parser_err_msg_without_line("\n");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }
                }
                break;

            case kOpComma: {
                sCLNodeType* left_type;
                sCLNodeType* right_type;

                left_type = NULL;
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                right_type = NULL;
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                *type_ = right_type;
                }
                break;
            }
            break;
    }

    return TRUE;
}

