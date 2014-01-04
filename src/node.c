#include "clover.h"
#include "common.h"
#include <ctype.h>

//////////////////////////////////////////////////
// define node tree and memory management
//////////////////////////////////////////////////
sNodeTree* gNodes;

static uint gSizeNodes;
static uint gUsedNodes;

static void init_nodes()
{
    const int node_size = 32;

    gNodes = CALLOC(1, sizeof(sNodeTree)*node_size);
    gSizeNodes = node_size;
    gUsedNodes = 1;   // 0 of index means null
}

static void free_nodes()
{
    int i;
    for(i=1; i<gUsedNodes; i++) {
        switch(gNodes[i].mType) {
            case NODE_TYPE_STRING_VALUE:
                FREE(gNodes[i].mStringValue);
                break;

            case NODE_TYPE_VARIABLE_NAME:
            case NODE_TYPE_EQUAL_VARIABLE_NAME:
            case NODE_TYPE_FIELD:
            case NODE_TYPE_EQUAL_FIELD:
            case NODE_TYPE_METHOD_CALL:
            case NODE_TYPE_DEFINE_VARIABLE_NAME:
            case NODE_TYPE_EQUAL_DEFINE_VARIABLE_NAME:
            case NODE_TYPE_CLASS_METHOD_CALL:
            case NODE_TYPE_CLASS_FIELD:
            case NODE_TYPE_EQUAL_CLASS_FIELD:
                FREE(gNodes[i].mVarName);
                break;
        }
    }

    FREE(gNodes);
}

// return node index
static uint alloc_node()
{
    if(gSizeNodes == gUsedNodes) {
        const int new_size = (gSizeNodes+1) * 2;
        gNodes = REALLOC(gNodes, sizeof(sNodeTree)*new_size);
        memset(gNodes + gSizeNodes, 0, sizeof(sNodeTree)*(new_size - gSizeNodes));
    }

    return gUsedNodes++;
}

// return node index
uint sNodeTree_create_operand(enum eOperand operand, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_OPERAND;
    gNodes[i].mOperand = operand;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mClass = NULL;

    return i;
}

uint sNodeTree_create_value(int value, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_VALUE;
    gNodes[i].mValue = value;

    gNodes[i].mClass = gIntClass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_string_value(MANAGED char* value, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_STRING_VALUE;
    gNodes[i].mStringValue = MANAGED value;

    gNodes[i].mClass = gStringClass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_var(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_VARIABLE_NAME;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mClass = klass;

    return i;
}

uint sNodeTree_create_define_var(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_DEFINE_VARIABLE_NAME;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mClass = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_return(sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_RETURN;

    gNodes[i].mClass = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_class_method_call(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_CLASS_METHOD_CALL;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mClass = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_class_field(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_CLASS_FIELD;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mClass = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_param(uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_PARAM;
    gNodes[i].mVarName = NULL;

    gNodes[i].mClass = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_new_expression(sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_NEW;
    gNodes[i].mVarName = NULL;

    gNodes[i].mClass = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_fields(char* name, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_FIELD;
    gNodes[i].mVarName = STRDUP(name);

    gNodes[i].mClass = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_method_call(char* var_name, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_METHOD_CALL;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mClass = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

/// for debug
static void show_node(uint node)
{
    printf("type %d var_name %s class %p left %d right %d middle %d\n", gNodes[node].mType, gNodes[node].mVarName, gNodes[node].mClass, gNodes[node].mLeft, gNodes[node].mRight, gNodes[node].mMiddle);
    if(gNodes[node].mClass) printf("class_name %s::%s\n", NAMESPACE_NAME(gNodes[node].mClass), CLASS_NAME(gNodes[node].mClass));
    else printf("\n");
}

//////////////////////////////////////////////////
// Compile nodes to byte codes
//////////////////////////////////////////////////
static void append_opecode_to_bytecodes(sByteCode* code, uchar c)
{
    sByteCode_append(code, &c, sizeof(uchar));
}

static void append_int_value_to_bytecodes(sByteCode* code, uint n)
{
    sByteCode_append(code, &n, sizeof(uint));
}

static void append_char_value_to_bytecodes(sByteCode* code, uchar c)
{
    sByteCode_append(code, &c, sizeof(uchar));
}

static void append_int_value_to_constant_pool(sByteCode* code, sConst* const_pool, uint n)
{
    uint l = const_pool->mLen;
    sByteCode_append(code, &l, sizeof(uint));

    sConst_append_int(const_pool, n);
}

static void append_char_value_to_constant_pool(sByteCode* code, sConst* const_pool, uchar c)
{
    uint n = const_pool->mLen;
    sByteCode_append(code, &n, sizeof(uint));

    sConst_append(const_pool, &c, sizeof(uchar));
}

static void append_wstr_to_constant_pool(sByteCode* code, sConst* const_pool, char* wstr)
{
    uint n = const_pool->mLen;
    sByteCode_append(code, &n, sizeof(uint));

    sConst_append_wstr(const_pool, wstr);
}

static void append_str_to_constant_pool(sByteCode* code, sConst* const_pool, char* str)
{
    uint n = const_pool->mLen;
    sByteCode_append(code, &n, sizeof(uint));

    sConst_append_str(const_pool, str);
}

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

static BOOL call_static_method(sCLClass* cl_klass, sCLClass* klass, sCLMethod* method, uint method_index, char* method_name, int num_params, int* err_num, sCLClass** type_, sCLClass** class_params, char* sname, int* sline, int* stack_num, int* max_stack, sByteCode* code, sConst* constant)
{
    if(method == NULL || method_index == -1) {
        uint method_index2 = get_method_index(cl_klass, method_name);

        if(method_index2 != -1) {
            char buf2[128];
            snprintf(buf2, 128, "Invalid parametor types");
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;
        }
        else {
            char buf2[128];
            snprintf(buf2, 128, "There is not this method(%s)", method_name);
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;
        }

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// is this static method ? ///
    if((method->mHeader & CL_STATIC_METHOD) == 0) {
        char buf2[128];
        snprintf(buf2, 128, "This is not static method(%s)", METHOD_NAME(cl_klass, method_index));
        parser_err_msg(buf2, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// is this private method ? ///
    if(method->mHeader & CL_PRIVATE_METHOD && cl_klass != klass) {
        char buf[128];
        snprintf(buf, 128, "this is private field(%s).", METHOD_NAME(cl_klass, method_index));
        parser_err_msg(buf, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy;
        return FALSE;
    }

    const int method_num_params = get_method_num_params(cl_klass, method_index);
    ASSERT(method_num_params != -1);

    if(num_params != method_num_params) {
        char buf2[128];
        snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
        parser_err_msg(buf2, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    BOOL err_flg = FALSE;
    int i;
    for(i=0; i<num_params; i++) {
        sCLClass* class_params2 = get_method_param_types(cl_klass, method_index, i);
        if(class_params2 == NULL || class_params[i] == NULL) {
            parser_err_msg("unexpected error of parametor", sname, *sline);
            return FALSE;
        }

        if(class_params[i] != class_params2) {
            char buf2[128];
            snprintf(buf2, 128, "(%d) parametor is not %s::%s but %s::%s", i, NAMESPACE_NAME(class_params[i]), CLASS_NAME(class_params[i]), NAMESPACE_NAME(class_params2), CLASS_NAME(class_params2));
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;

            err_flg = TRUE;
        }
    }

    if(err_flg) {
        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// calling method go ///
    sCLClass* result_type = get_method_result_type(cl_klass, method_index);
    ASSERT(result_type);

    append_opecode_to_bytecodes(code, OP_INVOKE_STATIC_METHOD);
    append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));
    append_int_value_to_bytecodes(code, method_index);
    append_char_value_to_bytecodes(code, result_type != gVoidClass);

    if(result_type != gVoidClass) {
        inc_stack_num(stack_num, max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL call_method(sCLClass* cl_klass, sCLClass* klass, sCLMethod* method, uint method_index, char* method_name, int num_params, int* err_num, sCLClass** type_, sCLClass** class_params, char* sname, int* sline, int* stack_num, int* max_stack, sByteCode* code, sConst* constant)
{
    if(method == NULL || method_index == -1) {
        uint method_index2 = get_method_index(cl_klass, method_name);

        if(method_index2 != -1) {
            char buf2[128];
            snprintf(buf2, 128, "Invalid parametor types");
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;
        }
        else {
            char buf2[128];
            snprintf(buf2, 128, "There is not this method(%s)", method_name);
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;
        }

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// is this static method ? ///
    if(method->mHeader & CL_STATIC_METHOD) {
        char buf2[128];
        snprintf(buf2, 128, "This is static method(%s)", METHOD_NAME(cl_klass, method_index));
        parser_err_msg(buf2, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// is this private method ? ///
    if(method->mHeader & CL_PRIVATE_METHOD && cl_klass != klass) {
        char buf[128];
        snprintf(buf, 128, "this is private field(%s).", METHOD_NAME(cl_klass, method_index));
        parser_err_msg(buf, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy;
        return FALSE;
    }

    int method_num_params = get_method_num_params(cl_klass, method_index);
    ASSERT(method_num_params != -1);

    if(num_params != method_num_params) {
        char buf2[128];
        snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
        parser_err_msg(buf2, sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    BOOL err_flg = FALSE;
    int i;
    for(i=0; i<num_params; i++) {
        sCLClass* class_params2 = get_method_param_types(cl_klass, method_index, i);
        if(class_params2 == NULL || class_params[i] == NULL) {
            parser_err_msg("unexpected error of parametor", sname, *sline);
            return FALSE;
        }

        if(class_params[i] != class_params2) {
            char buf2[128];
            snprintf(buf2, 128, "(%d) parametor is not %s::%s but %s::%s", i, NAMESPACE_NAME(class_params[i]), CLASS_NAME(class_params[i]), NAMESPACE_NAME(class_params2), CLASS_NAME(class_params2));
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;

            err_flg = TRUE;
        }
    }

    if(err_flg) {
        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// calling method go ///
    sCLClass* result_type = get_method_result_type(cl_klass, method_index);
    ASSERT(result_type);

    append_opecode_to_bytecodes(code, OP_INVOKE_METHOD);

    if(cl_klass == gVoidClass) {
        append_char_value_to_bytecodes(code, INVOKE_METHOD_OBJECT_TYPE_VOID);
    }
    else if(cl_klass == gStringClass) {
        append_char_value_to_bytecodes(code, INVOKE_METHOD_OBJECT_TYPE_STRING);
    }
    else if(cl_klass == gIntClass) {
        append_char_value_to_bytecodes(code, INVOKE_METHOD_OBJECT_TYPE_INT);
    }
    else if(cl_klass == gFloatClass) {
        append_char_value_to_bytecodes(code, INVOKE_METHOD_OBJECT_TYPE_FLOAT);
    }
    else {
        append_char_value_to_bytecodes(code, INVOKE_METHOD_OBJECT_TYPE_OBJECT);
    }

    append_int_value_to_bytecodes(code, method_num_params);
    append_int_value_to_bytecodes(code, method_index);
    append_char_value_to_bytecodes(code, result_type != gVoidClass); // an existance of result flag

    if(result_type != gVoidClass) {
        inc_stack_num(stack_num, max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL compile_node(uint node, sCLClass* klass, sCLMethod* method, sCLClass** type_, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, sVarTable* lv_table, int* stack_num, int* max_stack, BOOL* exist_return, int* num_params, sCLClass* class_params[])
{
    if(node == 0) {
        parser_err_msg("no expression", sname, *sline);
        (*err_num)++;
        return TRUE;
    }

    switch(gNodes[node].mType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
            append_opecode_to_bytecodes(code, OP_LDC);
            append_int_value_to_constant_pool(code, constant, gNodes[node].mValue);

            inc_stack_num(stack_num, max_stack, 1);

            *type_ = gIntClass;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
            append_opecode_to_bytecodes(code, OP_LDC);

            append_wstr_to_constant_pool(code, constant, gNodes[node].mStringValue);

            inc_stack_num(stack_num, max_stack, 1);

            *type_ = gStringClass;
            }
            break;

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;
            sCLClass* klass2 = gNodes[node].mClass;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var) {
                char buf2[128];
                snprintf(buf2, 128, "there is a same name variable(%s)", name);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
            }
            else {
                if(!add_variable_to_table(lv_table, name, klass2)) {
                    parser_err_msg("overflow global variable table", sname, *sline);
                    return FALSE;
                }

                *type_ = klass2;
            }
            }
            break;

        /// define variable and store a value ///
        case NODE_TYPE_EQUAL_DEFINE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;
            sCLClass* klass2 = gNodes[node].mClass;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var) {
                char buf2[128];
                snprintf(buf2, 128, "there is a same name variable(%s)", name);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }
            else {
                if(!add_variable_to_table(lv_table, name, klass2)) {
                    parser_err_msg("overflow a variable table", sname, *sline);
                    return FALSE;
                }
            }

            var = get_variable_from_table(lv_table, name);

            ASSERT(var);

            sCLClass* left_type = var->mClass;

            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            if(left_type == NULL || right_type == NULL) {
                parser_err_msg("no type left or right value", sname, *sline);
                break;
            }
            if(left_type != right_type) {
                char buf2[128];
                snprintf(buf2, 128, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            if(left_type == gIntClass) {
                append_opecode_to_bytecodes(code, OP_ISTORE);
            }
            else if(left_type == gStringClass) {
                append_opecode_to_bytecodes(code, OP_ASTORE);
            }
            else if(left_type == gFloatClass) {
                append_opecode_to_bytecodes(code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(code, var->mIndex);

            *type_ = var->mClass;

            //(*stack_num)--;
            }
            break;

        /// variable name ///
        case NODE_TYPE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is not this varialbe (%s)", name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            if(var->mClass == gIntClass) {
                append_opecode_to_bytecodes(code, OP_ILOAD);
            }
            else if(var->mClass == gStringClass) {
                append_opecode_to_bytecodes(code, OP_ALOAD);
            }
            else if(var->mClass == gFloatClass) {
                append_opecode_to_bytecodes(code, OP_FLOAD);
            }
            else {
                append_opecode_to_bytecodes(code, OP_OLOAD);
            }

            append_int_value_to_bytecodes(code, var->mIndex);

            inc_stack_num(stack_num, max_stack, 1);

            *type_ = var->mClass;

            }
            break;

        case NODE_TYPE_EQUAL_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is not this varialbe (%s)", name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            sCLClass* left_type = var->mClass;

            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            if(left_type == NULL || right_type == NULL) {
                parser_err_msg("no type left or right value", sname, *sline);
                break;
            }
            if(left_type != right_type) {
                char buf2[128];
                snprintf(buf2, 128, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            if(left_type == gIntClass) {
                append_opecode_to_bytecodes(code, OP_ISTORE);
            }
            else if(left_type == gStringClass) {
                append_opecode_to_bytecodes(code, OP_ASTORE);
            }
            else if(left_type == gFloatClass) {
                append_opecode_to_bytecodes(code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(code, var->mIndex);

            *type_ = var->mClass;

            //(*stack_num)--;
            }
            break;
        
        /// field name ///
        case NODE_TYPE_FIELD: {
            /// left_value ///
            sCLClass* left_type = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            sCLClass* cl_klass = left_type;
            char* field_name = gNodes[node].mVarName;

            if(cl_klass == NULL) {
                parser_err_msg("left value has not class. can't get field", sname, *sline);
                (*err_num)++;
                *type_ = gIntClass; // dummy
                break;
            }

            sCLField* field = get_field(cl_klass, field_name);
            int field_index = get_field_index(cl_klass, field_name);

            if(field == NULL || field_index == -1) {
                char buf[128];
                snprintf(buf, 128, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if(field->mHeader & CL_PRIVATE_FIELD && cl_klass != klass) {
                char buf[128];
                snprintf(buf, 128, "this is private field(%s).", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy;
                break;
            }

            if(field->mHeader & CL_STATIC_FIELD) {
                char buf[128];
                snprintf(buf, 128, "this is static field(%s)", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            sCLClass* cl_klass2 = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

            if(cl_klass2 == NULL || cl_klass2 == gVoidClass) {
                parser_err_msg("this field has not class", sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy;
            }
            else {
                append_opecode_to_bytecodes(code, OP_LDFIELD);
                append_int_value_to_bytecodes(code, field_index);

                inc_stack_num(stack_num, max_stack, 1);

                *type_ = cl_klass2;
            }
            }
            break;

        case NODE_TYPE_EQUAL_FIELD: {
            /// left_value ///
            sCLClass* cl_klass = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &cl_klass, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            char* field_name = gNodes[node].mVarName;

            sCLField* field = get_field(cl_klass, field_name);
            int field_index = get_field_index(cl_klass, field_name);

            if(field == NULL || field_index == -1) {
                char buf[128];
                snprintf(buf, 128, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if(field->mHeader & CL_STATIC_FIELD) {
                char buf[128];
                snprintf(buf, 128, "this is static field(%s)", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if(field->mHeader & CL_PRIVATE_FIELD && cl_klass != klass) {
                char buf[128];
                snprintf(buf, 128, "this is private field(%s).", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy;
                break;
            }

            sCLClass* left_type = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

            if(left_type == NULL || left_type == gVoidClass) {
                parser_err_msg("this field has not type.", sname, *sline);
                (*err_num)++;
                *type_ = gIntClass; // dummy
                break;
            }

            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            if(left_type == NULL || right_type == NULL) {
                parser_err_msg("no type left or right value", sname, *sline);
                break;
            }
            if(left_type != right_type) {
                char buf2[128];
                snprintf(buf2, 128, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            append_opecode_to_bytecodes(code, OP_SRFIELD);
            append_int_value_to_bytecodes(code, field_index);

            *type_ = left_type;
            }
            break;

        case NODE_TYPE_CLASS_FIELD: {
            sCLClass* cl_klass = gNodes[node].mClass;
            char* field_name = gNodes[node].mVarName;

            ASSERT(cl_klass); // must be not NULL

            sCLField* field = get_field(cl_klass, field_name);
            int field_index = get_field_index(cl_klass, field_name);

            if(field == NULL || field_index == -1) {
                char buf[128];
                snprintf(buf, 128, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if((field->mHeader & CL_STATIC_FIELD) == 0) {
                char buf[128];
                snprintf(buf, 128, "this is not static field(%s)", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if(field->mHeader & CL_PRIVATE_FIELD && cl_klass != klass) {
                char buf[128];
                snprintf(buf, 128, "this is private field(%s).", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy;
                break;
            }

            sCLClass* cl_klass2 = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

            if(cl_klass2 == NULL || cl_klass2 == gVoidClass) {
                parser_err_msg("this field has not class", sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy;
            }
            else {
                append_opecode_to_bytecodes(code, OP_LD_STATIC_FIELD);

                append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));

                append_int_value_to_bytecodes(code, field_index);

                inc_stack_num(stack_num, max_stack, 1);

                *type_ = cl_klass2;
            }
            }
            break;

        case NODE_TYPE_EQUAL_CLASS_FIELD: {
            sCLClass* cl_klass = gNodes[node].mClass;
            char* field_name = gNodes[node].mVarName;

            sCLField* field = get_field(cl_klass, field_name);
            int field_index = get_field_index(cl_klass, field_name);

            if(field == NULL || field_index == -1) {
                char buf[128];
                snprintf(buf, 128, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            if((field->mHeader & CL_STATIC_FIELD) == 0) {
                char buf[128];
                snprintf(buf, 128, "this is not static field(%s)", field_name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy
                break;
            }

            sCLClass* left_type = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

            if(left_type == NULL || left_type == gVoidClass) {
                parser_err_msg("this field has not type.", sname, *sline);
                (*err_num)++;
                *type_ = gIntClass; // dummy
                break;
            }

            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            if(left_type == NULL) {
                parser_err_msg("no type left value", sname, *sline);
                
                *type_ = gIntClass; // dummy
                break;
            }
            if(right_type == NULL) {
                parser_err_msg("no type right value", sname, *sline);

                *type_ = gIntClass;  // dummy
                break;
            }
            if(left_type != right_type) {
                char buf2[128];
                snprintf(buf2, 128, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            append_opecode_to_bytecodes(code, OP_SR_STATIC_FIELD);

            append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));

            append_int_value_to_bytecodes(code, field_index);

            *type_ = left_type;
            }
            break;

        case NODE_TYPE_NEW: {
            sCLClass* cl_klass = gNodes[node].mClass;

            /// creating new object ///
            append_opecode_to_bytecodes(code, OP_NEW_OBJECT);

            append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));

            inc_stack_num(stack_num, max_stack, 1);

            /// params go ///
            sCLClass* left_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            char* method_name = CLASS_NAME(cl_klass);
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant);
            }
            break;

        case NODE_TYPE_METHOD_CALL: {
            /// left_value ///
            sCLClass* left_type = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// params go ///
            sCLClass* right_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            sCLClass* cl_klass = left_type;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant);
            }
            break;

        case NODE_TYPE_CLASS_METHOD_CALL: {
            /// params go ///
            sCLClass* left_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params)) {
                    return FALSE;
                }
            }

            /// type checking ///
            sCLClass* cl_klass = gNodes[node].mClass;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_static_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant);
            }
            break;

        case NODE_TYPE_PARAM: {
            sCLClass* left_type  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }
            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            if(right_type == NULL) {
                *type_ = gIntClass;  // dummy
            }
            else {
                *type_ = right_type;
            }

            class_params[*num_params] = *type_;
            (*num_params)++;
            }
            break;

        case NODE_TYPE_RETURN: {
            sCLClass* left_type  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            if(klass == NULL || method == NULL) {
                parser_err_msg("this is not method. can't return", sname, *sline);
                return FALSE;
            }

            sCLClass* result_type = cl_get_class(CONS_str(klass->mConstPool, method->mResultType));

            if(result_type == NULL) {
                parser_err_msg("unexpected err. no result type", sname, *sline);
                return FALSE;
            }

            if(left_type != result_type) {
                char buf2[128];
                snprintf(buf2, 128, "type error. Requiring class is not %s::%s but %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(result_type), CLASS_NAME(result_type));
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            append_opecode_to_bytecodes(code, OP_RETURN);

            //(*stack_num)++;

            *exist_return = TRUE;

            *type_ = gVoidClass;
            }
            break;

        /// operand ///
        case NODE_TYPE_OPERAND:
            switch((int)gNodes[node].mOperand) {
            case kOpAdd: {
                sCLClass* left_type = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                        return FALSE;
                    }
                }
                sCLClass* right_type = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                        return FALSE;
                    }
                }

                if(left_type == NULL || right_type == NULL) {
                    parser_err_msg("no class type", sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass; // dummy class
                }
                else if(left_type != right_type) {
                    parser_err_msg("addition with not same class", sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass;  // dummy class
                }
                else {
                    if(left_type == gStringClass) {
                        append_opecode_to_bytecodes(code, OP_SADD);
                        *type_ = gStringClass;
                    }
                    else if(left_type == gIntClass) {
                        append_opecode_to_bytecodes(code, OP_IADD);
                        *type_ = gIntClass;
                    }
                    else if(left_type == gFloatClass) {
                        append_opecode_to_bytecodes(code, OP_FADD);
                        *type_ = gFloatClass;
                    }

                    dec_stack_num(stack_num, 1);
                }

                }
                break;

            case kOpSub: 
                break;

            case kOpMult: 
                break;

            case kOpDiv: 
                break;

            case kOpMod: 
                break;
        }
        break;
    }

    return TRUE;
}

static void correct_stack_pointer(int* stack_num, char* sname, int* sline, sByteCode* code, int* err_num)
{
    if(*stack_num < 0) {
        parser_err_msg("unexpected error. Stack pointer is invalid", sname, *sline);
        (*err_num)++;
    }
    else if(*stack_num == 1) {
        append_opecode_to_bytecodes(code, OP_POP);
    }
    else if(*stack_num > 0) {
        append_opecode_to_bytecodes(code, OP_POP_N);
        append_int_value_to_bytecodes(code, *stack_num);
    }

    *stack_num = 0;
}

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace)
{
    alloc_bytecode(method);

    init_nodes();

    int max_stack = 0;
    int stack_num = 0;
    BOOL exist_return = FALSE;

    while(*p) {
        int saved_err_num = *err_num;

        uint node = 0;
        if(!node_expression(&node, p, sname, sline, err_num, lv_table, current_namespace)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLClass* type_ = NULL;
            if(!compile_node(node, klass, method, &type_, &method->mByteCodes, &klass->mConstPool, sname, sline, err_num, lv_table, &stack_num, &max_stack, &exist_return, NULL, NULL)) {
                free_nodes();
                return FALSE;
            }
        }

        if(**p == ';' || **p == '\n' || **p == '}') {
            while(**p == ';' || **p == '\n') {
                if(**p == '\n') (*sline)++;

                (*p)++;
                skip_spaces(p);
            }

            correct_stack_pointer(&stack_num, sname, sline, &method->mByteCodes, err_num);

            if(**p == '}') {
                (*p)++;
                break;
            }
        }
        else {
            char buf[128];
            snprintf(buf, 128, "unexpected character(%c)", **p);
            parser_err_msg(buf, sname, *sline);
            free_nodes();
            return FALSE;
        }
    }

    /// add "return self" to the constructor ///
    if(constructor) {
        append_opecode_to_bytecodes(&method->mByteCodes, OP_OLOAD);
        append_int_value_to_bytecodes(&method->mByteCodes, 0);
    }

    sCLClass* result_type = cl_get_class(CONS_str(klass->mConstPool, method->mResultType));
    if(result_type != gVoidClass && !exist_return && !(method->mHeader & CL_CONSTRUCTOR)) {
        parser_err_msg("require return sentence", sname, *sline);
        free_nodes();
        return FALSE;
    }

    method->mMaxStack = max_stack;

    free_nodes();

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, BOOL flg_main, int* err_num, int* max_stack, char* current_namespace)
{
    init_nodes();

    *max_stack = 0;
    int stack_num = 0;
    BOOL exist_return = FALSE;

    char* p = source;
    while(*p) {
        int saved_err_num = *err_num;
        uint node = 0;
        if(!node_expression(&node, &p, sname, sline, err_num, &gGVTable, current_namespace)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLClass* type_ = NULL;
            if(!compile_node(node, NULL, NULL, &type_, code, constant, sname, sline, err_num, &gGVTable, &stack_num, max_stack, &exist_return, NULL, NULL)) {
                free_nodes();
                return FALSE;
            }
        }

        if(*p == ';' || *p == '\n') {
            while(*p == ';' || *p == '\n') {
                p++;
                skip_spaces(&p);
            }

            correct_stack_pointer(&stack_num, sname, sline, code, err_num);
        }
        else {
            char buf[128];
            snprintf(buf, 128, "unexpected character(%c)", *p);
            parser_err_msg(buf, sname, *sline);
            free_nodes();
            return FALSE;
        }
    }

    free_nodes();

    return TRUE;
}
