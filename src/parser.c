#include "clover.h"

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod
};

#define NODE_TYPE_OPERAND 0
#define NODE_TYPE_VALUE 1
#define NODE_TYPE_STRING_VALUE 2

struct _sNodeTree {
    unsigned char mType;

    union {
        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
    };

    struct _sNodeTree* mLeft;
    struct _sNodeTree* mRight;
    struct _sNodeTree* mMiddle;
};

typedef struct _sNodeTree sNodeTree;

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

// skip spaces
static void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static sNodeTree* sNodeTree_create_operand(enum eOperand operand, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_OPERAND;
    self->mOperand = operand;
    self->mLeft = left;
    self->mRight = right;
    self->mMiddle = middle;

    return self;
}

static sNodeTree* sNodeTree_create_value(int value, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_VALUE;
    self->mValue = value;

    self->mLeft = left;
    self->mRight = right;
    self->mMiddle = middle;

    return self;
}

static sNodeTree* sNodeTree_create_string_value(MANAGED char* value, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_STRING_VALUE;
    self->mStringValue = MANAGED value;

    self->mLeft = left;
    self->mRight = right;
    self->mMiddle = middle;

    return self;
}

static void sNodeTree_free(sNodeTree* self)
{
    if(self) {
        if(self->mLeft) sNodeTree_free(self->mLeft);
        if(self->mRight) sNodeTree_free(self->mRight);
        if(self->mMiddle) sNodeTree_free(self->mMiddle);

        if(self->mType == NODE_TYPE_STRING_VALUE) {
            FREE(self->mStringValue);
        }

        FREE(self);
    }
}

static parser_err_msg(char* msg, char* sname, int sline)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline, msg);
}

static BOOL expression_node(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
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

        *node = ALLOC sNodeTree_create_value(atoi(buf), NULL, NULL, NULL);
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

        *node = ALLOC sNodeTree_create_string_value(MANAGED value.mBuf, NULL, NULL, NULL);
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces(p);

        if(!node_expression(ALLOC node, p, sname, sline)) {
            return FALSE;
        }
        skip_spaces(p);

        if(**p != ')') {
            parser_err_msg("require )", sname, *sline);
            return FALSE;
        }
        (*p)++;
        skip_spaces(p);

        if(*node == NULL) {
            parser_err_msg("require expression as ( operand", sname, *sline);
            return FALSE;
        }
    }
    else if(**p == ';') {
        *node = NULL;
    }
    else {
        char buf[128];
        snprintf(buf, 128, "invalid character (%c)", **p);
        parser_err_msg(buf, sname, *sline);
        *node = NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL expression_mult_div(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
{
    if(!expression_node(ALLOC node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '*' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);
            sNodeTree* right;
            if(!expression_node(ALLOC &right, p, sname, sline)) {
                sNodeTree_free(right);
                return FALSE;
            }

            if(*node == NULL) {
                parser_err_msg("require left value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }
            if(right == NULL) {
                parser_err_msg("require right value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpMult, *node, right, NULL);
        }
        else if(**p == '/' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);
            sNodeTree* right;
            if(!expression_node(ALLOC &right, p, sname, sline)) {
                sNodeTree_free(right);
                return FALSE;
            }

            if(*node == NULL) {
                parser_err_msg("require left value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }
            if(right == NULL) {
                parser_err_msg("require right value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpDiv, *node, right, NULL);
        }
        else if(**p == '%' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);
            sNodeTree* right;
            if(!expression_node(ALLOC &right, p, sname, sline)) {
                sNodeTree_free(right);
                return FALSE;
            }

            if(*node == NULL) {
                parser_err_msg("require left value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }
            if(right == NULL) {
                parser_err_msg("require right value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpMod, *node, right, NULL);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_add_sub(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
{
    if(!expression_mult_div(ALLOC node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) != '=' && *(*p+1) != '+') {
            (*p)++;
            skip_spaces(p);

            sNodeTree* right;
            if(!expression_mult_div(ALLOC &right, p, sname, sline)) {
                sNodeTree_free(right);
                return FALSE;
            }

            if(*node == NULL) {
                parser_err_msg("require left value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }
            if(right == NULL) {
                parser_err_msg("require right value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpAdd, *node, right, NULL);
        }
        else if(**p == '-' && *(*p+1) != '=') {
            (*p)++;
            skip_spaces(p);

            sNodeTree* right;
            if(!expression_mult_div(ALLOC &right, p, sname, sline)) {
                sNodeTree_free(right);
                return FALSE;
            }

            if(*node == NULL) {
                parser_err_msg("require left value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }
            if(right == NULL) {
                parser_err_msg("require right value", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpSub, *node, right, NULL);
        }
        else {
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
{
    return expression_add_sub(ALLOC node, p, sname, sline);
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

static BOOL compile_node(sNodeTree* node, sByteCode* code, sConst* constant)
{
    switch(node->mType) {
        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(node->mOperand) {
            case kOpAdd: {
                if(node->mLeft) {
                    if(!compile_node(node->mLeft, code, constant)) {
                        return FALSE;
                    }
                }
                if(node->mRight) {
                    if(!compile_node(node->mRight, code, constant)) {
                        return FALSE;
                    }
                }
                uchar c = OP_IADD;
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
            }
            break;

        /// number ///
        case NODE_TYPE_VALUE: {
            uchar c = OP_LDC;
            sByteCode_append(code, &c, sizeof(uchar));
            int constant_number = constant->mLen;
            sByteCode_append(code, &constant_number, sizeof(int));

            int data = node->mValue;
            sConst_append(constant, &data, sizeof(data));
            }
            break;

        //// string value ///
        case NODE_TYPE_STRING_VALUE:
            break;
    }

    return TRUE;
}

static parse_method(char* source, char* sname, int* sline, sByteCode* code, sConst* constant)
{
    char* p = source;
    sNodeTree* node = NULL;
    if(!node_expression(&node, &p, sname, sline)) {
        sNodeTree_free(node);
        return FALSE;
    }

    if(!compile_node(node, code, constant)) {
        sNodeTree_free(node);
        return FALSE;
    }

    sNodeTree_free(node);

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline)
{
    sByteCode code;
    code.mSize = 1024;
    code.mLen = 0;
    code.mCode = MALLOC(sizeof(uchar)*code.mSize);

    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    if(!parse_method(source, sname, sline, &code, &constant)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }
if(!cl_vm(&code, &constant)) {
    FREE(code.mCode);
    FREE(constant.mConst);
    return FALSE;
}

    FREE(code.mCode);
    FREE(constant.mConst);

    return TRUE;
}
