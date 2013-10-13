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

#define NODE_TYPE_OPERAND 1
#define NODE_TYPE_VALUE 2
#define NODE_TYPE_STRING_VALUE 3
#define NODE_TYPE_VARIABLE_NAME 4
#define NODE_TYPE_DEFINE_VARIABLE_NAME 5
#define NODE_TYPE_CLASS_METHOD_CALL 6
#define NODE_TYPE_CLASS_FIELD 7
#define NODE_TYPE_PARAM 8
#define NODE_TYPE_RETURN 9
#define NODE_TYPE_NEW 10
#define NODE_TYPE_METHOD_CALL 11
#define NODE_TYPE_FIELD 12

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
            case NODE_TYPE_FIELD:
            case NODE_TYPE_METHOD_CALL:
            case NODE_TYPE_DEFINE_VARIABLE_NAME:
            case NODE_TYPE_CLASS_METHOD_CALL:
            case NODE_TYPE_CLASS_FIELD:
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

static uint sNodeTree_create_class_field(char* var_name, sCLClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_new_expression(sCLClass* klass, uint left, uint right, uint middle)
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

static uint sNodeTree_create_fields(uchar* name, uint left, uint right, uint middle)
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

static uint sNodeTree_create_method_call(char* var_name, uint left, uint right, uint middle)
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

static void show_node(uint node)
{
    printf("type %d var_name %s class %p left %d right %d middle %d\n", gNodes[node].mType, gNodes[node].mVarName, gNodes[node].mClass, gNodes[node].mLeft, gNodes[node].mRight, gNodes[node].mMiddle);
    if(gNodes[node].mClass) printf("class_name %s\n", CLASS_NAME(gNodes[node].mClass));
    else printf("\n");
}

//////////////////////////////////////////////////
// parse it
//////////////////////////////////////////////////
void parser_err_msg(char* msg, char* sname, int sline)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline, msg);
}

static BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num)
{
    buf[0] = 0;

    char* p2 = buf;

    if(isalpha(**p)) {
        while(isalnum(**p) || **p == '_') {
            if(p2 - buf < buf_size-1) {
                *p2++ = **p;
                (*p)++;
            }
            else {
                char buf[WORDSIZ];
                snprintf(buf, WORDSIZ, "length of word is too long");
                parser_err_msg(buf, sname, *sline);
                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(**p == 0) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "require word(alphabet or _ or number). this is the end of source");
        parser_err_msg(buf, sname, *sline);
        return FALSE;
    }

    if(buf[0] == 0) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "require word(alphabet or _ or number). this is (%c)\n", **p);
        parser_err_msg(buf, sname, *sline);

        (*err_num)++;

        if(**p == '\n') (*sline)++;

        (*p)++;
    }

    return TRUE;
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

static BOOL get_params(char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, int* res_node)
{
    *res_node = 0;
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
                    *res_node = sNodeTree_create_param(*res_node, new_node,  0);
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
    else if(isalpha(**p)) {
        char buf[128];

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        if(strcmp(buf, "new") == 0) {
            if(!parse_word(buf, 128, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces(p);

            sCLClass* klass = cl_get_class(buf);

            if(klass == NULL) {
                char buf2[128];
                snprintf(buf2, 128, "invalid class name(%s)", buf);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
            }
            else {
                uint param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node)) {
                    return FALSE;
                }

                *node = sNodeTree_create_new_expression(klass, param_node, 0, 0);
            }
        }
        else if(strcmp(buf, "return") == 0) {
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

                    if(!parse_word(buf, 128, p, sname, sline, err_num)) {
                        return FALSE;
                    }
                    skip_spaces(p);

                    if(**p == '(') {
                        uint param_node = 0;
                        if(!get_params(p, sname, sline, err_num, lv_table, &param_node)) {
                            return FALSE;
                        }

                        *node = sNodeTree_create_class_method_call(buf, klass, param_node, 0, 0);
                    }
                    else {
                        *node = sNodeTree_create_class_field(buf, klass, 0, 0, 0);
                    }
                }
                /// define variable ///
                else {
                    if(!parse_word(buf, 128, p, sname, sline, err_num)) {
                        return FALSE;
                    }
                    skip_spaces(p);

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
        return TRUE;
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

    /// method call ///
    if(**p == '.') {
        (*p)++;
        skip_spaces(p);

        if(isalpha(**p)) {
            char buf[128];

            if(!parse_word(buf, 128, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces(p);

            /// methods ///
            if(**p == '(') {
                uint param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node)) {
                    return FALSE;
                }

                *node = sNodeTree_create_method_call(buf, *node, param_node, 0);
            }
            /// fields ///
            else {
                *node = sNodeTree_create_fields(buf, *node, 0, 0);
            }
        }
        else {
            parser_err_msg("require method name after .", sname, *sline);
            (*err_num)++;

            *node = 0;
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
                if(gNodes[*node].mType == NODE_TYPE_VARIABLE_NAME || gNodes[*node].mType == NODE_TYPE_DEFINE_VARIABLE_NAME || gNodes[*node].mType == NODE_TYPE_FIELD) {
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

void sByteCode_free(sByteCode* self)
{
    FREE(self->mCode);
}

void sByteCode_append(sByteCode* self, void* code, uint size)
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

void sConst_free(sConst* self)
{
    FREE(self->mConst);
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

static BOOL compile_node(uint node, sCLClass* klass, sCLMethod* method, sCLClass** type_, sByteCode* code, sConst* constant, char* sname, int* sline, int* err_num, sVarTable* lv_table, int* stack_num, int* max_stack, BOOL* exist_return, int* num_params, sCLClass* class_params[], BOOL left_node_of_equal)
{
    if(node == 0) {
        parser_err_msg("no expression", sname, *sline);
        (*err_num)++;
        return TRUE;
    }

    switch(gNodes[node].mType) {
        /// number value ///
        case NODE_TYPE_VALUE: {
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
            uchar* name = gNodes[node].mVarName;

            sVar* var = get_variable_from_table(lv_table, name);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is not this varialbe (%s)", name);
                parser_err_msg(buf, sname, *sline);
                (*err_num)++;

                *type_ = gIntClass; // dummy class
            }
            else if(!left_node_of_equal) {
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
                else {
                    c = OP_OLOAD;
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
        
        /// field name ///
        case NODE_TYPE_FIELD: {
            /// left_value ///
            sCLClass* left_type = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }

            if(left_node_of_equal) {
                sCLClass* cl_klass = left_type;
                uchar* field_name = gNodes[node].mVarName;

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
                    snprintf(buf, 128, "there is not this field(%s) in this class(%s)", field_name, CLASS_NAME(cl_klass));
                    parser_err_msg(buf, sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass; // dummy
                    break;
                }

                sCLClass* cl_klass2 = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

                if(cl_klass2 == NULL || cl_klass2 == gVoidClass) {
                    parser_err_msg("this field has not type.", sname, *sline);
                    (*err_num)++;
                    *type_ = gIntClass; // dummy
                    break;
                }

                *type_ = left_type;
            }
            else {
                sCLClass* cl_klass = left_type;
                uchar* field_name = gNodes[node].mVarName;

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
                    snprintf(buf, 128, "there is not this field(%s) in this class(%s)", field_name, CLASS_NAME(cl_klass));
                    parser_err_msg(buf, sname, *sline);
                    (*err_num)++;
                }

                sCLClass* cl_klass2 = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

                if(cl_klass2 == NULL || cl_klass2 == gVoidClass) {
                    parser_err_msg("this field has not class", sname, *sline);
                    (*err_num)++;

                    *type_ = gIntClass; // dummy;
                }
                else {
                    uchar c = OP_LDFIELD;

                    sByteCode_append(code, &c, sizeof(uchar));
                    sByteCode_append(code, &field_index, sizeof(int));

                    *type_ = cl_klass2;

                    (*stack_num)++;
                    if(*stack_num > *max_stack) *max_stack = *stack_num;
                }
            }
            }
            break;

        case NODE_TYPE_CLASS_FIELD: {
            }
            break;

        case NODE_TYPE_NEW: {
            /// params go ///
            sCLClass* left_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }

            /// type checking ///
            sCLClass* cl_klass = gNodes[node].mClass;
            char* method_name = CLASS_NAME(cl_klass);
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            if(method == NULL || method_index == -1) {
                method = get_method(cl_klass, method_name);
                method_index = get_method_index(cl_klass, method_name);

                if(method == NULL || method_index == -1) {
                    char buf2[128];
                    snprintf(buf2, 128, "There is not this method(%s)", method_name);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;
                    break;
                }
            }

            const int method_num_params = get_method_num_params(cl_klass, method_index);
            ASSERT(method_num_params != -1);

            if(num_params != method_num_params) {
                char buf2[128];
                snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
            }

            BOOL err_flg = FALSE;
            int i;
            for(i=0; i<num_params; i++) {
                sCLClass* class_params2 = get_method_param_types(cl_klass, method_index, i);
                if(class_params2 == NULL || class_params[i] == NULL) {
                    parser_err_msg("unexpected error of parametor", sname, *sline);
                    break;
                }

                if(class_params[i] != class_params2) {
                    char buf2[128];
                    snprintf(buf2, 128, "(%d) parametor is not %s but %s", i, CLASS_NAME(class_params[i]), CLASS_NAME(class_params2));
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;

                    err_flg = TRUE;
                }
            }

            if(err_flg) {
                break;
            }
            
            /// creating new object ///
            uchar c = OP_NEW_OBJECT;
            sByteCode_append(code, &c, sizeof(uchar));

            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            sConst_append_str(constant, CLASS_NAME(cl_klass));

            (*stack_num)++;
            if(*stack_num > *max_stack) *max_stack = *stack_num;

            /// calling constructor goes ///
            c = OP_INVOKE_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));

            sConst_append_str(constant, CLASS_NAME(cl_klass));

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

        case NODE_TYPE_METHOD_CALL: {
            /// left_value ///
            sCLClass* left_type = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }

            /// params go ///
            sCLClass* right_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }

            /// type checking ///
            sCLClass* cl_klass = left_type;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            if(method == NULL || method_index == -1) {
                method = get_method(cl_klass, method_name);
                method_index = get_method_index(cl_klass, method_name);

                if(method == NULL || method_index == -1) {
                    char buf2[128];
                    snprintf(buf2, 128, "There is not this method(%s)", method_name);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;
                    break;
                }
            }

            /// is this static method ? ///
            if((method->mHeader & CL_STATIC_METHOD)) {
                char buf2[128];
                snprintf(buf2, 128, "This is static method(%s)", METHOD_NAME(klass, method_index));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
                break;
            }

            const int method_num_params = get_method_num_params(cl_klass, method_index);
            ASSERT(method_num_params != -1);

            if(num_params != method_num_params) {
                char buf2[128];
                snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
            }

            BOOL err_flg = FALSE;
            int i;
            for(i=0; i<num_params; i++) {
                sCLClass* class_params2 = get_method_param_types(cl_klass, method_index, i);
                if(class_params2 == NULL || class_params[i] == NULL) {
                    parser_err_msg("unexpected error of parametor", sname, *sline);
                    break;
                }

                if(class_params[i] != class_params2) {
                    char buf2[128];
                    snprintf(buf2, 128, "(%d) parametor is not %s but %s", i, CLASS_NAME(class_params[i]), CLASS_NAME(class_params2));
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;

                    err_flg = TRUE;
                }
            }

            if(err_flg) {
                break;
            }

            /// calling method go ///
            uchar c = OP_INVOKE_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            uint constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));
            sConst_append_str(constant, CLASS_NAME(cl_klass));

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

        case NODE_TYPE_CLASS_METHOD_CALL: {
            /// params go ///
            sCLClass* left_type = NULL;
            int num_params = 0;
            sCLClass* class_params[CL_METHOD_PARAM_MAX];
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, &num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }

            /// type checking ///
            sCLClass* cl_klass = gNodes[node].mClass;
            char* method_name = gNodes[node].mVarName;
            uint method_index = get_method_index_with_params(cl_klass, method_name, class_params, num_params);
            sCLMethod* method = get_method_with_params(cl_klass, method_name, class_params, num_params);

            if(method == NULL || method_index == -1) {
                method = get_method(cl_klass, method_name);
                method_index = get_method_index(cl_klass, method_name);

                if(method == NULL || method_index == -1) {
                    char buf2[128];
                    snprintf(buf2, 128, "There is not this method(%s)", method_name);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;
                    break;
                }
            }

            /// is this static method ? ///
            if((method->mHeader & CL_STATIC_METHOD) == 0) {
                char buf2[128];
                snprintf(buf2, 128, "This is not static method(%s)", METHOD_NAME(klass, method_index));
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
                break;
            }

            const int method_num_params = get_method_num_params(cl_klass, method_index);
            ASSERT(method_num_params != -1);

            if(num_params != method_num_params) {
                char buf2[128];
                snprintf(buf2, 128, "Parametor number of (%s) is not %d but %d", method_name, num_params, method_num_params);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
            }

            BOOL err_flg = FALSE;
            int i;
            for(i=0; i<num_params; i++) {
                sCLClass* class_params2 = get_method_param_types(cl_klass, method_index, i);
                if(class_params2 == NULL || class_params[i] == NULL) {
                    parser_err_msg("unexpected error of parametor", sname, *sline);
                    break;
                }

                if(class_params[i] != class_params2) {
                    char buf2[128];
                    snprintf(buf2, 128, "(%d) parametor is not %s but %s", i, CLASS_NAME(class_params[i]), CLASS_NAME(class_params2));
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;

                    err_flg = TRUE;
                }
            }

            if(err_flg) {
                break;
            }

            /// calling method go ///
            uchar c = OP_INVOKE_STATIC_METHOD;
            sByteCode_append(code, &c, sizeof(uchar));

            uint constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(uint));
            sConst_append_str(constant, CLASS_NAME(cl_klass));

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

        case NODE_TYPE_PARAM: {
            sCLClass* left_type  = NULL;
            if(gNodes[node].mLeft) {
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
                    return FALSE;
                }
            }
            sCLClass* right_type = NULL;
            if(gNodes[node].mRight) {
                if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
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
                if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
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

        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(gNodes[node].mOperand) {
            case kOpAdd: {
                sCLClass* left_type = NULL;
                if(gNodes[node].mLeft) {
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
                        return FALSE;
                    }
                }
                sCLClass* right_type = NULL;
                if(gNodes[node].mRight) {
                    if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
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
                break;

            case kOpMult: 
                break;

            case kOpDiv: 
                break;

            case kOpMod: 
                break;

            case kOpEqual: {
                uint lnode = gNodes[node].mLeft; // lnode must be variable name or definition of variable or field name

                if(gNodes[lnode].mType == NODE_TYPE_VARIABLE_NAME || gNodes[lnode].mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    sCLClass* left_type = NULL;
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, TRUE)) {
                        return FALSE;
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
                        if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
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
                        snprintf(buf2, 128, "type error. left class is %s. right class is %s", CLASS_NAME(left_type), CLASS_NAME(right_type));
                        parser_err_msg(buf2, sname, *sline);
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
                    else {
                        c = OP_OSTORE;
                    }

                    sByteCode_append(code, &c, sizeof(uchar));
                    int var_num = var->mIndex;
                    sByteCode_append(code, &var_num, sizeof(int));

                    *type_ = var->mClass;

                    //(*stack_num)--;
                }
                else {
                    sCLClass* left_type = NULL;
                    if(!compile_node(gNodes[node].mLeft, klass, method, &left_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, TRUE)) {
                        return FALSE;
                    }

                    sCLClass* cl_klass = left_type;
                    uchar* field_name = gNodes[lnode].mVarName;

                    if(cl_klass == NULL) {
                        parser_err_msg("there is not class for getting field", sname, *sline);
                        (*err_num)++;
                        break;
                    }

                    sCLField* field = get_field(cl_klass, field_name);
                    int field_index = get_field_index(cl_klass, field_name);

                    if(field == NULL || field_index == -1) {
                        char buf[128];
                        snprintf(buf, 128, "there is not this field(%s) in this class(%s)", field_name, CLASS_NAME(cl_klass));
                        parser_err_msg(buf, sname, *sline);
                        (*err_num)++;
                        break;
                    }

                    left_type = cl_get_class(FIELD_CLASS_NAME(cl_klass, field_index));

                    if(left_type == NULL || left_type == gVoidClass) {
                        parser_err_msg("this field has no class", sname, *sline);
                        (*err_num)++;
                        break;
                    }

                    sCLClass* right_type = NULL;
                    if(gNodes[node].mRight) {
                        if(!compile_node(gNodes[node].mRight, klass, method, &right_type, code, constant, sname, sline, err_num, lv_table, stack_num, max_stack, exist_return, num_params, class_params, FALSE)) {
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
                        snprintf(buf2, 128, "type error. left class is %s. right class is %s", CLASS_NAME(left_type), CLASS_NAME(right_type));
                        parser_err_msg(buf2, sname, *sline);
                        (*err_num)++;

                        *type_ = gIntClass; // dummy class
                        break;
                    }

                    uchar c;
                    c = OP_SRFIELD;
                    sByteCode_append(code, &c, sizeof(uchar));
                    sByteCode_append(code, &field_index, sizeof(int));

                    *type_ = left_type;
                }
                break;
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
    alloc_bytecode(method);

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
        if(!compile_node(node, klass, method, &type_, &method->mByteCodes, &klass->mConstPool, sname, sline, err_num, lv_table, &stack_num, &max_stack, &exist_return, NULL, NULL, FALSE)) {
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
        if(!compile_node(node, NULL, NULL, &type_, code, constant, sname, sline, err_num, &gGVTable, &stack_num, max_stack, &exist_return, NULL, NULL, FALSE)) {
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

