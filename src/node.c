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
            case NODE_TYPE_STORE_VARIABLE_NAME:
            case NODE_TYPE_FIELD:
            case NODE_TYPE_STORE_FIELD:
            case NODE_TYPE_METHOD_CALL:
            case NODE_TYPE_DEFINE_VARIABLE_NAME:
            case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME:
            case NODE_TYPE_CLASS_METHOD_CALL:
            case NODE_TYPE_CLASS_FIELD:
            case NODE_TYPE_STORE_CLASS_FIELD:
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

BOOL type_checking(sCLClass* left_type, sCLClass* right_type)
{
    if(left_type != right_type) {
        return FALSE;
    }

    return TRUE;

/*
    if((left_type->mFlags & CLASS_FLAGS_IMMEDIATE_CLASS) && (right_type->mFlags & CLASS_FLAGS_IMMEDIATE_CLASS)) {
        if(strcmp(CLASS_NAME(left_type), CLASS_NAME(right_type)) != 0) {
            return FALSE;
        }
    }
    else {
        if(left_type != right_type) {
            return FALSE;
        }
    }

    return TRUE;
*/
}

static BOOL param_type_checking(sCLClass* cl_klass, sCLClass* klass, uint method_index, char* method_name, int num_params, int* err_num, sCLClass** type_, sCLClass** class_params, char* sname, int* sline)
{
    const int method_num_params = get_method_num_params(cl_klass, method_index);
    ASSERT(method_num_params != -1);

    if(num_params != method_num_params) {
        parser_err_msg_format(sname, *sline, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
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

        if(!type_checking(class_params[i], class_params2)) {
            parser_err_msg_format(sname, *sline, "(%d) parametor is not %s::%s but %s::%s", i, NAMESPACE_NAME(class_params[i]), CLASS_NAME(class_params[i]), NAMESPACE_NAME(class_params2), CLASS_NAME(class_params2));
            (*err_num)++;

            err_flg = TRUE;
        }
    }

    if(err_flg) {
        *type_ = gIntClass; // dummy
        return FALSE;
    }

    return TRUE;
}

static BOOL call_method(sCLClass* cl_klass, sCLClass* klass, sCLMethod* method, uint method_index, char* method_name, int num_params, int* err_num, sCLClass** type_, sCLClass** class_params, char* sname, int* sline, int* stack_num, int* max_stack, sByteCode* code, sConst* constant, BOOL static_method)
{
    /// method not found ///
    if(method == NULL || method_index == -1) {
        uint method_index2 = get_method_index(cl_klass, method_name);

        if(method_index2 != -1) {
            parser_err_msg_format(sname, *sline, "Invalid parametor types on this method(%s::%s)", CLASS_NAME(cl_klass),method_name);
            (*err_num)++;
        }
        else {
            parser_err_msg_format(sname, *sline, "There is not this method(%s)", method_name);
            (*err_num)++;
        }

        *type_ = gIntClass; // dummy
        return FALSE;
    }

    /// is this private method ? ///
    if(method->mHeader & CL_PRIVATE_METHOD && cl_klass != klass) {
        parser_err_msg_format(sname, *sline, "this is private field(%s).", METHOD_NAME(cl_klass, method_index));
        (*err_num)++;

        *type_ = gIntClass; // dummy;
        return FALSE;
    }

    /// is this static method ? ///
    if(static_method) {
        if((method->mHeader & CL_STATIC_METHOD) == 0) {
            parser_err_msg_format(sname, *sline, "This is not static method(%s)", METHOD_NAME(cl_klass, method_index));
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return FALSE;
        }
    }
    else {
        if(method->mHeader & CL_STATIC_METHOD) {
            parser_err_msg_format(sname, *sline, "This is static method(%s)", METHOD_NAME(cl_klass, method_index));
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return FALSE;
        }
    }

    /// parametor type checking ///
    if(!param_type_checking(cl_klass, klass, method_index, method_name, num_params, err_num, type_, class_params, sname, sline)) {
        return FALSE;
    }

    /// calling method go ///
    sCLClass* result_type = get_method_result_type(cl_klass, method_index);
    ASSERT(result_type);

    if(static_method) {
        append_opecode_to_bytecodes(code, OP_INVOKE_CLASS_METHOD);
        append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));
        append_int_value_to_bytecodes(code, method_index);
        append_char_value_to_bytecodes(code, !type_checking(result_type, gVoidClass)); // an existance of result flag
    }
    else {
        append_opecode_to_bytecodes(code, OP_INVOKE_METHOD);

        append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));

        const int method_num_params = get_method_num_params(cl_klass, method_index);
        ASSERT(method_num_params != -1);

        append_int_value_to_bytecodes(code, method_index);
        append_char_value_to_bytecodes(code, !type_checking(result_type,gVoidClass)); // an existance of result flag
    }

    if(!type_checking(result_type,gVoidClass)) {
        inc_stack_num(stack_num, max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL store_field(sCLClass* cl_klass, char* field_name, sCLClass* right_type, sCLClass* klass, sCLClass** type_, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, int* stack_num, BOOL static_field)
{
    sCLField* field = get_field(cl_klass, field_name);
    int field_index = get_field_index(cl_klass, field_name);

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(sname, *sline, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
        (*err_num)++;

        *type_ = gIntClass; // dummy
        return TRUE;
    }

    if(field->mHeader & CL_PRIVATE_FIELD && cl_klass != klass) {
        parser_err_msg_format(sname, *sline, "this is private field(%s).", field_name);
        (*err_num)++;

        *type_ = gIntClass; // dummy;
        return TRUE;
    }

    if(static_field) {
        if((field->mHeader & CL_STATIC_FIELD) == 0) {
            parser_err_msg_format(sname, *sline, "this is not static field(%s)", field_name);
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return TRUE;
        }
    }
    else {
        if(field->mHeader & CL_STATIC_FIELD) {
            parser_err_msg_format(sname, *sline, "this is static field(%s)", field_name);
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return TRUE;
        }
    }

    /// type checking ///
    sCLClass* field_class = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

    if(field_class == NULL || type_checking(field_class, gVoidClass)) {
        parser_err_msg("this field has no type.", sname, *sline);
        (*err_num)++;
        *type_ = gIntClass; // dummy
        return TRUE;
    }

    if(field_class == NULL || right_type == NULL) {
        parser_err_msg("no type left or right value", sname, *sline);
        return TRUE;
    }
    if(!type_checking(field_class, right_type)) {
        parser_err_msg_format(sname, *sline, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(field_class), CLASS_NAME(field_class), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
        (*err_num)++;

        *type_ = gIntClass; // dummy class
        return TRUE;
    }

    if(static_field) {
        append_opecode_to_bytecodes(code, OP_SR_STATIC_FIELD);
        append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));
        append_int_value_to_bytecodes(code, field_index);
    }
    else {
        append_opecode_to_bytecodes(code, OP_SRFIELD);
        append_int_value_to_bytecodes(code, field_index);

        dec_stack_num(stack_num, 1);
    }

    *type_ = field_class;

    return TRUE;
}

static BOOL load_field(sCLClass* cl_klass, char* field_name, sCLClass* klass, sCLClass** type_, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, int* stack_num, int* max_stack, BOOL static_field)
{
    if(cl_klass == NULL) {
        parser_err_msg("left value has not class. can't get field", sname, *sline);
        (*err_num)++;
        *type_ = gIntClass; // dummy

        return TRUE;
    }

    sCLField* field = get_field(cl_klass, field_name);
    int field_index = get_field_index(cl_klass, field_name);

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(sname, *sline, "there is not this field(%s) in this class(%s::%s)", field_name, NAMESPACE_NAME(cl_klass), CLASS_NAME(cl_klass));
        (*err_num)++;

        *type_ = gIntClass; // dummy

        return TRUE;
    }

    if(field->mHeader & CL_PRIVATE_FIELD && cl_klass != klass) {
        parser_err_msg_format(sname, *sline, "this is private field(%s).", field_name);
        (*err_num)++;

        *type_ = gIntClass; // dummy;

        return TRUE;
    }

    if(static_field) {
        if((field->mHeader & CL_STATIC_FIELD) == 0) {
            parser_err_msg_format(sname, *sline, "this is not static field(%s)", field_name);
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return TRUE;
        }
    }
    else {
        if(field->mHeader & CL_STATIC_FIELD) {
            parser_err_msg_format(sname, *sline, "this is static field(%s)", field_name);
            (*err_num)++;

            *type_ = gIntClass; // dummy
            return TRUE;
        }
    }

    sCLClass* field_class = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

    if(field_class == NULL || type_checking(field_class, gVoidClass)) {
        parser_err_msg("this field has not class", sname, *sline);
        (*err_num)++;

        *type_ = gIntClass; // dummy;
        return TRUE;
    }

    if(static_field) {
        append_opecode_to_bytecodes(code, OP_LD_STATIC_FIELD);

        append_str_to_constant_pool(code, constant, REAL_CLASS_NAME(cl_klass));

        append_int_value_to_bytecodes(code, field_index);

        inc_stack_num(stack_num, max_stack, 1);
    }
    else {
        append_opecode_to_bytecodes(code, OP_LDFIELD);
        append_int_value_to_bytecodes(code, field_index);
    }

    *type_ = field_class;

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
                parser_err_msg_format(sname, *sline, "there is a same name variable(%s)", name);
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

        /// load variable ///
        case NODE_TYPE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(sname, *sline, "there is not this varialbe (%s)", name);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            if(type_checking(var->mClass, gIntClass)) {
                append_opecode_to_bytecodes(code, OP_ILOAD);
            }
            else if(type_checking(var->mClass, gStringClass)) {
                append_opecode_to_bytecodes(code, OP_ALOAD);
            }
            else if(type_checking(var->mClass, gFloatClass)) {
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

        /// store value to variable ///
        case NODE_TYPE_STORE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(sname, *sline, "there is not this varialbe (%s)", name);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            sCLClass* left_type = var->mClass;

            /// a right value goes ///
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
            if(!type_checking(left_type, right_type)) {
                parser_err_msg_format(sname, *sline, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            /// append opecode to bytecodes ///
            if(type_checking(left_type, gIntClass)) {
                append_opecode_to_bytecodes(code, OP_ISTORE);
            }
            else if(type_checking(left_type, gStringClass)) {
                append_opecode_to_bytecodes(code, OP_ASTORE);
            }
            else if(type_checking(left_type, gFloatClass)) {
                append_opecode_to_bytecodes(code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(code, var->mIndex);

            *type_ = var->mClass;
            }
            break;

        /// define variable and store a value ///
        case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME: {
            /// define variable ///
            char* name = gNodes[node].mVarName;
            sCLClass* klass2 = gNodes[node].mClass;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var) {
                parser_err_msg_format(sname, *sline, "there is a same name variable(%s)", name);
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

            /// store ///
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
            if(!type_checking(left_type, right_type)) {
                parser_err_msg_format(sname, *sline, "type error. left class is %s::%s. right class is %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(right_type), CLASS_NAME(right_type));
                (*err_num)++;

                *type_ = gIntClass; // dummy class
                break;
            }

            /// append an opecode to bytecodes ///
            if(type_checking(left_type, gIntClass)) {
                append_opecode_to_bytecodes(code, OP_ISTORE);
            }
            else if(type_checking(left_type, gStringClass)) {
                append_opecode_to_bytecodes(code, OP_ASTORE);
            }
            else if(type_checking(left_type, gFloatClass)) {
                append_opecode_to_bytecodes(code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(code, var->mIndex);


            }
            break;
        
        /// load field  ///
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

            if(!load_field(cl_klass, field_name, klass, type_, code, constant, sname, sline, err_num, stack_num, max_stack, FALSE)) {
                return FALSE;
            }
            }
            break;

        /// load class field ///
        case NODE_TYPE_CLASS_FIELD: {
            sCLClass* cl_klass = gNodes[node].mClass;
            char* field_name = gNodes[node].mVarName;

            ASSERT(cl_klass); // must be not NULL

            if(!load_field(cl_klass, field_name, klass, type_, code, constant, sname, sline, stack_num, err_num, max_stack, TRUE)) {
                return FALSE;
            }
            }
            break;

        /// store field ///
        case NODE_TYPE_STORE_FIELD: {
            /// left_value ///
            sCLClass* cl_klass = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &cl_klass, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            /// right value  ///
            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            char* field_name = gNodes[node].mVarName;

            if(!store_field(cl_klass, field_name, right_type, klass, type_, code, constant, sname, sline, err_num, stack_num, FALSE)) {
                return FALSE;
            }
            }
            break;

        /// store class field ///
        case NODE_TYPE_STORE_CLASS_FIELD: {
            sCLClass* cl_klass = gNodes[node].mClass;
            char* field_name = gNodes[node].mVarName;

            /// right value ///
            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params)) {
                    return FALSE;
                }
            }

            if(!store_field(cl_klass, field_name, right_type, klass, type_, code, constant, sname, sline, err_num, stack_num, TRUE)) {
                return FALSE;
            }

            }
            break;

        /// new operand ///
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

            /// call constructor ///
            char* method_name = CLASS_NAME(cl_klass);
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant, FALSE);
            }
            break;

        /// call method ///
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

            /// call method ///
            sCLClass* cl_klass = left_type;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant, FALSE);
            }
            break;

        /// call class method ///
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

            /// call static method //
            sCLClass* cl_klass = gNodes[node].mClass;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            (void)call_method(cl_klass, klass, method, method_index, method_name, num_params, err_num, type_, class_params, sname, sline, stack_num, max_stack, code, constant, TRUE);
            }
            break;

        /// params ///
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

        /// return ///
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

            if(!type_checking(left_type, result_type)) {
                parser_err_msg_format(sname, *sline, "type error. Requiring class is not %s::%s but %s::%s", NAMESPACE_NAME(left_type), CLASS_NAME(left_type), NAMESPACE_NAME(result_type), CLASS_NAME(result_type));
                return FALSE;
            }

            append_opecode_to_bytecodes(code, OP_RETURN);

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
                else if(!type_checking(left_type, right_type)) {
                    parser_err_msg("addition with not same class", sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass;  // dummy class
                }
                else {
                    if(type_checking(left_type, gStringClass)) {
                        append_opecode_to_bytecodes(code, OP_SADD);
                        *type_ = gStringClass;
                    }
                    else if(type_checking(left_type, gIntClass)) {
                        append_opecode_to_bytecodes(code, OP_IADD);
                        *type_ = gIntClass;
                    }
                    else if(type_checking(left_type, gFloatClass)) {
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
            parser_err_msg_format(sname, *sline, "unexpected character(%c)", **p);
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
    if(!type_checking(result_type, gVoidClass) && !exist_return && !(method->mHeader & CL_CONSTRUCTOR)) {
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
            parser_err_msg_format(sname, *sline, "unexpected character(%c)", *p);
            free_nodes();
            return FALSE;
        }
    }

    free_nodes();

    return TRUE;
}
