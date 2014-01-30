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
        switch(gNodes[i].mNodeType) {
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

    gNodes[i].mNodeType = NODE_TYPE_OPERAND;
    gNodes[i].mOperand = operand;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    return i;
}

uint sNodeTree_create_value(int value, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_VALUE;
    gNodes[i].mValue = value;

    gNodes[i].mType = gIntType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_string_value(MANAGED char* value, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_STRING_VALUE;
    gNodes[i].mStringValue = MANAGED value;

    gNodes[i].mType = gStringType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_array(uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_ARRAY_VALUE;
    gNodes[i].mType = gArrayType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_var(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_VARIABLE_NAME;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mType = *klass;

    return i;
}

uint sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_DEFINE_VARIABLE_NAME;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_return(sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_RETURN;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CLASS_METHOD_CALL;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_class_field(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CLASS_FIELD;
    gNodes[i].mVarName = STRDUP(var_name);

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_param(uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_PARAM;
    gNodes[i].mVarName = NULL;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_new_expression(sCLNodeType* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_NEW;
    gNodes[i].mVarName = NULL;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_fields(char* name, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_FIELD;
    gNodes[i].mVarName = STRDUP(name);

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_method_call(char* var_name, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_METHOD_CALL;
    gNodes[i].mVarName = STRDUP(var_name);

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_inherit(uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_INHERIT;
    gNodes[i].mVarName = NULL;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

uint sNodeTree_create_super(uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_SUPER;
    gNodes[i].mVarName = NULL;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

/// for debug
static void show_node(uint node)
{
    printf("type %d var_name %s class %p left %d right %d middle %d\n", gNodes[node].mNodeType, gNodes[node].mVarName, gNodes[node].mType.mClass, gNodes[node].mLeft, gNodes[node].mRight, gNodes[node].mMiddle);
    if(gNodes[node].mType.mClass) printf("class_name %s\n", REAL_CLASS_NAME(gNodes[node].mType.mClass));
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

// left_type is stored calss. right_type is class of value.
BOOL type_checking_with_class(sCLClass* left_type, sCLClass* right_type)
{
    /// there is compatibility of immediate value classes in the name space which is different ///
    if((left_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS)) {
        if(strcmp(CLASS_NAME(left_type), CLASS_NAME(right_type)) != 0) {
            return FALSE;
        }
    }
    else {
        if(left_type != right_type) {
            if(!search_for_super_class(right_type, left_type)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL type_checking(sCLNodeType* left_type, sCLNodeType* right_type)
{
    /// there is compatibility of immediate value classes in the name space which is different ///
    if((left_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS)) {
        if(strcmp(CLASS_NAME(left_type->mClass), CLASS_NAME(right_type->mClass)) != 0) {
            return FALSE;
        }
    }
    else {
        if(left_type->mClass != right_type->mClass) {
            if(!search_for_super_class(right_type->mClass, left_type->mClass)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2)
{
    if(type1->mClass != type2->mClass) {
        return FALSE;
    }

    if(type1->mGenericsTypesNum != type2->mGenericsTypesNum) {
        return FALSE;
    }

    int i;
    for(i=0; i<type1->mGenericsTypesNum; i++) {
        if(type1->mGenericsTypes[i] != type2->mGenericsTypes[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL check_private_access(sCLClass* klass, sCLClass* access_class)
{
    if(klass == access_class) {
        return TRUE;
    }

    return FALSE;
}

typedef struct {
    sCLClass* caller_class;
    sCLMethod* caller_method;
    sByteCode* code;
    sConst* constant;
    char* sname;
    int* sline;
    int* err_num;
    sVarTable* lv_table;
    int* stack_num;
    int* max_stack;
    BOOL* exist_return;
    int num_params;
    sCLNodeType* class_params;
} sCompileInfo;

/*
static BOOL param_type_checking(sCLClass* klass, sCLMethod* method, char* method_name, sCLNodeType* type_, sCompileInfo* info)
{
    const int method_num_params = get_method_num_params(method);

    if(info->num_params != method_num_params) {
        parser_err_msg_format(info->sname, *info->sline, "Parametor number of (%s) is not %d but %d", method_name, info->num_params, method_num_params);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    BOOL err_flg = FALSE;
    int i;
    for(i=0; i<info->num_params; i++) {
        sCLNodeType class_params2;
        memset(&class_params2, 0, sizeof(class_params2));
        get_param_type_of_method(klass, method, i, &class_params2);

        if(class_params2.mClass == NULL || info->class_params[i].mClass == NULL) {
            parser_err_msg("unexpected error of parametor", info->sname, *info->sline);
            *type_ = gIntType; // dummy
            return FALSE;
        }

        if(!type_checking(&class_params2, &info->class_params[i])) {
            parser_err_msg_format(info->sname, *info->sline, "(%d) parametor is not %s but %s", i, REAL_CLASS_NAME(info->class_params[i].mClass), REAL_CLASS_NAME(class_params2.mClass));
            (*info->err_num)++;

            err_flg = TRUE;
        }
    }

    if(err_flg) {
        *type_ = gIntType; // dummy
        return TRUE;
    }

    return TRUE;
}
*/

static BOOL do_call_inherit(sCLMethod* method, int method_index, sCLClass* klass, BOOL class_method, sCLNodeType* type_, sCompileInfo* info)
{
    /// check of private method ///
    if(method->mFlags & CL_PRIVATE_METHOD && !check_private_access(klass, info->caller_class)) {
        parser_err_msg_format(info->sname, *info->sline, "this is private method(%s).", METHOD_NAME2(klass, method));
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

    /// make code ///
    sCLNodeType result_type;
    memset(&result_type, 0, sizeof(result_type));
    if(info->caller_class == type_->mClass) {
        if(!get_result_type_of_method(klass, method, &result_type, NULL)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }
    else {
        if(!get_result_type_of_method(klass, method, &result_type, type_)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_INHERIT);
    append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(klass));
    append_int_value_to_bytecodes(info->code, method_index);
    append_char_value_to_bytecodes(info->code, !type_checking(&result_type, &gVoidType));

    const int method_num_params = get_method_num_params(method);

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params);
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1);
    }

    if(!type_checking(&result_type, &gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL do_call_method(sCLMethod* method, char* method_name, sCLClass* klass, BOOL class_method, BOOL calling_super, sCLNodeType* type_, sCompileInfo* info)
{
    /// check of private method ///
    if(method->mFlags & CL_PRIVATE_METHOD && !check_private_access(klass, info->caller_class)) {
        parser_err_msg_format(info->sname, *info->sline, "this is private method(%s).", METHOD_NAME2(klass, method));
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

    /// make code ///
    sCLNodeType result_type;
    memset(&result_type, 0, sizeof(result_type));
    if(info->caller_class == type_->mClass) {
        if(!get_result_type_of_method(klass, method, &result_type, NULL)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }
    else {
        if(!get_result_type_of_method(klass, method, &result_type, type_)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_METHOD);

    append_str_to_constant_pool(info->code, info->constant, method_name);   // method name

    const int method_num_params = get_method_num_params(method);
    append_int_value_to_bytecodes(info->code, method_num_params); // method num params

    int i;
    for(i=0; i<method_num_params; i++) {
        append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(info->class_params[i].mClass));  // method params
        append_char_value_to_bytecodes(info->code, info->class_params[i].mGenericsTypesNum);

        int j;
        for(j=0; j<info->class_params[i].mGenericsTypesNum; j++) {
            append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(info->class_params[i].mGenericsTypes[j]));
        }
    }

    append_char_value_to_bytecodes(info->code, !type_checking(&result_type, &gVoidType)); // an existance of result flag
    append_char_value_to_bytecodes(info->code, calling_super);  // a flag of calling super
    append_char_value_to_bytecodes(info->code, class_method);  // a flag of class method kind

    if(class_method || type_checking_with_class(klass, gIntType.mClass) || type_checking_with_class(klass, gFloatType.mClass) || type_checking_with_class(klass, gVoidType.mClass)) {
        append_char_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS);

        append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(klass));
    }
    else {
        append_char_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT);
    }

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params);
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1);
    }

    if(!type_checking(&result_type, &gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL call_inherit(sCLNodeType* type_, sCompileInfo* info)
{
    if(info->caller_class == NULL || info->caller_method == NULL) {
        parser_err_msg("can't call inherit method because there are not the caller method or the caller class.", info->sname, *info->sline);
        *type_ = gIntType; // dummy
        return TRUE;
    }

    const int caller_method_index = get_method_index(info->caller_class, info->caller_method);
    ASSERT(caller_method_index != -1);

    char* method_name = METHOD_NAME(info->caller_class, caller_method_index);

    sCLMethod* method;
    if(info->caller_class == type_->mClass) {  // if it is true, don7t solve generics type
        method = get_method_with_type_params(info->caller_class, method_name, info->class_params, info->num_params, info->caller_method->mFlags & CL_CLASS_METHOD, NULL, caller_method_index-1);
    }
    else {
        method = get_method_with_type_params(info->caller_class, method_name, info->class_params, info->num_params, info->caller_method->mFlags & CL_CLASS_METHOD, type_, caller_method_index-1);
    }

    int method_index = get_method_index(info->caller_class, method);

    if(method == NULL) {
        method_index = get_method_index_from_the_parametor_point(info->caller_class, method_name, caller_method_index, info->caller_method->mFlags & CL_CLASS_METHOD);

        if(method_index != -1) {
            parser_err_msg_format(info->sname, *info->sline, "can't inherit. Invalid parametor types on this method(%s::%s)", CLASS_NAME(info->caller_class), method_name);
            (*info->err_num)++;
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "can't inherit. There is not this method before(%s).", method_name);
            (*info->err_num)++;
        }

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if((info->caller_method->mFlags & CL_CLASS_METHOD) != (method->mFlags & CL_CLASS_METHOD)) {
        parser_err_msg_format(info->sname, *info->sline, "can't inherit because caller method and inherit method is the differ type");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    sCLClass* klass = info->caller_class;

    if(!do_call_inherit(method, method_index, klass, method->mFlags & CL_CLASS_METHOD, type_, info))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL call_super(sCLNodeType* type_, sCompileInfo* info)
{
    /// statically checking ///
    if(info->caller_class == NULL || info->caller_method == NULL) {
        parser_err_msg("can't call super because there are not the caller method or the caller class.", info->sname, *info->sline);
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(info->caller_method->mFlags & CL_CLASS_METHOD) {
        parser_err_msg_format(info->sname, *info->sline, "can't call super because this method is class method(%s).", METHOD_NAME2(info->caller_class, info->caller_method));
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(info->caller_class->mNumSuperClasses == 0) {
        parser_err_msg_format(info->sname, *info->sline, "there is not a super class of this class(%s).", REAL_CLASS_NAME(info->caller_class));
        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// search for method ///
    sCLClass* klass = info->caller_class;

    const int caller_method_index = get_method_index(klass, info->caller_method);
    ASSERT(caller_method_index != -1);

    char* method_name = METHOD_NAME(klass, caller_method_index);

    /// search from super classes ///
    sCLClass* new_class;
    sCLMethod* method;
    if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics type
        method = get_method_with_type_params_on_super_classes(klass, method_name, info->class_params, info->num_params, &new_class, FALSE, NULL);
    }
    else {
        method = get_method_with_type_params_on_super_classes(klass, method_name, info->class_params, info->num_params, &new_class, FALSE, type_);
    }

    /// found at super classes ///
    if(method) {
        klass = new_class;
    }
    /// the method is not found ///
    else {
        method = get_method_on_super_classes(klass, method_name, &new_class);

        if(method) {
            parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types on this method(%s::%s)", CLASS_NAME(new_class), method_name);
            (*info->err_num)++;
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "There is not this method on super classes(%s)", method_name);
            (*info->err_num)++;
        }

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(method->mFlags & CL_CLASS_METHOD) {
        parser_err_msg_format(info->sname, *info->sline, "can't call super method because this is class method(%s).", method_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(!do_call_method(method, method_name, klass, FALSE, TRUE, type_, info))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL call_method(sCLClass* klass, char* method_name, BOOL class_method, sCLNodeType* type_, sCompileInfo* info)
{
    sCLMethod* method;
    if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics types
        method = get_method_with_type_params(klass, method_name, info->class_params, info->num_params, class_method, NULL, klass->mNumMethods-1);
    }
    else {
        method = get_method_with_type_params(klass, method_name, info->class_params, info->num_params, class_method, type_, klass->mNumMethods-1);
    }

    /// next, searched for super classes ///
    if(method == NULL) {
        /// search from super classes ///
        sCLClass* new_class;
        if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics types
            method = get_method_with_type_params_on_super_classes(klass, method_name, info->class_params, info->num_params, &new_class, class_method, NULL);
        }
        else {
            method = get_method_with_type_params_on_super_classes(klass, method_name, info->class_params, info->num_params, &new_class, class_method, type_);
        }

        /// found on super classes ///
        if(method) {
            klass = new_class;
        }
        /// method not found ///
        else {
            method = get_method(klass, method_name);

            if(method) {
                parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types on this method(%s::%s)", CLASS_NAME(klass), method_name);
                (*info->err_num)++;
            }
            else {
                sCLClass* new_class;
                method = get_method_on_super_classes(klass, method_name, &new_class);

                if(method) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types on this method(%s::%s)", CLASS_NAME(new_class), method_name);
                    (*info->err_num)++;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "There is not this method(%s)", method_name);
                    (*info->err_num)++;
                }
            }

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    if(!do_call_method(method, method_name, klass, class_method, FALSE, type_, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL store_field(sCLClass* klass, char* field_name, sCLNodeType* right_type, BOOL class_field, sCLNodeType* type_, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType field_type;
    memset(&field_type, 0, sizeof(field_type));

    if(class_field) {
        field = get_field(klass, field_name);
        field_index = get_field_index(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(klass, field, &field_type, NULL);
            }
            else {
                get_field_type(klass, field, &field_type, type_);
            }
        }
    }
    else {
        sCLClass* found_class;
        field = get_field_including_super_classes(klass, field_name, &found_class);
        field_index = get_field_index_including_super_classes(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(found_class, field, &field_type, NULL);
            }
            else {
                get_field_type(found_class, field, &field_type, type_);
            }
        }
    }

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this field(%s) in this class(%s)", field_name, REAL_CLASS_NAME(klass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field->mFlags & CL_PRIVATE_FIELD && !check_private_access(klass, info->caller_class)) {
        parser_err_msg_format(info->sname, *info->sline, "this is private field(%s).", field_name);
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
    if(field_type.mClass == NULL || type_checking(&field_type, &gVoidType)) {
        parser_err_msg("this field has no type.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field_type.mClass == NULL || right_type->mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!type_checking(&field_type, right_type)) {
        parser_err_msg_format(info->sname, *info->sline, "type error. left class is %s. right class is %s", REAL_CLASS_NAME(field_type.mClass), REAL_CLASS_NAME(right_type->mClass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_SR_STATIC_FIELD);
        append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(klass));
        append_int_value_to_bytecodes(info->code, field_index);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_SRFIELD);
        append_int_value_to_bytecodes(info->code, field_index);

        dec_stack_num(info->stack_num, 1);
    }

    *type_ = field_type;

    return TRUE;
}

static BOOL load_field(sCLClass* klass, char* field_name, BOOL class_field, sCLNodeType* type_, sCompileInfo* info)
{
    if(klass == NULL) {
        parser_err_msg("left value has not class. can't get field", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy

        return TRUE;
    }

    sCLField* field;
    int field_index;
    sCLNodeType field_type;
    memset(&field_type, 0, sizeof(field_type));

    if(class_field) {
        field = get_field(klass, field_name);
        field_index = get_field_index(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(klass, field, &field_type, NULL);
            }
            else {
                get_field_type(klass, field, &field_type, type_);
            }
        }
    }
    else {
        sCLClass* found_class;
        field = get_field_including_super_classes(klass, field_name, &found_class);
        field_index = get_field_index_including_super_classes(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(found_class, field, &field_type, NULL);
            }
            else {
                get_field_type(found_class, field, &field_type, type_);
            }
        }
    }

    if(field == NULL || field_index == -1) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this field(%s) in this class(%s)", field_name, REAL_CLASS_NAME(klass));
        (*info->err_num)++;

        *type_ = gIntType; // dummy

        return TRUE;
    }

    if(field->mFlags & CL_PRIVATE_FIELD && !check_private_access(klass, info->caller_class)){
        parser_err_msg_format(info->sname, *info->sline, "this is private field(%s).", field_name);
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

    if(field_type.mClass == NULL || type_checking(&field_type, &gVoidType)) {
        parser_err_msg("this field has not class", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_LD_STATIC_FIELD);
        append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(klass));
        append_int_value_to_bytecodes(info->code, field_index);

        inc_stack_num(info->stack_num, info->max_stack, 1);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_LDFIELD);
        append_int_value_to_bytecodes(info->code, field_index);
    }

    *type_ = field_type;

    return TRUE;
}

static BOOL load_local_varialbe(char* name, sCLNodeType* type_, sCompileInfo* info)
{
    sVar* var = get_variable_from_table(info->lv_table, name);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this varialbe (%s)", name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(type_checking(&var->mType, &gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ILOAD);
    }
    else if(type_checking(&var->mType, &gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ALOAD);
    }
    else if(type_checking(&var->mType, &gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FLOAD);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OLOAD);
    }

    append_int_value_to_bytecodes(info->code, var->mIndex);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    *type_ = var->mType;

    return TRUE;
}

static BOOL compile_node(uint node, sCLNodeType* type_, sCompileInfo* info);

static BOOL compile_left_node(uint node, sCLNodeType* left_type, sCompileInfo* info)
{
    if(gNodes[node].mLeft) {
        if(!compile_node(gNodes[node].mLeft, left_type, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_right_node(uint node, sCLNodeType* right_type, sCompileInfo* info)
{
    if(gNodes[node].mRight) {
        if(!compile_node(gNodes[node].mRight, right_type, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_node(uint node, sCLNodeType* type_, sCompileInfo* info)
{
    if(node == 0) {
        parser_err_msg("no expression", info->sname, *info->sline);
        (*info->err_num)++;
        return TRUE;
    }

    switch(gNodes[node].mNodeType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDC);
            append_int_value_to_constant_pool(info->code, info->constant, gNodes[node].mValue);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gIntType;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDC);
            append_wstr_to_constant_pool(info->code, info->constant, gNodes[node].mStringValue);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gStringType;
            }
            break;

        /// array value ///
        case NODE_TYPE_ARRAY_VALUE: {
            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// elements go ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_NEW_ARRAY);
            append_int_value_to_bytecodes(info->code, info->num_params);

            dec_stack_num(info->stack_num, info->num_params);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gArrayType;
            }
            break;

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;
            sCLNodeType* type2 = &gNodes[node].mType;

            sVar* var = get_variable_from_table(info->lv_table, name);

            if(var) {
                parser_err_msg_format(info->sname, *info->sline, "there is a same name variable(%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
            }
            else {
                if(!add_variable_to_table(info->lv_table, name, type2)) {
                    parser_err_msg("overflow global variable table", info->sname, *info->sline);
                    return FALSE;
                }

                *type_ = *type2;
            }
            }
            break;

        /// load variable ///
        case NODE_TYPE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;
            if(!load_local_varialbe(name, type_, info)) {
                return FALSE;
            }
            }
            break;

        /// store value to variable ///
        case NODE_TYPE_STORE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(info->lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "there is not this varialbe (%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }

            sCLNodeType left_type = var->mType;

            /// a right value goes ///
            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            /// type checking ///
            if(left_type.mClass == NULL || right_type.mClass == NULL) {
                parser_err_msg("no type left or right value", info->sname, *info->sline);
                break;
            }
            if(!type_checking(&left_type, &right_type)) {
                parser_err_msg_format(info->sname, *info->sline, "type error. left class is %s. right class is %s", REAL_CLASS_NAME(left_type.mClass), REAL_CLASS_NAME(right_type.mClass));
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }

            /// append opecode to bytecodes ///
            if(type_checking(&left_type, &gIntType)) {
                append_opecode_to_bytecodes(info->code, OP_ISTORE);
            }
            else if(type_checking(&left_type, &gStringType)) {
                append_opecode_to_bytecodes(info->code, OP_ASTORE);
            }
            else if(type_checking(&left_type, &gFloatType)) {
                append_opecode_to_bytecodes(info->code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(info->code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(info->code, var->mIndex);

            *type_ = var->mType;
            }
            break;

        /// define variable and store a value ///
        case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME: {
            /// define variable ///
            char* name = gNodes[node].mVarName;
            sCLNodeType* type2 = &gNodes[node].mType;

            sVar* var = get_variable_from_table(info->lv_table, name);

            if(var) {
                parser_err_msg_format(info->sname, *info->sline, "there is a same name variable(%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }
            else {
                if(!add_variable_to_table(info->lv_table, name, type2)) {
                    parser_err_msg("overflow a variable table", info->sname, *info->sline);
                    return FALSE;
                }
            }

            var = get_variable_from_table(info->lv_table, name);

            ASSERT(var);

            /// store ///
            sCLNodeType left_type = var->mType;

            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            /// type checking ///
            if(left_type.mClass == NULL || right_type.mClass == NULL) {
                parser_err_msg("no type left or right value", info->sname, *info->sline);
                break;
            }
            if(!type_checking(&left_type, &right_type)) {
                parser_err_msg_format(info->sname, *info->sline, "type error. left class is %s. right class is %s", REAL_CLASS_NAME(left_type.mClass), REAL_CLASS_NAME(right_type.mClass));
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }

            /// append an opecode to bytecodes ///
            if(type_checking(&left_type, &gIntType)) {
                append_opecode_to_bytecodes(info->code, OP_ISTORE);
            }
            else if(type_checking(&left_type, &gStringType)) {
                append_opecode_to_bytecodes(info->code, OP_ASTORE);
            }
            else if(type_checking(&left_type, &gFloatType)) {
                append_opecode_to_bytecodes(info->code, OP_FSTORE);
            }
            else {
                append_opecode_to_bytecodes(info->code, OP_OSTORE);
            }

            append_int_value_to_bytecodes(info->code, var->mIndex);
            }
            break;
        
        /// load field  ///
        case NODE_TYPE_FIELD: {
            /// left_value ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            sCLClass* klass = left_type.mClass;
            char* field_name = gNodes[node].mVarName;
            *type_ = left_type;

            if(!load_field(klass, field_name, FALSE, type_, info)) {
                return FALSE;
            }
            }
            break;

        /// load class field ///
        case NODE_TYPE_CLASS_FIELD: {
            sCLClass* klass = gNodes[node].mType.mClass;
            char* field_name = gNodes[node].mVarName;

            ASSERT(klass); // must be not NULL
            *type_ = gNodes[node].mType;

            if(!load_field(klass, field_name, TRUE, type_, info)) {
                return FALSE;
            }
            }
            break;

        /// store field ///
        case NODE_TYPE_STORE_FIELD: {
            /// left_value ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// right value  ///
            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            sCLClass* klass = left_type.mClass;
            char* field_name = gNodes[node].mVarName;
            *type_ = left_type;

            if(!store_field(klass, field_name, &right_type, FALSE, type_, info)) {
                return FALSE;
            }
            }
            break;

        /// store class field ///
        case NODE_TYPE_STORE_CLASS_FIELD: {
            sCLClass* klass = gNodes[node].mType.mClass;
            char* field_name = gNodes[node].mVarName;

            /// right value ///
            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            *type_ = gNodes[node].mType;

            if(!store_field(klass, field_name, &right_type, TRUE, type_, info)) {
                return FALSE;
            }

            }
            break;

        /// new operand ///
        case NODE_TYPE_NEW: {
            sCLNodeType* type2 = &gNodes[node].mType;
            sCLClass* klass = type2->mClass;

            /// creating new object ///
            append_opecode_to_bytecodes(info->code, OP_NEW_OBJECT);

            append_str_to_constant_pool(info->code, info->constant, REAL_CLASS_NAME(klass));

            inc_stack_num(info->stack_num, info->max_stack, 1);

            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// params go ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// call constructor ///
            char* method_name = CLASS_NAME(klass);
            *type_ = *type2;

            if(!call_method(klass, method_name, FALSE, type_, info))
            {
                return FALSE;
            }

            }
            break;

        /// call method ///
        case NODE_TYPE_METHOD_CALL: {
            /// left_value ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// params go ///
            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            /// call method ///
            sCLClass* klass = left_type.mClass;
            char* method_name = gNodes[node].mVarName;
            *type_ = left_type;

            if(!call_method(klass, method_name, FALSE, type_, info))
            {
                return FALSE;
            }
            }
            break;

        /// call class method ///
        case NODE_TYPE_CLASS_METHOD_CALL: {
            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// params go ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// call class method //
            sCLClass* klass = gNodes[node].mType.mClass;
            char* method_name = gNodes[node].mVarName;
            *type_ = gNodes[node].mType;

            if(!call_method(klass, method_name, TRUE, type_, info))
            {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_SUPER: {
            /// load self ///
            if(!(info->caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, info)) {
                    return FALSE;
                }
            }

            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// params go ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// call method ///
            if(!call_super(type_, info)) {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_INHERIT: {
            /// load self ///
            if(!(info->caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, info)) {
                    return FALSE;
                }
            }

            /// initilize class params ///
            info->num_params = 0;

            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            memset(class_params, 0, sizeof(class_params));
            info->class_params = class_params;

            /// params go ///
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            /// call method ///
            if(!call_inherit(type_, info)) {
                return FALSE;
            }
            }
            break;

        /// params ///
        case NODE_TYPE_PARAM: {
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            sCLNodeType right_type;
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, info)) {
                return FALSE;
            }

            if(right_type.mClass == NULL) {
                *type_ = gIntType; // dummy
            }
            else {
                *type_ = right_type;
            }

            info->class_params[info->num_params] = *type_;
            info->num_params++;

            }
            break;

        /// return ///
        case NODE_TYPE_RETURN: {
            sCLNodeType left_type;
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, info)) {
                return FALSE;
            }

            if(info->caller_class == NULL || info->caller_method == NULL) {
                parser_err_msg("there is not caller method. can't return", info->sname, *info->sline);
                return FALSE;
            }

            sCLNodeType result_type;
            memset(&result_type, 0, sizeof(result_type));
            if(!get_result_type_of_method(info->caller_class, info->caller_method, &result_type, NULL)) {
                parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(info->caller_class), METHOD_NAME2(info->caller_class, info->caller_method));
                (*info->err_num)++;
                return TRUE;
            }

            if(result_type.mClass == NULL) {
                parser_err_msg("unexpected err. no result type", info->sname, *info->sline);
                return FALSE;
            }

            if(!type_checking(&left_type, &result_type)) {
                parser_err_msg_format(info->sname, *info->sline, "type error. Requiring class is not %s but %s", REAL_CLASS_NAME(left_type.mClass), REAL_CLASS_NAME(result_type.mClass));
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_RETURN);

            *(info->exist_return) = TRUE;

            *type_ = gVoidType;
            }
            break;

        /// operand ///
        case NODE_TYPE_OPERAND:
            switch((int)gNodes[node].mOperand) {
            case kOpAdd: {
                sCLNodeType left_type;
                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, info)) {
                    return FALSE;
                }

                sCLNodeType right_type;
                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, info)) {
                    return FALSE;
                }

                if(left_type.mClass == NULL || right_type.mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else if(left_type.mClass != right_type.mClass) {
                    parser_err_msg("addition with not same class", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    if(type_checking(&left_type, &gStringType)) {
                        append_opecode_to_bytecodes(info->code, OP_SADD);
                        *type_ = gStringType;
                    }
                    else if(type_checking(&left_type, &gIntType)) {
                        append_opecode_to_bytecodes(info->code, OP_IADD);
                        *type_ = gIntType;
                    }
                    else if(type_checking(&left_type, &gFloatType)) {
                        append_opecode_to_bytecodes(info->code, OP_FADD);
                        *type_ = gFloatType;
                    }
                    else {
                        parser_err_msg("additin with invalid class", info->sname, *info->sline);
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                    }

                    dec_stack_num(info->stack_num, 1);
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

//#define STACK_DEBUG

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

        if(!node_expression(&node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLNodeType type_;
            memset(&type_, 0, sizeof(type_));

            sCompileInfo info;

            info.caller_class = klass;
            info.caller_method = method;
            info.code = &method->mByteCodes;
            info.constant = &klass->mConstPool;
            info.sname = sname;
            info.sline = sline;
            info.err_num = err_num;
            info.lv_table = lv_table;
            info.stack_num = &stack_num;
            info.max_stack = &max_stack;
            info.exist_return = &exist_return;
            info.num_params = 0;
            info.class_params = NULL;

            if(!compile_node(node, &type_, &info)) {
                free_nodes();
                return FALSE;
            }
        }

#ifdef STACK_DEBUG
printf("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif
        if(**p == ';' || **p == '\n' || **p == '}') {
            while(**p == ';' || **p == '\n') {
                if(**p == '\n') (*sline)++;

                (*p)++;
                skip_spaces(p);
            }

//puts("call correct_stack_pointer");
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

    sCLNodeType result_type;
    memset(&result_type, 0, sizeof(result_type));
    if(!get_result_type_of_method(klass, method, &result_type, NULL)) {
        parser_err_msg_format(sname, *sline, "can't found result type of the method named %s::%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
        (*err_num)++;
        return TRUE;
    }

    if(!type_checking(&result_type, &gVoidType) && !exist_return && !(method->mFlags & CL_CONSTRUCTOR)) {
        parser_err_msg("require return sentence", sname, *sline);
        (*err_num)++;
        free_nodes();
        return TRUE;
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
        if(!node_expression(&node, &p, sname, sline, err_num, &gGVTable, current_namespace, NULL)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLNodeType type_;
            memset(&type_, 0, sizeof(type_));

            sCompileInfo info;

            info.caller_class = NULL;
            info.caller_method = NULL;
            info.code = code;
            info.constant = constant;
            info.sname = sname;
            info.sline = sline;
            info.err_num = err_num;
            info.lv_table = &gGVTable;
            info.stack_num = &stack_num;
            info.max_stack = max_stack;
            info.exist_return = &exist_return;
            info.num_params = 0;
            info.class_params = NULL;

            if(!compile_node(node, &type_, &info)) {
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
