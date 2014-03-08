#include "clover.h"
#include "common.h"
#include <ctype.h>

//////////////////////////////////////////////////
// define node tree and memory management
//////////////////////////////////////////////////
static void init_node_blocks();
static void free_node_blocks();

sNodeTree* gNodes;

static unsigned int gSizeNodes;
static unsigned int gUsedNodes;

static void init_nodes()
{
    const int node_size = 32;

    if(gUsedNodes == 0) {
        gNodes = CALLOC(1, sizeof(sNodeTree)*node_size);
        gSizeNodes = node_size;
        gUsedNodes = 1;   // 0 of index means null

        init_node_blocks();
    }
}

static void free_nodes()
{
    int i;

    if(gUsedNodes > 0) {
        for(i=1; i<gUsedNodes; i++) {
            switch(gNodes[i].mNodeType) {
                case NODE_TYPE_STRING_VALUE:
                    FREE(gNodes[i].uValue.mStringValue);
                    break;

                case NODE_TYPE_CLASS_METHOD_CALL:
                case NODE_TYPE_METHOD_CALL:
                    FREE(gNodes[i].uValue.sMethod.mVarName);
                    break;

                case NODE_TYPE_BLOCK_CALL:
                case NODE_TYPE_VARIABLE_NAME:
                case NODE_TYPE_STORE_VARIABLE_NAME:
                case NODE_TYPE_FIELD:
                case NODE_TYPE_STORE_FIELD:
                case NODE_TYPE_DEFINE_VARIABLE_NAME:
                case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME:
                case NODE_TYPE_CLASS_FIELD:
                case NODE_TYPE_STORE_CLASS_FIELD:
                    FREE(gNodes[i].uValue.sVarName.mVarName);
                    break;
            }
        }

        FREE(gNodes);

        free_node_blocks();

        gSizeNodes = 0;
        gUsedNodes = 0;
    }
}

// return node index
static unsigned int alloc_node()
{
    if(gSizeNodes == gUsedNodes) {
        int new_size;

        new_size = (gSizeNodes+1) * 2;
        gNodes = REALLOC(gNodes, sizeof(sNodeTree)*new_size);
        memset(gNodes + gSizeNodes, 0, sizeof(sNodeTree)*(new_size - gSizeNodes));

        gSizeNodes = new_size;
    }

    return gUsedNodes++;
}

// return node index
unsigned int sNodeTree_create_operand(enum eOperand operand, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_OPERAND;
    gNodes[i].uValue.mOperand = operand;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;


    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    return i;
}

unsigned int sNodeTree_create_value(int value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_VALUE;
    gNodes[i].uValue.mValue = value;

    gNodes[i].mType = gIntType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_fvalue(float fvalue, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_FVALUE;
    gNodes[i].uValue.mFValue = fvalue;

    gNodes[i].mType = gFloatType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_string_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_STRING_VALUE;
    gNodes[i].uValue.mStringValue = MANAGED value;

    gNodes[i].mType = gStringType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_array(unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_ARRAY_VALUE;
    gNodes[i].mType = gArrayType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_var(char* var_name, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_VARIABLE_NAME;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(var_name);

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mType.mClass = NULL;

    return i;
}

unsigned int sNodeTree_create_call_block(char* var_name, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BLOCK_CALL;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(var_name);

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mType.mClass = NULL;

    return i;
}

unsigned int sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_DEFINE_VARIABLE_NAME;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(var_name);

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_return(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_RETURN;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_break(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BREAK;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_continue()
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CONTINUE;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_null()
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_NULL;

    gNodes[i].mType.mClass = NULL;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_true()
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_TRUE;

    gNodes[i].mType.mClass = NULL;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_false()
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_FALSE;

    gNodes[i].mType.mClass = NULL;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CLASS_METHOD_CALL;
    gNodes[i].uValue.sMethod.mVarName = STRDUP(var_name);
    gNodes[i].uValue.sMethod.mBlock = block;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_class_field(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CLASS_FIELD;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(var_name);

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_param(unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_PARAM;
    gNodes[i].uValue.sVarName.mVarName = NULL;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_new_expression(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_NEW;
    gNodes[i].uValue.sMethod.mVarName = NULL;
    gNodes[i].uValue.sMethod.mBlock = block;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_fields(char* name, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_FIELD;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(name);

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_method_call(char* var_name, unsigned int left, unsigned int right, unsigned int middle, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_METHOD_CALL;
    gNodes[i].uValue.sMethod.mVarName = STRDUP(var_name);
    gNodes[i].uValue.sMethod.mBlock = block;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_inherit(unsigned int left, unsigned int right, unsigned int middle, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_INHERIT;
    gNodes[i].uValue.sMethod.mBlock = block;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_super(unsigned int left, unsigned int right, unsigned int middle, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_SUPER;
    gNodes[i].uValue.sMethod.mBlock = block;

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_if(unsigned int if_conditional, unsigned int if_block, unsigned int else_block, unsigned int* else_if_conditional, unsigned int* else_if_block, int else_if_num, sCLNodeType* type_)
{
    unsigned int i;
    unsigned int j;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_IF;

    gNodes[i].uValue.sIfBlock.mIfBlock = if_block;
    gNodes[i].uValue.sIfBlock.mElseBlock = else_block;
    gNodes[i].uValue.sIfBlock.mElseIfBlockNum = else_if_num;
    for(j=0; j<else_if_num; j++) {
        gNodes[i].uValue.sIfBlock.mElseIfConditional[j] = else_if_conditional[j];
        gNodes[i].uValue.sIfBlock.mElseIfBlock[j] = else_if_block[j];
    }

    ASSERT(type_ != NULL);
    gNodes[i].mType = *type_;

    gNodes[i].mLeft = if_conditional;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_while(unsigned int conditional, unsigned int block, sCLNodeType* type_)
{
    unsigned int i;
    unsigned int j;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_WHILE;

    gNodes[i].uValue.mWhileBlock = block;

    ASSERT(type_ != NULL);
    gNodes[i].mType = *type_;

    gNodes[i].mLeft = conditional;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_for(unsigned int conditional, unsigned int conditional2, unsigned int conditional3, unsigned int block, sCLNodeType* type_)
{
    unsigned int i;
    unsigned int j;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_FOR;

    gNodes[i].uValue.mForBlock = block;

    ASSERT(type_ != NULL);
    gNodes[i].mType = *type_;

    gNodes[i].mLeft = conditional;
    gNodes[i].mRight = conditional2;
    gNodes[i].mMiddle = conditional3;

    return i;
}

unsigned int sNodeTree_create_do(unsigned int conditional, unsigned int block, sCLNodeType* type_)
{
    unsigned int i;
    unsigned int j;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_DO;

    gNodes[i].uValue.mDoBlock = block;

    ASSERT(type_ != NULL);
    gNodes[i].mType = *type_;

    gNodes[i].mLeft = conditional;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

/// for debug
char* node_type_string[NODE_TYPE_MAX] = {
    "NODE_TYPE_OPERAND", 
    "NODE_TYPE_VALUE", 
    "NODE_TYPE_STRING_VALUE", 
    "NODE_TYPE_VARIABLE_NAME", 
    "NODE_TYPE_ARRAY_VALUE", 
    "NODE_TYPE_DEFINE_VARIABLE_NAME", 
    "NODE_TYPE_FIELD", 
    "NODE_TYPE_CLASS_FIELD", 
    "NODE_TYPE_STORE_VARIABLE_NAME", 
    "NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME", 
    "NODE_TYPE_STORE_FIELD", 
    "NODE_TYPE_STORE_CLASS_FIELD", 
    "NODE_TYPE_CLASS_METHOD_CALL", 
    "NODE_TYPE_PARAM", 
    "NODE_TYPE_RETURN", 
    "NODE_TYPE_NEW", 
    "NODE_TYPE_METHOD_CALL", 
    "NODE_TYPE_SUPER", 
    "NODE_TYPE_INHERIT", 
    "NODE_TYPE_NULL", 
    "NODE_TYPE_TRUE", 
    "NODE_TYPE_FALSE", 
    "NODE_TYPE_FVALUE", 
    "NODE_TYPE_IF", 
    "NODE_TYPE_WHILE", 
    "NODE_TYPE_BREAK", 
    "NODE_TYPE_DO", 
    "NODE_TYPE_FOR",
    "NODE_TYPE_CONTINUE",
    "NODE_TYPE_BLOCK_CALL"
};

static void show_node(unsigned int node)
{
    compile_error("type %s left %d right %d middle %d\n", node_type_string[gNodes[node].mNodeType-1], gNodes[node].mLeft, gNodes[node].mRight, gNodes[node].mMiddle);
}

//////////////////////////////////////////////////
// Compile time block
//////////////////////////////////////////////////
sNodeBlock* gNodeBlocks;

static unsigned int gSizeBlocks;
static unsigned int gUsedBlocks;

static void init_node_blocks()
{
    const int block_size = 32;

    gNodeBlocks = CALLOC(1, sizeof(sNodeBlock)*block_size);
    gSizeBlocks = block_size;
    gUsedBlocks = 1;     // 0 of index means null
}

static void free_node_blocks()
{
    int i;
    for(i=1; i<gUsedBlocks; i++) {
        sNodeBlock* block = gNodeBlocks + i;

        FREE(block->mNodes);
    }

    FREE(gNodeBlocks);

    gSizeBlocks = 0;
    gUsedBlocks = 0;
}

static unsigned int alloc_node_block(sCLNodeType block_type)
{
    int size;
    int i;

    if(gUsedBlocks == gSizeBlocks) {
        int new_size;

        new_size = (gSizeBlocks+1) * 2;
        gNodeBlocks = REALLOC(gNodeBlocks, sizeof(sNodeBlock)*new_size);
        memset(gNodeBlocks + gSizeBlocks, 0, sizeof(sNodeBlock)*(new_size - gSizeBlocks));

        gSizeBlocks = new_size;
    }

    size = 16;
    gNodeBlocks[gUsedBlocks].mNodes = CALLOC(1, sizeof(sNode)*size);
    gNodeBlocks[gUsedBlocks].mSizeNodes = size;
    gNodeBlocks[gUsedBlocks].mLenNodes = 0;
    gNodeBlocks[gUsedBlocks].mBlockType = block_type;

    memset(&gNodeBlocks[gUsedBlocks].mLVTable, 0, sizeof(gNodeBlocks[gUsedBlocks].mLVTable));

    gNodeBlocks[gUsedBlocks].mNumParams = 0;
    memset(&gNodeBlocks[gUsedBlocks].mClassParams, 0, sizeof(gNodeBlocks[gUsedBlocks].mClassParams));

    gNodeBlocks[gUsedBlocks].mMaxStack = 0;
    gNodeBlocks[gUsedBlocks].mNumLocals = 0;

    return gUsedBlocks++;
}

static void append_node_to_node_block(unsigned int node_block_id, sNode* node)
{
    sNodeBlock* block = gNodeBlocks + node_block_id;

    if(block->mSizeNodes == block->mLenNodes) {
        int new_size = (block->mSizeNodes + 1) * 2;
        block->mNodes = REALLOC(block->mNodes, sizeof(sNode)*new_size);
        memset(block->mNodes + block->mSizeNodes, 0, sizeof(sNode)*(new_size - block->mSizeNodes));
        block->mSizeNodes = new_size;
    }

    block->mNodes[block->mLenNodes] = *node;
    block->mLenNodes++;
}

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

// left_type is stored calss. right_type is class of value.
static BOOL substition_posibility_of_class(sCLClass* left_type, sCLClass* right_type)
{
ASSERT(left_type != NULL);
ASSERT(right_type != NULL);

    /// null type is special ///
    if(right_type == gNullType.mClass) {
        if(search_for_super_class(left_type, gObjectType.mClass)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    /// there is compatibility of immediate value classes in the name space which is different ///
    if( ((left_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mFlags & CLASS_FLAGS_SPECIAL_CLASS)))
    {
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

// left_type is stored type. right_type is value type.
BOOL substition_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
ASSERT(left_type->mClass != NULL);
ASSERT(right_type->mClass != NULL);

    /// null type is special ///
    if(right_type->mClass == gNullType.mClass) {
        if(search_for_super_class(left_type->mClass, gObjectType.mClass)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    /// there is compatibility of immediate value classes and special classin the name space which is different ///
    else if(((left_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS))) 
    {
        if(strcmp(CLASS_NAME(left_type->mClass), CLASS_NAME(right_type->mClass)) != 0) {
            return FALSE;
        }
    }
    else {
        int i;

        if(left_type->mClass != right_type->mClass) {
            if(!search_for_super_class(right_type->mClass, left_type->mClass)) {
                return FALSE;
            }
        }
        if(left_type->mGenericsTypesNum != right_type->mGenericsTypesNum) {
            return FALSE;
        }

        for(i=0; i<left_type->mGenericsTypesNum; i++) {
            if(!substition_posibility_of_class(left_type->mGenericsTypes[i], right_type->mGenericsTypes[i])) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    /// there is compatibility of immediate value classes and special classin the name space which is different ///
    if(((left_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS))
        || ((left_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) && (right_type->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS))) 
    {
        if(strcmp(CLASS_NAME(left_type->mClass), CLASS_NAME(right_type->mClass)) != 0) {
            return FALSE;
        }
    }
    else {
        if(!type_identity(left_type, right_type)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2)
{
    int i;

    if(type1->mClass != type2->mClass) {
        return FALSE;
    }

    if(type1->mGenericsTypesNum != type2->mGenericsTypesNum) {
        return FALSE;
    }

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

struct sCompileInfoStruct {
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
    BOOL* exist_break;
    unsigned int* break_labels;
    int* break_labels_len;
    unsigned int* continue_labels;
    int* continue_labels_len;
    sCLNodeType* while_type;
};

typedef struct sCompileInfoStruct sCompileInfo;
static BOOL compile_block(sNodeBlock* block, sVarTable* new_table, sCLNodeType* type_, sCompileInfo* info);
static BOOL compile_block_of_method(sNodeBlock* block, sConst* constant, sByteCode* code, sVarTable* new_table, sCLNodeType* type_, sCompileInfo* info, sCLClass* caller_class, sCLMethod* caller_method);

static void show_caller_method(char* method_name, sCLNodeType* class_params, int num_params, BOOL existance_of_block, sCLNodeType* block_class_params, int block_num_params, sCLNodeType* block_type)
{
    int i;

    cl_print("called method type --> %s(", method_name);

    for(i=0; i<num_params; i++) {
        show_node_type(class_params + i);

        if(i != num_params-1) cl_print(",");
    }

    if(!existance_of_block) {
        cl_print(")\n");
    }
    else {
        cl_print(") with ");
        show_node_type(block_type);
        cl_print(" block{|");
        for(i=0; i<block_num_params; i++) {
            show_node_type(&block_class_params[i]);

            if(i != block_num_params-1) cl_print(",");
        }
        cl_print("|}\n");
    }
}

static BOOL do_call_method(sCLMethod* method, char* method_name, sCLClass* klass, BOOL class_method, BOOL calling_super, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info, unsigned int block_id, BOOL block_exist, int block_num_params, sCLNodeType* block_param_types, sCLNodeType* block_type)
{
    sCLNodeType result_type;
    int method_num_params;
    int i;
    int n;

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
    
    /// make block ///
    if(block_id) {
        sConst constant;
        sByteCode code;
        sVarTable new_table;

        sConst_init(&constant);
        sByteCode_init(&code);

        /// make var table ///
        ASSERT(info->lv_table != NULL);
        memset(&new_table, 0, sizeof(sVarTable));
        copy_var_table(&new_table, info->lv_table);

        /// compile block ///
        if(!compile_block_of_method(&gNodeBlocks[block_id], &constant, &code, &new_table, type_, info, klass, method)) {
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return TRUE;
        }

        append_opecode_to_bytecodes(info->code, OP_NEW_BLOCK);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(gBlockType.mClass));

        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mMaxStack);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumLocals);
        append_int_value_to_bytecodes(info->code, gNodeBlocks[block_id].mNumParams);

        append_constant_pool_to_bytecodes(info->code, info->constant, &constant);
        append_code_to_bytecodes(info->code, info->constant, &code);

        FREE(constant.mConst);
        FREE(code.mCode);
    }
    else {
        if(!calling_super && method->mNumBlockType == 1) {
            parser_err_msg_format(info->sname, *info->sline, "this method(%s::%s) should call with block", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    /// make code ///
    memset(&result_type, 0, sizeof(result_type));
    if(info->caller_class == type_->mClass) {
        if(!get_result_type_of_method(klass, method, &result_type, NULL)) {
            parser_err_msg_format(info->sname, *info->sline, "1 can't found result type of the method named %s.%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }
    else {
        if(!get_result_type_of_method(klass, method, &result_type, type_)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s.%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_METHOD);

    n = type_->mGenericsTypesNum;
    append_int_value_to_bytecodes(info->code, n);

    for(i=0; i<type_->mGenericsTypesNum; i++) {
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(type_->mGenericsTypes[i]));
    }

    append_str_to_bytecodes(info->code, info->constant, method_name);   // method name

    method_num_params = get_method_num_params(method);
    append_int_value_to_bytecodes(info->code, method_num_params); // method num params

    for(i=0; i<method_num_params; i++) {
        int j;

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(class_params[i].mClass));  // method params
        append_int_value_to_bytecodes(info->code, class_params[i].mGenericsTypesNum);

        for(j=0; j<class_params[i].mGenericsTypesNum; j++) {
            append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(class_params[i].mGenericsTypes[j]));
        }
    }

    if(block_exist) {
        append_int_value_to_bytecodes(info->code, 1);                                 // existance of block

        append_int_value_to_bytecodes(info->code, block_num_params);                  // method block num params

        for(i=0; i<block_num_params; i++) {
            int j;

            append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(block_param_types[i].mClass));  // method block params
            append_int_value_to_bytecodes(info->code, block_param_types[i].mGenericsTypesNum);

            for(j=0; j<block_param_types[i].mGenericsTypesNum; j++) {
                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(block_param_types[i].mGenericsTypes[j]));
            }
        }

        append_int_value_to_bytecodes(info->code, 1);                                 // the existance of block result

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(block_type->mClass));  // method block params
        append_int_value_to_bytecodes(info->code, block_type->mGenericsTypesNum);

        for(i=0; i<block_type->mGenericsTypesNum; i++) {
            append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(block_type->mGenericsTypes[i]));
        }
    }
    else {
        append_int_value_to_bytecodes(info->code, 0);                                 // existance of block
        append_int_value_to_bytecodes(info->code, 0);                                 // method block num params
        append_int_value_to_bytecodes(info->code, 0);                                 // the existance of block result
    }

    append_int_value_to_bytecodes(info->code, !substition_posibility(&result_type, &gVoidType)); // an existance of result flag
    append_int_value_to_bytecodes(info->code, calling_super);  // a flag of calling super
    append_int_value_to_bytecodes(info->code, class_method);  // a flag of class method kind
    append_int_value_to_bytecodes(info->code, method->mNumBlockType);       // method num block type

    if(class_method || (klass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS)) 
    {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_CLASS);

        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
    }
    else {
        append_int_value_to_bytecodes(info->code, INVOKE_METHOD_KIND_OBJECT);
    }

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params);
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1);
    }

    if(!substition_posibility(&result_type, &gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL do_call_inherit(sCLMethod* method, int method_index, sCLClass* klass, BOOL class_method, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType result_type;
    int method_num_params;
    int offset;

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
    memset(&result_type, 0, sizeof(result_type));
    if(info->caller_class == type_->mClass) {
        if(!get_result_type_of_method(klass, method, &result_type, NULL)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s.%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }
    else {
        if(!get_result_type_of_method(klass, method, &result_type, type_)) {
            parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s.%s", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            (*info->err_num)++;
            return TRUE;
        }
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_INHERIT);
    append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
    append_int_value_to_bytecodes(info->code, method_index);
    append_int_value_to_bytecodes(info->code, !substition_posibility(&result_type, &gVoidType));

    method_num_params = get_method_num_params(method);

    if(class_method) {
        dec_stack_num(info->stack_num, method_num_params);
    }
    else {
        dec_stack_num(info->stack_num, method_num_params+1);
    }

    if(!substition_posibility(&result_type, &gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL load_local_varialbe_from_var_index(int index, sCLNodeType* type_, sCompileInfo* info);

static BOOL params_of_cl_type_to_params_of_node_type(sCLNodeType* result, sCLType* params, int num_params, sCLClass* klass)
{
    int i;

    for(i=0; i<num_params; i++) {
        if(!cl_type_to_node_type(&result[i], &params[i], NULL, klass)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL call_method(sCLClass* klass, char* method_name, BOOL class_method, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info, unsigned int block_id)
{
    sCLMethod* method;
    int block_num_params;
    sCLNodeType* block_param_types;
    sCLNodeType* block_type;
    BOOL block_exist;

    if(klass == NULL || type_->mClass == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid method call. there is not type or class");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// get block params ///
    if(block_id) {
        block_exist = TRUE;

        block_num_params = gNodeBlocks[block_id].mNumParams;
        block_param_types = gNodeBlocks[block_id].mClassParams;
        block_type = &gNodeBlocks[block_id].mBlockType;
    }
    else {
        block_exist = FALSE;

        block_num_params = 0;
        block_param_types = NULL;
        block_type = NULL;
    }

    if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics types
        method = get_method_with_type_params(klass, method_name, class_params, *num_params, class_method, NULL, klass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type);
    }
    else {
        /// check generics type  ///
        if(klass->mGenericsTypesNum != type_->mGenericsTypesNum) {
            parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(klass));
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return TRUE;
        }

        method = get_method_with_type_params(klass, method_name, class_params, *num_params, class_method, type_, klass->mNumMethods-1, block_exist, block_num_params, block_param_types, block_type);
    }

    /// next, searched for super classes ///
    if(method == NULL) {
        /// search from super classes ///
        sCLClass* new_class;

        if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics types
            method = get_method_with_type_params_on_super_classes(klass, method_name, class_params, *num_params, &new_class, class_method, NULL, block_id != 0 ? 1: 0, block_num_params, block_param_types, block_type);
        }
        else {
            method = get_method_with_type_params_on_super_classes(klass, method_name, class_params, *num_params, &new_class, class_method, type_, block_id != 0 ? 1:0, block_num_params, block_param_types, block_type);
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
                show_all_method(klass, method_name);
                if(block_id) {
                    show_caller_method(method_name, class_params, *num_params, block_id, gNodeBlocks[block_id].mClassParams, gNodeBlocks[block_id].mNumParams, &gNodeBlocks[block_id].mBlockType);
                }
                else {
                    show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, NULL);
                }
                (*info->err_num)++;
            }
            else {
                sCLClass* new_class;

                method = get_method_on_super_classes(klass, method_name, &new_class);

                if(method) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid parametor types on this method(%s::%s)", CLASS_NAME(new_class), method_name);
                    show_all_method(new_class, method_name);
                    if(block_id) {
                        show_caller_method(method_name, class_params, *num_params, block_id, gNodeBlocks[block_id].mClassParams, gNodeBlocks[block_id].mNumParams, &gNodeBlocks[block_id].mBlockType);
                    }
                    else {
                        show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, NULL);
                    }

                    (*info->err_num)++;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "There is not this method(%s) on this class(%s)", method_name, REAL_CLASS_NAME(klass));
                    (*info->err_num)++;
                }
            }

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    if(!do_call_method(method, method_name, klass, class_method, FALSE, type_, class_params, num_params, info, block_id, block_exist, block_num_params, block_param_types, block_type)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL call_super(sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info, unsigned int block_id)
{
    sCLClass* klass;
    int caller_method_index;
    char* method_name;
    sCLClass* new_class;
    sCLMethod* method;
    int block_num_params;
    sCLNodeType block_param_types[CL_METHOD_PARAM_MAX];
    sCLNodeType block_type;
    BOOL block_exist;

    /// statically checking ///
    if(info->caller_class == NULL || info->caller_method == NULL) {
        parser_err_msg("can't call super because there are not the caller method or the caller class.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(info->caller_method->mFlags & CL_CLASS_METHOD) {
        parser_err_msg_format(info->sname, *info->sline, "can't call super because this method is class method(%s).", METHOD_NAME2(info->caller_class, info->caller_method));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(info->caller_class->mNumSuperClasses == 0) {
        parser_err_msg_format(info->sname, *info->sline, "there is not a super class of this class(%s).", REAL_CLASS_NAME(info->caller_class));
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// search for method ///
    klass = info->caller_class;

    if(info->caller_method->mFlags & CL_CONSTRUCTOR) {
        sCLClass* super_class = get_super(klass);
        method_name = CLASS_NAME(super_class);
    }
    else {
        caller_method_index = get_method_index(klass, info->caller_method);
        ASSERT(caller_method_index != -1);
        method_name = METHOD_NAME(klass, caller_method_index);
    }

    /// get block params ///
    if(block_id) {
        int j;

        block_exist = TRUE;

        block_num_params = gNodeBlocks[block_id].mNumParams;
        for(j=0; j<block_num_params; j++) {
            block_param_types[j] = gNodeBlocks[block_id].mClassParams[j];            // ! struct copy
        }
        block_type = gNodeBlocks[block_id].mBlockType;                      // ! struct copy
    }
    /// Does block exist at caller method ?. if it is true, use it for method searching ///
    else if(klass->mMethods[caller_method_index].mNumBlockType > 0) {
        int block_var_index;

        block_exist = TRUE;

        block_var_index = ((klass->mMethods[caller_method_index].mFlags & CL_CLASS_METHOD) ? 0:1) + klass->mMethods[caller_method_index].mNumParams;

        if(!load_local_varialbe_from_var_index(block_var_index, type_, info)) {    // load block
            parser_err_msg("can't load block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }

        block_num_params = klass->mMethods[caller_method_index].mBlockType.mNumParams;

        if(!params_of_cl_type_to_params_of_node_type(block_param_types, klass->mMethods[caller_method_index].mBlockType.mParamTypes, klass->mMethods[caller_method_index].mBlockType.mNumParams, klass))
        {
            parser_err_msg("can't load params of block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }

        if(!cl_type_to_node_type(&block_type, &klass->mMethods[caller_method_index].mBlockType.mResultType, NULL, klass)) {
            parser_err_msg("can't load result type of block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }
    }
    else {
        block_exist = FALSE;
        
        block_num_params = 0;
        memset(block_param_types, 0, sizeof(block_param_types));
        memset(&block_type, 0, sizeof(sCLNodeType));
    }

    /// search from super classes ///
    if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics type
        method = get_method_with_type_params_on_super_classes(klass, method_name, class_params, *num_params, &new_class, FALSE, NULL, block_exist, block_num_params, block_param_types, &block_type);
    }
    else {
        /// check generics type  ///
        if(klass->mGenericsTypesNum != type_->mGenericsTypesNum) {
            parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(klass));
            (*info->err_num)++;
        }

        method = get_method_with_type_params_on_super_classes(klass, method_name, class_params, *num_params, &new_class, FALSE, type_, block_exist, block_num_params, block_param_types, &block_type);
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
            show_all_method(new_class, method_name);
            if(block_exist) {
                show_caller_method(method_name, class_params, *num_params, TRUE, block_param_types, block_num_params, &block_type);
            }
            else {
                show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, NULL);
            }
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

    if(!do_call_method(method, method_name, klass, FALSE, TRUE, type_, class_params, num_params, info, block_id, block_exist, block_num_params, block_param_types, &block_type))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL call_inherit(sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info, unsigned int block_id)
{
    int caller_method_index;
    char* method_name;
    sCLMethod* method;
    int method_index;
    sCLClass* klass;
    int block_num_params;
    sCLNodeType block_param_types[CL_METHOD_PARAM_MAX];
    sCLNodeType block_type;
    BOOL block_exist;

    if(info->caller_class == NULL || info->caller_method == NULL) {
        parser_err_msg("can't call inherit method because there are not the caller method or the caller class.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    caller_method_index = get_method_index(info->caller_class, info->caller_method);
    ASSERT(caller_method_index != -1);

    method_name = METHOD_NAME(info->caller_class, caller_method_index);
    klass = info->caller_class;

    /// get block params ///
    if(block_id) {
        int j;

        block_exist = TRUE;

        block_num_params = gNodeBlocks[block_id].mNumParams;
        for(j=0; j<block_num_params; j++) {
            block_param_types[j] = gNodeBlocks[block_id].mClassParams[j];            // ! struct copy
        }
        block_type = gNodeBlocks[block_id].mBlockType;                      // ! struct copy
    }
    /// Does block exist at caller method ?. if it is true, use it for method searching ///
    else if(klass->mMethods[caller_method_index].mNumBlockType > 0) {
        int block_var_index;

        block_exist = TRUE;

        block_var_index = ((klass->mMethods[caller_method_index].mFlags & CL_CLASS_METHOD) ? 0:1) + klass->mMethods[caller_method_index].mNumParams;

        if(!load_local_varialbe_from_var_index(block_var_index, type_, info)) {    // load block
            parser_err_msg("can't load block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }

        block_num_params = klass->mMethods[caller_method_index].mBlockType.mNumParams;

        if(!params_of_cl_type_to_params_of_node_type(block_param_types, klass->mMethods[caller_method_index].mBlockType.mParamTypes, klass->mMethods[caller_method_index].mBlockType.mNumParams, klass))
        {
            parser_err_msg("can't load params of block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }

        if(!cl_type_to_node_type(&block_type, &klass->mMethods[caller_method_index].mBlockType.mResultType, NULL, klass)) {
            parser_err_msg("can't load result type of block", info->sname, *info->sline);
            (*info->err_num)++;
            *type_ = gIntType; // dummy
            return FALSE;
        }
    }
    else {
        block_exist = FALSE;

        block_num_params = 0;
        memset(block_param_types, 0, sizeof(block_param_types));
        memset(&block_type, 0, sizeof(sCLNodeType));
    }

    if(info->caller_class == type_->mClass) {  // if it is true, don't solve generics type
        method = get_method_with_type_params(info->caller_class, method_name, class_params, *num_params, info->caller_method->mFlags & CL_CLASS_METHOD, NULL, caller_method_index-1, block_exist, block_num_params, block_param_types, &block_type);
    }
    else {
        method = get_method_with_type_params(info->caller_class, method_name, class_params, *num_params, info->caller_method->mFlags & CL_CLASS_METHOD, type_, caller_method_index-1, block_exist, block_num_params, block_param_types, &block_type);
    }

    method_index = get_method_index(info->caller_class, method);

    if(method == NULL) {
        method_index = get_method_index_from_the_parametor_point(info->caller_class, method_name, caller_method_index, info->caller_method->mFlags & CL_CLASS_METHOD);

        if(method_index != -1) {
            parser_err_msg_format(info->sname, *info->sline, "can't inherit. Invalid parametor types on this method(%s::%s)", CLASS_NAME(info->caller_class), method_name);
            show_all_method(info->caller_class, method_name);
            if(block_exist) {
                show_caller_method(method_name, class_params, *num_params, TRUE, block_param_types, block_num_params, &block_type);
            }
            else {
                show_caller_method(method_name, class_params, *num_params, FALSE, NULL, 0, NULL);
            }
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

    if(!do_call_inherit(method, method_index, klass, method->mFlags & CL_CLASS_METHOD, type_, class_params, num_params, info))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL call_method_block(sCLClass* klass, sCLNodeType* type_, sCLMethod* method, char* block_name, sCLNodeType* class_params, int* num_params, sCompileInfo* info, BOOL static_method)
{
    sCLNodeType result_type;
    sVar* var;
    int i;

    if(klass == NULL || type_->mClass == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not caller class");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(method == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not caller method");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(method->mNumBlockType == 0) {
        parser_err_msg_format(info->sname, *info->sline, "Invalid block call. there is not block");
        (*info->err_num)++;

        *type_ = gIntType;
        return TRUE;
    }
    
    var = get_variable_from_table(info->lv_table, block_name);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", block_name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// type checking ///
    if(*num_params != method->mBlockType.mNumParams) {
        parser_err_msg_format(info->sname, *info->sline, "invalid block call because of the number of block parametors.");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    for(i=0; i<*num_params; i++) {
        sCLNodeType node_type;

        memset(&node_type, 0, sizeof(node_type));

        if(!cl_type_to_node_type(&node_type, &method->mBlockType.mParamTypes[i], NULL, klass)) {
            parser_err_msg_format(info->sname, *info->sline, "invalid block of method.");
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }

        if(!substition_posibility(&node_type, &class_params[i])) {
            parser_err_msg_format(info->sname, *info->sline, "type error of block call");
            cl_print("left type is ");
            show_node_type(&node_type);
            cl_print(". right type is ");
            show_node_type(&class_params[i]);
            (*info->err_num)++;

            *type_ = gIntType; // dummy
            return TRUE;
        }
    }

    /// make code ///
    memset(&result_type, 0, sizeof(result_type));

    if(!cl_type_to_node_type(&result_type, &method->mBlockType.mResultType, NULL, klass)) {
        parser_err_msg_format(info->sname, *info->sline, "can't get the result type of method(%s::%s) block", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    append_opecode_to_bytecodes(info->code, OP_INVOKE_BLOCK);
    append_int_value_to_bytecodes(info->code, var->mIndex);
    append_int_value_to_bytecodes(info->code, !substition_posibility(&result_type, &gVoidType));
    append_int_value_to_bytecodes(info->code, static_method);

    if(!static_method) {
        dec_stack_num(info->stack_num, 1);
    }
    dec_stack_num(info->stack_num, method->mBlockType.mNumParams);

    if(!substition_posibility(&result_type, &gVoidType)) {
        inc_stack_num(info->stack_num, info->max_stack, 1);
    }

    *type_ = result_type;

    return TRUE;
}

static BOOL compile_node(unsigned int node, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info);

static BOOL compile_left_node(unsigned int node, sCLNodeType* left_type, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mLeft) {
        if(!compile_node(gNodes[node].mLeft, left_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_right_node(unsigned int node, sCLNodeType* right_type, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mRight) {
        if(!compile_node(gNodes[node].mRight, right_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_middle_node(unsigned int node, sCLNodeType* middle_type, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    if(gNodes[node].mMiddle) {
        if(!compile_node(gNodes[node].mMiddle, middle_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL load_local_varialbe(char* name, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sVar* var;

    var = get_variable_from_table(info->lv_table, name);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(substition_posibility(&var->mType, &gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ILOAD);
    }
    else if(substition_posibility(&var->mType, &gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ALOAD);
    }
    else if(substition_posibility(&var->mType, &gFloatType)) {
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

static BOOL load_local_varialbe_from_var_index(int index, sCLNodeType* type_, sCompileInfo* info)
{
    sVar* var;

    var = get_variable_from_table_by_var_index(info->lv_table, index);

    if(var == NULL) {
        parser_err_msg_format(info->sname, *info->sline, "there is not this variable index(%d)", index);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }
    if(substition_posibility(&var->mType, &gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ILOAD);
    }
    else if(substition_posibility(&var->mType, &gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ALOAD);
    }
    else if(substition_posibility(&var->mType, &gFloatType)) {
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

static BOOL binary_operator_add(sCLNodeType left_type, sCLNodeType right_type, sCLNodeType* type_, sCompileInfo* info)
{
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no class type", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        if(operand_posibility(&left_type, &gStringType) && operand_posibility(&right_type, &gStringType)) {
            append_opecode_to_bytecodes(info->code, OP_SADD);
            *type_ = gStringType;
            dec_stack_num(info->stack_num, 1);
        }
        else if(operand_posibility(&left_type, &gIntType) && operand_posibility(&right_type, &gIntType)) {
            append_opecode_to_bytecodes(info->code, OP_IADD);
            *type_ = gIntType;
            dec_stack_num(info->stack_num, 1);
        }
        else if(operand_posibility(&left_type, &gFloatType) && operand_posibility(&right_type, &gFloatType)) {
            append_opecode_to_bytecodes(info->code, OP_FADD);
            *type_ = gFloatType;
            dec_stack_num(info->stack_num, 1);
        }
        else {
            sCLNodeType class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            *type_ = left_type;

            class_params2[0] = right_type;
            num_params2 = 1;

            if(!call_method(type_->mClass, "+", FALSE, type_, class_params2, &num_params2, info, 0)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL binary_operator(sCLNodeType left_type, sCLNodeType right_type, sCLNodeType* type_, sCompileInfo* info, int op_int, int op_float, char* operation_name, char* operand_symbol, sCLNodeType* int_result_type, sCLNodeType* float_result_type)
{
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no class type", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        if(operand_posibility(&left_type, &gIntType) && operand_posibility(&right_type, &gIntType)) {
            append_opecode_to_bytecodes(info->code, op_int);
            *type_ = *int_result_type;
            dec_stack_num(info->stack_num, 1);
        }
        else if(operand_posibility(&left_type, &gFloatType) && operand_posibility(&right_type, &gFloatType)) {
            append_opecode_to_bytecodes(info->code, op_float);
            *type_ = *float_result_type;
            dec_stack_num(info->stack_num, 1);
        }
        else {
            sCLNodeType class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            *type_ = left_type;

            class_params2[0] = right_type;
            num_params2 = 1;

            if(!call_method(type_->mClass, operand_symbol, FALSE, type_, class_params2, &num_params2, info, 0)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL binary_operator_without_float(sCLNodeType left_type, sCLNodeType right_type, sCLNodeType* type_, sCompileInfo* info, int op_int, char* operation_name, char* operand_symbol, sCLNodeType* int_result_type)
{
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no class type", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        if(operand_posibility(&left_type, &gIntType) && operand_posibility(&right_type, &gIntType)) {
            append_opecode_to_bytecodes(info->code, op_int);
            *type_ = *int_result_type;
            dec_stack_num(info->stack_num, 1);
        }
        else {
            sCLNodeType class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            *type_ = left_type;

            class_params2[0] = right_type;
            num_params2 = 1;

            if(!call_method(type_->mClass, operand_symbol, FALSE, type_, class_params2, &num_params2, info, 0)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL binary_operator_bool(sCLNodeType left_type, sCLNodeType right_type, sCLNodeType* type_, sCompileInfo* info, int op_int, char* operation_name, char* operand_symbol)
{
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no class type", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
    }
    else {
        if(operand_posibility(&left_type, &gBoolType) && operand_posibility(&right_type, &gBoolType)) {
            append_opecode_to_bytecodes(info->code, op_int);
            *type_ = gBoolType;
            dec_stack_num(info->stack_num, 1);
        }
        else {
            sCLNodeType class_params2[CL_METHOD_PARAM_MAX];
            int num_params2;

            *type_ = left_type;

            class_params2[0] = right_type;
            num_params2 = 1;

            if(!call_method(type_->mClass, operand_symbol, FALSE, type_, class_params2, &num_params2, info, 0)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL store_local_variable(char* name, sVar* var, sCLNodeType left_type, unsigned int node, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType right_type;

    /// load self ///
    if(gNodes[node].uValue.sVarName.mNodeSubstitutionType != kNSNone) {
        if(!load_local_varialbe(name, type_, class_params, num_params, info)) {
            return FALSE;
        }
    }

    /// a right value goes ///
    memset(&right_type, 0, sizeof(right_type));
    if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
        return FALSE;
    }

    /// operand ///
    switch((int)gNodes[node].uValue.sVarName.mNodeSubstitutionType) {
        case kNSPlus:
            if(!binary_operator_add(left_type, right_type, type_, info)) {
                return FALSE;
            }
            break;
            
        case kNSMinus:
            if(!binary_operator(left_type, right_type, type_, info, OP_ISUB, OP_FSUB, "substraction", "-", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSMult:
            if(!binary_operator(left_type, right_type, type_, info, OP_IMULT, OP_FMULT, "multiplication", "*", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSDiv:
            if(!binary_operator(left_type, right_type, type_, info, OP_IDIV, OP_FDIV, "division", "/", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSMod:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IMOD, "modulo operation", "%", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSLShift:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_ILSHIFT, "left shift", "<<", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSRShift:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IRSHIFT, "right shift", ">>", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSAnd:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IAND, "and operation", "&", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSXor:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IXOR, "xor operation", "^", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSOr:
            if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IOR, "or operation", "|", &gIntType)) {
                return FALSE;
            }
            break;
        
    }

    /// type checking ///
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substition_posibility(&left_type, &right_type)) {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        cl_print("left type is ");
        show_node_type(&left_type);
        cl_print(". right type is ");
        show_node_type(&right_type);
        puts("");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// append opecode to bytecodes ///
    if(substition_posibility(&left_type, &gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ISTORE);
    }
    else if(substition_posibility(&left_type, &gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ASTORE);
    }
    else if(substition_posibility(&left_type, &gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FSTORE);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OSTORE);
    }

    append_int_value_to_bytecodes(info->code, var->mIndex);

    *type_ = var->mType;

    return TRUE;
}

static BOOL load_field(sCLClass* klass, char* field_name, BOOL class_field, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType field_type;

    if(klass == NULL) {
        parser_err_msg("left value has not class. can't get field", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy

        return TRUE;
    }

    memset(&field_type, 0, sizeof(field_type));

    if(class_field) {
        field = get_field(klass, field_name);
        field_index = get_field_index(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(klass, field, &field_type, NULL);
            }
            else {
                /// check generics type  ///
                if(type_->mGenericsTypesNum != klass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(klass));
                    (*info->err_num)++;
                }

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
                /// check generics type  ///
                if(type_->mGenericsTypesNum != klass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(found_class));
                    (*info->err_num)++;
                }

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

    if(field_type.mClass == NULL || substition_posibility(&field_type, &gVoidType)) {
        parser_err_msg("this field has not class", info->sname, *info->sline);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_LD_STATIC_FIELD);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
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

static BOOL increase_or_decrease_local_variable(char* name, sVar* var, sCLNodeType left_type, unsigned int node, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLNodeType right_type;

    /// load self ///
    if(!load_local_varialbe(name, type_, class_params, num_params, info)) {
        return FALSE;
    }

    /// load constant number as 1 value ///
    append_opecode_to_bytecodes(info->code, OP_LDCINT);
    append_int_value_to_bytecodes(info->code, 1);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    right_type = gIntType;

    /// operand ///
    switch((int)gNodes[node].uValue.mOperand) {
        case kOpPlusPlus:
        case kOpPlusPlus2:
            if(!binary_operator_add(left_type, right_type, type_, info)) {
                return FALSE;
            }
            break;
            
        case kOpMinusMinus:
        case kOpMinusMinus2:
            if(!binary_operator(left_type, right_type, type_, info, OP_ISUB, OP_FSUB, "substraction", "-", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
    }

    /// type checking ///
    if(left_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substition_posibility(&left_type, &right_type)) {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        cl_print("left type is ");
        show_node_type(&left_type);
        cl_print(". right type is ");
        show_node_type(&right_type);
        puts("");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    /// append opecode to bytecodes ///
    if(substition_posibility(&left_type, &gIntType)) {
        append_opecode_to_bytecodes(info->code, OP_ISTORE);
    }
    else if(substition_posibility(&left_type, &gStringType)) {
        append_opecode_to_bytecodes(info->code, OP_ASTORE);
    }
    else if(substition_posibility(&left_type, &gFloatType)) {
        append_opecode_to_bytecodes(info->code, OP_FSTORE);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_OSTORE);
    }

    append_int_value_to_bytecodes(info->code, var->mIndex);

    /// operand ///
    switch((int)gNodes[node].uValue.mOperand) {
        case kOpPlusPlus2:
            append_opecode_to_bytecodes(info->code, OP_DEC_VALUE);
            append_int_value_to_bytecodes(info->code, 1);
            break;
            
        case kOpMinusMinus2:
            append_opecode_to_bytecodes(info->code, OP_INC_VALUE);
            append_int_value_to_bytecodes(info->code, 1);
            break;
    }

    *type_ = var->mType;

    return TRUE;
}

static BOOL store_field(unsigned int node, sCLClass* klass, char* field_name, BOOL class_field, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType field_type;
    sCLNodeType right_type;
    sCLNodeType dummy_type;

    memset(&field_type, 0, sizeof(field_type));

    /// load field ///
    if(gNodes[node].uValue.sVarName.mNodeSubstitutionType != kNSNone) {
        if(!class_field) {  // require compiling left node for load field
            sCLNodeType dummy_type2;

            memset(&dummy_type2, 0, sizeof(dummy_type2));
            if(!compile_left_node(node, &dummy_type2, class_params, num_params, info)) {
                return FALSE;
            }
        }

        dummy_type = *type_;
        if(!load_field(klass, field_name, class_field, &dummy_type, class_params, num_params, info))
        {
            return FALSE;
        }
    }

    /// right value ///
    memset(&right_type, 0, sizeof(right_type));
    if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
        return FALSE;
    }

    /// get field type ////
    if(class_field) {
        field = get_field(klass, field_name);
        field_index = get_field_index(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(klass, field, &field_type, NULL);
            }
            else {
                /// check generics type  ///
                if(type_->mGenericsTypesNum != klass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(klass));
                    (*info->err_num)++;
                }

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
                /// check generics type  ///
                if(type_->mGenericsTypesNum != found_class->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(found_class));
                    (*info->err_num)++;
                }

                get_field_type(found_class, field, &field_type, type_);
            }
        }
    }

    /// operand ///
    dummy_type = *type_;
    switch((int)gNodes[node].uValue.sVarName.mNodeSubstitutionType) {
        case kNSPlus:
            if(!binary_operator_add(field_type, right_type, &dummy_type, info)) {
                return FALSE;
            }
            break;
            
        case kNSMinus:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_ISUB, OP_FSUB, "substraction", "-", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSMult:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IMULT, OP_FMULT, "multiplication", "*", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSDiv:
            if(!binary_operator(field_type, right_type, &dummy_type, info, OP_IDIV, OP_FDIV, "division", "/", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
            
        case kNSMod:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_IMOD, "modulo operation", "%", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSLShift:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_ILSHIFT, "left shift", "<<", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSRShift:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_IRSHIFT, "right shift", ">>", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSAnd:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_IAND, "and operation", "&", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSXor:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_IXOR, "xor operation", "^", &gIntType)) {
                return FALSE;
            }
            break;
            
        case kNSOr:
            if(!binary_operator_without_float(field_type, right_type, &dummy_type, info, OP_IOR, "or operation", "|", &gIntType)) {
                return FALSE;
            }
            break;
        
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
    if(field_type.mClass == NULL || substition_posibility(&field_type, &gVoidType)) {
        parser_err_msg("this field has no type.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substition_posibility(&field_type, &right_type)) {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        cl_print("left type is ");
        show_node_type(&field_type);
        cl_print(". right type is ");
        show_node_type(&right_type);
        puts("");

        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_SR_STATIC_FIELD);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
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

static BOOL increase_or_decrease_field(unsigned int node, unsigned int left_node, sCLClass* klass, char* field_name, BOOL class_field, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    sCLField* field;
    int field_index;
    sCLNodeType field_type;
    sCLNodeType right_type;
    sCLNodeType dummy_type;

    memset(&field_type, 0, sizeof(field_type));

    /// load field ///
    if(!class_field) {  // require compiling left node for load field
        sCLNodeType dummy_type2;

        memset(&dummy_type2, 0, sizeof(dummy_type2));
        if(!compile_left_node(left_node, &dummy_type2, class_params, num_params, info)) {
            return FALSE;
        }
    }

    dummy_type = *type_;
    if(!load_field(klass, field_name, class_field, &dummy_type, class_params, num_params, info))
    {
        return FALSE;
    }

    /// load constant number as 1 value ///
    append_opecode_to_bytecodes(info->code, OP_LDCINT);
    append_int_value_to_bytecodes(info->code, 1);

    inc_stack_num(info->stack_num, info->max_stack, 1);

    right_type = gIntType;

    /// get field type ////
    if(class_field) {
        field = get_field(klass, field_name);
        field_index = get_field_index(klass, field_name);
        if(field) {
            if(info->caller_class == type_->mClass) { // if it is true, don't solve generics types
                get_field_type(klass, field, &field_type, NULL);
            }
            else {
                /// check generics type  ///
                if(type_->mGenericsTypesNum != klass->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(klass));
                    (*info->err_num)++;
                }

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
                /// check generics type  ///
                if(type_->mGenericsTypesNum != found_class->mGenericsTypesNum) {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(found_class));
                    (*info->err_num)++;
                }

                get_field_type(found_class, field, &field_type, type_);
            }
        }
    }

    /// operand ///
    switch((int)gNodes[node].uValue.mOperand) {
        case kOpPlusPlus:
        case kOpPlusPlus2:
            if(!binary_operator_add(field_type, right_type, type_, info)) {
                return FALSE;
            }
            break;
            
        case kOpMinusMinus:
        case kOpMinusMinus2:
            if(!binary_operator(field_type, right_type, type_, info, OP_ISUB, OP_FSUB, "substraction", "-", &gIntType, &gFloatType)) {
                return FALSE;
            }
            break;
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
    if(field_type.mClass == NULL || substition_posibility(&field_type, &gVoidType)) {
        parser_err_msg("this field has no type.", info->sname, *info->sline);
        (*info->err_num)++;
        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(field_type.mClass == NULL || right_type.mClass == NULL) {
        parser_err_msg("no type left or right value", info->sname, *info->sline);
        return TRUE;
    }
    if(!substition_posibility(&field_type, &right_type)) {
        parser_err_msg_format(info->sname, *info->sline, "type error.");
        cl_print("left type is ");
        show_node_type(&field_type);
        cl_print(". right type is ");
        show_node_type(&right_type);
        puts("");

        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return TRUE;
    }

    if(class_field) {
        append_opecode_to_bytecodes(info->code, OP_SR_STATIC_FIELD);
        append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
        append_int_value_to_bytecodes(info->code, field_index);
    }
    else {
        append_opecode_to_bytecodes(info->code, OP_SRFIELD);
        append_int_value_to_bytecodes(info->code, field_index);

        dec_stack_num(info->stack_num, 1);
    }

    /// operand ///
    switch((int)gNodes[node].uValue.mOperand) {
        case kOpPlusPlus2:
            append_opecode_to_bytecodes(info->code, OP_DEC_VALUE);
            append_int_value_to_bytecodes(info->code, 1);
            break;
            
        case kOpMinusMinus2:
            append_opecode_to_bytecodes(info->code, OP_INC_VALUE);
            append_int_value_to_bytecodes(info->code, 1);
            break;
    }

    *type_ = field_type;

    return TRUE;
}

static void prepare_for_break_labels(unsigned int** break_labels_before, int** break_labels_len_before, unsigned int break_labels[], int* break_labels_len, sCompileInfo* info)
{
    *break_labels_len = 0;

    *break_labels_before = info->break_labels;
    *break_labels_len_before = info->break_labels_len;
    info->break_labels = break_labels;                               // for NODE_TYPE_BREAK to determine the goto point
    info->break_labels_len = break_labels_len;
}

static void determine_the_goto_point_of_break(unsigned int* break_labels_before, int* break_labels_len_before, sCompileInfo* info)
{
    int j;
    for(j=0; j<*info->break_labels_len; j++) {
        *(info->code->mCode + info->break_labels[j]) = info->code->mLen;   // for the label of goto when break is caleld. see NODE_TYPE_BREAK
    }

    info->break_labels = break_labels_before;               // restore the value
    info->break_labels_len = break_labels_len_before;
}

// FALSE: overflow break labels TRUE: success
static BOOL set_zero_goto_point_of_break(sCompileInfo* info)
{
    append_opecode_to_bytecodes(info->code, OP_GOTO);

    info->break_labels[*info->break_labels_len] = info->code->mLen;  // after compiling while loop, this is setted on the value of loop out. see NODE_TYPE_WHILE
    (*info->break_labels_len)++;

    if(*info->break_labels_len >= CL_BREAK_MAX) {
        return FALSE;
    }
    append_int_value_to_bytecodes(info->code, 0);

    return TRUE;
}

static void prepare_for_continue_labels(unsigned int** continue_labels_before, int** continue_labels_len_before, unsigned int continue_labels[], int* continue_labels_len, sCompileInfo* info)
{
    *continue_labels_len = 0;

    *continue_labels_before = info->continue_labels;
    *continue_labels_len_before = info->continue_labels_len;
    info->continue_labels = continue_labels;                               // for NODE_TYPE_CONTINUE to determine the jump point
    info->continue_labels_len = continue_labels_len;
}

static void determine_the_goto_point_of_continue(unsigned int* continue_labels_before, int* continue_labels_len_before, sCompileInfo* info)
{
    int j;
    for(j=0; j<*info->continue_labels_len; j++) {
        *(info->code->mCode + info->continue_labels[j]) = info->code->mLen;
    }

    info->continue_labels = continue_labels_before;               // restore the value
    info->continue_labels_len = continue_labels_len_before;
}

// FALSE: overflow continue labels TRUE: success
static BOOL set_zero_goto_point_of_continue(sCompileInfo* info)
{
    append_opecode_to_bytecodes(info->code, OP_GOTO);

    info->continue_labels[*info->continue_labels_len] = info->code->mLen;  // after compiling while loop, this is setted on the value of loop out. see NODE_TYPE_WHILE
    (*info->continue_labels_len)++;

    if(*info->continue_labels_len >= CL_BREAK_MAX) {
        return FALSE;
    }
    append_int_value_to_bytecodes(info->code, 0);

    return TRUE;
}

static BOOL compile_expressiont_in_loop(unsigned int conditional_node, sCLNodeType* conditional_type, sCLNodeType* class_params, int* num_params, sCompileInfo* info, sVarTable* new_table)
{
    sVarTable* lv_table_before;
    int* stack_num_before;
    int conditional_stack_num;

    lv_table_before = info->lv_table;
    info->lv_table = new_table;

    if(conditional_node) {
        if(!compile_node(conditional_node, conditional_type, class_params, num_params, info)) {
            return FALSE;
        }
    }

    info->lv_table = lv_table_before;

    return TRUE;
}

static BOOL compile_conditional(unsigned int conditional_node, sCLNodeType* conditional_type, sCLNodeType* class_params, int* num_params, sCompileInfo* info, sVarTable* new_table, sCLNodeType* type_)
{
    sVarTable* lv_table_before;
    int* stack_num_before;
    int conditional_stack_num;

    memset(conditional_type, 0, sizeof(*conditional_type));

    lv_table_before = info->lv_table;

    if(new_table) { info->lv_table = new_table; }

    stack_num_before = info->stack_num;
    conditional_stack_num = 0;
    info->stack_num = &conditional_stack_num;

    ASSERT(conditional_node != 0);

    if(!compile_node(conditional_node, conditional_type, class_params, num_params, info)) {
        return FALSE;
    }

    info->stack_num = stack_num_before;

    if(conditional_stack_num != 1) {
        parser_err_msg_format(info->sname, *info->sline, "stack error. conditional stack num is %d. this should be 1.\n.", conditional_stack_num);
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return FALSE;
    }

    if(!type_identity(conditional_type, &gBoolType)) {
        parser_err_msg_format(info->sname, *info->sline, "require the bool type for conditional");
        (*info->err_num)++;

        *type_ = gIntType; // dummy
        return FALSE;
    }

    info->lv_table = lv_table_before;

    return TRUE;
}

static BOOL compile_node(unsigned int node, sCLNodeType* type_, sCLNodeType* class_params, int* num_params, sCompileInfo* info)
{
    if(node == 0) {
        parser_err_msg("no expression", info->sname, *info->sline);
        (*info->err_num)++;
        return TRUE;
    }

    switch(gNodes[node].mNodeType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCINT);
            append_int_value_to_bytecodes(info->code, gNodes[node].uValue.mValue);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gIntType;
            }
            break;

        /// number float value ///
        case NODE_TYPE_FVALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCFLOAT);
            offset = append_float_value_to_constant_pool(info->constant, gNodes[node].uValue.mFValue);
            append_int_value_to_bytecodes(info->code, offset);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gFloatType;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
            int offset;

            append_opecode_to_bytecodes(info->code, OP_LDCWSTR);
            offset = append_wstr_to_constant_pool(info->constant, gNodes[node].uValue.mStringValue);
            append_int_value_to_bytecodes(info->code, offset);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gStringType;
            }
            break;

        /// array value ///
        case NODE_TYPE_ARRAY_VALUE: {
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            sCLClass* klass;

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// elements go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            klass = gNodes[node].mType.mClass;

            append_opecode_to_bytecodes(info->code, OP_NEW_ARRAY);
            append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
            append_int_value_to_bytecodes(info->code, num_params);

            dec_stack_num(info->stack_num, num_params);
            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gArrayType;
            }
            break;

        /// null value ///
        case NODE_TYPE_NULL: {
            append_opecode_to_bytecodes(info->code, OP_LDCINT);
            append_int_value_to_bytecodes(info->code, 0);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gNullType;
            }
            break;

        /// true value ///
        case NODE_TYPE_TRUE: {
            append_opecode_to_bytecodes(info->code, OP_LDCINT);
            append_int_value_to_bytecodes(info->code, 1);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gBoolType;
            }
            break;

        /// false value ///
        case NODE_TYPE_FALSE: {
            append_opecode_to_bytecodes(info->code, OP_LDCINT);
            append_int_value_to_bytecodes(info->code, 0);

            inc_stack_num(info->stack_num, info->max_stack, 1);

            *type_ = gBoolType;
            }
            break;

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
            char* name;
            sVar* var;
            sCLNodeType* type2;

            name = gNodes[node].uValue.sVarName.mVarName;
            type2 = &gNodes[node].mType;
            var = get_variable_from_table(info->lv_table, name);

            /// check generics type  ///
            if(type2->mClass->mGenericsTypesNum != type2->mGenericsTypesNum) {
                parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(type2->mClass));
                (*info->err_num)++;
            }

            if(var) {
                parser_err_msg_format(info->sname, *info->sline, "there is a same name variable(%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
            }
            else if(*info->err_num > 0) {
                *type_ = gIntType; // dummy
            }
            else {
                if(!add_variable_to_table(info->lv_table, name, type2)) {
                    parser_err_msg("overflow variable table", info->sname, *info->sline);
                    return FALSE;
                }

                *type_ = *type2;
            }
            }
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
            sCLNodeType left_type;
            
            name = gNodes[node].uValue.sVarName.mVarName;

            var = get_variable_from_table(info->lv_table, name);

            if(var == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                return TRUE;
            }

            left_type = var->mType;

            if(!store_local_variable(name, var, left_type, node, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// define variable and store a value ///
        case NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME: {
            char* name;
            sCLNodeType* type2;
            sVar* var;
            sCLNodeType left_type;

            /// define variable ///
            name = gNodes[node].uValue.sVarName.mVarName;
            type2 = &gNodes[node].mType;

            var = get_variable_from_table(info->lv_table, name);

            /// check generics type  ///
            if(type2->mClass->mGenericsTypesNum != type2->mGenericsTypesNum) {
                parser_err_msg_format(info->sname, *info->sline, "Invalid generics types number(%s)", REAL_CLASS_NAME(type2->mClass));
                (*info->err_num)++;
            }

            if(var) {
                parser_err_msg_format(info->sname, *info->sline, "there is a same name variable(%s)", name);
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }
            else if(*info->err_num > 0) {
                *type_ = gIntType; // dummy
                break;
            }
            else {
                if(!add_variable_to_table(info->lv_table, name, type2)) {
                    parser_err_msg("overflow variable table", info->sname, *info->sline);
                    return FALSE;
                }
            }

            var = get_variable_from_table(info->lv_table, name);

            ASSERT(var);

            /// store ///
            left_type = var->mType;

            if(!store_local_variable(name, var, left_type, node, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;
        
        /// load field  ///
        case NODE_TYPE_FIELD: {
            /// left_value ///
            sCLNodeType left_type;
            sCLClass* klass;
            char* field_name;

            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            klass = left_type.mClass;
            field_name = gNodes[node].uValue.sVarName.mVarName;
            *type_ = left_type;

            if(!load_field(klass, field_name, FALSE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// load class field ///
        case NODE_TYPE_CLASS_FIELD: {
            sCLClass* klass;
            char* field_name;
            
            klass = gNodes[node].mType.mClass;
            field_name = gNodes[node].uValue.sVarName.mVarName;

            ASSERT(klass); // must be not NULL
            *type_ = gNodes[node].mType;

            if(!load_field(klass, field_name, TRUE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store field ///
        case NODE_TYPE_STORE_FIELD: {
            /// left_value ///
            sCLNodeType field_type;
            sCLClass* klass;
            char* field_name;

            memset(&field_type, 0, sizeof(field_type));
            if(!compile_left_node(node, &field_type, class_params, num_params, info)) {
                return FALSE;
            }

            klass = field_type.mClass;
            *type_ = field_type;
            field_name = gNodes[node].uValue.sVarName.mVarName;

            if(!store_field(node, klass, field_name, FALSE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// store class field ///
        case NODE_TYPE_STORE_CLASS_FIELD: {
            sCLClass* klass;
            char* field_name;

            klass = gNodes[node].mType.mClass;
            *type_ = gNodes[node].mType;
            field_name = gNodes[node].uValue.sVarName.mVarName;

            if(!store_field(node, klass, field_name, TRUE, type_, class_params, num_params, info)) {
                return FALSE;
            }
            }
            break;

        /// new operand ///
        case NODE_TYPE_NEW: {
            sCLNodeType type2;
            sCLClass* klass;
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            char* method_name;
            unsigned int block_id;
            sVar* var;
            
            type2 = gNodes[node].mType;    // new type2();
            klass = type2.mClass;

            if(klass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
                parser_err_msg_format(info->sname, *info->sline, "this is immediate class. can't create object with new operator.");
                (*info->err_num)++;
                break;
            }
            else if(klass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) {
                if(strcmp(CLASS_NAME(klass), "Array") == 0) {
                    append_opecode_to_bytecodes(info->code, OP_NEW_ARRAY);
                    append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
                    append_int_value_to_bytecodes(info->code, 0);
                    
                    inc_stack_num(info->stack_num, info->max_stack, 1);
                }
                else if(strcmp(CLASS_NAME(klass), "Hash") == 0) {
                    append_opecode_to_bytecodes(info->code, OP_NEW_HASH);
                    append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));
                    append_int_value_to_bytecodes(info->code, 0);

                    inc_stack_num(info->stack_num, info->max_stack, 1);
                }
                else if(strcmp(CLASS_NAME(klass), "String") == 0) {
                    append_opecode_to_bytecodes(info->code, OP_NEW_STRING);
                    append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));

                    inc_stack_num(info->stack_num, info->max_stack, 1);
                }
            }
            else {
                append_opecode_to_bytecodes(info->code, OP_NEW_OBJECT);

                append_str_to_bytecodes(info->code, info->constant, REAL_CLASS_NAME(klass));

                inc_stack_num(info->stack_num, info->max_stack, 1);
            }

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call constructor ///
            method_name = CLASS_NAME(klass);
            *type_ = type2;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            /// set "self" type ///
            if(block_id) {
                var = get_variable_from_table(&gNodeBlocks[block_id].mLVTable, "self");
                var->mType = type2;
            }

            if(!call_method(klass, method_name, FALSE, type_, class_params, &num_params, info, block_id))
            {
                return FALSE;
            }

            }
            break;

        /// call method ///
        case NODE_TYPE_METHOD_CALL: {
            sCLNodeType left_type;
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType right_type;
            sCLClass* klass;
            char* method_name;
            unsigned int block_id;
            sVar* var;

            /// left_value ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call method ///
            klass = left_type.mClass;
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = left_type;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            /// set "self" type ///
            if(block_id) {
                var = get_variable_from_table(&gNodeBlocks[block_id].mLVTable, "self");
                var->mType = left_type;
            }

            if(!call_method(klass, method_name, FALSE, type_, class_params, &num_params, info, block_id))
            {
                return FALSE;
            }
            }
            break;

        /// call class method ///
        case NODE_TYPE_CLASS_METHOD_CALL: {
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            sCLClass* klass;
            char* method_name;
            unsigned int block_id;

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call class method //
            klass = gNodes[node].mType.mClass;
            method_name = gNodes[node].uValue.sMethod.mVarName;
            *type_ = gNodes[node].mType;
            block_id = gNodes[node].uValue.sMethod.mBlock;

            if(!call_method(klass, method_name, TRUE, type_, class_params, &num_params, info, block_id))
            {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_SUPER: {
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            unsigned int block_id;
            sVar* var;

            /// load self ///
            if(!(info->caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, class_params, &num_params, info)) {
                    return FALSE;
                }
            }

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call method ///
            block_id = gNodes[node].uValue.sMethod.mBlock;

            /// set "self" type ///
            if(block_id) {
                var = get_variable_from_table(&gNodeBlocks[block_id].mLVTable, "self");
                var->mType = left_type;
            }

            if(!call_super(type_, class_params, &num_params, info, block_id)) {
                return FALSE;
            }
            }
            break;

        case NODE_TYPE_INHERIT: {
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            unsigned int block_id;
            sVar* var;

            /// load self ///
            if(!(info->caller_method->mFlags & CL_CLASS_METHOD)) {
                if(!load_local_varialbe("self", type_, class_params, &num_params, info)) {
                    return FALSE;
                }
            }

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call method ///
            block_id = gNodes[node].uValue.sMethod.mBlock;

            /// set "self" type ///
            if(block_id) {
                var = get_variable_from_table(&gNodeBlocks[block_id].mLVTable, "self");
                var->mType = left_type;
            }

            if(!call_inherit(type_, class_params, &num_params, info, block_id)) {
                return FALSE;
            }
            }
            break;

        /// params ///
        case NODE_TYPE_PARAM: {
            sCLNodeType left_type;
            sCLNodeType right_type;

            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            memset(&right_type, 0, sizeof(right_type));
            if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                return FALSE;
            }

            if(right_type.mClass == NULL) {
                *type_ = gIntType; // dummy
                (*info->err_num)++;
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
            sCLNodeType left_type;
            sCLNodeType result_type;

            if(info->caller_class == NULL || info->caller_method == NULL) {
                parser_err_msg("there is not caller method. can't return", info->sname, *info->sline);
                return FALSE;
            }

            memset(&result_type, 0, sizeof(result_type));
            if(!get_result_type_of_method(info->caller_class, info->caller_method, &result_type, NULL)) {
                parser_err_msg_format(info->sname, *info->sline, "can't found result type of the method named %s.%s", REAL_CLASS_NAME(info->caller_class), METHOD_NAME2(info->caller_class, info->caller_method));
                (*info->err_num)++;

                *type_ = gIntType; // dummy
                break;
            }

            if(result_type.mClass == NULL) {
                parser_err_msg("unexpected err. no result type", info->sname, *info->sline);
                return FALSE;
            }

            if(type_identity(&result_type, &gVoidType)) {
                if(gNodes[node].mLeft) {
                    parser_err_msg_format(info->sname, *info->sline, "the result type of this method(%s::%s) is void. can't return a value", REAL_CLASS_NAME(info->caller_class), METHOD_NAME2(info->caller_class, info->caller_method));
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                append_opecode_to_bytecodes(info->code, OP_RETURN);

                if(info->exist_return) *(info->exist_return) = TRUE;

                *type_ = gVoidType;
            }
            else {
                if(gNodes[node].mLeft == 0) {
                    parser_err_msg_format(info->sname, *info->sline, "the result type of this method(%s::%s) is not void. should return a value", REAL_CLASS_NAME(info->caller_class), METHOD_NAME2(info->caller_class, info->caller_method));
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!substition_posibility(&left_type, &result_type)) {
                    parser_err_msg_format(info->sname, *info->sline, "type error.");
                    cl_print("require type is ");
                    show_node_type(&result_type);
                    cl_print(". but this type is ");
                    show_node_type(&left_type);
                    puts("");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }

                append_opecode_to_bytecodes(info->code, OP_RETURN);

                if(info->exist_return) *(info->exist_return) = TRUE;

                if(*info->stack_num > 1) {
                    parser_err_msg_format(info->sname, *info->sline, "too many value of return");
                    (*info->err_num)++;
                }
                else if(*info->stack_num == 0) {
                    parser_err_msg_format(info->sname, *info->sline, "the value of return statment is required ");
                    (*info->err_num)++;
                }

                *info->stack_num = 0;      // no pop please

                *type_ = gVoidType;
            }
            }
            break;

        case NODE_TYPE_BREAK: {
            sCLNodeType left_type;
            sCLNodeType result_type;

            if(info->while_type == NULL || info->break_labels == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "requires while type or goto label for break. it is not in loop");
                *type_ = gIntType; // dummy
                (*info->err_num)++;
                break;
            }

            result_type = *info->while_type;  // this is gotten from NODE_TYPE_WHILE or NODE_TYPE_DO. this is the result type of while.

            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                return FALSE;
            }

            if(!set_zero_goto_point_of_break(info)) {
                parser_err_msg_format(info->sname, *info->sline, "too many break. overflow");
                (*info->err_num)++;
                *type_ = gIntType;
                break;
            }

            if(info->exist_break) *(info->exist_break) = TRUE;

            ASSERT(result_type.mClass != NULL);
            if(type_identity(&result_type, &gVoidType)) {
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

                if(left_type.mClass != NULL) {
                    if(!substition_posibility(&result_type, &left_type)) {
                        parser_err_msg_format(info->sname, *info->sline, "type error.");
                        cl_print("left type is ");
                        show_node_type(&result_type);
                        cl_print(". right type is ");
                        show_node_type(&left_type);
                        puts("");
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
            break;

        case NODE_TYPE_CONTINUE: {
            sCLNodeType result_type;

            if(info->continue_labels == NULL) {
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

        case NODE_TYPE_IF: {
            int j;
            sCLNodeType conditional_type;
            sNodeBlock* block;
            int ivalue[CL_ELSE_IF_MAX + 1];
            int ivalue_num = 0;
            int ivalue2;
            BOOL no_pop_last_one_value;
            sVarTable new_table;

            /// new table ///
            ASSERT(info->lv_table != NULL);
            memset(&new_table, 0, sizeof(sVarTable));
            copy_var_table(&new_table, info->lv_table);

            /// conditional ///
            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, &new_table, type_)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF);
            append_int_value_to_bytecodes(info->code, 2);                   // jump to if block

            /// block of if ///
            block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mIfBlock;

            append_opecode_to_bytecodes(info->code, OP_GOTO);    // jump to else or else if
            ivalue2 = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0);

            if(!compile_block(block, &new_table, type_, info)) {
                return FALSE;
            }

            if(gNodes[node].uValue.sIfBlock.mElseBlock == 0) {
                *(info->code->mCode + ivalue2) = info->code->mLen + 2;
            }
            else if(gNodes[node].uValue.sIfBlock.mElseIfBlockNum == 0) {
                *(info->code->mCode + ivalue2) = info->code->mLen + 4;
            }
            else {
                *(info->code->mCode + ivalue2) = info->code->mLen + 2;
            }

            append_opecode_to_bytecodes(info->code, OP_GOTO);
            ivalue[ivalue_num++] = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0);

            for(j=0; j<gNodes[node].uValue.sIfBlock.mElseIfBlockNum; j++) {
                sCLNodeType else_if_type;
                sVarTable new_table;

                /// new table ///
                memset(&new_table, 0, sizeof(sVarTable));
                copy_var_table(&new_table, info->lv_table);

                /// else if conditional ///
                if(!compile_conditional(gNodes[node].uValue.sIfBlock.mElseIfConditional[j], &else_if_type, class_params, num_params, info, &new_table, type_)) {
                    return FALSE;
                }

                append_opecode_to_bytecodes(info->code, OP_IF);
                append_int_value_to_bytecodes(info->code, 2);

                /// append else if block to bytecodes ///
                block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mElseIfBlock[j];

                append_opecode_to_bytecodes(info->code, OP_GOTO);
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0);

                if(!compile_block(block, &new_table, type_, info)) {
                    return FALSE;
                }

                if(gNodes[node].uValue.sIfBlock.mElseBlock == 0) {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 2;
                }
                else if(j == gNodes[node].uValue.sIfBlock.mElseIfBlockNum-1) {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 4;
                }
                else {
                    *(info->code->mCode + ivalue2) = info->code->mLen + 2;
                }

                append_opecode_to_bytecodes(info->code, OP_GOTO);
                ivalue[ivalue_num++] = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0);
            }

            if(gNodes[node].uValue.sIfBlock.mElseBlock) {
                sVarTable new_table;

                /// new table ///
                memset(&new_table, 0, sizeof(sVarTable));
                copy_var_table(&new_table, info->lv_table);

                block = gNodeBlocks + gNodes[node].uValue.sIfBlock.mElseBlock;

                append_opecode_to_bytecodes(info->code, OP_GOTO);
                ivalue2 = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0);

                if(!compile_block(block, &new_table, type_, info)) {
                    return FALSE;
                }

                *(info->code->mCode + ivalue2) = info->code->mLen;
            }

            for(j=0; j<ivalue_num; j++) {
                *(info->code->mCode + ivalue[j]) = info->code->mLen;
            }

            ASSERT(gNodes[node].mType.mClass != NULL);
            if(type_identity(&gNodes[node].mType, &gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                *type_ = gNodes[node].mType;
                *info->stack_num = 1;
            }
            }
            break;

        case NODE_TYPE_WHILE: {
            sCLNodeType conditional_type;
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
            sVarTable new_table;

            /// new table ///
            memset(&new_table, 0, sizeof(sVarTable));
            copy_var_table(&new_table, info->lv_table);

            /// conditional ///
            conditional_label = info->code->mLen;                           // save label for goto

            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, &new_table, type_)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF);
            append_int_value_to_bytecodes(info->code, 2);                           // jump to while block

            append_opecode_to_bytecodes(info->code, OP_GOTO);    // jump to while block out
            while_loop_out_label = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0);

            /// while block ///
            block = gNodeBlocks + gNodes[node].uValue.mWhileBlock;

            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->while_type;                            // save the value
            info->while_type = &gNodes[node].mType;                          // for NODE_TYPE_BREAK to get while type

            if(!compile_block(block, &new_table, type_, info)) {
                return FALSE;
            }

            info->while_type = while_type_before;                   // restore the value
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);

            append_opecode_to_bytecodes(info->code, OP_GOTO);    // jump to conditional_label
            append_int_value_to_bytecodes(info->code, conditional_label);

            // determine the jump point 
            *(info->code->mCode + while_loop_out_label) = info->code->mLen;

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            ASSERT(gNodes[node].mType.mClass != NULL);
            if(type_identity(&gNodes[node].mType, &gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                if(*info->exist_break == FALSE) {
                    parser_err_msg_format(info->sname, *info->sline, "require the return value of break");
                    (*info->err_num)++;
                    *type_ = gIntType; // dummy
                    break;
                }

                *type_ = gNodes[node].mType;
                *info->stack_num = 1;
            }
            }
            break;

        case NODE_TYPE_DO: {
            sCLNodeType conditional_type;
            sNodeBlock* block;
            int loop_top_label;
            unsigned int break_labels[CL_BREAK_MAX];
            unsigned int* break_labels_before;
            int break_labels_len;
            int* break_labels_len_before;
            int j;
            sCLNodeType* while_type_before;
            sVarTable new_table;
            unsigned int continue_labels[CL_BREAK_MAX];
            unsigned int* continue_labels_before;
            int continue_labels_len;
            int* continue_labels_len_before;

            /// new table ///
            memset(&new_table, 0, sizeof(sVarTable));
            copy_var_table(&new_table, info->lv_table);

            /// do block ///
            loop_top_label = info->code->mLen;                              // save label for goto

            block = gNodeBlocks + gNodes[node].uValue.mDoBlock;

            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->while_type;                            // save the value
            info->while_type = &gNodes[node].mType;                          // for NODE_TYPE_BREAK to get while type

            if(!compile_block(block, &new_table, type_, info)) {
                return FALSE;
            }

            info->while_type = while_type_before;                           // restore

            //// conditional ///
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);

            if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, &new_table, type_)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_NOTIF);
            append_int_value_to_bytecodes(info->code, 2);               // jump to the loop out

            append_opecode_to_bytecodes(info->code, OP_GOTO);    // jump to the top of loop
            append_int_value_to_bytecodes(info->code, loop_top_label);

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            ASSERT(gNodes[node].mType.mClass != NULL);

            if(type_identity(&gNodes[node].mType, &gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                if(*info->exist_break == FALSE) {
                    parser_err_msg_format(info->sname, *info->sline, "require the return value of break");
                    (*info->err_num)++;
                    *type_ = gIntType; // dummy
                    break;
                }

                *type_ = gNodes[node].mType;
                *info->stack_num = 1;
            }
            }
            break;

        case NODE_TYPE_FOR: {
            sCLNodeType expression_type;
            sNodeBlock* block;
            int conditional_label;
            unsigned int for_loop_out_label;
            unsigned int break_labels[CL_BREAK_MAX];
            unsigned int* break_labels_before;
            int break_labels_len;
            int* break_labels_len_before;
            int j;
            sCLNodeType* while_type_before;
            sVarTable new_table;
            unsigned int continue_labels[CL_BREAK_MAX];
            unsigned int* continue_labels_before;
            int continue_labels_len;
            int* continue_labels_len_before;

            /// new table ///
            memset(&new_table, 0, sizeof(sVarTable));
            copy_var_table(&new_table, info->lv_table);

            /// initilize expression ///
            memset(&expression_type, 0, sizeof(expression_type));
            if(!compile_expressiont_in_loop(gNodes[node].mLeft, &expression_type, class_params, num_params, info, &new_table)) {
                return FALSE;
            }

            /// conditional ///
            conditional_label = info->code->mLen;

            if(!compile_conditional(gNodes[node].mRight, &expression_type, class_params, num_params, info, &new_table, type_)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_IF);
            append_int_value_to_bytecodes(info->code, 2);               // jump to for block

            /// for block ///
            block = gNodeBlocks + gNodes[node].uValue.mForBlock;

            append_opecode_to_bytecodes(info->code, OP_GOTO);       // jump to for block out
            for_loop_out_label = info->code->mLen;
            append_int_value_to_bytecodes(info->code, 0);

            prepare_for_break_labels(&break_labels_before, &break_labels_len_before, break_labels, &break_labels_len, info);
            prepare_for_continue_labels(&continue_labels_before, &continue_labels_len_before, continue_labels, &continue_labels_len, info);

            while_type_before = info->while_type;                   // save the value
            info->while_type = &gNodes[node].mType;                 // for NODE_TYPE_BREAK to get while type

            if(!compile_block(block, &new_table, type_, info)) {
                return FALSE;
            }

            info->while_type = while_type_before;                   // restore the value

            /// finalization expression ///
            determine_the_goto_point_of_continue(continue_labels_before, continue_labels_len_before, info);
            memset(&expression_type, 0, sizeof(expression_type));

            if(!compile_expressiont_in_loop(gNodes[node].mMiddle, &expression_type, class_params, num_params, info, &new_table)) {
                return FALSE;
            }

            append_opecode_to_bytecodes(info->code, OP_GOTO);    // jump to the conditional label
            append_int_value_to_bytecodes(info->code, conditional_label);

            // determine the jump point 
            *(info->code->mCode + for_loop_out_label) = info->code->mLen;

            /// this is for break statment to determine the jump point
            determine_the_goto_point_of_break(break_labels_before, break_labels_len_before, info);

            ASSERT(gNodes[node].mType.mClass != NULL);
            if(type_identity(&gNodes[node].mType, &gVoidType)) {
                *type_ = gVoidType;
                *info->stack_num = 0;
            }
            else {
                if(*info->exist_break == FALSE) {
                    parser_err_msg_format(info->sname, *info->sline, "require the return value of break");
                    (*info->err_num)++;
                    *type_ = gIntType; // dummy
                    break;
                }

                *type_ = gNodes[node].mType;
                *info->stack_num = 1;
            }
            }
            break;

        case NODE_TYPE_BLOCK_CALL: {
            sCLNodeType class_params[CL_METHOD_PARAM_MAX];
            int num_params;
            sCLNodeType left_type;
            sCLClass* klass;
            sCLMethod* method;
            char* block_name;
            BOOL static_method;

            static_method = info->caller_method->mFlags & CL_CLASS_METHOD;

            /// load self ///
            if(!static_method) {
                if(!load_local_varialbe("self", type_, class_params, &num_params, info)) {
                    return FALSE;
                }
            }

            /// initilize class params ///
            num_params = 0;
            memset(class_params, 0, sizeof(class_params));

            /// params go ///
            memset(&left_type, 0, sizeof(left_type));
            if(!compile_left_node(node, &left_type, class_params, &num_params, info)) {
                return FALSE;
            }

            /// call method ///
            klass = info->caller_class;
            *type_ = gBlockType;
            method = info->caller_method;
            block_name = gNodes[node].uValue.sVarName.mVarName;

            if(!call_method_block(klass, type_, method, block_name, class_params, &num_params, info, static_method)) {
                return FALSE;
            }
            }
            break;

        /// operand ///
        case NODE_TYPE_OPERAND:
            switch((int)gNodes[node].uValue.mOperand) {
            case kOpAdd: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_add(left_type, right_type, type_, info)) {
                    return FALSE;
                }
                }
                break;

            case kOpSub: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ISUB, OP_FSUB, "substraction", "-", &gIntType, &gFloatType)) {
                    return FALSE;
                }
                }
                break;

            case kOpMult: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IMULT, OP_FMULT, "multiplication", "*", &gIntType, &gFloatType)) {
                    return FALSE;
                }
                }
                break;

            case kOpDiv: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IDIV, OP_FDIV, "division", "/", &gIntType, &gFloatType)) {
                    return FALSE;
                }
                }
                break;

            case kOpMod: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IMOD, "modulo operation", "%", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpLeftShift: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_ILSHIFT, "left shift", "<<", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpRightShift: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IRSHIFT, "right shift", ">>", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonGreater: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IGTR, OP_FGTR, "comparison greater", ">", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonGreaterEqual: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IGTR_EQ, OP_FGTR_EQ, "comparison greater", ">=", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonLesser: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ILESS, OP_FLESS, "comparison lesser", "<", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonLesserEqual: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_ILESS_EQ, OP_FLESS_EQ, "comparison lesser equal", "<=", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonEqual: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_IEQ, OP_FEQ, "comparison equal", "==", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpComparisonNotEqual: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator(left_type, right_type, type_, info, OP_INOTEQ, OP_FNOTEQ, "comparison not equal", "!=", &gBoolType, &gBoolType)) {
                    return FALSE;
                }
                }
                break;

            case kOpAnd: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IAND, "and operation", "&", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpXor: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IXOR, "xor operation", "^", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpOr: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_without_float(left_type, right_type, type_, info, OP_IOR, "or operation", "|", &gIntType)) {
                    return FALSE;
                }
                }
                break;

            case kOpIndexing: {
                sCLNodeType left_type;
                sCLNodeType right_type;
                sCLNodeType class_params2[CL_METHOD_PARAM_MAX];
                int num_params2;

                num_params2 = 0;
                memset(class_params2, 0, sizeof(class_params2));

                /// left node (self) ///
                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                /// right node go (params) ///
                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params2, &num_params2, info)) {
                    return FALSE;
                }

                if(left_type.mClass == NULL || right_type.mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else {
                    if(operand_posibility(&left_type, &gStringType)) {
                        *type_ = gIntType;







                        dec_stack_num(info->stack_num, num_params2 + 1);
                        inc_stack_num(info->stack_num, info->max_stack, 1);
                    }
                    else {
                        *type_ = left_type;
                        if(!call_method(type_->mClass, "[]", FALSE, type_, class_params2, &num_params2, info, 0)) {
                            return FALSE;
                        }
                    }
                }

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
                    sCLNodeType left_type;

                    name = gNodes[left_node].uValue.sVarName.mVarName;
                    var = get_variable_from_table(info->lv_table, name);

                    if(var == NULL) {
                        parser_err_msg_format(info->sname, *info->sline, "there is not this variable (%s)", name);
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }

                    left_type = var->mType;

                    if(!increase_or_decrease_local_variable(name, var, left_type, node, type_, class_params, num_params, info))
                    {
                        return FALSE;
                    }
                }
                else if(gNodes[left_node].mNodeType == NODE_TYPE_FIELD) {
                    /// left_value ///
                    sCLNodeType field_type;
                    sCLClass* klass;
                    char* field_name;

                    memset(&field_type, 0, sizeof(field_type));
                    if(!compile_left_node(left_node, &field_type, class_params, num_params, info)) {
                        return FALSE;
                    }

                    klass = field_type.mClass;
                    *type_ = field_type;
                    field_name = gNodes[left_node].uValue.sVarName.mVarName;

                    if(!increase_or_decrease_field(node, left_node, klass, field_name, FALSE, type_, class_params, num_params, info))
                    {
                        return FALSE;
                    }
                }
                else if(gNodes[left_node].mNodeType == NODE_TYPE_CLASS_FIELD) {
                    sCLClass* klass;
                    char* field_name;
                    
                    klass = gNodes[left_node].mType.mClass;
                    field_name = gNodes[left_node].uValue.sVarName.mVarName;

                    ASSERT(klass); // must be not NULL
                    *type_ = gNodes[left_node].mType;

                    if(!increase_or_decrease_field(node, left_node, klass, field_name, TRUE, type_, class_params, num_params, info)) {
                        return FALSE;
                    }
                }
                else {
                    parser_err_msg("unexpected error on ++ or -- operator", info->sname, *info->sline);
                    return FALSE;
                }

                }
                break;

            case kOpComplement: {
                sCLNodeType left_type;

                /// left node ///
                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(left_type.mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else if(operand_posibility(&left_type, &gIntType)) {
                    append_opecode_to_bytecodes(info->code, OP_COMPLEMENT);

                    *type_ = gIntType;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "can't get complement from this type(%s).", REAL_CLASS_NAME(left_type.mClass));
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                }
                break;
 
            case kOpLogicalDenial: {
                sCLNodeType left_type;

                /// left node ///
                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(left_type.mClass == NULL) {
                    parser_err_msg("no class type", info->sname, *info->sline);
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                else if(operand_posibility(&left_type, &gBoolType)) {
                    append_opecode_to_bytecodes(info->code, OP_LOGICAL_DENIAL);

                    *type_ = gBoolType;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "can't get logical denial from this type(%s).", REAL_CLASS_NAME(left_type.mClass));
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                }
                }
                break;

            case kOpOrOr: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_bool(left_type, right_type, type_, info, OP_IOROR, "|| operation", "||"))
                {
                    return FALSE;
                }
                }
                break;

            case kOpAndAnd: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
                if(!compile_right_node(node, &right_type, class_params, num_params, info)) {
                    return FALSE;
                }

                if(!binary_operator_bool(left_type, right_type, type_, info, OP_IANDAND, "&& operation", "&&"))
                {
                    return FALSE;
                }
                }
                break;

            case kOpConditional:{
                int j;
                sCLNodeType conditional_type;
                sCLNodeType true_value_type;
                sCLNodeType false_value_type;
                int else_label;
                int block_out_label;
                int true_stack_num;
                int false_stack_num;
                int* stack_num_before;

                /// conditional ///
                if(!compile_conditional(gNodes[node].mLeft, &conditional_type, class_params, num_params, info, NULL, type_)) {
                    return FALSE;
                }

                append_opecode_to_bytecodes(info->code, OP_IF);
                append_int_value_to_bytecodes(info->code, 2);

                append_opecode_to_bytecodes(info->code, OP_GOTO);    // goto else label
                else_label = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0);

                /// true value ///
                true_stack_num = 0;
                stack_num_before = info->stack_num;
                info->stack_num = &true_stack_num;
                memset(&true_value_type, 0, sizeof(true_value_type));
                if(!compile_middle_node(node, &true_value_type, class_params, num_params, info)) {
                    return FALSE;
                }
                info->stack_num = stack_num_before;

                append_opecode_to_bytecodes(info->code, OP_GOTO);    // goto end of ? operator
                block_out_label = info->code->mLen;
                append_int_value_to_bytecodes(info->code, 0);

                /// false value ///
                *(info->code->mCode + else_label) = info->code->mLen;

                false_stack_num = 0;
                stack_num_before = info->stack_num;
                info->stack_num = &false_stack_num;
                memset(&false_value_type, 0, sizeof(false_value_type));
                if(!compile_right_node(node, &false_value_type, class_params, num_params, info)) {
                    return FALSE;
                }
                info->stack_num = stack_num_before;

                *(info->code->mCode + block_out_label) = info->code->mLen;

                if(type_identity(&true_value_type, &false_value_type)) {
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
                    cl_print("true expression type is ");
                    show_node_type(&true_value_type);
                    cl_print(". false expression type is ");
                    show_node_type(&false_value_type);
                    puts("");
                    (*info->err_num)++;

                    *type_ = gIntType; // dummy
                    break;
                }
                }
                break;

            case kOpComma: {
                sCLNodeType left_type;
                sCLNodeType right_type;

                memset(&left_type, 0, sizeof(left_type));
                if(!compile_left_node(node, &left_type, class_params, num_params, info)) {
                    return FALSE;
                }

                memset(&right_type, 0, sizeof(right_type));
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

BOOL compile_method(sCLMethod* method, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace)
{
    int max_stack;
    int stack_num;
    BOOL exist_return;
    sCLNodeType result_type;
    BOOL exist_break;

    alloc_bytecode_of_method(method);

    init_nodes();

    max_stack = 0;
    stack_num = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    while(*p) {
        int saved_err_num;
        unsigned int node;
        int sline_top;

        skip_spaces_and_lf(p, sline);
        
        saved_err_num = *err_num;
        node = 0;

        sline_top = *sline;

        if(!node_expression(&node, p, sname, sline, err_num, current_namespace, klass, method)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLNodeType type_;
            sCompileInfo info;

            memset(&type_, 0, sizeof(type_));
            memset(&info, 0, sizeof(sCompileInfo));

            info.sname = sname;
            info.sline = &sline_top;
            info.caller_class = klass->mClass;
            info.caller_method = method;
            info.code = &method->uCode.mByteCodes;
            info.constant = &klass->mClass->mConstPool;
            info.err_num = err_num;
            info.lv_table = lv_table;
            info.stack_num = &stack_num;
            info.max_stack = &max_stack;
            info.exist_return = &exist_return;
            info.exist_break = &exist_break;
            info.break_labels = NULL;
            info.break_labels_len = NULL;
            info.while_type = NULL;
            info.continue_labels = NULL;
            info.continue_labels_len = NULL;

            if(!compile_node(node, &type_, NULL, 0, &info)) {
                free_nodes();
                return FALSE;
            }
        }

#ifdef STACK_DEBUG
compile_error("sname (%s) sline (%d) stack_num (%d)\n", sname, *sline, stack_num);
#endif
        if(**p == ';' || **p == '}') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            correct_stack_pointer(&stack_num, sname, sline, &method->uCode.mByteCodes, err_num);

            if(**p == '}') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
        }
        else if(**p == 0) {
            correct_stack_pointer(&stack_num, sname, sline, &method->uCode.mByteCodes, err_num);
            break;
        }
        else if(node == 0) {
            parser_err_msg_format(sname, *sline, "unexpected character(%d)(%c)", **p, **p);
            free_nodes();
            return FALSE;
        }
    }

    /// add "return self" to the constructor ///
    if(constructor) {
        append_opecode_to_bytecodes(&method->uCode.mByteCodes, OP_OLOAD);
        append_int_value_to_bytecodes(&method->uCode.mByteCodes, 0);
    }

    memset(&result_type, 0, sizeof(result_type));
    if(!get_result_type_of_method(klass->mClass, method, &result_type, NULL)) {
        parser_err_msg_format(sname, *sline, "can't found result type of the method named %s.%s", REAL_CLASS_NAME(klass->mClass), METHOD_NAME2(klass->mClass, method));
        (*err_num)++;
        return TRUE;
    }

    if(!substition_posibility(&result_type, &gVoidType) && !exist_return && !(method->mFlags & CL_CONSTRUCTOR)) {
        parser_err_msg("require return sentence", sname, *sline);
        (*err_num)++;
        free_nodes();
        return TRUE;
    }

    method->mMaxStack = max_stack;
    method->mNumLocals = lv_table->mVarNum + lv_table->mBlockVarNum;

    free_nodes();

    return TRUE;
}

BOOL parse_statment(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table)
{
    int stack_num;
    BOOL exist_return;
    BOOL exist_break;

    init_nodes();

    *max_stack = 0;
    stack_num = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    while(**p) {
        int saved_err_num;
        unsigned int node;
        int sline_top;

        skip_spaces_and_lf(p, sline);

        sline_top = *sline;

        saved_err_num = *err_num;
        node = 0;
        if(!node_expression(&node, p, sname, sline, err_num, current_namespace, NULL, NULL)) {
            free_nodes();
            return FALSE;
        }

        if(node != 0 && *err_num == saved_err_num) {
            sCLNodeType type_;
            sCompileInfo info;

            memset(&info, 0, sizeof(sCompileInfo));

            memset(&type_, 0, sizeof(type_));

            info.sname = sname;
            info.sline = &sline_top;
            info.caller_class = NULL;
            info.caller_method = NULL;
            info.code = code;
            info.constant = constant;
            info.err_num = err_num;
            info.lv_table = var_table;
            info.stack_num = &stack_num;
            info.max_stack = max_stack;
            info.exist_return = &exist_return;
            info.exist_break = &exist_break;
            info.break_labels = NULL;
            info.break_labels_len = NULL;
            info.while_type = NULL;
            info.continue_labels = NULL;
            info.continue_labels_len = NULL;

            if(!compile_node(node, &type_, NULL, 0, &info)) {
                free_nodes();
                return FALSE;
            }
        }

        if(**p == ';' || **p == '}') {
            while(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            correct_stack_pointer(&stack_num, sname, sline, code, err_num);

            if(**p == '}') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
            break;
        }
        else if(**p == 0) {
            correct_stack_pointer(&stack_num, sname, sline, code, err_num);
            break;
        }
        else if(node == 0) {
            parser_err_msg_format(sname, *sline, "unexpected character(%d)(%c)", **p, **p);
            free_nodes();
            return FALSE;
        }
    }

    free_nodes();
    return TRUE;
}

BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType block_type, BOOL enable_param, BOOL static_method, sCLMethod* method)
{
    sNode statment_end_node;

    *block_id = alloc_node_block(block_type);

    if(enable_param) {
        if(!static_method) {
            if(!add_variable_to_table(&gNodeBlocks[*block_id].mLVTable, "self", NULL)) {
                parser_err_msg("overflow variable table", sname, *sline);
                return FALSE;
            }
        }

        if(**p == '|') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!parse_params(gNodeBlocks[*block_id].mClassParams, &gNodeBlocks[*block_id].mNumParams, p, sname, sline, err_num, current_namespace, klass ? klass->mClass:NULL, &gNodeBlocks[*block_id].mLVTable, '|')) {
                return FALSE;
            }
        }
    }

    while(**p) {
        int saved_err_num;
        sNode node;

        skip_spaces_and_lf(p, sline);

        saved_err_num = *err_num;
        node.mNode = 0;
        node.mSName = sname;
        node.mSLine = *sline;
        if(!node_expression(&node.mNode, p, sname, sline, err_num, current_namespace, klass, method)) {
            return FALSE;
        }
/*
        node.mSName = sname;
        node.mSLine = *sline;
*/

        if(node.mNode != 0 && *err_num == saved_err_num) {
            append_node_to_node_block(*block_id, &node);
        }

        if(**p == ';' || **p == '}') {
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
        }
        else if(**p == 0) {
            statment_end_node.mNode = 0;
            statment_end_node.mSName = sname;
            statment_end_node.mSLine = *sline;

            append_node_to_node_block(*block_id, &statment_end_node);
            break;
        }
        else if(node.mNode == 0) {
            parser_err_msg_format(sname, *sline, "unexpected character(%d)(%c)", **p, **p);
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_block(sNodeBlock* block, sVarTable* new_table, sCLNodeType* type_, sCompileInfo* info)
{
    int i;
    int lv_num_of_this_block;
    int stack_num;
    int max_stack;

    max_stack = 0;
    stack_num = 0;
    *(info->exist_break) = FALSE;

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;

        node = block->mNodes + i;

        if(node->mNode != 0) {
            memset(&info2, 0, sizeof(info2));

            info2.caller_class = info->caller_class;
            info2.caller_method = info->caller_method;
            info2.code = info->code;
            info2.constant = info->constant;
            info2.sname = node->mSName;
            info2.sline = &node->mSLine;
            info2.err_num = info->err_num;
            info2.lv_table = new_table;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = info->exist_return;
            info2.exist_break = info->exist_break;
            info2.break_labels = info->break_labels;          // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.break_labels_len = info->break_labels_len;  // this is used by NODE_TYPE_BREAK and NODE_TYPE_WHILE. another ignores the value
            info2.continue_labels = info->continue_labels;      // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.continue_labels_len = info->continue_labels_len; // this is used by NODE_TYPE_CONTINUE and NOT_TYPE_WHILE. another ignores the value
            info2.while_type = info->while_type;

            if(!compile_node(node->mNode, type_, NULL, 0, &info2)) {
                free_nodes();
                return FALSE;
            }
        }
        else {
            if(i == block->mLenNodes -1) {              // last one
                if(block->mBlockType.mClass == NULL || type_identity(&block->mBlockType, &gVoidType)) {
                    correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, info->code, info->err_num);
                }
                else {
                    if(stack_num != 1) {
                        parser_err_msg_format(node->mSName, node->mSLine, "require one return value of this block");
                        (*info->err_num)++;

                        *type_ = gIntType;   // dummy
                        return TRUE;
                    }
                    if(!substition_posibility(&block->mBlockType, type_)) {
                        parser_err_msg_format(node->mSName, node->mSLine, "type error.");
                        cl_print("left type is ");
                        show_node_type(&block->mBlockType);
                        cl_print(". right type is ");
                        show_node_type(type_);
                        puts("");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                }
            }
            else {
                correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, info->code, info->err_num);
            }
        }
    }

    /// get local var number of this block ///
    lv_num_of_this_block = new_table->mVarNum - info->lv_table->mVarNum + new_table->mBlockVarNum;
    if(lv_num_of_this_block > info->lv_table->mBlockVarNum) {
        info->lv_table->mBlockVarNum = lv_num_of_this_block;
    }

    return TRUE;
}

static BOOL compile_block_of_method(sNodeBlock* block, sConst* constant, sByteCode* code, sVarTable* new_table, sCLNodeType* type_, sCompileInfo* info, sCLClass* caller_class, sCLMethod* caller_method)
{
    int i;
    int stack_num;
    int max_stack;
    BOOL exist_return;
    BOOL exist_break;
    int num_locals_before;

    max_stack = 0;
    stack_num = 0;
    exist_return = FALSE;
    exist_break = FALSE;

    num_locals_before = new_table->mVarNum;

    /// append params ///
    if(!append_var_table(new_table, &block->mLVTable)) {
        parser_err_msg("overflow variable table", info->sname, *info->sline);
        return FALSE;
    }

    for(i=0; i<block->mLenNodes; i++) {
        sCompileInfo info2;
        sNode* node;

        node = block->mNodes + i;

        if(node->mNode != 0) {
            memset(&info2, 0, sizeof(info2));

            info2.caller_class = caller_class;
            info2.caller_method = caller_method;
            info2.code = code;
            info2.constant = constant;
            info2.sname = node->mSName;
            info2.sline = &node->mSLine;
            info2.err_num = info->err_num;
            info2.lv_table = new_table;
            info2.stack_num = &stack_num;
            info2.max_stack = &max_stack;
            info2.exist_return = &exist_return;
            info2.exist_break = &exist_break;
            info2.break_labels = NULL;
            info2.break_labels_len = NULL;
            info2.continue_labels = NULL;
            info2.continue_labels_len = NULL;
            info2.while_type = NULL;

            if(!compile_node(node->mNode, type_, NULL, 0, &info2)) {
                free_nodes();
                return FALSE;
            }
        }
        else {
            if(i == block->mLenNodes -1) {              // last one
                if(block->mBlockType.mClass == NULL || type_identity(&block->mBlockType, &gVoidType)) {
                    correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, code, info->err_num);
                }
                else {
                    if(stack_num != 1) {
                        parser_err_msg_format(node->mSName, node->mSLine, "require one return value of this block");
                        (*info->err_num)++;

                        *type_ = gIntType;   // dummy
                        return TRUE;
                    }
                    if(!substition_posibility(&block->mBlockType, type_)) {
                        parser_err_msg_format(node->mSName, node->mSLine, "type error.");
                        cl_print("left type is ");
                        show_node_type(&block->mBlockType);
                        cl_print(". right type is ");
                        show_node_type(type_);
                        puts("");
                        (*info->err_num)++;

                        *type_ = gIntType; // dummy
                        return TRUE;
                    }
                }
            }
            else {
                correct_stack_pointer(&stack_num, node->mSName, &node->mSLine, code, info->err_num);
            }
        }
    }

    block->mMaxStack = max_stack;
    block->mNumLocals = new_table->mVarNum - num_locals_before;

    return TRUE;
}
