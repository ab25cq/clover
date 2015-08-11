#include "clover.h"
#include "common.h"
#include <ctype.h>

void cl_compiler_init()
{
    load_class_init();

    init_vtable();
    init_node_types();
    module_init();
}

void cl_compiler_final()
{
    module_final();
    free_node_types();
    final_vtable();

    load_class_final();
}

void correct_stack_pointer(int* stack_num, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes)
{
    if(*stack_num < 0) {
        parser_err_msg("unexpected error. Stack pointer is invalid", sname, *sline);
        (*err_num)++;
    }
    else if(*stack_num == 1) {
        append_opecode_to_bytecodes(code, OP_POP, no_output_to_bytecodes);
    }
    else if(*stack_num > 0) {
        append_opecode_to_bytecodes(code, OP_POP_N, no_output_to_bytecodes);
        append_int_value_to_bytecodes(code, *stack_num, no_output_to_bytecodes);
    }

    *stack_num = 0;
}

void correct_stack_pointer_with_output_result(int* stack_num, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes)
{
    if(*stack_num < 0) {
        parser_err_msg("unexpected error. Stack pointer is invalid", sname, *sline);
        (*err_num)++;
    }
    else if(*stack_num == 1) {
        append_opecode_to_bytecodes(code, OP_OUTPUT_RESULT, no_output_to_bytecodes);
        //append_opecode_to_bytecodes(code, OP_POP, no_output_to_bytecodes);
    }
    else if(*stack_num > 0) {
        append_opecode_to_bytecodes(code, OP_OUTPUT_RESULT, no_output_to_bytecodes);
        if(*stack_num-1 > 0) {
            append_opecode_to_bytecodes(code, OP_POP_N, no_output_to_bytecodes);
            append_int_value_to_bytecodes(code, (*stack_num-1), no_output_to_bytecodes);
        }
    }

    *stack_num = 0;
}

void correct_stack_pointer_n(int* stack_num, int n, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes)
{
    int difference;

    difference = *stack_num - n;

    if(difference < 0) {
        parser_err_msg("unexpected error. Stack pointer is invalid", sname, *sline);
        (*err_num)++;
    }
    else if(difference == 1) {
        append_opecode_to_bytecodes(code, OP_POP, no_output_to_bytecodes);
    }
    else if(difference > 0) {
        append_opecode_to_bytecodes(code, OP_POP_N, no_output_to_bytecodes);
        append_int_value_to_bytecodes(code, difference, no_output_to_bytecodes);
    }

    *stack_num = n;
}

void make_block_result(sCLNodeType** result_type)
{
    sCLNodeType* breakable_result_type;
    sCLClass* tuple_class;

    breakable_result_type = alloc_node_type();

    tuple_class = cl_get_class("Tuple$2");

    ASSERT(tuple_class != NULL);

    breakable_result_type->mClass = tuple_class;
    breakable_result_type->mGenericsTypesNum = 2;
    breakable_result_type->mGenericsTypes[0] = gBoolType;
    breakable_result_type->mGenericsTypes[1] = *result_type;

    *result_type = breakable_result_type;
}

//#define STACK_DEBUG

/// if, for, while, do, (type) block
BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table)
{
    sNode statment_end_node;

    *block_id = alloc_node_block(block_type);

    while(1) {
        int saved_err_num;
        sNode node;
        sParserInfo info;

        memset(&info, 0, sizeof(info));

        skip_spaces_and_lf(p, sline);

        if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
            break;
        }

        saved_err_num = *err_num;
        node.mNode = 0;
        node.mSName = sname;
        node.mSLine = *sline;

        info.p = p;
        info.sname = sname;
        info.sline = sline;
        info.err_num = err_num;
        info.current_namespace = current_namespace;
        info.klass = klass;
        info.method = method;

        if(!node_expression(&node.mNode, &info, lv_table)) 
        {
            return FALSE;
        }

        if(node.mNode != 0 && *err_num == saved_err_num) {
            append_node_to_node_block(*block_id, &node);
        }

        if(**p == ';') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            statment_end_node.mNode = 0;
            statment_end_node.mSName = sname;
            statment_end_node.mSLine = *sline;

            append_node_to_node_block(*block_id, &statment_end_node);

            if(**p == '}') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == 0) {
                break;
            }
        }
        else if(**p == 0) {
            statment_end_node.mNode = 0;
            statment_end_node.mSName = sname;
            statment_end_node.mSLine = *sline;

            append_node_to_node_block(*block_id, &statment_end_node);
            break;
        }
        else {
            if(node.mNode == 0) {
                parser_err_msg_format(sname, *sline, "require ; character. unexpected character'%c' --> character code %d", **p, **p);
                (*err_num)++;
                (*p)++;
            }
            else {
                statment_end_node.mNode = 0;
                statment_end_node.mSName = sname;
                statment_end_node.mSLine = *sline;

                append_node_to_node_block(*block_id, &statment_end_node);
            }
        }
    }

    return TRUE;
}

//  try, method with block
BOOL parse_block_object(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table, int sline_top, int num_params, sCLNodeType** class_params)
{
    sNode statment_end_node;

    *block_id = alloc_node_block(block_type);

    while(1) {
        int saved_err_num;
        sNode node;
        sParserInfo info;

        memset(&info, 0, sizeof(info));

        skip_spaces_and_lf(p, sline);

        if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
            break;
        }

        saved_err_num = *err_num;
        node.mNode = 0;
        node.mSName = sname;
        node.mSLine = *sline;

        info.p = p;
        info.sname = sname;
        info.sline = sline;
        info.err_num = err_num;
        info.current_namespace = current_namespace;
        info.klass = klass;
        info.method = method;

        if(!node_expression(&node.mNode, &info, lv_table)) {
            return FALSE;
        }

        if(node.mNode != 0 && *err_num == saved_err_num) {
            append_node_to_node_block(*block_id, &node);
        }

        if(**p == ';') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            statment_end_node.mNode = 0;
            statment_end_node.mSName = sname;
            statment_end_node.mSLine = *sline;

            append_node_to_node_block(*block_id, &statment_end_node);

            if(**p == '}') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == 0) {
                break;
            }
        }
        else if(**p == 0) {
            statment_end_node.mNode = 0;
            statment_end_node.mSName = sname;
            statment_end_node.mSLine = *sline;

            append_node_to_node_block(*block_id, &statment_end_node);
            break;
        }
        else {
            if(node.mNode == 0) {
                parser_err_msg_format(sname, *sline, "require ; character. unexpected character'%c' --> character code %d", **p, **p);
                (*p)++;
                (*err_num)++;
            }
            else {
                statment_end_node.mNode = 0;
                statment_end_node.mSName = sname;
                statment_end_node.mSLine = *sline;

                append_node_to_node_block(*block_id, &statment_end_node);
            }
        }
    }


    gNodeBlocks[*block_id].mLVTable = lv_table;
    gNodeBlocks[*block_id].mNumLocals = lv_table->mVarNum + lv_table->mMaxBlockVarNum;
    gNodeBlocks[*block_id].mNumParams = num_params;
    if(num_params > 0) {
        int i;
        for(i=0; i<num_params; i++) {
            gNodeBlocks[*block_id].mClassParams[i] = class_params[i];
        }
    }
    else {
        memset(gNodeBlocks[*block_id].mClassParams, 0, sizeof(sCLNodeType*)*CL_METHOD_PARAM_MAX);
    }

    return TRUE;
}

BOOL compile_statments_for_interpreter(int nodes[], int stack_nums[], int sline_tops[], int* num_nodes, int* max_stack, char** p, char* sname, int* sline, int* err_num, sCLNodeType** type_, char* current_namespace, sVarTable* var_table)
{
    BOOL exist_return;
    BOOL exist_break;
    sByteCode dummy_code;
    sConst dummy_constant;
    int i;

    sByteCode_init(&dummy_code);
    sConst_init(&dummy_constant);

    init_nodes();

    exist_return = FALSE;
    exist_break = FALSE;
    *max_stack = 0;
    *num_nodes = 0;

    /// parse statments ///
    while(1) {
        unsigned int node;
        int stack_num;

        int saved_err_num;
        int sline_top;

        sParserInfo info;

        memset(&info, 0, sizeof(info));

        skip_spaces_and_lf(p, sline);

        stack_num = 0;
        sline_top = *sline;
        saved_err_num = *err_num;
        node = 0;

        info.p = p;
        info.sname = sname;
        info.sline = sline;
        info.err_num = err_num;
        info.current_namespace = current_namespace;
        info.method = NULL;
        info.klass = NULL;

        if(!node_expression(&node, &info, var_table)) {
            free_nodes();
            sByteCode_free(&dummy_code);
            sConst_free(&dummy_constant);
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            nodes[*num_nodes] = node;
            stack_nums[*num_nodes] = stack_num;
            sline_tops[*num_nodes] = sline_top;
            
            (*num_nodes)++;

            if(*num_nodes >= SCRIPT_STATMENT_MAX) {
                free_nodes();
                sByteCode_free(&dummy_code);
                sConst_free(&dummy_constant);
                parser_err_msg_format(sname, *sline, "overflow statment max in a script file");
                return FALSE;
            }
        }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif

        if(**p == ';') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            if(**p == 0) {
                break;
            }
        }
        else if(**p == 0) {
            break;
        }
        else {
            if(node == 0) {
                parser_err_msg_format(sname, sline_top, "require ; character. unexpected character '%c' --> character code %d", **p, **p);
                (*p)++;
                (*err_num)++;
            }
        }
    }

    /// for get type_ to compile nodes ///
    for(i=0; i<*num_nodes; i++) {
        sCompileInfo info;

        *type_ = NULL;

        memset(&info, 0, sizeof(sCompileInfo));

        info.sname = sname;
        info.sline = &sline_tops[i];
        info.caller_class = NULL;
        info.caller_method = NULL;
        info.real_caller_class = NULL;
        info.real_caller_method = NULL;
        info.sBlockInfo.method_block = NULL;
        info.code = &dummy_code;
        info.constant = &dummy_constant;
        info.err_num = err_num;
        info.lv_table = var_table;
        info.stack_num = &stack_nums[i];
        info.max_stack = max_stack;
        info.exist_return = &exist_return;
        info.exist_break = &exist_break;
        info.sLoopInfo.break_labels = NULL;
        info.sLoopInfo.break_labels_len = NULL;
        info.sBlockInfo.while_type = 0;
        info.sLoopInfo.continue_labels = NULL;
        info.sLoopInfo.continue_labels_len = NULL;
        info.sBlockInfo.block_kind = kBKNone;
        info.sBlockInfo.in_try_block = FALSE;
        info.no_output_to_bytecodes = FALSE;
        info.sParamInfo.calling_method = NULL;
        info.sParamInfo.class_of_calling_method = NULL;
        info.sParamInfo.calling_block = FALSE;
        info.mNestOfMethodFromDefinitionPoint = 0;

        if(!compile_node(nodes[i], type_, NULL, 0, &info)) {
            free_nodes();
            sByteCode_free(&dummy_code);
            sConst_free(&dummy_constant);
            return FALSE;
        }

        correct_stack_pointer(&stack_nums[i], sname, sline, &dummy_code, err_num, FALSE);
    }

    free_nodes();
    sByteCode_free(&dummy_code);
    sConst_free(&dummy_constant);

    return TRUE;
}

BOOL compile_statments(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table, BOOL output_result)
{
    BOOL exist_return;
    BOOL exist_break;

    int nodes[SCRIPT_STATMENT_MAX];
    int stack_nums[SCRIPT_STATMENT_MAX];
    int sline_tops[SCRIPT_STATMENT_MAX];
    int num_nodes;
    int i;

    init_nodes();

    *max_stack = 0;
    exist_return = FALSE;
    exist_break = FALSE;
    num_nodes = 0;

    /// parse statments ///
    while(1) {
        unsigned int node;
        int stack_num;

        int saved_err_num;
        int sline_top;

        sParserInfo info;

        memset(&info, 0, sizeof(info));

        skip_spaces_and_lf(p, sline);

        stack_num = 0;
        sline_top = *sline;
        saved_err_num = *err_num;
        node = 0;

        info.p = p;
        info.sname = sname;
        info.sline = sline;
        info.err_num = err_num;
        info.current_namespace = current_namespace;
        info.method = NULL;
        info.klass = NULL;

        if(!node_expression(&node, &info, var_table)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            nodes[num_nodes] = node;
            stack_nums[num_nodes] = stack_num;
            sline_tops[num_nodes] = sline_top;
            
            num_nodes++;

            if(num_nodes >= SCRIPT_STATMENT_MAX) {
                parser_err_msg_format(sname, *sline, "overflow statment max in a script file");
                free_nodes();
                return FALSE;
            }
        }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif

        if(**p == ';') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            if(**p == 0) {
                break;
            }
        }
        else if(**p == 0) {
            break;
        }
        else {
            if(node == 0) {
                parser_err_msg_format(sname, sline_top, "require ; character. unexpected character '%c' --> character code %d", **p, **p);
                (*p)++;
                (*err_num)++;
            }
        }
    }

    /// compile nodes ///
    for(i=0; i<num_nodes; i++) {
        sCLNodeType* type_;
        sCompileInfo info;

        type_ = NULL;

        memset(&info, 0, sizeof(sCompileInfo));

        info.sname = sname;
        info.sline = &sline_tops[i];
        info.caller_class = NULL;
        info.caller_method = NULL;
        info.real_caller_class = NULL;
        info.real_caller_method = NULL;
        info.sBlockInfo.method_block = NULL;
        info.code = code;
        info.constant = constant;
        info.err_num = err_num;
        info.lv_table = var_table;
        info.stack_num = &stack_nums[i];
        info.max_stack = max_stack;
        info.exist_return = &exist_return;
        info.exist_break = &exist_break;
        info.sLoopInfo.break_labels = NULL;
        info.sLoopInfo.break_labels_len = NULL;
        info.sBlockInfo.while_type = 0;
        info.sLoopInfo.continue_labels = NULL;
        info.sLoopInfo.continue_labels_len = NULL;
        info.sBlockInfo.block_kind = kBKNone;
        info.sBlockInfo.in_try_block = FALSE;
        info.no_output_to_bytecodes = FALSE;
        info.sParamInfo.calling_method = NULL;
        info.sParamInfo.class_of_calling_method = NULL;
        info.sParamInfo.calling_block = FALSE;
        info.mNestOfMethodFromDefinitionPoint = 0;

        if(!compile_node(nodes[i], &type_, NULL, 0, &info)) {
            free_nodes();
            return FALSE;
        }

        if(output_result) {
            correct_stack_pointer_with_output_result(&stack_nums[i], sname, sline, code, err_num, FALSE);
        }
        else {
            correct_stack_pointer(&stack_nums[i], sname, sline, code, err_num, FALSE);
        }
    }

    free_nodes();

    return TRUE;
}

BOOL compile_method(sCLMethod* method, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace)
{
    int max_stack;
    sCLNodeType* result_type;
    BOOL exist_return;
    BOOL exist_break;
    int sline_top_of_method;

    int nodes[SCRIPT_STATMENT_MAX];
    int stack_nums[SCRIPT_STATMENT_MAX];
    int sline_tops[SCRIPT_STATMENT_MAX];
    int num_nodes;
    int i;

    sline_top_of_method = *sline;

    num_nodes = 0;

    (*p)++;
    skip_spaces_and_lf(p, sline);

    alloc_bytecode_of_method(method);

    init_nodes();

    max_stack = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    /// parse souce ///
    while(1) {
        int stack_num;
        unsigned int node;

        int saved_err_num;
        int sline_top;

        sParserInfo info;

        memset(&info, 0, sizeof(info));

        skip_spaces_and_lf(p, sline);

        if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
            break;
        }
        
        stack_num = 0;
        saved_err_num = *err_num;
        node = 0;

        sline_top = *sline;

        info.p = p;;
        info.sname = sname;
        info.sline = sline;
        info.err_num = err_num;
        info.current_namespace = current_namespace;
        info.klass = klass;
        info.method = method;

        if(!node_expression(&node, &info, lv_table)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            nodes[num_nodes] = node;
            stack_nums[num_nodes] = stack_num;
            sline_tops[num_nodes] = sline_top;

            num_nodes++;

            if(num_nodes >= SCRIPT_STATMENT_MAX) {
                parser_err_msg_format(sname, *sline, "overflow statment max in a method");
                free_nodes();
                return FALSE;
            }
        }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif
        if(**p == ';') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            if(**p == '}') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == 0) {
                break;
            }
        }
        else if(**p == 0) {
            break;
        }
        else {
            if(node == 0) {
                parser_err_msg_format(sname, *sline, "require ; character. unexpected character '%c' --> character code %d", **p, **p);
                (*p)++;
                (*err_num)++;
            }
        }
    }

//    append_opecode_to_bytecodes(&method->uCode.mByteCodes, OP_LDTYPE_CONTEXT, FALSE);

    /// compile nodes ///
    for(i=0; i<num_nodes; i++) {
        sCLNodeType* type_;
        sCompileInfo info;

        type_ = NULL;
        memset(&info, 0, sizeof(sCompileInfo));

        info.sname = sname;
        info.sline = &sline_tops[i];
        info.caller_class = klass;
        info.caller_method = method;
        info.real_caller_class = klass;
        info.real_caller_method = method;
        info.sBlockInfo.method_block = NULL;
        info.code = &method->uCode.mByteCodes;
        info.constant = &klass->mClass->mConstPool;
        info.err_num = err_num;
        info.lv_table = lv_table;
        info.stack_num = &stack_nums[i];
        info.max_stack = &max_stack;
        info.exist_return = &exist_return;
        info.exist_break = &exist_break;
        info.sLoopInfo.break_labels = NULL;
        info.sLoopInfo.break_labels_len = NULL;
        info.sBlockInfo.while_type = 0;
        info.sLoopInfo.continue_labels = NULL;
        info.sLoopInfo.continue_labels_len = NULL;
        info.sBlockInfo.block_kind = kBKNone;
        info.sBlockInfo.in_try_block = FALSE;
        info.no_output_to_bytecodes = FALSE;
        info.sParamInfo.calling_method = NULL;
        info.sParamInfo.class_of_calling_method = NULL;
        info.sParamInfo.calling_block = FALSE;
        info.mNestOfMethodFromDefinitionPoint = 0;

        if(!compile_node(nodes[i], &type_, NULL, 0, &info)) {
            free_nodes();
            return FALSE;
        }

        correct_stack_pointer(&stack_nums[i], sname, sline, &method->uCode.mByteCodes, err_num, FALSE);
    }

    /// add "return self" to the constructor ///
    result_type = ALLOC get_result_type_of_method(klass, method);

    if(constructor) {
        append_opecode_to_bytecodes(&method->uCode.mByteCodes, OP_OLOAD, FALSE);
        append_int_value_to_bytecodes(&method->uCode.mByteCodes, 0, FALSE);
    }
    /// add "return null" to the void result type ///
    else if(substitution_posibility(result_type, gVoidType)) {
        append_opecode_to_bytecodes(&method->uCode.mByteCodes, OP_LDCNULL, FALSE);
    }

    if(!substitution_posibility(result_type, gVoidType) && !exist_return && !(method->mFlags & CL_CONSTRUCTOR)) {
        parser_err_msg("require return sentence", sname, sline_top_of_method);
        (*err_num)++;
        free_nodes();
        return TRUE;
    }

    method->mMaxStack = max_stack;
    method->mNumLocals = lv_table->mVarNum + lv_table->mMaxBlockVarNum;

    free_nodes();

    return TRUE;
}

// if block or normal block
BOOL compile_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info)
{
    int i;
    int stack_num;
    int max_stack;

    max_stack = 0;
    stack_num = 0;

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;
        int sline_top;

        node = block->mNodes + i;

        sline_top = node->mSLine;

        if(node->mNode != 0) {
            memset(&info2, 0, sizeof(info2));

            info2.caller_class = info->caller_class;
            info2.caller_method = info->caller_method;
            info2.real_caller_class = info->real_caller_class;
            info2.real_caller_method = info->real_caller_method;
            info2.sBlockInfo.method_block = info->sBlockInfo.method_block;
            info2.code = info->code;
            info2.constant = info->constant;
            info2.sname = node->mSName;
            info2.sline = &sline_top;
            info2.err_num = info->err_num;
            info2.lv_table = block->mLVTable;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = info->exist_return;
            info2.exist_break = info->exist_break;
            info2.sLoopInfo.break_labels = info->sLoopInfo.break_labels;          // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.break_labels_len = info->sLoopInfo.break_labels_len;  // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.continue_labels = info->sLoopInfo.continue_labels;      // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.continue_labels_len = info->sLoopInfo.continue_labels_len; // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.sBlockInfo.while_type = info->sBlockInfo.while_type;
            info2.sBlockInfo.block_kind = info->sBlockInfo.block_kind;
            info2.sBlockInfo.in_try_block = info->sBlockInfo.in_try_block;
            info2.no_output_to_bytecodes = info->no_output_to_bytecodes;
            info2.sParamInfo.calling_method = NULL;
            info2.sParamInfo.class_of_calling_method = NULL;
            info2.sParamInfo.calling_block = FALSE;
            info2.mNestOfMethodFromDefinitionPoint = info->mNestOfMethodFromDefinitionPoint;

            if(!compile_node(node->mNode, type_, NULL, 0, &info2)) {
                return FALSE;
            }
        }
        else {
            if(i == block->mLenNodes -1) {              // last one
                if(block->mBlockType->mClass == NULL || type_identity(block->mBlockType, gVoidType)) 
                {
                    correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, info->code, info->err_num, info->no_output_to_bytecodes);
                }
                else {
                    if(stack_num != 1) {
                        parser_err_msg_format(node->mSName, node->mSLine, "require one return value of this block");
                        (*info->err_num)++;

                        *type_ = gIntType;   // dummy
                        return TRUE;
                    }
                    if(!substitution_posibility(block->mBlockType, *type_)) {
                        parser_err_msg_format(node->mSName, node->mSLine, "type error.");
                        parser_err_msg_without_line("left type is ");
                        show_node_type_for_errmsg(block->mBlockType);
                        parser_err_msg_without_line(". right type is ");
                        show_node_type_for_errmsg(*type_);
                        parser_err_msg_without_line("\n");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                }
            }
            else {
                correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, info->code, info->err_num, info->no_output_to_bytecodes);
            }
        }
    }

    return TRUE;
}

// while, do, for block
BOOL compile_loop_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info)
{
    int i;
    int stack_num;
    int max_stack;
    BOOL exist_break;

    max_stack = 0;
    stack_num = 0;
    exist_break = FALSE;

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;
        int sline_top;

        node = block->mNodes + i;

        sline_top = node->mSLine;

        if(node->mNode != 0) {
            memset(&info2, 0, sizeof(info2));

            info2.caller_class = info->caller_class;
            info2.caller_method = info->caller_method;
            info2.real_caller_class = info->real_caller_class;
            info2.real_caller_method = info->real_caller_method;
            info2.sBlockInfo.method_block = info->sBlockInfo.method_block;
            info2.code = info->code;
            info2.constant = info->constant;
            info2.sname = node->mSName;
            info2.sline = &sline_top;
            info2.err_num = info->err_num;
            info2.lv_table = block->mLVTable;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = info->exist_return;
            info2.exist_break = &exist_break;
            info2.sLoopInfo.break_labels = info->sLoopInfo.break_labels;          // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.break_labels_len = info->sLoopInfo.break_labels_len;  // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.continue_labels = info->sLoopInfo.continue_labels;      // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.sLoopInfo.continue_labels_len = info->sLoopInfo.continue_labels_len; // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.sBlockInfo.while_type = info->sBlockInfo.while_type;
            info2.sBlockInfo.block_kind = kBKWhileDoForBlock;
            info2.sBlockInfo.in_try_block = info->sBlockInfo.in_try_block;
            info2.no_output_to_bytecodes = info->no_output_to_bytecodes;
            info2.sParamInfo.calling_method = NULL;
            info2.sParamInfo.class_of_calling_method = NULL;
            info2.sParamInfo.calling_block = FALSE;
            info2.mNestOfMethodFromDefinitionPoint = info->mNestOfMethodFromDefinitionPoint;

            if(!compile_node(node->mNode, type_, NULL, 0, &info2)) {
                return FALSE;
            }
        }
        else {
            correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, info->code, info->err_num, info->no_output_to_bytecodes);
        }
    }

    if(!substitution_posibility(block->mBlockType, gVoidType) && !exist_break) {
        parser_err_msg("require break sentence", info->sname, *info->sline);
        (*info->err_num)++;
    }

    return TRUE;
}

// try block or method block
BOOL compile_block_object(sNodeBlock* block, sConst* constant, sByteCode* code, sCLNodeType** type_, sCompileInfo* info, sCLNodeType* caller_class, sCLMethod* caller_method, enum eBlockKind block_kind)
{
    int i;
    int stack_num;
    int max_stack;
    BOOL exist_return;

    max_stack = 0;
    stack_num = 0;
    exist_return = FALSE;

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;
        int sline_top;

        node = block->mNodes + i;
        sline_top = node->mSLine;

        if(node->mNode != 0) {
            memset(&info2, 0, sizeof(info2));

            info2.caller_class = caller_class;
            info2.caller_method = caller_method;
            info2.real_caller_class = info->real_caller_class;
            info2.real_caller_method = info->real_caller_method;
            info2.sBlockInfo.method_block = block;
            info2.code = code;
            info2.constant = constant;
            info2.sname = node->mSName;
            info2.sline = &sline_top;
            info2.err_num = info->err_num;
            info2.lv_table = block->mLVTable;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = &exist_return;
            info2.exist_break = info->exist_break;
            info2.sLoopInfo.break_labels = NULL;
            info2.sLoopInfo.break_labels_len = NULL;
            info2.sLoopInfo.continue_labels = NULL;
            info2.sLoopInfo.continue_labels_len = NULL;
            info2.sBlockInfo.while_type = 0;
            info2.sBlockInfo.block_kind = block_kind;
            info2.sBlockInfo.in_try_block = info->sBlockInfo.in_try_block;
            info2.no_output_to_bytecodes = info->no_output_to_bytecodes;
            info2.sParamInfo.calling_method = NULL;
            info2.sParamInfo.class_of_calling_method = NULL;
            info2.sParamInfo.calling_block = FALSE;
            info2.mNestOfMethodFromDefinitionPoint = info->mNestOfMethodFromDefinitionPoint;

            if(!compile_node(node->mNode, type_, NULL, 0, &info2)) {
                return FALSE;
            }
        }
        else {
            correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, code, info->err_num, info->no_output_to_bytecodes);
        }
    }

    block->mMaxStack = max_stack;

    if(!substitution_posibility(block->mBlockType, gVoidType) && !substitution_posibility(block->mBlockType, gBoolType) && !exist_return) {
        parser_err_msg("require return sentence", info->sname, *info->sline);
        (*info->err_num)++;
    }

    return TRUE;
}

static BOOL compile_node_for_getting_result_type_of_method_block(sNodeBlock* method_block, unsigned int node, sCompileInfo* info)
{
    if(gNodes[node].mNodeType == NODE_TYPE_RETURN) {
        sCLNodeType* left_type;

        if(type_identity(method_block->mBlockType, gVoidType) || type_identity(method_block->mBlockType, gBoolType)) 
        {
            if(gNodes[node].mLeft) {
                left_type = NULL;
                if(!compile_left_node(node, &left_type, NULL, 0, info)) {
                    return FALSE;
                }

                make_block_result(&left_type);

                /// determine the block result type from the left node ///
                method_block->mBlockType = left_type;
            }
        }
    }

    return TRUE;
}

BOOL get_result_type_of_method_block(sNodeBlock* block, sCompileInfo* info, enum eBlockKind block_kind)
{
    int i;
    int stack_num;
    int max_stack;
    BOOL exist_return;
    sConst dummy_constant;
    sByteCode dummy_code;

    max_stack = 0;
    stack_num = 0;
    exist_return = FALSE;

    sByteCode_init(&dummy_code);
    sConst_init(&dummy_constant);

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;
        int sline_top;

        node = block->mNodes + i;
        sline_top = node->mSLine;

        if(node->mNode != 0) {
            int dummy_err_num;
            memset(&info2, 0, sizeof(info2));

            dummy_err_num = 0;

            info2.caller_class = NULL;
            info2.caller_method = NULL;
            info2.real_caller_class = NULL;
            info2.real_caller_method = NULL;
            //info2.real_caller_class = info->real_caller_class;
            //info2.real_caller_method = info->real_caller_method;
            info2.sBlockInfo.method_block = NULL; //block;
            info2.code = &dummy_code;
            info2.constant = &dummy_constant;
            info2.sname = node->mSName;
            info2.sline = &sline_top;
            info2.err_num = &dummy_err_num;
            info2.lv_table = block->mLVTable;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = &exist_return;
            info2.exist_break = NULL;
            //info2.exist_break = info->exist_break;
            info2.sLoopInfo.break_labels = NULL;
            info2.sLoopInfo.break_labels_len = NULL;
            info2.sLoopInfo.continue_labels = NULL;
            info2.sLoopInfo.continue_labels_len = NULL;
            info2.sBlockInfo.while_type = 0;
            info2.sBlockInfo.block_kind = block_kind;
            info2.sBlockInfo.in_try_block = info->sBlockInfo.in_try_block;
            info2.no_output_to_bytecodes = info->no_output_to_bytecodes;
            info2.sParamInfo.calling_method = NULL;
            info2.sParamInfo.class_of_calling_method = NULL;
            info2.sParamInfo.calling_block = FALSE;
            info2.mNestOfMethodFromDefinitionPoint = info->mNestOfMethodFromDefinitionPoint;

            if(!compile_node_for_getting_result_type_of_method_block(block, node->mNode, &info2)) 
            {
                sByteCode_free(&dummy_code);
                sConst_free(&dummy_constant);
                return FALSE;
            }
        }
    }

    sByteCode_free(&dummy_code);
    sConst_free(&dummy_constant);

    return TRUE;
}

BOOL compile_field_initializer(sByteCode* initializer, ALLOC sCLNodeType** initializer_code_type, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sVarTable* lv_table, int* max_stack)
{
    unsigned int node;
    int saved_err_num;
    int sline_top;
    int stack_num;
    BOOL exist_return;
    BOOL exist_break;
    sParserInfo info;

    memset(&info, 0, sizeof(info));

    init_nodes();

    skip_spaces_and_lf(p, sline);

    node = 0;
    saved_err_num = *err_num;
    sline_top = *sline;
    stack_num = 0;
    *max_stack = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    *initializer_code_type = gIntType;

    info.p = p;
    info.sname = sname;
    info.sline = sline;
    info.err_num = err_num;
    info.current_namespace = current_namespace;
    info.klass = klass;
    info.method = NULL;

    if(!node_expression(&node, &info, lv_table)) {
        free_nodes();
        return FALSE;
    }

    if(node != 0 && *err_num == saved_err_num) {
        sCLNodeType* type_;
        sCompileInfo info;

        type_ = NULL;
        memset(&info, 0, sizeof(sCompileInfo));

        info.sname = sname;
        info.sline = &sline_top;
        info.caller_class = klass;
        info.caller_method = NULL;
        info.real_caller_class = klass;
        info.real_caller_method = NULL;
        info.sBlockInfo.method_block = NULL;
        info.code = initializer;
        info.constant = &klass->mClass->mConstPool;
        info.err_num = err_num;
        info.lv_table = lv_table;
        info.stack_num = &stack_num;
        info.max_stack = max_stack;
        info.exist_return = &exist_return;
        info.exist_break = &exist_break;
        info.sLoopInfo.break_labels = NULL;
        info.sLoopInfo.break_labels_len = NULL;
        info.sBlockInfo.while_type = 0;
        info.sLoopInfo.continue_labels = NULL;
        info.sLoopInfo.continue_labels_len = NULL;
        info.no_output_to_bytecodes = FALSE;
        info.sParamInfo.calling_method = NULL;
        info.sParamInfo.class_of_calling_method = NULL;
        info.sParamInfo.calling_block = FALSE;
        info.mNestOfMethodFromDefinitionPoint = 0;

        if(!compile_node(node, &type_, NULL, 0, &info)) {
            free_nodes();
            return FALSE;
        }

        *initializer_code_type = type_;
    }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif

    if(**p == ';') {
        (*p)++;
        skip_spaces_and_lf(p, sline);
        //correct_stack_pointer(&stack_num, sname, sline, initializer, err_num, FALSE);
    }
    else if(**p == 0) {
        parser_err_msg_format(sname, *sline, "fowarded the end of source.");
        free_nodes();
        return FALSE;
    }
    else {
        parser_err_msg_format(sname, *sline, "require ; character");
        parser_err_msg_format(sname, *sline, "unexpected character '%c' --> character code %d", **p, **p);
        (*p)++;
        (*err_num)++;
    }

    free_nodes();

    return TRUE;
}

BOOL compile_param_initializer(ALLOC sByteCode* initializer, sCLNodeType** initializer_code_type, int* max_stack, int* lv_var_num, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    unsigned int node;
    int saved_err_num;
    int sline_top;
    int stack_num;
    BOOL exist_return;
    BOOL exist_break;
    sVarTable* lv_table;
    sParserInfo info;
    
    memset(&info, 0, sizeof(info));

    init_nodes();

    lv_table = init_var_table();
    sByteCode_init(initializer);

    node = 0;
    saved_err_num = *err_num;
    sline_top = *sline;
    stack_num = 0;
    *max_stack = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    *initializer_code_type = gIntType;

    info.p = p;
    info.sname = sname;
    info.sline = sline;
    info.err_num = err_num;
    info.current_namespace = current_namespace;
    info.klass = klass;
    info.method = NULL;

    if(!node_expression_without_comma(&node, &info, lv_table)) {
        free_nodes();
        sByteCode_free(initializer);
        return FALSE;
    }

    if(node != 0 && *err_num == saved_err_num) {
        sCLNodeType* type_;
        sCompileInfo info;
        
        type_ = NULL;
        memset(&info, 0, sizeof(sCompileInfo));

        info.sname = sname;
        info.sline = &sline_top;
        info.caller_class = klass;
        info.caller_method = NULL;
        info.real_caller_class = klass;
        info.real_caller_method = NULL;
        info.sBlockInfo.method_block = NULL;
        info.code = initializer;
        info.constant = &klass->mClass->mConstPool;
        info.err_num = err_num;
        info.lv_table = lv_table;
        info.stack_num = &stack_num;
        info.max_stack = max_stack;
        info.exist_return = &exist_return;
        info.exist_break = &exist_break;
        info.sLoopInfo.break_labels = NULL;
        info.sLoopInfo.break_labels_len = NULL;
        info.sBlockInfo.while_type = 0;
        info.sLoopInfo.continue_labels = NULL;
        info.sLoopInfo.continue_labels_len = NULL;
        info.no_output_to_bytecodes = FALSE;
        info.sParamInfo.calling_method = NULL;
        info.sParamInfo.class_of_calling_method = NULL;
        info.sParamInfo.calling_block = FALSE;
        info.mNestOfMethodFromDefinitionPoint = 0;

        if(!compile_node(node, &type_, NULL, 0, &info)) {
            free_nodes();
            sByteCode_free(initializer);
            return FALSE;
        }

        *initializer_code_type = type_;
    }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif

    if(**p != ',' && **p != ')') {
        if(**p == 0) {
            parser_err_msg_format(sname, *sline, "fowarded the end of source.");
            free_nodes();
            sByteCode_free(initializer);
            return FALSE;
        }
        else {
            parser_err_msg_format(sname, *sline, "unexpected character '%c' --> character code %d", **p, **p);
            (*p)++;
            (*err_num)++;
        }
    }

    free_nodes();

    *lv_var_num = lv_table->mVarNum + lv_table->mMaxBlockVarNum;

    return TRUE;
}


BOOL skip_field_initializer(char** p, char* sname, int* sline, char* current_namespace, sCLNodeType* klass, sVarTable* lv_table)
{
    unsigned int node;
    int err_num;
    sParserInfo info;

    memset(&info, 0, sizeof(info));

    init_nodes();

    node = 0;
    err_num = 0;

    info.p = p;
    info.sname = sname;
    info.sline = sline;
    info.err_num = &err_num;
    info.current_namespace = current_namespace;
    info.klass = klass;
    info.method = NULL;

    if(!node_expression(&node, &info, lv_table)) {
        free_nodes();
        return FALSE;
    }

    if(**p == ';') {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else if(**p == 0) {
        parser_err_msg_format(sname, *sline, "fowarded the end of source.");
        free_nodes();
        return FALSE;
    }

    free_nodes();

    return TRUE;
}
