#include "clover.h"
#include "common.h"

sVarTable gGVTable;       // global variable table

static BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table);

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

//////////////////////////////////////////////////
// local variable and global variable
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, uchar* name, sCLClass* klass)
{
    int hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    sVar* p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            xstrncpy(p->mName, name, CL_METHOD_NAME_MAX);
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

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, uchar* name)
{
    int hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    sVar* p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            return NULL;
        }
        else if(strcmp((char*)p->mName, name) == 0) {
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
#define NODE_TYPE_RETURN 8

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

static uint sNodeTree_create_return(sCLClass* klass, uint left, uint right, uint middle)
{
    uint i = alloc_node();

    gNodes[i].mType = NODE_TYPE_RETURN;

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

static BOOL expression_node(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
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

        if(**p >= '0' && **p <= '9') {
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
        else { 
            parser_err_msg("require number after + or -", sname, *sline);
            *node = 0;
            (*err_num)++;
        }
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

        if(strcmp(buf, "return") == 0) {
            uint rv_node;
            if(**p == '(') {
                (*p)++;
                skip_spaces(p);

                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table)) {
                    return FALSE;
                }
                skip_spaces(p);

                if(!expect_next_character(")", err_num, p, sname, sline)) {
                    return FALSE;
                }
                (*p)++;
                skip_spaces(p);
            }
            else {
                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table)) {
                    return FALSE;
                }
                skip_spaces(p);
            }

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, *sline);
                (*err_num)++;
            }

            *node = sNodeTree_create_return(gNodes[rv_node].mClass, rv_node, 0, 0);
        }
        /// user words ///
        else {
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

                        if(**p == ')') {
                            (*p)++;
                            skip_spaces(p);
                        }
                        else {
                            while(1) {
                                uint new_node;
                                if(!node_expression(&new_node, p, sname, sline, err_num, lv_table)) {
                                    return FALSE;
                                }

                                skip_spaces(p);

                                if(new_node) {
                                    old_node = sNodeTree_create_param(old_node, new_node,  0);
                                    num_params++;
                                }

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
                    }

                    /// get method ///
                    sCLMethod* method = get_method(klass, buf);
                    int method_index = get_method_index(klass, buf);
                    if(method) {
                        /// type checking ///
                        const int method_num_params = get_method_num_params(klass, method_index);
                        ASSERT(method_num_params != -1);

                        if(num_params != method_num_params) {
                            char buf2[128];
                            snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", METHOD_NAME(klass, method_index), num_params, method_num_params);
                            parser_err_msg(buf2, sname, *sline);
                            (*err_num)++;
                        }

                        if(num_params > 0) {
                            uint node_num = old_node;
                            int i;
                            for(i=(num_params < method_num_params ? num_params-1:method_num_params-1); i>= 0; i--) {
                                sCLClass* klass2 = get_method_param_types(klass, method_index, i);
                                ASSERT(klass2 != NULL);
                                uint right_node = gNodes[node_num].mRight;

                                if(gNodes[right_node].mClass != klass2) {
                                    char buf2[128];
                                    snprintf(buf2, 128, "Type of parametor number %d is a different type. Requiring type is not %s but %s", i+1, CLASS_NAME(gNodes[right_node].mClass), CLASS_NAME(klass2));
                                    parser_err_msg(buf2, sname, *sline);
                                    (*err_num)++;
                                }
                                node_num = gNodes[node_num].mLeft;
                            }
                        }

                        /// is this static method ? ///
                        if((method->mHeader & CL_STATIC_METHOD) == 0) {
                            char buf2[128];
                            snprintf(buf2, 128, "This is not static method(%s)", METHOD_NAME(klass, method_index));
                            parser_err_msg(buf2, sname, *sline);
                            return FALSE;
                        }

                        *node = sNodeTree_create_class_method_call(buf, klass, old_node, 0, 0);
                    }
                    else {
                        char buf2[128];
                        snprintf(buf2, 128, "not defined this method(%s)", buf);
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
                char* name = buf;
                sVar* var = get_variable_from_table(lv_table, name);

                if(var == NULL) {
                    char buf[128];
                    snprintf(buf, 128, "there is no definition of this variable(%s)", name);
                    parser_err_msg(buf, sname, *sline);
                    (*err_num)++;

                    *node = 0;
                }
                else {
                    *node = sNodeTree_create_var(buf, var->mClass, 0, 0, 0);
                }
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
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces(p);

        if(!node_expression(node, p, sname, sline, err_num, lv_table)) {
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

static BOOL expression_mult_div(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
{
    if(!expression_node(node, p, sname, sline, err_num, lv_table)) {
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
            if(!expression_node(&right, p, sname, sline, err_num, lv_table)) {
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

            *node = sNodeTree_create_operand(kOpMult, *node, right, 0);
        }
        else if(**p == '/' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_node(&right, p, sname, sline, err_num, lv_table)) {
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

            *node = sNodeTree_create_operand(kOpDiv, *node, right, 0);
        }
        else if(**p == '%' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_node(&right, p, sname, sline, err_num, lv_table)) {
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

            *node = sNodeTree_create_operand(kOpMod, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_add_sub(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
{
    if(!expression_mult_div(node, p, sname, sline, err_num, lv_table)) {
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
            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table)) {
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

            *node = sNodeTree_create_operand(kOpAdd, *node, right, 0);
        }
        else if(**p == '-' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            uint right;
            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table)) {
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

            *node = sNodeTree_create_operand(kOpSub, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from right to left order
static BOOL expression_equal(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
{
    if(!expression_add_sub(node, p, sname, sline, err_num, lv_table)) {
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
            if(!expression_equal(&right, p, sname, sline, err_num, lv_table)) {
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
                    parser_err_msg("require varible name on left node of equal", sname, *sline);
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

static BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
{
    return expression_equal(node, p, sname, sline, err_num, lv_table);
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

static BOOL compile_node(uint node, sCLClass* klass, sCLMethod* method, sCLClass** type_, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, sVarTable* lv_table, int* stack_num, int* max_stack, BOOL* exist_return)
{
    if(node == 0) {
        parser_err_msg("no expression", sname, *sline);
        (*err_num)++;
        return TRUE;
    }

puts("compile_node");
    switch(gNodes[node].mType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
puts("NODE_TYPE_VALUE");
            uchar c = OP_LDC;
            sByteCode_append(code, &c, sizeof(uchar));
            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            uchar const_type = CONSTANT_INT;
            sConst_append(constant, &const_type, sizeof(uchar));
            int data = gNodes[node].mValue;
            sConst_append(constant, &data, sizeof(data));

            *type_ = gIntClass;

            (*stack_num)++;
            if(*stack_num > *max_stack) *max_stack = *stack_num;
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE: {
puts("NODE_TYPE_STRING_VALUE");
            uchar c = OP_LDC;
            sByteCode_append(code, &c, sizeof(uchar));
            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            sConst_append_wstr(constant, gNodes[node].mStringValue);

            *type_ = gStringClass;

            (*stack_num)++;
            if(*stack_num > *max_stack) *max_stack = *stack_num;
            }
            break;

        /// define variable ///
        case NODE_TYPE_DEFINE_VARIABLE_NAME: {
puts("NODE_TYPE_DEFINE_VARIABLE_NAME");
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

        /// variable name ///
        case NODE_TYPE_VARIABLE_NAME: {
puts("NODE_TYPE_VARIABLE_NAME");
            uchar* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is this varialbe (%s)", name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
            }
            else {
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
                int var_num = var->mIndex;
                sByteCode_append(code, &var_num, sizeof(int));

                *type_ = var->mClass;

                (*stack_num)++;
                if(*stack_num > *max_stack) *max_stack = *stack_num;
            }
            }
            break;
        /// operand ///
        case NODE_TYPE_OPERAND:
puts("NODE_TYPE_OPERAND");
            switch(gNodes[node].mOperand) {
            case kOpAdd: {
puts("Add");
                sCLClass* left_type = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                        return FALSE;
                    }
                }
                sCLClass* right_type = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
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
                    uchar c;
                    if(left_type == gStringClass) {
                        c = OP_SADD;
                        *type_ = gStringClass;
                    }
                    else if(left_type == gIntClass) {
                        c = OP_IADD;
                        *type_ = gIntClass;
                    }
                    else if(left_type == gFloatClass) {
                        c = OP_FADD;

                        *type_ = gFloatClass;
                    }

                    sByteCode_append(code, &c, sizeof(uchar));

                    (*stack_num)--;
                }

                }
                break;

            case kOpSub: 
puts("Sub");
                break;

            case kOpMult: 
puts("Mult");
                break;

            case kOpDiv: 
puts("div");
                break;

            case kOpMod: 
puts("mod");
                break;

            case kOpEqual: {
puts("equal");
                uint lnode = gNodes[node].mLeft; // lnode must be variable name or definition of variable.

                sCLClass* left_type = NULL;
                if(gNodes[lnode].mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                        return FALSE;
                    }
                }

                uchar* name = gNodes[lnode].mVarName;

                sVar* var = get_variable_from_table(lv_table, name);

                if(var == NULL) {
                    char buf[128];
                    snprintf(buf, 128, "there is no definition of this variable(%s)", name);
                    parser_err_msg(buf, sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass;  // dummy class
                    break;
                }

                left_type = var->mClass;

                sCLClass* right_type = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                        return FALSE;
                    }
                }

                /// type checking ///
                if(left_type == NULL || right_type == NULL || left_type != right_type) {
                    parser_err_msg("type error", sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass; // dummy class
                    break;
                }

                uchar c;
                if(left_type == gIntClass) {
                    c = OP_ISTORE;
                }
                else if(left_type == gStringClass) {
                    c = OP_ASTORE;
                }
                else if(left_type == gFloatClass) {
                    c = OP_FSTORE;
                }

                sByteCode_append(code, &c, sizeof(uchar));
                int var_num = var->mIndex;
                sByteCode_append(code, &var_num, sizeof(int));

                *type_ = var->mClass;

                //(*stack_num)--;
                }
                break;
            }
            break;

        case NODE_TYPE_PARAM: {
puts("NODE_TYPE_PARAM");
            sCLClass* left_type  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                    return FALSE;
                }
            }
            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                    return FALSE;
                }
            }

            if(right_type == NULL) {
                *type_ = gIntClass;  // dummy
            }
            else {
                *type_ = right_type;
            }
            }
            break;

        case NODE_TYPE_CLASS_METHOD_CALL: {
puts("NODE_TYPE_CLASS_METHOD_CALL");
            sCLClass* left_type = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
                    return FALSE;
                }
            }

            char* method_name = gNodes[node].mVarName;
            sCLClass* cl_klass = gNodes[node].mClass;

            uchar c = OP_INVOKE_STATIC_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            uint constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));

            sConst_append_str(constant, CLASS_NAME(cl_klass));

            uint method_index = get_method_index(cl_klass, method_name);
            sByteCode_append(code, &method_index, sizeof(uint));

            sCLClass* result_type = get_method_result_type(cl_klass, method_index);
            ASSERT(result_type);

            uchar exist_result = result_type != gVoidClass;
            sByteCode_append(code, &exist_result, sizeof(uchar));

            *type_ = result_type;

            if(exist_result) {
                *stack_num = 1;
            }
            else {
                *stack_num = 0;
            }

            }
            break;

        case NODE_TYPE_RETURN: {
            sCLClass* left_type  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return)) {
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
                snprintf(buf2, 128, "type error. Requiring class is not %s but %s", CLASS_NAME(left_type), CLASS_NAME(result_type));
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            uchar c = OP_RETURN;
            sByteCode_append(code, &c, sizeof(uchar));

            //(*stack_num)++;

            *type_ = gVoidClass;

            *exist_return = TRUE;
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
        uchar c = OP_POP;
        sByteCode_append(code, &c, sizeof(uchar));
    }
    else if(*stack_num > 0) {
        uchar c = OP_POP_N;
        sByteCode_append(code, &c, sizeof(uchar));
        uint n = *stack_num;
        sByteCode_append(code, &n, sizeof(uint));
    }

    *stack_num = 0;
}

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table)
{
    sByteCode_init(&method->mByteCodes);

    init_nodes();

    int max_stack = 0;
    int stack_num = 0;
    BOOL exist_return = FALSE;

    while(*p) {
        uint node = 0;
        if(!node_expression(&node, p, sname, sline, err_num, lv_table)) {
            free_nodes();
            return FALSE;
        }

        sCLClass* type_ = NULL;
        if(!compile_node(node, klass, method, &type_, &method->mByteCodes, &klass->mConstPool, sname, sline, err_num, lv_table, &stack_num, &max_stack, &exist_return)) {
            free_nodes();
            return FALSE;
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

    sCLClass* result_type = cl_get_class(CONS_str(klass->mConstPool, method->mResultType));
    if(result_type != gVoidClass && !exist_return) {
        parser_err_msg("require return sentence", sname, *sline);
        free_nodes();
        return FALSE;
    }

    method->mMaxStack = max_stack;

    free_nodes();

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, BOOL flg_main, int* err_num, int* max_stack)
{
    init_nodes();

    *max_stack = 0;
    int stack_num = 0;
    BOOL exist_return = FALSE;

    char* p = source;
    while(*p) {
        uint node = 0;
        if(!node_expression(&node, &p, sname, sline, err_num, &gGVTable)) {
            free_nodes();
            return FALSE;
        }

        sCLClass* type_ = NULL;
        if(!compile_node(node, NULL, NULL, &type_, code, constant, sname, sline, err_num, &gGVTable, &stack_num, max_stack, &exist_return)) {
            free_nodes();
            return FALSE;
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

void parser_init(BOOL load_foundamental_class)
{
    memset(&gGVTable, 0, sizeof(gGVTable));
}

void parser_final()
{
}

