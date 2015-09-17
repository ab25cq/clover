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
    int j;

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
        gNodes = xxrealloc(gNodes, sizeof(sNodeTree)*gSizeNodes, sizeof(sNodeTree)*new_size);
        memset(gNodes + gSizeNodes, 0, sizeof(sNodeTree)*(new_size - gSizeNodes));

        gSizeNodes = new_size;
    }

    return gUsedNodes++;
}

// return node index
unsigned int sNodeTree_create_operand(enum eOperand operand, unsigned int left, unsigned int right, unsigned int middle, BOOL quote)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_OPERAND;
    gNodes[i].uValue.sOperand.mOperand = operand;
    gNodes[i].uValue.sOperand.mQuote = quote;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    gNodes[i].mType = NULL;

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

unsigned int sNodeTree_create_byte_value(unsigned char value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BYTE_VALUE;
    gNodes[i].uValue.mByteValue = value;

    gNodes[i].mType = gByteType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_short_value(unsigned short value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_SHORT_VALUE;
    gNodes[i].uValue.mShortValue = value;

    gNodes[i].mType = gShortType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_uint_value(unsigned int value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_UINT_VALUE;
    gNodes[i].uValue.mUIntValue = value;

    gNodes[i].mType = gUIntType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_long_value(unsigned long value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_LONG_VALUE;
    gNodes[i].uValue.mLongValue = value;

    gNodes[i].mType = gLongType;

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

unsigned int sNodeTree_create_dvalue(double dvalue, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_DVALUE;
    gNodes[i].uValue.mDValue = dvalue;

    gNodes[i].mType = gDoubleType;

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

unsigned int sNodeTree_create_regex(char* regex, BOOL global, BOOL multiline, BOOL ignore_case)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_REGEX_VALUE;

    xstrncpy(gNodes[i].uValue.sRegex.mRegexString, regex, REGEX_LENGTH_MAX);

    gNodes[i].uValue.sRegex.mGlobal = global;
    gNodes[i].uValue.sRegex.mMultiline = multiline;
    gNodes[i].uValue.sRegex.mIgnoreCase = ignore_case;

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

unsigned int sNodeTree_create_bytes_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BYTES_VALUE;
    gNodes[i].uValue.mStringValue = MANAGED value;

    gNodes[i].mType = gBytesType;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_path_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_PATH_VALUE;
    gNodes[i].uValue.mStringValue = MANAGED value;

    gNodes[i].mType = gBytesType;

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

unsigned int sNodeTree_create_tuple(unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_TUPLE_VALUE;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_hash(unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_HASH_VALUE;

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

    gNodes[i].mType = NULL;

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

    gNodes[i].mType = NULL;

    return i;
}

unsigned int sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_DEFINE_VARIABLE_NAME;
    gNodes[i].uValue.sVarName.mVarName = STRDUP(var_name);

    gNodes[i].mType = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_class_name(sCLNodeType* type)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mType = type;
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

    gNodes[i].mType = klass;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_revert(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_REVERT;

    gNodes[i].mType = klass;

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

    gNodes[i].mType = klass;

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

    gNodes[i].mType = klass;

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

    gNodes[i].mType = NULL;

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

    gNodes[i].mType = NULL;

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

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node)
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_CLASS_METHOD_CALL;
    gNodes[i].uValue.sMethod.mVarName = STRDUP(var_name);
    gNodes[i].uValue.sMethod.mBlock = block_object;
    gNodes[i].uValue.sMethod.mBlockNode = block_node;

    gNodes[i].mType = klass;

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

    gNodes[i].mType = klass;

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

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_new_expression(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_NEW;
    gNodes[i].uValue.sMethod.mVarName = NULL;
    gNodes[i].uValue.sMethod.mBlock = block_object;
    gNodes[i].uValue.sMethod.mBlockNode = block_node;

    gNodes[i].mType = klass;

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

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_method_call(char* var_name, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_METHOD_CALL;
    gNodes[i].uValue.sMethod.mVarName = STRDUP(var_name);
    gNodes[i].uValue.sMethod.mBlock = block_object;
    gNodes[i].uValue.sMethod.mBlockNode = block_node;

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_inherit(unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_INHERIT;
    gNodes[i].uValue.sMethod.mBlock = block_object;
    gNodes[i].uValue.sMethod.mBlockNode = block_node;

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = left;
    gNodes[i].mRight = right;
    gNodes[i].mMiddle = middle;

    return i;
}

unsigned int sNodeTree_create_super(unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_SUPER;
    gNodes[i].uValue.sMethod.mBlock = block_object;
    gNodes[i].uValue.sMethod.mBlockNode = block_node;

    gNodes[i].mType = NULL;

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
    gNodes[i].mType = type_;

    gNodes[i].mLeft = if_conditional;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_try(unsigned int try_block, unsigned int* catch_blocks, int catch_block_number, unsigned int finally_block, sCLNodeType** exception_type, char exception_variable_name[CL_CATCH_BLOCK_NUMBER_MAX][CL_VARIABLE_NAME_MAX+1])
{
    unsigned int i;
    int j;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_TRY;

    gNodes[i].uValue.sTryBlock.mTryBlock = try_block;
    for(j=0;j<catch_block_number; j++) {
        gNodes[i].uValue.sTryBlock.mCatchBlocks[j] = catch_blocks[j];
    }
    gNodes[i].uValue.sTryBlock.mCatchBlockNumber = catch_block_number;
    gNodes[i].uValue.sTryBlock.mFinallyBlock = finally_block;
    for(j=0; j<catch_block_number; j++) {
        gNodes[i].uValue.sTryBlock.mExceptionType[j] = exception_type[j];
        xstrncpy(gNodes[i].uValue.sTryBlock.mExceptionVariableName[j], exception_variable_name[j], CL_VARIABLE_NAME_MAX);
    }

    gNodes[i].mType = NULL;

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
    gNodes[i].mType = type_;

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

    ASSERT(type_ != 0);
    gNodes[i].mType = type_;

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

    ASSERT(type_ != 0);
    gNodes[i].mType = type_;

    gNodes[i].mLeft = conditional;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_range(unsigned int head, unsigned int tail)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_RANGE_VALUE;

    gNodes[i].mLeft = head;
    gNodes[i].mRight = tail;
    gNodes[i].mMiddle = 0;

    return i;
}

unsigned int sNodeTree_create_block(sCLNodeType* type_, unsigned int block)
{
    unsigned int i;
    
    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_BLOCK;

    gNodes[i].uValue.mBlock = block;

    ASSERT(type_ != 0);
    gNodes[i].mType = type_;

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
    "NODE_TYPE_REVERT", 
    "NODE_TYPE_BLOCK",
    "NODE_TYPE_CHARACTER_VALUE",
    "NODE_TYPE_THROW",
    "NODE_TYPE_TRY",
    "NODE_TYPE_CLASS_NAME",
    "NODE_TYPE_BYTES_VALUE", 
    "NODE_TYPE_RANGE_VALUE", 
    "NODE_TYPE_HASH_VALUE", 
    "NODE_TYPE_TUPLE_VALUE", 
    "NODE_TYPE_REGEX_VALUE",
    "NODE_TYPE_STORE_TUPLE",
    "NODE_TYPE_BYTE_VALUE",
    "NODE_TYPE_SHORT_VALUE",
    "NODE_TYPE_UINT_VALUE",
    "NODE_TYPE_LONG_VALUE",
    "NODE_TYPE_PATH_VALUE", 
};

void show_node(unsigned int node)
{
    unsigned int left_node;
    unsigned int right_node;
    unsigned int middle_node;

    left_node = gNodes[node].mLeft;
    right_node = gNodes[node].mRight;
    middle_node = gNodes[node].mMiddle;

    if(node) {
        printf("type %s", node_type_string[gNodes[node].mNodeType-1]);
    }
    if(left_node) {
        printf(" left %s", node_type_string[gNodes[left_node].mNodeType-1]);
    }
    if(right_node) {
        printf(" right %s", node_type_string[gNodes[right_node].mNodeType-1]);
    }
    if(middle_node) {
        printf(" middle %s", node_type_string[gNodes[middle_node].mNodeType-1]);
    }
    puts("");
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

unsigned int alloc_node_block(sCLNodeType* block_type)
{
    int size;
    int i;

    if(gUsedBlocks == gSizeBlocks) {
        int new_size;

        new_size = (gSizeBlocks+1) * 2;
        gNodeBlocks = xxrealloc(gNodeBlocks, sizeof(sNodeBlock)*gSizeBlocks, sizeof(sNodeBlock)*new_size);
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
        block->mNodes = xxrealloc(block->mNodes, sizeof(sNode)*block->mSizeNodes, sizeof(sNode)*new_size);
        memset(block->mNodes + block->mSizeNodes, 0, sizeof(sNode)*(new_size - block->mSizeNodes));
        block->mSizeNodes = new_size;
    }

    block->mNodes[block->mLenNodes] = *node;
    block->mLenNodes++;
}

unsigned int sNodeTree_create_null()
{
    unsigned int i;

    i = alloc_node();

    gNodes[i].mNodeType = NODE_TYPE_NULL;

    gNodes[i].mType = NULL;

    gNodes[i].mLeft = 0;
    gNodes[i].mRight = 0;
    gNodes[i].mMiddle = 0;

    return i;
}
