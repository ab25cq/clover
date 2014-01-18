#include "clover.h"
#include "common.h"
#include <ctype.h>

// skip spaces
void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

void skip_spaces_and_lf(char** p, int* sline)
{
    while(**p == ' ' || **p == '\t' || (**p == '\n' && (*sline)++)) {
        (*p)++;
    }
}

void parser_err_msg(char* msg, char* sname, int sline)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline, msg);
}

void parser_err_msg_format(char* sname, int sline, char* msg, ...)
{
    char msg2[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s %d: %s\n", sname, sline, msg2);
}

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num)
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
                parser_err_msg_format(sname, *sline, "length of word is too long");
                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(**p == 0) {
        parser_err_msg_format(sname, *sline, "require word(alphabet or _ or number). this is the end of source");
        return FALSE;
    }

    if(buf[0] == 0) {
        parser_err_msg_format(sname, *sline, "require word(alphabet or _ or number). this is (%c)\n", **p);

        (*err_num)++;

        if(**p == '\n') (*sline)++;

        (*p)++;
    }

    return TRUE;
}

// characters is null-terminated
void expect_next_character_without_forward_pointer(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    skip_spaces_and_lf(p, sline);

    BOOL found = FALSE;
    char* p2 = characters;
    while(*p2) {
        if(**p == *p2) {
            found = TRUE;
        }
        p2++;
    }

    if(found) {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        parser_err_msg_format(sname, *sline, "expected that next character is ;");
        (*err_num)++;
    }
}

// characters is null-terminated
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    char c;
    BOOL err = FALSE;
    while(1) {
        if(**p == '0') {
            parser_err_msg_format(sname, *sline, "clover has expected that next characters are '%s', but it arrived at source end", characters);
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
            c = **p;
            if(**p == '\n') { (*sline)++; }
            (*p)++;
        }
    }

    if(err) {
        parser_err_msg_format(sname, *sline, "clover has expected that next characters are '%s', but there are some characters(%c) before them", characters, c);
        (*err_num)++;
    }

    return TRUE;
}

//////////////////////////////////////////////////
// resizable buf
//////////////////////////////////////////////////
void sBuf_init(sBuf* self)
{
    self->mBuf = MALLOC(sizeof(char)*64);
    self->mSize = 64;
    self->mLen = 0;
    *(self->mBuf) = 0;
}

void sBuf_append_char(sBuf* self, char c)
{
    if(self->mSize <= self->mLen + 1 + 1) {
        self->mSize = (self->mSize + 1 + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    self->mBuf[self->mLen] = c;
    self->mBuf[self->mLen+1] = 0;
    self->mLen++;
}

void sBuf_append(sBuf* self, void* str, size_t size)
{
    const int len = strlen(str);

    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size + 1) * 2;
        self->mBuf = REALLOC(self->mBuf, sizeof(char)*self->mSize);
    }

    memcpy(self->mBuf + self->mLen, str, size);

    self->mLen += size;
    self->mBuf[self->mLen] = 0;
}

//////////////////////////////////////////////////
// Byte Code operation. Make it resizable
//////////////////////////////////////////////////
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

//////////////////////////////////////////////////
// Constant Pool operation. Make it resizable
//////////////////////////////////////////////////
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

void sConst_append_int(sConst* constant, int n)
{
    uchar type = CONSTANT_INT;
    sConst_append(constant, &type, sizeof(uchar));

    sConst_append(constant, &n, sizeof(int));
}

void sConst_append_str(sConst* constant, char* str)
{
    uchar type = CONSTANT_STRING;
    sConst_append(constant, &type, sizeof(uchar));

    int len = strlen(str);
    sConst_append(constant, &len, sizeof(len));
    sConst_append(constant, str, len+1);
}

void sConst_append_wstr(sConst* constant, char* str)
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

//////////////////////////////////////////////////
// local variable and global variable table
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLClass* klass)
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
sVar* get_variable_from_table(sVarTable* table, char* name)
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
// parse the source and make nodes
//////////////////////////////////////////////////
static BOOL get_params(char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, uint* res_node, char* current_namespace)
{
    uint params_num = 0;

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
                if(!node_expression(&new_node, p, sname, sline, err_num, lv_table, current_namespace)) {
                    return FALSE;
                }

                skip_spaces(p);

                if(new_node) {
                    if(params_num < CL_METHOD_PARAM_MAX) {
                        *res_node = sNodeTree_create_param(*res_node, new_node,  0);
                        params_num++;
                    }
                    else {
                        parser_err_msg_format(sname, *sline, "parametor number is overflow");
                        (*err_num)++;
                        return FALSE;
                    }
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

// result: (FALSE) there is an error (TRUE) success
// result class is setted on first parametor
BOOL parse_namespace_and_class(sCLClass** klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace) 
{
    char buf[128];

    //// default namespace ///
    if(**p == ':' && *(*p + 1) == ':') {
        (*p)+=2;

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        *klass = cl_get_class_with_namespace("", buf);

        if(*klass == NULL) {
            parser_err_msg_format(sname, *sline, "invalid class name(::%s)", buf);
            (*err_num)++;
        }
    }
    /// original namespace ///
    else {
        /// a first word ///
        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        /// a second word ///
        if(**p == ':' && *(*p + 1) == ':') {
            (*p)+=2;

            char buf2[128];

            if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces(p);

            *klass = cl_get_class_with_argument_namespace_only(buf, buf2);

            if(*klass == NULL) {
                parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", buf, buf2);
                (*err_num)++;
            }
        }
        else {
            *klass = cl_get_class_with_namespace(current_namespace, buf);

            if(*klass == NULL) {
                parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", current_namespace, buf);
                (*err_num)++;
            }
        }
    }

    return TRUE;
}

static BOOL parse_class_method_or_class_field_or_variable_definition(sCLClass* klass, uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    char buf[128];

    /// class method or class field ///
    if(**p == '.') {
        (*p)++;
        skip_spaces(p);

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        /// call class method ///
        if(**p == '(') {
            uint param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace)) {
                return FALSE;
            }

            *node = sNodeTree_create_class_method_call(buf, klass, param_node, 0, 0);
        }
        /// access class field ///
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

    return TRUE;
}

static BOOL expression_node(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    if((**p >= '0' && **p <= '9') || **p == '-' || **p == '+') {
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
    else if(**p == '{') {
        (*p)++;
        skip_spaces(p);

        int sline2 = *sline;

        uint new_node = 0;
        uint elements_num = 0;

        if(**p == '}') {
            (*p)++;
            skip_spaces(p);
        }
        else {
            while(1) {
                if(**p == 0) {
                    parser_err_msg_format(sname, sline2, "require } to end of the source");
                    (*err_num)++;
                    break;
                }
                else {
                    uint new_node2;
                    if(!node_expression(&new_node2, p, sname, sline, err_num, lv_table, current_namespace)) {
                        return FALSE;
                    }
                    skip_spaces(p);

                    if(new_node2) {
                        if(elements_num < CL_ARRAY_ELEMENTS_MAX) {
                            new_node = sNodeTree_create_param(new_node, new_node2,  0);
                            elements_num++;
                        }
                        else {
                            parser_err_msg_format(sname, *sline, "number of array elements overflow");
                            return FALSE;
                        }
                    }

                    if(**p == ',') {
                        (*p)++;
                        skip_spaces(p);
                    }
                    else if(**p == '}') {
                        (*p)++;
                        skip_spaces(p);
                        break;
                    }
                    else {
                        parser_err_msg_format(sname, *sline, "require , or } after an array element");
                        (*err_num)++;
                        break;
                    }
                }
            }
        }

        *node = sNodeTree_create_array(new_node, 0, 0);
    }
    else if(**p == ':' && *(*p+1) == ':') {
        (*p)+=2;

        char buf[128];

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        sCLClass* klass = cl_get_class_with_namespace("", buf);

        if(klass == NULL) {
            parser_err_msg_format(sname, *sline, "there is no definition of this namespace and class name(::%s)", buf);
            (*err_num)++;

            *node = 0;
        }
        /// class method , class field, or variable definition ///
        else {
            if(!parse_class_method_or_class_field_or_variable_definition(klass, node, p, sname, sline, err_num, lv_table, current_namespace)) {
                return FALSE;
            }
        }
    }
    else if(isalpha(**p)) {
        char buf[128];

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        if(strcmp(buf, "new") == 0) {
            sCLClass* klass;
            if(!parse_namespace_and_class(&klass, p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }

            if(**p == '(') {
                uint param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace)) {
                    return FALSE;
                }

                if(klass) {
                    *node = sNodeTree_create_new_expression(klass, param_node, 0, 0);
                }
                else {
                    *node = 0;
                }
            }
            else {
                parser_err_msg("require (", sname, *sline);
                (*err_num)++;

                *node = 0;
            }
        }
        else if(strcmp(buf, "inherit") == 0) {
            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// call inherit ///
            uint param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace)) {
                return FALSE;
            }

            *node = sNodeTree_create_inherit(param_node, 0, 0);
        }
        else if(strcmp(buf, "super") == 0) {
            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// call super ///
            uint param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace)) {
                return FALSE;
            }

            *node = sNodeTree_create_super(param_node, 0, 0);
        }
        else if(strcmp(buf, "return") == 0) {
            uint rv_node;
            if(**p == '(') {
                (*p)++;
                skip_spaces(p);

                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            /// class name with namespace ///
            if(**p == ':' && *(*p+1) == ':') {
                (*p)+=2;

                char buf2[128];

                if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces(p);

                sCLClass* klass = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(klass == NULL) {
                    parser_err_msg_format(sname, *sline, "there is no definition of this namespace and class name(%s::%s)", buf, buf2);
                    (*err_num)++;

                    *node = 0;
                }
                /// class method , class field, or variable definition ///
                else {
                    if(!parse_class_method_or_class_field_or_variable_definition(klass, node, p, sname, sline, err_num, lv_table, current_namespace)) {
                        return FALSE;
                    }
                }
            }
            /// user word ///
            else {
                sCLClass* klass = cl_get_class_with_namespace(current_namespace, buf);

                /// variable ///
                if(klass == NULL) {
                    char* name = buf;
                    sVar* var = get_variable_from_table(lv_table, name);

                    if(var == NULL) {
                        parser_err_msg_format(sname, *sline, "there is no definition of this variable(%s)", name);
                        (*err_num)++;

                        *node = 0;
                    }
                    else {
                        *node = sNodeTree_create_var(buf, var->mClass, 0, 0, 0);
                    }
                }
                /// class method , class field, or variable definition ///
                else {
                    if(!parse_class_method_or_class_field_or_variable_definition(klass, node, p, sname, sline, err_num, lv_table, current_namespace)) {
                        return FALSE;
                    }
                }
            }
        }
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces(p);

        if(!node_expression(node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
        parser_err_msg_format(sname, *sline, "invalid character (%c)", **p);
        *node = 0;
        if(**p == '\n') (*sline)++;
        (*p)++;
        (*err_num)++;
    }

    /// call method or access field ///
    while(**p == '.') {
        (*p)++;
        skip_spaces(p);

        if(isalpha(**p)) {
            char buf[128];

            if(!parse_word(buf, 128, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces(p);

            /// call methods ///
            if(**p == '(') {
                uint param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace)) {
                    return FALSE;
                }

                *node = sNodeTree_create_method_call(buf, *node, param_node, 0);
            }
            /// access fields ///
            else {
                *node = sNodeTree_create_fields(buf, *node, 0, 0);
            }
        }
        else {
            parser_err_msg("require method name or field name after .", sname, *sline);
            (*err_num)++;

            *node = 0;
            break;
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

static BOOL expression_mult_div(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    if(!expression_node(node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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

static BOOL expression_add_sub(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    if(!expression_mult_div(node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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
static BOOL expression_equal(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    if(!expression_add_sub(node, p, sname, sline, err_num, lv_table, current_namespace)) {
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
            if(!expression_equal(&right, p, sname, sline, err_num, lv_table, current_namespace)) {
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
                switch(gNodes[*node].mType) {
                    case NODE_TYPE_VARIABLE_NAME:
                        gNodes[*node].mType = NODE_TYPE_STORE_VARIABLE_NAME;
                        break;

                    case NODE_TYPE_DEFINE_VARIABLE_NAME: {
                        gNodes[*node].mType = NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME;
                        }
                        break;

                    case NODE_TYPE_FIELD:
                        gNodes[*node].mType = NODE_TYPE_STORE_FIELD;
                        break;

                    case NODE_TYPE_CLASS_FIELD:
                        gNodes[*node].mType = NODE_TYPE_STORE_CLASS_FIELD;
                        break;

                    default:
                        parser_err_msg("require varible name on left node of equal", sname, *sline);
                        (*err_num)++;
                        break;
                }
                gNodes[*node].mRight = right;
            }
        }
        else {
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace)
{
    return expression_equal(node, p, sname, sline, err_num, lv_table, current_namespace);
}

//////////////////////////////////////////////////
// Initialization and finalization of this module
//////////////////////////////////////////////////
sVarTable gGVTable;       // global variable table

void parser_init(BOOL load_foundamental_class)
{
    memset(&gGVTable, 0, sizeof(gGVTable));
}

void parser_final()
{
}

