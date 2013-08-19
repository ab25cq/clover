#include "clover.h"

// skip spaces
static void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
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
typedef struct {
    char* mName;

    char** mMethods;
    uint mNumMethods;
    uint mSizeMethods;
} sClass;

static sClass* sClass_new(char* name)
{
    sClass* self = MALLOC(sizeof(sClass));

    self->mName = STRDUP(name);

    self->mMethods = MALLOC(sizeof(char*)*128);
    self->mNumMethods = 0;
    self->mSizeMethods = 128;

    return self;
}

static void sClass_delete(sClass* self)
{
    FREE(self->mName);

    int i;
    for(i=0; i<self->mNumMethods; i++) {
        FREE(self->mMethods[i]);
    }
    FREE(self->mMethods);

    FREE(self);
}

static void sClass_add_method(sClass* self, char* name)
{
    if(self->mNumMethods == self->mSizeMethods) {
        const int new_size = (self->mSizeMethods + 1) * 2;
        self->mMethods = REALLOC(self->mMethods, sizeof(char*)*new_size);
        memset(self->mMethods + self->mSizeMethods, 0, sizeof(char*)*(new_size - self->mSizeMethods));
        self->mSizeMethods = new_size;
    }
    self->mMethods[self->mNumMethods] = STRDUP(name);
    self->mNumMethods++;
}

static hash_obj* gParserClasses;

static sClass* gIntClass;
static sClass* gStringClass;
static sClass* gFloatClass;

//////////////////////////////////////////////////
// parser var
//////////////////////////////////////////////////

typedef struct {
    char* mName;
    sClass* mClass;
    uint mVariableNumber;
} sVar;

static sVar* sVar_new(char* name, sClass* klass, uint varibale_num)
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

typedef struct {
    uchar mType;
    sClass* mClass;

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

static uint sNodeTree_create_var(char* var_name, sClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_define_var(char* var_name, sClass* klass, uint left, uint right, uint middle)
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

        sClass* klass = hash_item(gParserClasses, buf);

        if(klass) {
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

static void sConst_append(sConst* self, void* data, uint size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mConst = REALLOC(self->mConst, sizeof(uchar) * self->mSize);
    }

    memcpy(self->mConst + self->mLen, data, size);
    self->mLen += size;
}

static BOOL compile_node(uint node, sClass** klass, sByteCode* code, sConst* constant, char* sname, int* sline)
{
    switch(gNodes[node].mType) {
        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(gNodes[node].mOperand) {
            case kOpAdd: {
                sClass* left_class = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline)) {
                        return FALSE;
                    }
                }
                sClass* right_class = NULL;
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

                sClass* left_class;
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
                    left_class = var->mClass;

                    var = hash_item(gGlobalVars, gNodes[lnode].mVarName);

                    if(var == NULL) {
                        char buf[128];
                        snprintf(buf, 128, "there is no definition of this variable(%s)\n", gNodes[lnode].mVarName);
                        parser_err_msg(buf, sname, *sline);
                        return FALSE;
                    }
                }


                sClass* right_class = NULL;
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

            uchar const_type = CONSTANT_STRING;
            sConst_append(constant, &const_type, sizeof(const_type));

            uint len = strlen(gNodes[node].mStringValue);
            wchar_t* wcs = MALLOC(sizeof(wchar_t)*(len+1));
            mbstowcs(wcs, gNodes[node].mStringValue, len+1);

            sConst_append(constant, &len, sizeof(len));
            sConst_append(constant, wcs, sizeof(wchar_t)*len);

            FREE(wcs);

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
    }

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

        sClass* klass;
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

void parser_init()
{
    gParserClasses = HASH_NEW(10);
    hash_put(gParserClasses, "int", gIntClass = sClass_new("int"));
    hash_put(gParserClasses, "float", gFloatClass = sClass_new("float"));
    hash_put(gParserClasses, "String", gStringClass = sClass_new("String"));

    gGlobalVars = HASH_NEW(10);
}

void parser_final()
{
    hash_it* it = hash_loop_begin(gParserClasses);
    while(it) {
        sClass* klass = hash_loop_item(it);
        sClass_delete(klass);
        it = hash_loop_next(it);
    }
    hash_delete(gParserClasses);

    it = hash_loop_begin(gGlobalVars);
    while(it) {
        sVar* var = hash_loop_item(it);
        sVar_delete(var);
        it = hash_loop_next(it);
    }
    hash_delete(gGlobalVars);
}

