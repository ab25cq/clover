#include "clover.h"
#include "common.h"

static sVarTable gGVTable;

static BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num);

// skip spaces
static void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

void skip_spaces_and_lf(char** p, uint* sline)
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


static sCLClass* gIntClass;
static sCLClass* gStringClass;
static sCLClass* gFloatClass;

// result: (-1) --> not found (non -1) --> method index
uint get_method_index(sCLClass* klass, uchar* method_name)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            return i;
        }
    }

    return -1;
}

// result: (-1) --> not found (non -1) --> index
static uint get_method_num_params(sCLClass* klass, uint method_index)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        return klass->mMethods[method_index].mNumParams;
    }

    return -1;
}

// result (NULL) --> not found (pointer of sCLClass) --> found
static sCLClass* get_method_param_types(sCLClass* klass, uint method_index, uint param_num)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        sCLMethod* method = klass->mMethods + method_index;

        if(param_num >= 0 && param_num < method->mNumParams && method->mParamTypes != NULL) {
            uchar* class_name = CONS_str(klass->mConstPool, method->mParamTypes[param_num]);
            return cl_get_class(class_name);
        }
    }

    return NULL;
}

// result: (NULL) --> not found (sCLClass pointer) --> found
static sCLClass* get_method_result_type(sCLClass* klass, uint method_index)
{
    if(klass != NULL && method_index >= 0 && method_index < klass->mNumMethods) {
        sCLMethod* method = klass->mMethods + method_index;
        uchar* class_name = CONS_str(klass->mConstPool, method->mResultType);
        return cl_get_class(class_name);
    }

    return NULL;
}

//////////////////////////////////////////////////
// local variable and global variable
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table
BOOL add_variable(sVarTable* table, uchar* name, sCLClass* klass)
{
    int hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    sField* p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mFieldName[0] == 0) {
            xstrncpy(p->mFieldName, name, CL_METHOD_NAME_MAX);
            p->mIndex = table->mVarNum++;
            p->mClass = klass;

            return TRUE;
        }
        else {
            p++;

            if(p == table->mLocalVariables + CL_LOCAL_VARIABLE_MAX) {
                p = table->mLocalVariables;
            }
            else if(p == table->mLocalVariables + hash_value) {
                return FALSE;
            }
        }
    }
}

// result: (null) not found (sField*) found
sField* get_variable(sVarTable* table, uchar* name)
{
    int hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    sField* p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mFieldName[0] == 0) {
            return NULL;
        }
        else if(strcmp((char*)p->mFieldName, name) == 0) {
            return p;
        }

        p++;

        if(p == table->mLocalVariables + CL_LOCAL_VARIABLE_MAX) {
            p = table->mLocalVariables;
        }
        else if(p == table->mLocalVariables + hash_value) {
            return NULL;
        }
    }
}

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
#define NODE_TYPE_CLASS_METHOD_CALL 6
#define NODE_TYPE_PARAM 7

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
void parser_err_msg(char* msg, char* sname, int sline)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline, msg);
}

// characters is null-terminated
BOOL expect_next_character(uchar* characters, int* err_num, char** p, char* sname, int* sline)
{
    BOOL err = FALSE;
    while(1) {
        if(**p == '0') {
            char buf[WORDSIZ];
            snprintf(buf, WORDSIZ, "clover has expected that next characters are '%s', but it arrived at source end", characters);
            parser_err_msg(buf, sname, *sline);
            return FALSE;
        }

        BOOL found = FALSE;
        char* p2 = characters;
        while(*p2) {
            if(**p == *p2) {
                found = TRUE;
                break;
            }
            else {
                p2++;
            }
        }

        if(found) {
            break;
        }
        else {
            err = TRUE;
            if(**p == '\n') { (*sline)++; }
            (*p)++;
        }
    }

    if(err) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "clover has expected that next characters are '%s', but there are some characters before them", characters);
        parser_err_msg(buf, sname, *sline);
        (*err_num)++;
    }

    return TRUE;
}

static BOOL expected_string(int* err_num, char** p, char* sname, int* sline)
{
    BOOL err = FALSE;
    while(1) {
        if(**p == '0') {
            char buf[WORDSIZ];
            snprintf(buf, WORDSIZ, "clover has expected that next characters are alphabets or _, but it arrived at source end");
            parser_err_msg(buf, sname, *sline);
            return FALSE;
        }

        if(isalpha(**p) || **p == '_') {
            break;
        }
        else {
            err = TRUE;
            if(**p == '\n') { (*sline)++; }
            (*p)++;
        }
    }

    if(err) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "clover has expected that next characters are alphabets or _, but there are some characters before them");
        parser_err_msg(buf, sname, *sline);
        (*err_num)++;
    }

    return TRUE;
}

static BOOL expression_node(uint* node, char** p, char* sname, int* sline, int* err_num)
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
                if(**p =='\n') (*sline)++;

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
                break;
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

                if(!expected_string(err_num, p, sname, sline)) {
                    return FALSE;
                }

                if(isalpha(**p) || **p == '_') {
                    p2 = buf;

                    while(isalnum(**p) || **p == '_') {
                        *p2++ = **p;
                        (*p)++;

                        if(p2 - buf >= 128) {
                            parser_err_msg("overflow node of variable name",  sname, *sline);
                            break;
                        }
                    }
                    *p2 = 0;
                    skip_spaces(p);
                }

                uint old_node = 0;
                uint num_params = 0;
                if(**p == '(') {
                    (*p)++;
                    skip_spaces(p);

                    while(1) {
                        uint new_node;
                        if(!node_expression(&new_node, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        num_params++;

                        skip_spaces(p);

                        old_node = sNodeTree_create_param(old_node, new_node,  0);

                        if(!expect_next_character(",)", err_num, p, sname, sline)) {
                            return FALSE;
                        }

                        if(**p == ',') {
                            (*p)++;
                            skip_spaces(p);
                        }
                        else if(**p == ')') {
                            (*p)++;
                            skip_spaces(p);
                            break;
                        }
                    }
                }

                /// get method ///
                uint method_index = get_method_index(klass, buf);
                if(method_index > 0) {
                    /// type checking ///
                    const int method_num_params = get_method_num_params(klass, method_index);
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
                            sCLClass* klass2 = get_method_param_types(klass, method_index, i);
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
                else {
                    char buf2[128];
                    snprintf(buf2, 128, "not defined this method(%s)\n", buf);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;
                }
            }
            /// define variable ///
            else {
                p2 = buf;

                if(!expected_string(err_num, p, sname, sline)) {
                    return FALSE;
                }

                if(isalpha(**p) || **p == '_') {
                    while(isalnum(**p) || **p == '_') {
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

                *node = sNodeTree_create_define_var(buf, klass, 0, 0, 0);
            }
        }
        /// variable ///
        else {
            *node = sNodeTree_create_var(buf, NULL, 0, 0, 0);
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

        if(!node_expression(node, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        if(!expect_next_character(")", err_num, p, sname, sline)) {
            return FALSE;
        }
        (*p)++;
        skip_spaces(p);

        if(*node == 0) {
            parser_err_msg("require expression as ( operand", sname, *sline);
            (*err_num)++;
        }
    }
    else if(**p == ';' || **p == '\n' || **p == '}') {
        *node = 0;
    }
    else {
        char buf[128];
        snprintf(buf, 128, "invalid character (%c)", **p);
        parser_err_msg(buf, sname, *sline);
        *node = 0;
        if(**p == '\n') (*sline)++;
        (*p)++;
        (*err_num)++;
    }

    return TRUE;
}

static BOOL expression_mult_div(uint* node, char** p, char* sname, int* sline, int* err_num)
{
    if(!expression_node(node, p, sname, sline, err_num)) {
        return FALSE;
    }

    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '*' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);
            uint right;
            if(!expression_node(&right, p, sname, sline, err_num)) {
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
            if(!expression_node(&right, p, sname, sline, err_num)) {
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
            if(!expression_node(&right, p, sname, sline, err_num)) {
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

static BOOL expression_add_sub(uint* node, char** p, char* sname, int* sline, int* err_num)
{
    if(!expression_mult_div(node, p, sname, sline, err_num)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) != '=' && *(*p+1) != '+') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_mult_div(&right, p, sname, sline, err_num)) {
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
            if(!expression_mult_div(&right, p, sname, sline, err_num)) {
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
static BOOL expression_equal(uint* node, char** p, char* sname, int* sline, int* err_num)
{
    if(!expression_add_sub(node, p, sname, sline, err_num)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '=') {
            (*p)++;
            skip_spaces(p);
            uint right;
            if(!expression_equal(&right, p, sname, sline, err_num)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, *sline);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, *sline);
                (*err_num)++;
            }

            if(*node > 0 && right > 0) {
                if(gNodes[*node].mType == NODE_TYPE_VARIABLE_NAME || gNodes[*node].mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    *node = sNodeTree_create_operand(kOpEqual, *node, right, 0);
                }
                else {
                    parser_err_msg("require varible name on left value of equal", sname, *sline);
                    (*err_num)++;
                }
            }
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num)
{
    return expression_equal(node, p, sname, sline, err_num);
}

void sByteCode_init(sByteCode* self)
{
    self->mSize = 1024;
    self->mLen = 0;
    self->mCode = MALLOC(sizeof(uchar)*self->mSize);
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

void sConst_init(sConst* self)
{
    self->mSize = 1024;
    self->mLen = 0;
    self->mConst = CALLOC(1, sizeof(uchar)*self->mSize);
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

static BOOL compile_node(uint node, sCLClass** klass, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, sFieldTable* field_table, sVarTable* lv_table)
{
    switch(gNodes[node].mType) {
        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(gNodes[node].mOperand) {
            case kOpAdd: {
                sCLClass* left_class = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                        return FALSE;
                    }
                }
                sCLClass* right_class = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                        return FALSE;
                    }
                }

                if(left_class != right_class) {
                    parser_err_msg("addition with not same class", sname, *sline);
                    (*err_num)++;

                    *klass = left_class;
                }
                else {
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
                uint lnode = gNodes[node].mLeft; // lnode must be variable name or definition of variable.

                sCLClass* left_class = NULL;
                if(gNodes[lnode].mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                        return FALSE;
                    }
                }

                uchar* name = gNodes[lnode].mVarName;

                sField* field = get_variable(lv_table, name);

                if(field == NULL) {
                    field = get_field(field_table, gNodes[node].mVarName);

                    if(field == NULL) {
                        field = get_variable(&gGVTable, name);
                    }
                }

                if(field == NULL) {
                    char buf[128];
                    snprintf(buf, 128, "there is no definition of this variable(%s)\n", gNodes[lnode].mVarName);
                    parser_err_msg(buf, sname, *sline);
                    (*err_num)++;

                    *klass = gIntClass;  // dummy class
                    break;
                }

                left_class = field->mClass;

                sCLClass* right_class = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                        return FALSE;
                    }
                }

                /// type checking ///
                if(left_class != right_class) {
                    parser_err_msg("type error", sname, *sline);
                    (*err_num)++;

                    *klass = gIntClass; // dummy class
                    break;
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
                int var_num = field->mIndex;
                sByteCode_append(code, &var_num, sizeof(int));

                *klass = field->mClass;
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

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
            char* name = gNodes[node].mVarName;
            sCLClass* klass2 = gNodes[node].mClass;

            /// global variable ///
            if(lv_table) {
                sField* var = get_variable(&gGVTable, name);

                if(var) {
                    char buf2[128];
                    snprintf(buf2, 128, "there is a same name variable(%s)\n", name);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;

                    *klass = gIntClass; // dummy
                }
                else {
                    if(!add_variable(&gGVTable, name, klass2)) {
                        parser_err_msg("overflow global variable table", sname, *sline);
                        return FALSE;
                    }

                    *klass = klass2;
                }
            }
            /// local variable ///
            else {
                sField* var = get_variable(lv_table, name);

                if(var) {
                    char buf2[128];
                    snprintf(buf2, 128, "there is a same name variable(%s)\n", name);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;

                    *klass = gIntClass; // dummy
                }
                else {
                    if(!add_variable(lv_table, name, klass2)) {
                        parser_err_msg("overflow global variable table", sname, *sline);
                        return FALSE;
                    }

                    *klass = klass2;
                }
            }

            }
            break;

        /// variable name ///
        case NODE_TYPE_VARIABLE_NAME: {
            uchar* name = gNodes[node].mVarName;

            sField* field = get_variable(lv_table, name);

            if(field == NULL) {
                field = get_field(field_table, gNodes[node].mVarName);

                if(field == NULL) {
                    field = get_variable(&gGVTable, name);
                }
            }

            if(field == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is this varialbe (%s)\n", name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *klass = gIntClass; // dummy class
            }
            else {
                uchar c;
                if(field->mClass == gIntClass) {
                    c = OP_ILOAD;
                }
                else if(field->mClass == gStringClass) {
                    c = OP_ALOAD;
                }
                else if(field->mClass == gFloatClass) {
                    c = OP_FLOAD;
                }

                sByteCode_append(code, &c, sizeof(uchar));
                int var_num = field->mIndex;
                sByteCode_append(code, &var_num, sizeof(int));

                *klass = field->mClass;
            }
            }
            break;

        case NODE_TYPE_PARAM: {
            sCLClass* left_class  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                    return FALSE;
                }
            }
            sCLClass* right_class = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, &right_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                    return FALSE;
                }
            }

            *klass = right_class;
            }
            break;

        case NODE_TYPE_CLASS_METHOD_CALL: {
            sCLClass* left_class = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, &left_class, code, constant, sname, sline, err_num, field_table, lv_table)) {
                    return FALSE;
                }
            }

            char* method_name = gNodes[node].mVarName;
            sCLClass* cl_klass = gNodes[node].mClass;

            uchar c = OP_INVOKE_STATIC_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            uint constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));

            uint method_index = get_method_index(cl_klass, method_name);
            sByteCode_append(code, &method_index, sizeof(uint));

            sConst_append_str(constant, CLASS_NAME(cl_klass));

            sCLClass* result_type = get_method_result_type(cl_klass, method_index);
            ASSERT(result_type);

            *klass = result_type;
            }
            break;
    }

    return TRUE;
}

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sFieldTable* field_table, sVarTable* lv_table)
{
    sByteCode_init(&method->mByteCodes);

    init_nodes();

    while(*p) {
        uint node = 0;
        if(!node_expression(&node, p, sname, sline, err_num)) {
            free_nodes();
            return FALSE;
        }

        sCLClass* klass2 = NULL;
        if(!compile_node(node, &klass2, &method->mByteCodes, &klass->mConstPool, sname, sline, err_num, field_table, lv_table)) {
            free_nodes();
            return FALSE;
        }

        if(**p == ';' || **p == '\n') {
            while(**p == ';' || **p == '\n') {
                if(**p == '\n') (*sline)++;

                (*p)++;
                skip_spaces(p);
            }
        }
        else if(**p == '}') {
            (*p)++;
            break;
        }
        else {
            char buf[128];
            snprintf(buf, 128, "unexpected character(%c)\n", **p);
            parser_err_msg(buf, sname, *sline);
            free_nodes();
            return FALSE;
        }
    }

    free_nodes();

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, int* global_var_num, BOOL flg_main, int* err_num)
{
    init_nodes();

    char* p = source;
    while(*p) {
        uint node = 0;
        if(!node_expression(&node, &p, sname, sline, err_num)) {
            free_nodes();
            return FALSE;
        }

        sCLClass* klass = NULL;
        if(!compile_node(node, &klass, code, constant, sname, sline, err_num, NULL, NULL)) {
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

    *global_var_num = gGVTable.mVarNum;
    free_nodes();

    return TRUE;
}

void parser_init(BOOL load_foundamental_class)
{
    memset(&gGVTable, 0, sizeof(gGVTable));

    if(load_foundamental_class) {
        gIntClass = cl_get_class("int");
        gStringClass = cl_get_class("String");
        gFloatClass = cl_get_class("float");

        if(gIntClass == NULL || gStringClass == NULL || gFloatClass == NULL) {
            fprintf(stderr, "can't load foundamental class\n");
            exit(1);
        }
    }
}

void parser_final()
{
}

