#include "clover.h"
#include "common.h"
#include <ctype.h>

//////////////////////////////////////////////////
// general parse tools
//////////////////////////////////////////////////

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
    char* p2;

    buf[0] = 0;

    p2 = buf;

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
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    BOOL found;
    char* p2;

    skip_spaces_and_lf(p, sline);

    found = FALSE;
    p2 = characters;
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
        parser_err_msg_format(sname, *sline, "expected that next character is %s", characters);
        (*err_num)++;
        (*p)++;
    }
}

// characters is null-terminated
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    char c;
    BOOL err;
    
    err = FALSE;
    while(1) {
        BOOL found;
        char* p2;

        if(**p == '0') {
            parser_err_msg_format(sname, *sline, "clover has expected that next characters are '%s', but it arrived at source end", characters);
            return FALSE;
        }

        found = FALSE;
        p2 = characters;
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

/// result (-1): not found (>=0): found the generic type num
int get_generics_type_num(sCLClass* klass, char* type_name)
{
    int generics_type_num;

    /// get generics num ///
    generics_type_num = -1;

    if(klass != NULL) {
        if(klass->mGenericsTypesNum > 0) {
            int i;

            for(i=0; i<klass->mGenericsTypesNum; i++) {
                if(strcmp(type_name, CONS_str(klass->mConstPool, klass->mGenericsTypesOffset[i])) == 0) {
                    generics_type_num = i;
                    break;
                }
            }
        }
    }

    return generics_type_num;
}

// result: (FALSE) there is an error (TRUE) success
// result class is setted on first parametor
BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass)
{
    char buf[128];

    //// default namespace ///
    if(**p == ':' && *(*p + 1) == ':') {
        (*p)+=2;
        skip_spaces(p);

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        *result = cl_get_class_with_namespace("", buf);

        if(*result == NULL) {
            parser_err_msg_format(sname, *sline, "invalid class name(::%s)", buf);
            (*err_num)++;
        }
    }
    /// original namespace ///
    else {
        int generics_type_num;

        /// a first word ///
        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        /// get generics type num ///
        generics_type_num = get_generics_type_num(klass, buf);

        /// it is a generics type ///
        if(generics_type_num >= 0) {
            *result = gAnonymousType[generics_type_num].mClass;
        }
        /// it is not a generics type ///
        else {
            /// a second word ///
            if(**p == ':' && *(*p + 1) == ':') {
                char buf2[128];

                (*p)+=2;
                skip_spaces(p);

                if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces(p);

                *result = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(*result == NULL) {
                    parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", buf, buf2);
                    (*err_num)++;
                }
            }
            else {
                *result = cl_get_class_with_namespace(current_namespace, buf);

                if(*result == NULL) {
                    parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", current_namespace, buf);
                    (*err_num)++;
                }
            }
        }
    }

    return TRUE;
}

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLClass** generics_types, char* current_namespace, sCLClass* klass)
{
    if(**p == '<') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        while(1) {
            sCLClass* klass2;

            if(!parse_namespace_and_class(&klass2, p, sname, sline, err_num, current_namespace, klass)) {
                return FALSE;
            }

            generics_types[*generics_types_num] = klass2;
            (*generics_types_num)++;

            if(*generics_types_num >= CL_GENERICS_CLASS_PARAM_MAX) {
                parser_err_msg_format(sname, *sline, "Overflow generics types number");
                return FALSE;
            }

            if(**p == 0) {
                parser_err_msg_format(sname, *sline, "It arrived at the end of source before > closing\n");
                return FALSE;
            }
            else if(**p == '>') {
                break;
            }
            else {
                expect_next_character_with_one_forward(",", err_num, p, sname, sline);
            }
        }

        expect_next_character_with_one_forward(">", err_num, p, sname, sline);
    }
    else {
        *generics_types_num = 0;
    }

    return TRUE;
}

// result: (FALSE) there is an error (TRUE) success
// result type is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(sCLNodeType* type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass) 
{
    if(!parse_namespace_and_class(&type->mClass, p, sname, sline, err_num, current_namespace, klass)) {
        return FALSE;
    }
    if(!parse_generics_types_name(p, sname, sline, err_num, &type->mGenericsTypesNum, type->mGenericsTypes, current_namespace, klass)) 
    {
        return FALSE;
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
    int len;

    len = strlen(str);

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
    self->mCode = MALLOC(sizeof(unsigned char)*self->mSize);
}

void sByteCode_free(sByteCode* self)
{
    FREE(self->mCode);
}

void sByteCode_append(sByteCode* self, void* code, unsigned int size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mCode = REALLOC(self->mCode, sizeof(unsigned char) * self->mSize);
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
    self->mConst = CALLOC(1, sizeof(unsigned char)*self->mSize);
}

void sConst_free(sConst* self)
{
    FREE(self->mConst);
}

void sConst_append(sConst* self, void* data, unsigned int size)
{
    if(self->mSize <= self->mLen + size + 1) {
        self->mSize = (self->mSize + size) * 2;
        self->mConst = REALLOC(self->mConst, sizeof(unsigned char) * self->mSize);
    }

    memcpy(self->mConst + self->mLen, data, size);
    self->mLen += size;
}

void sConst_append_int(sConst* constant, int n)
{
    unsigned char type;

    type = CONSTANT_INT;
    sConst_append(constant, &type, sizeof(unsigned char));

    sConst_append(constant, &n, sizeof(int));
}

void sConst_append_str(sConst* constant, char* str)
{
    unsigned char type;
    int len;
    
    type = CONSTANT_STRING;
    sConst_append(constant, &type, sizeof(unsigned char));

    len = strlen(str);
    sConst_append(constant, &len, sizeof(len));
    sConst_append(constant, str, len+1);
}

void sConst_append_wstr(sConst* constant, char* str)
{
    unsigned char type;
    unsigned int len;
    wchar_t* wcs;

    type = CONSTANT_WSTRING;
    sConst_append(constant, &type, sizeof(unsigned char));

    len = strlen(str);
    wcs = MALLOC(sizeof(wchar_t)*(len+1));
    mbstowcs(wcs, str, len+1);

    sConst_append(constant, &len, sizeof(len));
    sConst_append(constant, wcs, sizeof(wchar_t)*(len+1));

    FREE(wcs);
}

//////////////////////////////////////////////////
// local variable and global variable table
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_)
{
    int hash_value;
    sVar* p;

    hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;
    p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            xstrncpy(p->mName, name, CL_METHOD_NAME_MAX);
            p->mIndex = table->mVarNum++;
            p->mType = *type_;

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
    int hash_value;
    sVar* p;

    hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    p = table->mLocalVariables + hash_value;

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
static BOOL get_params(char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, unsigned int* res_node, char* current_namespace, sCLClass* klass)
{
    unsigned int params_num;

    params_num = 0;

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
                unsigned int new_node;

                if(!node_expression(&new_node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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

static BOOL parse_class_method_or_class_field_or_variable_definition(sCLNodeType* type, unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
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
            unsigned int param_node;
            
            param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace, klass)) {
                return FALSE;
            }

            *node = sNodeTree_create_class_method_call(buf, type, param_node, 0, 0);
        }
        /// access class field ///
        else {
            *node = sNodeTree_create_class_field(buf, type, 0, 0, 0);
        }
    }
    /// define variable ///
    else {
        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        *node = sNodeTree_create_define_var(buf, type, 0, 0, 0);
    }

    return TRUE;
}

static BOOL expression_node(unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
{
    if((**p >= '0' && **p <= '9') || **p == '-' || **p == '+') {
        char buf[128];
        char* p2;

        p2 = buf;

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
        sBuf value;

        (*p)++;

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
        int sline2;
        unsigned int new_node;
        unsigned int elements_num;

        (*p)++;
        skip_spaces(p);

        sline2 = *sline;

        new_node = 0;
        elements_num = 0;

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
                    unsigned int new_node2;
                    if(!node_expression(&new_node2, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
    /// default namespace ///
    else if(**p == ':' && *(*p+1) == ':') {
        char buf[128];
        sCLNodeType type;

        (*p)+=2;

        if(!parse_word(buf, 128, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces(p);

        memset(&type, 0, sizeof(type));
        type.mClass = cl_get_class_with_namespace("", buf);

        if(type.mClass == NULL) {
            parser_err_msg_format(sname, *sline, "there is no definition of this namespace and class name(::%s)", buf);
            (*err_num)++;

            *node = 0;
        }
        /// class method , class field, or variable definition ///
        else {
            if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass))
            {
                return FALSE;
            }

            if(!parse_class_method_or_class_field_or_variable_definition(&type, node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
            sCLNodeType type;

            memset(&type, 0, sizeof(type));

            if(!parse_namespace_and_class_and_generics_type(&type, p, sname, sline, err_num, current_namespace, klass)) {
                return FALSE;
            }

            if(**p == '(') {
                unsigned int param_node;

                param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace, klass)) {
                    return FALSE;
                }

                if(type.mClass) {
                    *node = sNodeTree_create_new_expression(&type, param_node, 0, 0);
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
            unsigned int param_node;
            
            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// call inherit ///
            param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace, klass)) {
                return FALSE;
            }

            *node = sNodeTree_create_inherit(param_node, 0, 0);
        }
        else if(strcmp(buf, "super") == 0) {
            unsigned int param_node;
            
            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// call super ///
            param_node = 0;
            if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace, klass)) {
                return FALSE;
            }

            *node = sNodeTree_create_super(param_node, 0, 0);
        }
        else if(strcmp(buf, "return") == 0) {
            unsigned int rv_node;

            if(**p == '(') {
                (*p)++;
                skip_spaces(p);

                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
                if(!node_expression(&rv_node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
                    return FALSE;
                }
                skip_spaces(p);
            }

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, *sline);
                (*err_num)++;
            }

            *node = sNodeTree_create_return(&gNodes[rv_node].mType, rv_node, 0, 0);
        }
        /// user words ///
        else {
            /// class name with namespace ///
            if(**p == ':' && *(*p+1) == ':') {
                char buf2[128];
                sCLNodeType type;

                (*p)+=2;

                if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces(p);

                memset(&type, 0, sizeof(type));
                type.mClass = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(type.mClass == NULL) {
                    parser_err_msg_format(sname, *sline, "there is no definition of this namespace and class name(%s::%s)", buf, buf2);
                    (*err_num)++;

                    *node = 0;
                }
                /// class method , class field, or variable definition ///
                else {
                    if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass))
                    {
                        return FALSE;
                    }

                    if(!parse_class_method_or_class_field_or_variable_definition(&type, node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
                        return FALSE;
                    }
                }
            }
            /// user word ///
            else {
                int generics_type_num;

                /// is this generic type ? ///
                generics_type_num = get_generics_type_num(klass, buf);

                if(generics_type_num != -1) {
                    sCLNodeType type;

                    memset(&type, 0, sizeof(type));

                    type.mClass = gAnonymousType[generics_type_num].mClass;

                    if(!parse_class_method_or_class_field_or_variable_definition(&type, node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
                        return FALSE;
                    }
                }
                else {
                    sCLNodeType type;

                    memset(&type, 0, sizeof(type));

                    type.mClass = cl_get_class_with_namespace(current_namespace, buf);

                    /// variable ///
                    if(type.mClass == NULL) {
                        char* name;
                        sVar* var;

                        name = buf;
                        var = get_variable_from_table(lv_table, name);

                        if(var == NULL) {
                            parser_err_msg_format(sname, *sline, "there is no definition of this variable(%s)", name);
                            (*err_num)++;

                            *node = 0;
                        }
                        else {
                            *node = sNodeTree_create_var(buf, &var->mType, 0, 0, 0);
                        }
                    }
                    /// class method , class field, or variable definition ///
                    else {
                        if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass))
                        {
                            return FALSE;
                        }

                        if(!parse_class_method_or_class_field_or_variable_definition(&type, node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
                            return FALSE;
                        }
                    }
                }
            }
        }
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces(p);

        if(!node_expression(node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
                unsigned int param_node;

                param_node = 0;
                if(!get_params(p, sname, sline, err_num, lv_table, &param_node, current_namespace, klass)) {
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

static BOOL expression_mult_div(unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
{
    if(!expression_node(node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
        return FALSE;
    }

    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '*' && *(*p+1) != '=') {
            unsigned int right;

            (*p)++;
            skip_spaces(p);
            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
            unsigned int right;

            (*p)++;
            skip_spaces(p);

            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
            unsigned int right;

            (*p)++;
            skip_spaces(p);

            if(!expression_node(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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

static BOOL expression_add_sub(unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
{
    if(!expression_mult_div(node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) != '=' && *(*p+1) != '+') {
            unsigned int right;

            (*p)++;
            skip_spaces(p);

            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
            unsigned int right;

            (*p)++;
            skip_spaces(p);

            if(!expression_mult_div(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
static BOOL expression_equal(unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
{
    if(!expression_add_sub(node, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '=') {
            unsigned int right;

            (*p)++;
            skip_spaces(p);
            if(!expression_equal(&right, p, sname, sline, err_num, lv_table, current_namespace, klass)) {
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
                switch(gNodes[*node].mNodeType) {
                    case NODE_TYPE_VARIABLE_NAME:
                        gNodes[*node].mNodeType = NODE_TYPE_STORE_VARIABLE_NAME;
                        break;

                    case NODE_TYPE_DEFINE_VARIABLE_NAME: {
                        gNodes[*node].mNodeType = NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME;
                        }
                        break;

                    case NODE_TYPE_FIELD:
                        gNodes[*node].mNodeType = NODE_TYPE_STORE_FIELD;
                        break;

                    case NODE_TYPE_CLASS_FIELD:
                        gNodes[*node].mNodeType = NODE_TYPE_STORE_CLASS_FIELD;
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

BOOL node_expression(unsigned int* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass)
{
    return expression_equal(node, p, sname, sline, err_num, lv_table, current_namespace, klass);
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

