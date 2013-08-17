#include "clover.h"

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpEqual, kOpPlusPlus2, kOpMinusMinus2
};

#define NODE_TYPE_OPERAND 0
#define NODE_TYPE_VALUE 1
#define NODE_TYPE_STRING_VALUE 2
#define NODE_TYPE_VARIABLE_NAME 3
#define NODE_TYPE_DEFINE_VARIABLE_NAME 4

typedef struct {
    char* mName;

    char** mMethods;
    uint mNumMethods;
    uint mSizeMethods;
} sParserClass;

sParserClass* sParserClass_new(char* name)
{
    sParserClass* self = MALLOC(sizeof(sParserClass));

    self->mName = STRDUP(name);

    self->mMethods = MALLOC(sizeof(char*)*128);
    self->mNumMethods = 0;
    self->mSizeMethods = 128;

    return self;
}

void sParserClass_delete(sParserClass* self)
{
    FREE(self->mName);

    int i;
    for(i=0; i<self->mNumMethods; i++) {
        FREE(self->mMethods[i]);
    }
    FREE(self->mMethods);

    FREE(self);
}

void sParserClass_add_method(sParserClass* self, char* name)
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

static sParserClass* gIntClass;
static sParserClass* gStringClass;
static sParserClass* gFloatClass;

typedef struct {
    char* mName;
    sParserClass* mClass;
    uint mVariableNumber;
} sParserVar;

sParserVar* sParserVar_new(char* name, sParserClass* klass, uint varibale_num)
{
    sParserVar* self = MALLOC(sizeof(sParserVar));

    self->mName = STRDUP(name);
    self->mClass = klass;
    self->mVariableNumber = varibale_num;

    return self;
}

void sParserVar_delete(sParserVar* self)
{
    FREE(self->mName);
    FREE(self);
}

static hash_obj* gGlobalVars;

struct _sNodeTree {
    unsigned char mType;
    sParserClass* mClass;

    union {
        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
        char* mVarName;
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

    self->mClass = NULL;

    return self;
}

static sNodeTree* sNodeTree_create_value(int value, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_VALUE;
    self->mValue = value;

    self->mClass = gIntClass;

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

    self->mClass = gStringClass;

    self->mLeft = left;
    self->mRight = right;
    self->mMiddle = middle;

    return self;
}

sNodeTree* sNodeTree_create_var(char* var_name, sParserClass* klass, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_VARIABLE_NAME;
    self->mVarName = STRDUP(var_name);

    self->mLeft = left;
    self->mRight = right;
    self->mMiddle = middle;

    self->mClass = klass;

    return self;
}

sNodeTree* sNodeTree_create_define_var(char* var_name, sParserClass* klass, sNodeTree* left, sNodeTree* right, sNodeTree* middle)
{
    sNodeTree* self = MALLOC(sizeof(sNodeTree));

    self->mType = NODE_TYPE_DEFINE_VARIABLE_NAME;
    self->mVarName = STRDUP(var_name);

    self->mClass = klass;

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
        if(self->mType == NODE_TYPE_VARIABLE_NAME || self->mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
            FREE(self->mVarName);
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

        sParserClass* klass = hash_item(gParserClasses, buf);

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

            *node = ALLOC sNodeTree_create_define_var(buf, klass, NULL, NULL, NULL);
        }
        else {
            sParserVar* var = hash_item(gGlobalVars, buf);

            if(var == NULL) {
                char buf2[128];
                snprintf(buf2, 128, "there is no definition of this variable(%s)\n", buf);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            *node = ALLOC sNodeTree_create_var(buf, var->mClass, NULL, NULL, NULL);
        }

        /// tail ///
        if(**p == '+' && *(*p+1) == '+') {
            (*p)+=2;
            skip_spaces(p);

            *node = ALLOC sNodeTree_create_operand(kOpPlusPlus2, *node, NULL, NULL);
        }
        else if(**p == '-' && *(*p+1) == '-') {
            (*p)+=2;
            skip_spaces(p);

            *node = ALLOC sNodeTree_create_operand(kOpMinusMinus2, *node, NULL, NULL);
        }
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

// from right to left order
static BOOL expression_equal(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
{
    if(!expression_add_sub(ALLOC node, p, sname, sline)) {
        return FALSE;
    }

    while(**p) {
        if(**p == '=') {
            (*p)++;
            skip_spaces(p);
            sNodeTree* right;
            if(!expression_equal(ALLOC &right, p, sname, sline)) {
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

            if((*node)->mType != NODE_TYPE_VARIABLE_NAME && (*node)->mType != NODE_TYPE_DEFINE_VARIABLE_NAME) {
                parser_err_msg("require varible name on left value of equal", sname, *sline);
                sNodeTree_free(right);
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpEqual, *node, right, NULL);
        }
        else {
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(ALLOC sNodeTree** node, char** p, char* sname, int* sline)
{
    return expression_equal(ALLOC node, p, sname, sline);
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

static BOOL compile_node(sNodeTree* node, sParserClass** klass, sByteCode* code, sConst* constant, char* sname, int* sline)
{
    switch(node->mType) {
        /// operand ///
        case NODE_TYPE_OPERAND:
            switch(node->mOperand) {
            case kOpAdd: {
                sParserClass* left_class = NULL;
                if(node->mLeft) {
                    if(!compile_node(node->mLeft, &left_class, code, constant, sname, sline)) {
                        return FALSE;
                    }
                }
                sParserClass* right_class = NULL;
                if(node->mRight) {
                    if(!compile_node(node->mRight, &right_class, code, constant, sname, sline)) {
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
                sNodeTree* lnode = node->mLeft; // lnode must be variable name or definition of variable

                sParserClass* left_class;
                sParserVar* var;
                if(lnode->mType == NODE_TYPE_DEFINE_VARIABLE_NAME) {
                    if(!compile_node(node->mLeft, &left_class, code, constant, sname, sline)) {
                        return FALSE;
                    }

                    var = hash_item(gGlobalVars, lnode->mVarName);

                    if(var == NULL) {
                        char buf[128];
                        snprintf(buf, 128, "there is no definition of this variable(%s)\n", lnode->mVarName);
                        parser_err_msg(buf, sname, *sline);
                        return FALSE;
                    }
                }
                else { // NODE_TYPE_VARIABLE_NAME
                    left_class = var->mClass;

                    var = hash_item(gGlobalVars, lnode->mVarName);

                    if(var == NULL) {
                        char buf[128];
                        snprintf(buf, 128, "there is no definition of this variable(%s)\n", lnode->mVarName);
                        parser_err_msg(buf, sname, *sline);
                        return FALSE;
                    }
                }


                sParserClass* right_class = NULL;
                if(node->mRight) {
                    if(!compile_node(node->mRight, &right_class, code, constant, sname, sline)) {
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
            int data = node->mValue;
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

            uint len = strlen(node->mStringValue);
            wchar_t* wcs = MALLOC(sizeof(wchar_t)*(len+1));
            mbstowcs(wcs, node->mStringValue, len+1);

            sConst_append(constant, &len, sizeof(len));
            sConst_append(constant, wcs, sizeof(wchar_t)*len);

            FREE(wcs);

            *klass = gStringClass;
            }
            break;

        /// variable name ///
        case NODE_TYPE_VARIABLE_NAME: {
            sParserVar* var = hash_item(gGlobalVars, node->mVarName);

            if(var == NULL) {
                char buf[128];
                snprintf(buf, 128, "there is this varialbe (%s)\n", node->mVarName);
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
            char* name = node->mVarName;

            if(hash_item(gGlobalVars, name)) {
                char buf2[128];
                snprintf(buf2, 128, "there is a same name variable(%s)\n", name);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            hash_put(gGlobalVars, name, sParserVar_new(name, node->mClass, hash_count(gGlobalVars)));

            *klass = node->mClass;
            }
            break;
    }

    return TRUE;
}

// source is null-terminated
BOOL cl_parse(char* source, char* sname, int* sline, sByteCode* code, sConst* constant, int* global_var_num, BOOL flg_main)
{
    char* p = source;
    while(*p) {
        sNodeTree* node = NULL;
        if(!node_expression(ALLOC &node, &p, sname, sline)) {
            sNodeTree_free(node);
            return FALSE;
        }

        sParserClass* klass;
        if(!compile_node(ALLOC node, &klass, code, constant, sname, sline)) {
            sNodeTree_free(node);
            return FALSE;
        }

        sNodeTree_free(node);

        if(*p == ';' || *p == '\n') {
            while(*p == ';' || *p == '\n') {
                p++;
                skip_spaces(&p);
            }

//            uchar c = OP_POP;
//            sByteCode_append(code, &c, sizeof(uchar));
        }
        else {
            char buf[128];
            snprintf(buf, 128, "unexpected character(%c)\n", *p);
            parser_err_msg(buf, sname, *sline);
            return FALSE;
        }
    }

    *global_var_num = hash_count(gGlobalVars);

    return TRUE;
}

void parser_init()
{
    gParserClasses = HASH_NEW(10);
    hash_put(gParserClasses, "int", gIntClass = sParserClass_new("int"));
    hash_put(gParserClasses, "float", gFloatClass = sParserClass_new("float"));
    hash_put(gParserClasses, "String", gStringClass = sParserClass_new("String"));

    gGlobalVars = HASH_NEW(10);
}

void parser_final()
{
    hash_it* it = hash_loop_begin(gParserClasses);
    while(it) {
        sParserClass* klass = hash_loop_item(it);
        sParserClass_delete(klass);
        it = hash_loop_next(it);
    }
    hash_delete(gParserClasses);

    it = hash_loop_begin(gGlobalVars);
    while(it) {
        sParserVar* var = hash_loop_item(it);
        sParserVar_delete(var);
        it = hash_loop_next(it);
    }
    hash_delete(gGlobalVars);
}

