#include "clover.h"
#include "common.h"

// skip spaces
static void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static void skip_spaces_and_lf(char** p, uint* sline)
{
    while(**p == ' ' || **p == '\t' || **p == '\n' && (*sline)++) {
        (*p)++;
    }
}

//////////////////////////////////////////////////
// resizable buf
//////////////////////////////////////////////////
typedef struct {
    char* mBuf;
    uint mSize;
    uint mLen;
} sBuf;

static void sBuf_init(sBuf* self)
{
    self->mBuf = MALLOC(sizeof(char)*64);
    self->mSize = 64;
    self->mLen = 0;
    *(self->mBuf) = 0;
}

static void sBuf_append_char(sBuf* self, char c)
{
    if(self->mSize <= self->mLen + 1 + 1) {
        self->mSize = (self->mSize + 1 + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    self->mBuf[self->mLen] = c;
    self->mBuf[self->mLen+1] = 0;
    self->mLen++;
}

static void sBuf_append(sBuf* self, char* str)
{
    const int len = strlen(str);

    if(self->mSize <= self->mLen + len + 1) {
        self->mSize = (self->mSize + len + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    strcat(self->mBuf, str);

    self->mLen += len;
}

//////////////////////////////////////////////////
// parser class
//////////////////////////////////////////////////
static sCLClass* gIntClass;
static sCLClass* gStringClass;
static sCLClass* gFloatClass;

//////////////////////////////////////////////////
// parser var
//////////////////////////////////////////////////
typedef struct {
    char* mName;
    sCLClass* mClass;
    uint mVariableNumber;
} sVar;

static sVar* sVar_new(char* name, sCLClass* klass, uint varibale_num)
{
    sVar* self = MALLOC(sizeof(sVar));

    self->mName = STRDUP(name);
    self->mClass = klass;
    self->mVariableNumber = varibale_num;

    return self;
}

static void sVar_delete(sVar* self)
{
    FREE(self->mName);
    FREE(self);
}

static hash_obj* gGlobalVars;

//////////////////////////////////////////////////
// define node tree and memory management memory it
//////////////////////////////////////////////////
enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpEqual, kOpPlusPlus2, kOpMinusMinus2
};

#define NODE_TYPE_OPERAND 0
#define NODE_TYPE_VALUE 1
#define NODE_TYPE_STRING_VALUE 2
#define NODE_TYPE_VARIABLE_NAME 3
#define NODE_TYPE_DEFINE_VARIABLE_NAME 4
#define NODE_TYPE_CLASS_METHOD_CALL 5
#define NODE_TYPE_PARAM 6

typedef struct {
    uchar mType;
    sCLClass* mClass;

    union {
        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
        char* mVarName;
    };

    uint mLeft;     // node index
    uint mRight;
    uint mMiddle;
} sNodeTree;

static sNodeTree* gNodes;
static uint gSizeNode;
static uint gUsedNode;

static void init_nodes()
{
    const int node_size = 32;

    gNodes = CALLOC(1, sizeof(sNodeTree)*node_size);
    gSizeNode = node_size;
    gUsedNode = 1;   // 0 of index means null
}

static void free_nodes()
{
    int i;
    for(i=1; i<gUsedNode; i++) {
        switch(gNodes[i].mType) {
            case NODE_TYPE_STRING_VALUE:
                FREE(gNodes[i].mStringValue);
                break;

            case NODE_TYPE_VARIABLE_NAME:
                FREE(gNodes[i].mVarName);
                break;

            case NODE_TYPE_DEFINE_VARIABLE_NAME:
                FREE(gNodes[i].mVarName);
                break;

            case NODE_TYPE_CLASS_METHOD_CALL:
                FREE(gNodes[i].mVarName);
                break;
        }
    }

    FREE(gNodes);
}

// return node index
static uint alloc_node()
{
    if(gSizeNode == gUsedNode) {
        const int new_size = (gSizeNode+1) * 2;
        gNodes = REALLOC(gNodes, sizeof(sNodeTree)*new_size);
        memset(gNodes + gSizeNode, 0, sizeof(sNodeTree)*(new_size - gSizeNode));
    }

    return gUsedNode++;
}

// return node index
static uint sNodeTree_create_operand(enum eOperand operand, uint left, uint right, uint middle)
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

static uint sNodeTree_create_value(int value, uint left, uint right, uint middle)
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

static uint sNodeTree_create_string_value(MANAGED char* value, uint left, uint right, uint middle)
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

static uint sNodeTree_create_var(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_define_var(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_class_method_call(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_param(uint left, uint right, uint middle)
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

//////////////////////////////////////////////////
// parse it
//////////////////////////////////////////////////
static parser_err_msg(char* msg, char* sname, int sline)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline, msg);
}


static BOOL expression_node(uint* node, char** p, char* sname, int* sline)
{
    if(**p >= '0' && **p <= '9' || **p == '-' || **p == '+') {
        char buf[128];
        char* p2 = buf;

        if(**p == '-') {
            *p2++ = '-';
            (*p)++;
        }
        else if(**p =='+') {
            (*p)++;
        }

        while(**p >= '0' && **p <= '9') {
            *p2++ = **p;
            (*p)++;

            if(p2 - buf >= 128) {
                parser_err_msg("overflow node of number",  sname, *sline);
                return FALSE;
            }
        }
        *p2 = 0;
        skip_spaces(p);

        *node = sNodeTree_create_value(atoi(buf), 0, 0, 0);
    }
    else if(**p == '"') {
        (*p)++;

        sBuf value;
        sBuf_init(&value);

        while(1) {
            if(**p == '"') {
                (*p)++;
                break;
            }
            else if(**p == '\\') {
                (*p)++;
                switch(**p) {
                    case 'n':
                        sBuf_append_char(&value, '\n');
                        (*p)++;
                        break;

                    case 't':
                        sBuf_append_char(&value, '\t');
                        (*p)++;
                        break;

                    case 'r':
                        sBuf_append_char(&value, '\r');
                        (*p)++;
                        break;

                    case 'a':
                        sBuf_append_char(&value, '\a');
                        (*p)++;
                        break;

                    case '\\':
                        sBuf_append_char(&value, '\\');
                        (*p)++;
                        break;

                    default:
                        sBuf_append_char(&value, **p);
                        (*p)++;
                        break;
                }
            }
            else if(**p == 0) {
                parser_err_msg("close \" to make string value", sname, *sline);
                return FALSE;
            }
            else {
                sBuf_append_char(&value, **p);
                (*p)++;
            }
        }

        skip_spaces(p);

        *node = sNodeTree_create_string_value(MANAGED value.mBuf, 0, 0, 0);
    }
    else if(isalpha(**p) || **p == '_') {
        char buf[128];
        char* p2 = buf;

        while(isalpha(**p) || **p == '_') {
            *p2++ = **p;
            (*p)++;

            if(p2 - buf >= 128) {
                parser_err_msg("overflow node of variable name",  sname, *sline);
                return FALSE;
            }
        }
        *p2 = 0;
        skip_spaces(p);

        sCLClass* klass = cl_get_class(buf);

        if(klass) {
            /// call class method ///
            if(**p == '.') {
                (*p)++;
                skip_spaces(p);

                if(isalpha(**p) || **p == '_') {
                    p2 = buf;

                    while(isalpha(**p) || **p == '_') {
                        *p2++ = **p;
                        (*p)++;

                        if(p2 - buf >= 128) {
                            parser_err_msg("overflow node of variable name",  sname, *sline);
                            return FALSE;
                        }
                    }
                    *p2 = 0;
                    skip_spaces(p);
                }
                else {
                    parser_err_msg("require method name", sname, *sline);
                    return FALSE;
                }

                uint old_node = 0;
                uint num_params = 0;
                if(**p == '(') {
                    (*p)++;
                    skip_spaces(p);

                    while(1) {
                        uint new_node;
                        if(!node_expression(&new_node, p, sname, sline)) {
                            return FALSE;
                        }
                        num_params++;

                        skip_spaces(p);

                        old_node = sNodeTree_create_param(old_node, new_node,  0);

                        if(**p == ',') {
                            (*p)++;
                            skip_spaces(p);
                        }
                        else if(**p == ')') {
                            (*p)++;
                            skip_spaces(p);
                            break;
                        }
                        else {
                            char buf[128];
                            snprintf(buf, 128, "unexpected character (%c)\n", **p);
                            parser_err_msg(buf, sname, *sline);
                            return FALSE;
                        }
                    }
                }

                /// get method ///
                uint method_index = cl_get_method_index(klass, buf);
                if(method_index == -1) {
                    char buf2[128];
                    snprintf(buf2, 128, "not defined this method(%s)\n", buf);
                    parser_err_msg(buf2, sname, *sline);
                    return FALSE;
                }

                /// type checking ///
                const int method_num_params = cl_get_method_num_params(klass, method_index);
                ASSERT(method_num_params != -1);

                if(num_params != method_num_params) {
                    char buf2[128];
                    snprintf(buf2, 128, "Parametor number of (%s) is %d\n", METHOD_NAME(klass, method_index), method_num_params);
                    parser_err_msg(buf2, sname, *sline);
                    return FALSE;
                }

                if(method_num_params > 0) {
                    uint node_num = old_node;
                    int i;
                    for(i=method_num_params-1; i>= 0; i--) {
                        sCLClass* klass2 = cl_get_method_param_types(klass, method_index, i);
                        ASSERT(klass2 != NULL);
                        uint right_node = gNodes[node_num].mRight;

                        if(gNodes[right_node].mClass != klass2) {
                            char buf2[128];
                            snprintf(buf2, 128, "Type of parametor number %d is a different type.", i);
                            parser_err_msg(buf2, sname, *sline);
                            return FALSE;
                        }
                        node_num = gNodes[node_num].mLeft;
                    }
                }

                *node = sNodeTree_create_class_method_call(buf, klass, old_node, 0, 0);
            }
            /// define global var ///
            else {
                p2 = buf;

                while(isalpha(**p) || **p == '_') {
                    *p2++ = **p;
                    (*p)++;

                    if(p2 - buf >= 128) {
                        parser_err_msg("overflow node of variable name",  sname, *sline);
                        return FALSE;
                    }
                }
                *p2 = 0;
                skip_spaces(p);

                *node = sNodeTree_create_define_var(buf, klass, 0, 0, 0);
            }
        }
        /// global vars ///
        else {
            sVar* var = hash_item(gGlobalVars, buf);

            if(var == NULL) {
                char buf2[128];
                snprintf(buf2, 128, "there is no definition of this variable(%s)\n", buf);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_var(buf, var->mClass, 0, 0, 0);
        }

        /// tail ///
        if(**p == '+' && *(*p+1) == '+') {
            (*p)+=2;
            skip_spaces(p);

            *node = sNodeTree_create_operand(kOpPlusPlus2, *node, 0, 0);
        }
        else if(**p == '-' && *(*p+1) == '-') {
            (*p)+=2;
            skip_spaces(p);

            *node = sNodeTree_create_operand(kOpMinusMinus2, *node, 0, 0);
        }
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces(p);

        if(!node_expression(node, p, sname, sline)) {
            return FALSE;
        }
        skip_spaces(p);

        if(**p != ')') {
            parser_err_msg("require )", sname, *sline);
            return FALSE;
        }
        (*p)++;
        skip_spaces(p);

        if(*node == 0) {
            parser_err_msg("require expression as ( operand", sname, *sline);
            return FALSE;
        }
    }
    else if(**p == ';') {
        *node = 0;
    }
    else {
        char buf[128];
        snprintf(buf, 128, "invalid character (%c)", **p);
        parser_err_msg(buf, sname, *sline);
        *node = 0;
        return FALSE;
    }

    return TRUE;
}

static BOOL expression_mult_div(uint* node, char** p, char* sname, int* sline)
{
    if(!expression_node(node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '*' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);
            uint right;
            if(!expression_node(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpMult, *node, right, 0);
        }
        else if(**p == '/' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_node(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpDiv, *node, right, 0);
        }
        else if(**p == '%' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_node(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpMod, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_add_sub(uint* node, char** p, char* sname, int* sline)
{
    if(!expression_mult_div(node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) != '=' && *(*p+1) != '+') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_mult_div(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpAdd, *node, right, 0);
        }
        else if(**p == '-' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_mult_div(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpSub, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from right to left order
static BOOL expression_equal(uint* node, char** p, char* sname, int* sline)
{
    if(!expression_add_sub(node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '=') {
            (*p)++;
            skip_spaces(p);
            uint right;
            if(!expression_equal(&right, p, sname, sline)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                return FALSE;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                return FALSE;
            }

            if(gNodes[*node].mType != NODE_TYPE_VARIABLE_NAME && gNodes[*node].mType != NODE_TYPE_DEFINE_VARIABLE_NAME) {
                parser_err_msg("require varible name on left value of equal", sname, *sline);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpEqual, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(uint* node, char** p, char* sname, int* sline)
{
    return expression_equal(node, p, sname, sline);
}

static void sByteCode_append(sByteCode* self, void* code, uint size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mCode = REALLOC(self->mCode, sizeof(uchar) * self->mSize);
    }

    memcpy(self->mCode + self->mLen, code, size);
    self->mLen += size;
}

void sConst_append(sConst* self, void* data, uint size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mConst = REALLOC(self->mConst, sizeof(uchar) * self->mSize);
    }

    memcpy(self->mConst + self->mLen, data, size);
    self->mLen += size;
}

void sConst_append_str(sConst* constant, uchar* str)
{
    uchar type = CONSTANT_STRING;
    sConst_append(constant, &type, sizeof(uchar));

    uint len = strlen(str);
    sConst_append(constant, &len, sizeof(len));
    sConst_append(constant, str, len+1);
}

void sConst_append_wstr(sConst* constant, uchar* str)
{
    uchar type = CONSTANT_WSTRING;
    sConst_append(constant, &type, sizeof(uchar));

    uint len = strlen(str);
    wchar_t* wcs = MALLOC(sizeof(wchar_t)*(len+1));
    mbstowcs(wcs, str, len+1);

    sConst_append(constant, &len, sizeof(len));
    sConst_append(constant, wcs, sizeof(wchar_t)*(len+1));

    FREE(wcs);
}

static BOOL compile_node(uint node, sCLClass** klass, sByteCode* code, sConst* constant, char* sname, int* sline)
{
    switch(gNodes[node].mType) {
        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(gNodes[node].mOperand) {
            case kOpAdd: {
                sCLClass* left_class = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline)) {
                        return FALSE;
                    }
                }
                sCLClass* right_class = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline)) {
                        return FALSE;
                    }
                }

                if(left_class != right_class) {
                    parser_err_msg("addition with not same class", sname, *sline);
                    return FALSE;
                }

                uchar c;
                if(left_class == gStringClass) {
                    c = OP_SADD;
                    *klass = gStringClass;
                }
                else if(left_class == gIntClass) {
                    c = OP_IADD;
                    *klass = gIntClass;
                }
                else if(left_class == gFloatClass) {
                    c = OP_FADD;

                    *klass = gFloatClass;
                }

                sByteCode_append(code, &c, sizeof(uchar));

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

            case kOpEqual: {
                uint lnode = gNodes[node].mLeft; // lnode must be variable name or definition of variable

                sCLClass* left_class;
                sVar* var;
                if(gNodes[lnode].mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline)) {
                        return FALSE;
                    }

                    var = hash_item(gGlobalVars, gNodes[lnode].mVarName);

                    if(var == NULL) {
                        char buf[128];
                        snprintf(buf, 128, "there is no definition of this variable(%s)\n", gNodes[lnode].mVarName);
                        parser_err_msg(buf, sname, *sline);
                        return FALSE;
                    }
                }
                else { // NODE_TYPE_VARIABLE_NAME
                    var = hash_item(gGlobalVars, gNodes[lnode].mVarName);

                    if(var == NULL) {
                        char buf[128];
                        snprintf(buf, 128, "there is no definition of this variable(%s)\n", gNodes[lnode].mVarName);
                        parser_err_msg(buf, sname, *sline);
                        return FALSE;
                    }

                    left_class = var->mClass;
                }

                sCLClass* right_class = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline)) {
                        return FALSE;
                    }
                }

                /// type checking ///
                if(left_class != right_class) {
                    parser_err_msg("type error", sname, *sline);
                    return FALSE;
                }

                uchar c;
                if(left_class == gIntClass) {
                    c = OP_ISTORE;
                }
                else if(left_class == gStringClass) {
                    c = OP_ASTORE;
                }
                else if(left_class == gFloatClass) {
                    c = OP_FSTORE;
                }

                sByteCode_append(code, &c, sizeof(uchar));
                int var_num = var->mVariableNumber;
                sByteCode_append(code, &var_num, sizeof(int));

                *klass = var->mClass;
                }
                break;
            }
            break;

        /// number value ///
        case NODE_TYPE_VALUE: {
            uchar c = OP_LDC;
            sByteCode_append(code, &c, sizeof(uchar));
            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            uchar const_type = CONSTANT_INT;
            sConst_append(constant, &const_type, sizeof(const_type));
            int data = gNodes[node].mValue;
            sConst_append(constant, &data, sizeof(data));

            *klass = gIntClass;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
            uchar c = OP_LDC;
            sByteCode_append(code, &c, sizeof(uchar));
            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            sConst_append_wstr(constant, gNodes[node].mStringValue);

            *klass = gStringClass;
            }
            break;

        /// variable name ///
        case NODE_TYPE_VARIABLE_NAME: {
            sVar* var = hash_item(gGlobalVars, gNodes[node].mVarName);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is this varialbe (%s)\n", gNodes[node].mVarName);
                parser_err_msg(buf, sname, *sline);
                return FALSE;
            }

            uchar c;
            if(var->mClass == gIntClass) {
                c = OP_ILOAD;
            }
            else if(var->mClass == gStringClass) {
                c = OP_ALOAD;
            }
            else if(var->mClass == gFloatClass) {
                c = OP_FLOAD;
            }

            sByteCode_append(code, &c, sizeof(uchar));
            int var_num = var->mVariableNumber;
            sByteCode_append(code, &var_num, sizeof(int));

            *klass = var->mClass;
            }
            break;

        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;

            if(hash_item(gGlobalVars, name)) {
                char buf2[128];
                snprintf(buf2, 128, "there is a same name variable(%s)\n", name);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            hash_put(gGlobalVars, name, sVar_new(name, gNodes[node].mClass, hash_count(gGlobalVars)));

            *klass = gNodes[node].mClass;
            }
            break;

        case NODE_TYPE_PARAM: {
            sCLClass* left_class  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline)) {
                    return FALSE;
                }
            }
            sCLClass* right_class = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline)) {
                    return FALSE;
                }
            }

            *klass = right_class;
            }
            break;

        case NODE_TYPE_CLASS_METHOD_CALL: {
            sCLClass* left_class = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline)) {
                    return FALSE;
                }
            }

            char* method_name = gNodes[node].mVarName;
            sCLClass* cl_klass = gNodes[node].mClass;

            uchar c = OP_INVOKE_STATIC_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            uint constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));

            uint method_index = cl_get_method_index(cl_klass, method_name);
            sByteCode_append(code, &method_index, sizeof(uint));

            sConst_append_str(constant, CLASS_NAME(cl_klass));

            sCLClass* result_type = cl_get_method_result_type(cl_klass, method_index);
            ASSERT(result_type);

            *klass = result_type;
            }
            break;
    }

    return TRUE;
}

static BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline)
{
    buf[0] = 0;

    char* p2 = buf;

    if(isalpha(**p) || **p == '_') {
        while(isalnum(**p) || **p == '_') {
            if(p2 - buf < buf_size) {
                *p2++ = **p;
                (*p)++;
            }
            else {
                char buf[BUFSIZ];
                snprintf(buf, BUFSIZ, "length of word is too long");
                parser_err_msg(buf, sname, *sline);

                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(buf[0] == 0) {
        char buf[BUFSIZ];
        snprintf(buf, BUFSIZ, "require word. this is (%c)\n", **p);
        parser_err_msg(buf, sname, *sline);

        return FALSE;
    }

    return TRUE;
}

static BOOL skip_block(char** p, char* sname, int* sline)
{
    uint nest = 1;
    while(1) {
        if(**p == '{') {
            (*p)++;
            nest++;
        }
        else if(**p == '}') {
            (*p)++;

            nest--;
            if(nest == 0) {
                break;
            }
        }
        else if(**p == '\n') {
            (*p)++;
            (*sline)++;
        }
        else if(**p == 0) {
            char buf[128];
            snprintf(buf, 128, "It arrived at the end of source before block closing\n");
            
            parser_err_msg(buf, sname, *sline);
            return FALSE;
        }
        else {
            (*p)++;
        }
    }

    return TRUE;
}

typedef struct {
    uint mFlags;
    sConst mConstant;
    uint mNumMethods;
    sCLMethod mMethods[CL_METHODS_MAX];
    uint mNumFields;
    sCLField mFields[CL_METHODS_MAX];

    uchar* mClassName;

    uint mClassNameOffset;
} sDefClass;

static void sDefClass_init(sDefClass* self)
{
    memset(self, 0, sizeof(sDefClass));

    self->mConstant.mSize = 1024;
    self->mConstant.mConst = CALLOC(1, sizeof(uchar)*self->mConstant.mSize);
    self->mClassNameOffset = -1;
}

static void sDefClass_final(sDefClass* self)
{
    FREE(self->mConstant.mConst);

    int i;
    for(i=0; i<self->mNumMethods; i++) {
        FREE(self->mMethods[i].mParamTypes);
        //FREE(self->mMethods[i].mParamNames);
    }
}

static BOOL method_and_field_definition(char** p, char* buf, sDefClass* klass, char* sname, int* sline)
{
    while(1) {
        if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// prefix ///
        BOOL static_ = FALSE;
        BOOL private_ = FALSE;

        while(**p) {
            if(strcmp(buf, "static") == 0) {
                static_ = TRUE;

                if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

        /// type ///
        sCLClass* type_ = cl_get_class(buf);

        if(type_ == NULL) {
            char buf2[BUFSIZ];
            snprintf(buf2, BUFSIZ, "There is no definition of this class(%s)\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }

        /// name ///
        char name[CL_METHOD_NAME_MAX];
        if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// method ///
        if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            //uint name_params[CL_METHOD_PARAM_MAX];
            uint class_params[CL_METHOD_PARAM_MAX];
            uint num_params = 0;

            /// params ///
            if(**p == ')') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
            else {
                while(1) {
                    /// type ///
                    if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(p, sline);
                    
                    sCLClass* param_type = cl_get_class(buf);

                    if(param_type == NULL) {
                        char buf2[BUFSIZ];
                        snprintf(buf2, BUFSIZ, "There is no definition of this class(%s)\n", buf);
                        parser_err_msg(buf2, sname, *sline);
                        return FALSE;
                    }

                    /// name ///
                    char name[CL_METHOD_NAME_MAX];
                    if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(p, sline);

                    //name_params[num_params] = klass->mConstant.mLen;
                    //sConst_append_str(&klass->mConstant, name);
                    class_params[num_params] = klass->mConstant.mLen;
                    sConst_append_str(&klass->mConstant, CLASS_NAME(param_type));
                    num_params++;

                    if(num_params >= CL_METHOD_PARAM_MAX) {
                        parser_err_msg("overflow method parametor number", sname, *sline);
                        return FALSE;
                    }
                    
                    if(**p == ')') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
                        break;
                    }
                    else if(**p == ',') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
                    }
                    else if(**p == '\0') {
                        parser_err_msg("It arrived at the end of source in method definition\n", sname, *sline);
                        return FALSE;
                    }
                    else {
                        char buf2[BUFSIZ];
                        snprintf(buf2, BUFSIZ, "require ) or , for method definition. this is (%c)\n", **p);
                        parser_err_msg(buf2, sname, *sline);
                        return FALSE;
                    }
                }
            }

            if(**p == '{') {
                int sline2 = *sline;

                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!skip_block(p, sname, sline)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                if(**p == '\0') {
                    parser_err_msg("It arrived at the end of source.\n", sname, sline2);
                    return FALSE;
                }
            }
            else {
                parser_err_msg("require { for method definition\n", sname, *sline);
                return FALSE;
            }

            sCLMethod* method = klass->mMethods + klass->mNumMethods;
            method->mHeader = (static_ ? CL_STATIC_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0);

            method->mNameOffset = klass->mConstant.mLen;
            sConst_append_str(&klass->mConstant, name);

            method->mPathOffset = klass->mConstant.mLen;

            const int size = strlen(name) + strlen(klass->mClassName) + 2;
            char* buf = MALLOC(size);
            snprintf(buf, size, "%s.%s", klass->mClassName, name);
            sConst_append_str(&klass->mConstant, buf);
            FREE(buf);

            method->mResultType = klass->mConstant.mLen;
            sConst_append_str(&klass->mConstant, CLASS_NAME(type_));

            method->mParamTypes = CALLOC(1, sizeof(uint)*num_params);
            //method->mParamNames = CALLOC(1, sizeof(uint)*num_params);

            int i;
            for(i=0; i<num_params; i++) {
                method->mParamTypes[i] = class_params[i];
                //method->mParamNames[i] = name_params[i];
            }
            method->mNumParams = num_params;

            klass->mNumMethods++;
            
            if(klass->mNumMethods >= CL_METHODS_MAX) {
                parser_err_msg("overflow number methods", sname, *sline);
                return FALSE;
            }
        }
        /// field ///
        else if(**p == ';') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            sCLField* field = klass->mFields + klass->mNumFields;

            field->mHeader = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

            field->mNameOffset = klass->mConstant.mLen;
            sConst_append_str(&klass->mConstant, name);    // field name

            field->mClassNameOffset = klass->mConstant.mLen;
            sConst_append_str(&klass->mConstant, CLASS_NAME(type_));  // class name

            klass->mNumFields++;
            
            if(klass->mNumFields >= CL_FIELDS_MAX) {
                parser_err_msg("overflow number methods", sname, *sline);
                return FALSE;
            }
        }
        else {
            parser_err_msg("Syntex error. require method or field definition.\n", sname, *sline);
            return FALSE;
        }

        if(**p == '\0') {
            parser_err_msg("It arrived at the end of source.\n", sname, *sline);
            return FALSE;
        }
        else if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
            break;
        }
    }

    return TRUE;
}

static BOOL class_definition(char** p, char* buf, sDefClass* klass, char* sname, int* sline)
{
    while(**p) {
        if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
            return FALSE;
        }

        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "class") == 0) {
            /// class name ///
            if(!parse_word(buf, BUFSIZ, p, sname, sline)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            klass->mClassNameOffset = klass->mConstant.mLen;
            sConst_append_str(&klass->mConstant, buf);  // class name

            klass->mClassName = klass->mConstant.mConst + klass->mClassNameOffset;

            if(**p == '{') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(**p == '}') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);
                }
                else {
                    if(!method_and_field_definition(p, buf, klass, sname, sline)) {
                        return FALSE;
                    }
                }
            }
            else {
                char buf2[BUFSIZ];
                snprintf(buf2, BUFSIZ, "require { after class name. this is (%c)\n", **p);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }
        }
        else {
            char buf2[128];
            snprintf(buf2, 128, "syntax error(%s). require \"class\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

// source is null-terminated
BOOL cl_parse_class(char* source, char* sname, int* sline)
{
    sDefClass klass;

    sDefClass_init(&klass);

    /// get methods and fields ///
    char* p = source;
    char buf[BUFSIZ];

    skip_spaces_and_lf(&p, sline);

    if(!class_definition(&p, buf, &klass, sname, sline)) {
        sDefClass_final(&klass);
        return FALSE;
    }

    /// do compile ///
/*
    if(!class_definition2(&p, buf, &klass, sname, sline)) {
        sDefClass_final(&klass);
        return FALSE;
    }
*/

    sDefClass_final(&klass);

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, int* global_var_num, BOOL flg_main)
{
    init_nodes();

    char* p = source;
    while(*p) {
        uint node = 0;
        if(!node_expression(&node, &p, sname, sline)) {
            free_nodes();
            return FALSE;
        }

        sCLClass* klass;
        if(!compile_node(node, &klass, code, constant, sname, sline)) {
            free_nodes();
            return FALSE;
        }

        if(*p == ';' || *p == '\n') {
            while(*p == ';' || *p == '\n') {
                p++;
                skip_spaces(&p);
            }
        }
        else {
            char buf[128];
            snprintf(buf, 128, "unexpected character(%c)\n", *p);
            parser_err_msg(buf, sname, *sline);
            free_nodes();
            return FALSE;
        }
    }

    *global_var_num = hash_count(gGlobalVars);
    free_nodes();

    return TRUE;
}

void parser_init(BOOL load_foundamental_class)
{
    if(load_foundamental_class) {
        gIntClass = cl_get_class("int");
        gFloatClass = cl_get_class("float");
        gStringClass = cl_get_class("String");

        if(gIntClass == NULL || gFloatClass == NULL || gStringClass == NULL) {
            fprintf(stderr, "Loading Foundamental class is failed. abort. check your compiling of clover");
            exit(0);
        }
    }

    gGlobalVars = HASH_NEW(10);
}

void parser_final()
{
    hash_it* it = hash_loop_begin(gGlobalVars);
    while(it) {
        sVar* var = hash_loop_item(it);
        sVar_delete(var);
        it = hash_loop_next(it);
    }
    hash_delete(gGlobalVars);
}

