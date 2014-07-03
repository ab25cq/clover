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

void init_nodes()
{
    const int node_size = 32;

    if(gUsedNodes == 0) {
        gNodes = CALLOC(1, sizeof(sNodeTree)*node_size);
        gSizeNodes = node_size;
        gUsedNodes = 1;   // 0 of index means null

        init_node_blocks();
    }
}

void free_nodes()
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
unsigned int sNodeTree_create_character_value(wchar_t c)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CHARACTER_VALUE;
    gNodes[i].uValue.mCharacterValue = c;

    gNodes[i].mType = gStringType;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

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

unsigned int sNodeTree_create_class_name(sCLNodeType* type)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mType = *type;
    gNodes[i].mNodeType = NODE_TYPE_CLASS_NAME;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

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

unsigned int sNodeTree_create_throw(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_THROW;

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

/*
unsigned int sNodeTree_create_revert(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_REVERT;

    gNodes[i].mType = *klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}
*/

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

unsigned int sNodeTree_create_try(unsigned int try_block, unsigned int catch_block, unsigned int finally_block, sCLClass* exception_class, char* exception_variable_name)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_TRY;

    gNodes[i].uValue.sTryBlock.mTryBlock = try_block;
    gNodes[i].uValue.sTryBlock.mCatchBlock = catch_block;
    gNodes[i].uValue.sTryBlock.mFinallyBlock = finally_block;
    gNodes[i].uValue.sTryBlock.mExceptionClass = exception_class;
    xstrncpy(gNodes[i].uValue.sTryBlock.mExceptionVariableName,exception_variable_name, CL_VARIABLE_NAME_MAX);

    memset(&gNodes[i].mType, 0, sizeof(gNodes[i].mType));

    gNodes[i].mLeft = 0;
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

unsigned int sNodeTree_create_block(sCLNodeType* type_, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BLOCK;

    gNodes[i].uValue.mBlock = block;

    ASSERT(type_ != NULL);
    gNodes[i].mType = *type_;

    gNodes[i].mLeft = 0;
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
    "NODE_TYPE_BLOCK_CALL",
    "NODE_TYPE_BLOCK",
    "NODE_TYPE_CHARACTER_VALUE",
    "NODE_TYPE_THROW",
    "NODE_TYPE_TRY",
    "NODE_TYPE_CLASS_NAME",
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

unsigned int alloc_node_block(sCLNodeType block_type)
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

void append_node_to_node_block(unsigned int node_block_id, sNode* node)
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
