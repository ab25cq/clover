#include "clover.h"
#include "common.h"
#include <ctype.h>

//////////////////////////////////////////////////
// general parse tools
//////////////////////////////////////////////////
/*
// skip spaces
void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}
*/

void skip_spaces_and_lf(char** p, int* sline)
{
    while(**p == ' ' || **p == '\t' || (**p == '\n' && (*sline)++)) {
        (*p)++;
    }
}

void compile_error(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}

void parser_err_msg(char* msg, char* sname, int sline_top)
{
    fprintf(stderr, "%s %d: %s\n", sname, sline_top, msg);
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

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg)
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
                if(print_out_err_msg) {
                    parser_err_msg_format(sname, *sline, "length of word is too long");
                }
                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(**p == 0) {
        if(print_out_err_msg) {
            parser_err_msg_format(sname, *sline, "require word(alphabet or _ or number). this is the end of source");
        }
        return FALSE;
    }

    if(buf[0] == 0) {
        if(print_out_err_msg) {
            parser_err_msg_format(sname, *sline, "require word(alphabet or _ or number). this is (%c)\n", **p);
            (*err_num)++;
        }

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
        skip_spaces_and_lf(p, sline);
    }
}

// characters is null-terminated
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    char c;
    BOOL err;
    int sline_top;

    sline_top = *sline;
    
    err = FALSE;
    while(1) {
        BOOL found;
        char* p2;

        if(**p == 0) {
            parser_err_msg_format(sname, sline_top, "clover has expected that next characters are '%s', but it arrived at source end", characters);
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
        parser_err_msg_format(sname, sline_top, "clover has expected that next characters are '%s', but there are some characters(%c) before them", characters, c);
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
                if(strcmp(type_name, CONS_str(&klass->mConstPool, klass->mGenericsTypesOffset[i])) == 0) {
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
        skip_spaces_and_lf(p, sline);

        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        *result = cl_get_class_with_namespace("", buf, klass == NULL);         // when eval, vm runtime, and interpreter, klass is NULL

        if(*result == NULL) {
            parser_err_msg_format(sname, *sline, "invalid class name(::%s)", buf);
            (*err_num)++;
        }
    }
    /// original namespace ///
    else {
        int generics_type_num;

        /// a first word ///
        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

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
                skip_spaces_and_lf(p, sline);

                if(!parse_word(buf2, 128, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                *result = cl_get_class_with_argument_namespace_only(buf, buf2, klass == NULL);      // when eval, vm runtime, and interpreter, klass is NULL

                if(*result == NULL) {
                    parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", buf, buf2);
                    (*err_num)++;
                }
            }
            else {
                *result = cl_get_class_with_namespace(current_namespace, buf, klass == NULL);       // when eval, vm runtime, and interpreter, klass is NULL

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

BOOL delete_comment(sBuf* source, sBuf* source2)
{
    char* p;

    p = source->mBuf;

    while(*p) {
        if(*p == '/' && *(p+1) == '/') {
            while(1) {
                if(*p == 0) {
                    break;
                }
                else if(*p == '\n') {
                    //p++;      // no delete line field for error message
                    break;
                }
                else {
                    p++;
                }
            }
        }
        else if(*p == '/' && *(p+1) == '*') {
            int nest;

            p+=2;
            nest = 0;
            while(1) {
                if(*p == 0) {
                    compile_error("there is not a comment end until source end\n");
                    return FALSE;
                }
                else if(*p == '/' && *(p+1) == '*') {
                    p+=2;
                    nest++;
                }
                else if(*p == '*' && *(p+1) == '/') {
                    p+=2;
                    if(nest == 0) {
                        break;
                    }

                    nest--;
                }
                else if(*p == '\n') {
                    sBuf_append_char(source2, *p);   // no delete line field for error message
                    p++;
                }
                else {
                    p++;
                }
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}

BOOL parse_params(sCLNodeType* class_params, int* num_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sVarTable* lv_table, char close_character, int sline_top)
{
    *num_params = 0;

    if(**p == close_character) {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            sCLNodeType param_type;
            char param_name[CL_METHOD_NAME_MAX+1];
            char close_characters[64];

            memset(&param_type, 0, sizeof(param_type));

            /// class and generics types ///
            if(!parse_namespace_and_class_and_generics_type(&param_type, p, sname, sline, err_num, current_namespace, klass)) {
                return FALSE;
            }

            /// name ///
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(param_type.mClass) {
                class_params[*num_params] = param_type;
                (*num_params)++;

                if(lv_table) {
                    if(!add_variable_to_table(lv_table, param_name, &param_type)) {
                        parser_err_msg_format(sname, sline_top, "there is a same name variable(%s) or overflow local variable table", param_name);

                        (*err_num)++;
                        return TRUE;
                    }
                }
            }

            snprintf(close_characters, 64, "%c,", close_character);
            if(!expect_next_character(close_characters, err_num, p, sname, sline)) {
                return FALSE;
            }

            if(**p == close_character) {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == ',') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse the source and make nodes
//////////////////////////////////////////////////
static BOOL get_params(char** p, char* sname, int* sline, int* err_num, unsigned int* res_node, char* current_namespace, sCLNodeType* klass, char start_brace, char end_brace, sCLMethod* method, sVarTable* lv_table)
{
    int params_num;

    params_num = 0;

    *res_node = 0;
    if(**p == start_brace) {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(**p == end_brace) {
            (*p)++;
            skip_spaces_and_lf(p, sline);
        }
        else {
            while(1) {
                unsigned int new_node;
                char buf[128];

                if(!node_expression_without_comma(&new_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                    return FALSE;
                }

                skip_spaces_and_lf(p, sline);

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

                snprintf(buf, 128, ",%c", end_brace);
                if(!expect_next_character(buf, err_num, p, sname, sline)) {
                    return FALSE;
                }

                if(**p == ',') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);
                }
                else if(**p == end_brace) {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);
                    break;
                }
            }
        }
    }

    return TRUE;
}

static BOOL expression_node_while(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* type_, sCLMethod* method, sVarTable* lv_table)
{
    unsigned int conditional;
    unsigned int block;

    char buf[128];
    BOOL result;
    sVarTable new_table;

    /// new table ///
    init_block_vtable(&new_table, lv_table);

    if(!expect_next_character("(", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;

    /// conditional ///
    if(!node_expression(&conditional, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// block ///
    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!parse_block(&block, p, sname, sline, err_num, current_namespace, klass, gVoidType, method, &new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_block_vtable_to_node_block(&new_table, lv_table, block);

    *node = sNodeTree_create_while(conditional, block, type_);

    return TRUE;
}

static BOOL expression_node_do(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* type_, sCLMethod* method, sVarTable* lv_table)
{
    unsigned int conditional;
    unsigned int block;

    char buf[128];
    BOOL result;
    sVarTable new_table;

    /// new table ///
    init_block_vtable(&new_table, lv_table);

    /// block ///
    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!parse_block(&block, p, sname, sline, err_num, current_namespace, klass, gVoidType, method, &new_table)) {
        return FALSE;
    }

    /// while ///
    if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    if(strcmp(buf, "while") != 0) {
        parser_err_msg_format(sname, *sline, "require \"while\" for \"do\" statment");
        (*err_num)++;
    }

    if(!expect_next_character("(", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;

    /// conditional ///
    if(!node_expression(&conditional, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// entry new vtable to node block ///
    entry_block_vtable_to_node_block(&new_table, lv_table, block);

    *node = sNodeTree_create_do(conditional, block, type_);

    return TRUE;
}

static BOOL expression_node_for(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* type_, sCLMethod* method, sVarTable* lv_table)
{
    unsigned int conditional, conditional2, conditional3;
    unsigned int block;

    char buf[128];
    BOOL result;
    sVarTable new_table;

    /// new table ///
    init_block_vtable(&new_table, lv_table);

    if(!expect_next_character("(", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// conditional1 ///
    if(!node_expression(&conditional, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(";", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// conditional2 ///
    if(!node_expression(&conditional2, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(";", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// conditional3 ///
    if(!node_expression(&conditional3, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// block ///
    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!parse_block(&block, p, sname, sline, err_num, current_namespace, klass, gVoidType, method, &new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_block_vtable_to_node_block(&new_table, lv_table, block);

    *node = sNodeTree_create_for(conditional, conditional2, conditional3, block, type_);

    return TRUE;
}

static BOOL expression_node_if(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* type_, sCLMethod* method, sVarTable* lv_table)
{
    unsigned int if_conditional;
    unsigned int if_block;
    unsigned int else_block;
    unsigned int else_if_block[CL_ELSE_IF_MAX];
    unsigned int else_if_conditional[CL_ELSE_IF_MAX];
    int else_if_num;
    char buf[128];
    char* p2;
    BOOL result;
    sVarTable new_table;
    int sline_rewind;

    /// new table ///
    init_block_vtable(&new_table, lv_table);

    if(!expect_next_character("(", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;

    /// conditional ///
    if(!node_expression(&if_conditional, p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    /// block ///
    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }
    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!parse_block(&if_block, p, sname, sline, err_num, current_namespace, klass, *type_, method, &new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_block_vtable_to_node_block(&new_table, lv_table, if_block);

    /// "else" and "else if" block ///
    p2 = *p;
    sline_rewind = *sline;

    result = parse_word(buf, 128, p, sname, sline, err_num, FALSE);
    skip_spaces_and_lf(p, sline);

    else_if_num = 0;
    else_block = 0;

    if(result && strcmp(buf, "else") == 0) {
        while(1) {
            if(**p == '{') {
                /// new table ///
                init_block_vtable(&new_table, lv_table);

                /// block ///
                if(!expect_next_character("{", err_num, p, sname, sline)) {
                    return FALSE;
                }
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!parse_block(&else_block, p, sname, sline, err_num, current_namespace, klass, *type_, method, &new_table)) {
                    return FALSE;
                }

                /// entry new vtable to node block ///
                entry_block_vtable_to_node_block(&new_table, lv_table, else_block);

                *node = sNodeTree_create_if(if_conditional, if_block, else_block, else_if_conditional, else_if_block, else_if_num, type_);

                break;
            }
            else {
                p2 = *p;

                result = parse_word(buf, 128, p, sname, sline, err_num, FALSE);
                skip_spaces_and_lf(p, sline);

                if(result && strcmp(buf, "if") == 0) {
                    /// new table ///
                    init_block_vtable(&new_table, lv_table);

                    if(!expect_next_character("(", err_num, p, sname, sline)) {
                        return FALSE;
                    }
                    (*p)++;

                    /// conditional ///
                    if(!node_expression(&else_if_conditional[else_if_num], p, sname, sline, err_num, current_namespace, klass, method, &new_table)) {
                        return FALSE;
                    }

                    if(!expect_next_character(")", err_num, p, sname, sline)) {
                        return FALSE;
                    }
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    /// else if block ///
                    if(!expect_next_character("{", err_num, p, sname, sline)) {
                        return FALSE;
                    }
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(!parse_block(&else_if_block[else_if_num], p, sname, sline, err_num, current_namespace, klass, *type_, method, &new_table)) {
                        return FALSE;
                    }

                    /// entry new vtable to node block ///
                    entry_block_vtable_to_node_block(&new_table, lv_table, else_if_block[else_if_num]);

                    else_if_num++;

                    /// "else" and "else if" block ///
                    p2 = *p;

                    result = parse_word(buf, 128, p, sname, sline, err_num, FALSE);
                    skip_spaces_and_lf(p, sline);

                    if(!result || strcmp(buf, "else") != 0) {
                        *p = p2;   // rewind

                        *node = sNodeTree_create_if(if_conditional, if_block, else_block, else_if_conditional, else_if_block, else_if_num, type_);
                        break;
                    }
                }
                else {
                    parser_err_msg_format(sname, *sline, "require { or \"if\" after \"else\"");
                    (*err_num)++;
                    break;
                }
            }
        }
    }
    else {
        *p = p2;   // rewind
        *sline = sline_rewind;

        *node = sNodeTree_create_if(if_conditional, if_block, 0, NULL, NULL, 0, type_);
    }

    return TRUE;
}

static BOOL after_class_name(sCLNodeType* type, unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    char buf[128];

    /// class method or class field ///
    if(**p == '.') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// call class method ///
        if(**p == '(') {
            unsigned int param_node;
            unsigned int block;
            sCLNodeType result_type;
            
            param_node = 0;
            if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
                return FALSE;
            }

            if(**p == '(') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                memset(&result_type, 0, sizeof(result_type));

                if(!parse_namespace_and_class_and_generics_type(&result_type, p, sname, sline, err_num, current_namespace, (klass ? klass->mClass:NULL))) {
                    return FALSE;
                }

                expect_next_character_with_one_forward(")", err_num, p, sname, sline);
            }
            else {
                result_type = gVoidType;
            }

            /// method with block ///
            if(**p == '{') {
                sVarTable new_table;
                int local_var_num_before;

                (*p)++;
                skip_spaces_and_lf(p, sline);

                /// new table ///
                init_block_vtable(&new_table, lv_table);

                local_var_num_before = new_table.mVarNum;

                if(!parse_method_block(&block, p, sname, sline, err_num, current_namespace, klass, result_type, TRUE, method, &new_table, sline_top)) {
                    return FALSE;
                }

                /// entry new vtable to node block ///
                entry_method_block_vtable_to_node_block(&new_table, lv_table, block, local_var_num_before);
            }
            else {
                block = 0;
            }

            *node = sNodeTree_create_class_method_call(buf, type, param_node, 0, 0, block);
        }
        /// access class field ///
        else {
            *node = sNodeTree_create_class_field(buf, type, 0, 0, 0);
        }
    }

    /// define block ///
    else if(**p == '{') {
        unsigned int block;
        sVarTable new_table;
         
        (*p)++;
        skip_spaces_and_lf(p, sline);

        /// new table ///
        init_block_vtable(&new_table, lv_table);

        if(!parse_block(&block, p, sname, sline, err_num, current_namespace, klass, *type, method, &new_table)) {
            return FALSE;
        }

        /// entry new vtable to node block ///
        entry_block_vtable_to_node_block(&new_table, lv_table, block);

        *node = sNodeTree_create_block(type, block);
    }

    /// define variable or loops with type ///
    else {
        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "if") == 0) {
            if(!expression_node_if(node, p, sname, sline, err_num, current_namespace, klass, type, method, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "while") == 0) {
            if(!expression_node_while(node, p, sname, sline, err_num, current_namespace, klass, type, method, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "do") == 0) {
            if(!expression_node_do(node, p, sname, sline, err_num, current_namespace, klass, type, method, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "for") == 0) {
            if(!expression_node_for(node, p, sname, sline, err_num, current_namespace, klass, type, method, lv_table)) {
                return FALSE;
            }
        }
        //// define variable ///
        else {
            char* name;

            name = buf;

            if(lv_table == NULL) {
                parser_err_msg_format(sname, *sline, "there is not local variable table");
                (*err_num)++;

                *node = 0;
                return TRUE;
            }

            *node = sNodeTree_create_define_var(name, type, 0, 0, 0);

            if(!add_variable_to_table(lv_table, buf, type)) {
                parser_err_msg_format(sname, sline_top, "there is a same name variable(%s) or overflow local variable table", buf);

                (*err_num)++;
                *node = 0;
                return TRUE;
            }
        }
    }

    return TRUE;
}

static BOOL increment_and_decrement(enum eOperand op, unsigned int* node, unsigned int right, unsigned int middle, char* sname, int* sline, int* err_num, int sline_top)
{
    if(*node > 0) {
        switch(gNodes[*node].mNodeType) {
            case NODE_TYPE_VARIABLE_NAME:
            case NODE_TYPE_FIELD:
            case NODE_TYPE_CLASS_FIELD:
                break;

            default:
                parser_err_msg("require varible name for ++ or --", sname, sline_top);
                (*err_num)++;
                break;
        }

        *node = sNodeTree_create_operand(op, *node, 0, 0);
    }

    return TRUE;
}

// from left to right order
static BOOL postposition_operator(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(*node == 0) {
        return TRUE;
    }

    while(*p) {
        /// call method or access field ///
        if(**p == '.') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(isalpha(**p)) {
                char buf[128];

                if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                /// call methods ///
                if(**p == '(') {
                    unsigned int param_node;
                    unsigned int block;
                    sCLNodeType result_type;

                    param_node = 0;
                    if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
                        return FALSE;
                    }

                    if(**p == '(') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        memset(&result_type, 0, sizeof(result_type));

                        if(!parse_namespace_and_class_and_generics_type(&result_type, p, sname, sline, err_num, current_namespace, (klass ? klass->mClass:NULL))) {
                            return FALSE;
                        }

                        expect_next_character_with_one_forward(")", err_num, p, sname, sline);
                    }
                    else {
                        result_type = gVoidType;
                    }

                    /// method with block ///
                    if(**p == '{') {
                        sVarTable new_table;
                        int local_var_num_before;

                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        /// new table ///
                        init_block_vtable(&new_table, lv_table);

                        local_var_num_before = new_table.mVarNum;

                        if(!parse_method_block(&block, p, sname, sline, err_num, current_namespace, klass, result_type, FALSE, method, &new_table, sline_top)) {
                            return FALSE;
                        }

                        /// entry new vtable to node block ///
                        entry_method_block_vtable_to_node_block(&new_table, lv_table, block, local_var_num_before);
                    }
                    else {
                        block = 0;
                    }

                    *node = sNodeTree_create_method_call(buf, *node, param_node, 0, block);
                }
                /// access fields ///
                else {
                    *node = sNodeTree_create_fields(buf, *node, 0, 0);
                }
            }
            else {
                parser_err_msg("require method name or field name after .", sname, sline_top);
                (*err_num)++;

                *node = 0;
                break;
            }
        }
        /// indexing ///
        else if(**p == '[') {
            unsigned int param_node;
            
            param_node = 0;
            if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '[', ']', method, lv_table)) {
                return FALSE;
            }

            *node = sNodeTree_create_operand(kOpIndexing, *node, param_node, 0);
        }
        else if(**p == '+' && *(*p+1) == '+') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!increment_and_decrement(kOpPlusPlus2, node, 0, 0, sname, sline, err_num, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '-' && *(*p+1) == '-') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!increment_and_decrement(kOpMinusMinus2, node, 0, 0, sname, sline, err_num, sline_top)) {
                return FALSE;
            }
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL get_hex_number(char* buf, size_t buf_size, char* p2, unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, int sline_top)
{
    unsigned long value;

    while((**p >= '0' && **p <= '9') || (**p >= 'a' && **p <= 'f') || (**p >= 'A' && **p <= 'F')) {
        *p2++ = **p;
        (*p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;
    skip_spaces_and_lf(p, sline);

    value = strtoul(buf, NULL, 0);

    *node = sNodeTree_create_value((int)value, 0, 0, 0);

    return TRUE;
}

static BOOL get_oct_number(char* buf, size_t buf_size, char* p2, unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, int sline_top)
{
    unsigned long value;

    while(**p >= '0' && **p <= '7') {
        *p2++ = **p;
        (*p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;
    skip_spaces_and_lf(p, sline);

    value = strtoul(buf, NULL, 0);

    *node = sNodeTree_create_value((int)value, 0, 0, 0);

    return TRUE;
}

static BOOL get_number(char* buf, size_t buf_size, char* p2, unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, int sline_top)
{
    while(**p >= '0' && **p <= '9') {
        *p2++ = **p;
        (*p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;
    skip_spaces_and_lf(p, sline);

    if(**p == '.') {
        *p2++ = **p;
        (*p)++;
        skip_spaces_and_lf(p, sline);

        while(**p >= '0' && **p <= '9') {
            *p2++ = **p;
            (*p)++;

            if(p2 - buf >= buf_size) {
                parser_err_msg("overflow node of number",  sname, sline_top);
                return FALSE;
            }
        }
        *p2 = 0;
        skip_spaces_and_lf(p, sline);

        *node = sNodeTree_create_fvalue(atof(buf), 0, 0, 0);
    }
    else {
        *node = sNodeTree_create_value(atoi(buf), 0, 0, 0);
    }

    return TRUE;
}

BOOL reserved_words(BOOL* processed, char* buf, unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    *processed = TRUE;

    if(strcmp(buf, "new") == 0) {
        sCLNodeType type;

        memset(&type, 0, sizeof(type));

        if(!parse_namespace_and_class_and_generics_type(&type, p, sname, sline, err_num, current_namespace, klass ? klass->mClass:NULL)) {
            return FALSE;
        }

        if(**p == '(') {
            unsigned int param_node;
            unsigned int block;

            param_node = 0;
            if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
                return FALSE;
            }

            /// method with block ///
            if(**p == '{') {
                sVarTable new_table;
                int local_var_num_before;

                (*p)++;
                skip_spaces_and_lf(p, sline);

                /// new table ///
                init_block_vtable(&new_table, lv_table);

                local_var_num_before = new_table.mVarNum;

                if(!parse_method_block(&block, p, sname, sline, err_num, current_namespace, klass, gVoidType, FALSE, method, &new_table, sline_top)) {
                    return FALSE;
                }

                /// entry new vtable to node block ///
                entry_method_block_vtable_to_node_block(&new_table, lv_table, block, local_var_num_before);
            }
            else {
                block = 0;
            }

            if(type.mClass) {
                *node = sNodeTree_create_new_expression(&type, param_node, 0, 0, block);
            }
            else {
                *node = 0;
            }
        }
        else {
            parser_err_msg("require (", sname, sline_top);
            (*err_num)++;

            *node = 0;
        }
    }
    else if(strcmp(buf, "super") == 0) {
        unsigned int param_node;
        char** p2;
        char buf2[CL_VARIABLE_NAME_MAX];
        unsigned int block;
        sCLNodeType result_type;
        
        if(!expect_next_character("(", err_num, p, sname, sline)) {
            return FALSE;
        }

        /// call super ///
        param_node = 0;
        if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
            return FALSE;
        }

        if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            memset(&result_type, 0, sizeof(result_type));

            if(!parse_namespace_and_class_and_generics_type(&result_type, p, sname, sline, err_num, current_namespace, (klass ? klass->mClass:NULL))) {
                return FALSE;
            }

            expect_next_character_with_one_forward(")", err_num, p, sname, sline);
        }
        else {
            result_type = gVoidType;
        }

        /// method with block ///
        if(**p == '{') {
            BOOL static_method;
            sVarTable new_table;
            int local_var_num_before;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(method) {
                static_method = method->mFlags & CL_CLASS_METHOD;
            }
            else {
                parser_err_msg("there is not caller method. can't inherit", sname, sline_top);
                (*err_num)++;
            }

            /// new table ///
            init_block_vtable(&new_table, lv_table);

            local_var_num_before = new_table.mVarNum;

            if(!parse_method_block(&block, p, sname, sline, err_num, current_namespace, klass, result_type, static_method, method, &new_table, sline_top)) {
                return FALSE;
            }

            /// entry new vtable to node block ///
            entry_method_block_vtable_to_node_block(&new_table, lv_table, block, local_var_num_before);
        }
        else {
            block = 0;
        }

        *node = sNodeTree_create_super(param_node, 0, 0, block);
    }
    else if(strcmp(buf, "inherit") == 0) {
        unsigned int param_node;
        unsigned int block;
        sCLNodeType result_type;
        
        if(!expect_next_character("(", err_num, p, sname, sline)) {
            return FALSE;
        }

        /// call inherit ///
        param_node = 0;
        if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
            return FALSE;
        }

        if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            memset(&result_type, 0, sizeof(result_type));

            if(!parse_namespace_and_class_and_generics_type(&result_type, p, sname, sline, err_num, current_namespace, (klass ? klass->mClass:NULL))) {
                return FALSE;
            }

            expect_next_character_with_one_forward(")", err_num, p, sname, sline);
        }
        else {
            result_type = gVoidType;
        }

        /// method with block ///
        if(**p == '{') {
            BOOL static_method;
            sVarTable new_table;
            int local_var_num_before;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(method) {
                static_method = method->mFlags & CL_CLASS_METHOD;
            }
            else {
                parser_err_msg("there is not caller method. can't inherit", sname, sline_top);
                (*err_num)++;
            }

            /// new table ///
            init_block_vtable(&new_table, lv_table);

            local_var_num_before = new_table.mVarNum;

            if(!parse_method_block(&block, p, sname, sline, err_num, current_namespace, klass, result_type, static_method, method, &new_table, sline_top)) {
                return FALSE;
            }

            /// entry new vtable to node block ///
            entry_method_block_vtable_to_node_block(&new_table, lv_table, block, local_var_num_before);
        }
        else {
            block = 0;
        }

        *node = sNodeTree_create_inherit(param_node, 0, 0, block);
    }
    else if(strcmp(buf, "return") == 0) {
        unsigned int rv_node;

        if(**p == ';' || **p == '}') {
            rv_node = 0;
        }
        else if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!node_expression(&rv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(!expect_next_character(")", err_num, p, sname, sline)) {
                return FALSE;
            }
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }
        else {
            if(!node_expression(&rv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }

        *node = sNodeTree_create_return(&gNodes[rv_node].mType, rv_node, 0, 0);
    }
    else if(strcmp(buf, "null") == 0) {
        *node = sNodeTree_create_null();
    }
    else if(strcmp(buf, "true") == 0) {
        *node = sNodeTree_create_true();
    }
    else if(strcmp(buf, "false") == 0) {
        *node = sNodeTree_create_false();
    }
    else if(strcmp(buf, "if") == 0) {
        if(!expression_node_if(node, p, sname, sline, err_num, current_namespace, klass, &gVoidType, method, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "continue") == 0) {
        *node = sNodeTree_create_continue();
    }
    else if(strcmp(buf, "break") == 0) {
        unsigned int bv_node;

        if(**p == ';' || **p == '}') {
            bv_node = 0;
        }
        else if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!node_expression(&bv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(!expect_next_character(")", err_num, p, sname, sline)) {
                return FALSE;
            }
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(bv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }
        else {
            if(!node_expression(&bv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(bv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }

        *node = sNodeTree_create_break(&gNodes[bv_node].mType, bv_node, 0, 0);
    }
    else if(strcmp(buf, "revert") == 0) {
        unsigned int rv_node;

        if(**p == ';' || **p == '}') {
            rv_node = 0;
        }
        else if(**p == '(') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!node_expression(&rv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(!expect_next_character(")", err_num, p, sname, sline)) {
                return FALSE;
            }
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }
        else {
            if(!node_expression(&rv_node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as ( operand", sname, sline_top);
                (*err_num)++;
            }
        }

        *node = sNodeTree_create_revert(&gNodes[rv_node].mType, rv_node, 0, 0);
    }
    else if(strcmp(buf, "while") == 0) {
        if(!expression_node_while(node, p, sname, sline, err_num, current_namespace, klass, &gVoidType, method, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "do") == 0) {
        if(!expression_node_do(node, p, sname, sline, err_num, current_namespace, klass, &gVoidType, method, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "for") == 0) {
        if(!expression_node_for(node, p, sname, sline, err_num, current_namespace, klass, &gVoidType, method, lv_table)) {
            return FALSE;
        }
    }
    else {
        *processed = FALSE;
    }

    return TRUE;
}

static BOOL expression_node(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if((**p == '-' && *(*p+1) != '=' && *(*p+1) != '-') || (**p == '+' && *(*p+1) != '=' && *(*p+1) != '+')) {
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
            if(!get_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '0' && *(*p+1) == 'x') {
            *p2++ = **p;
            (*p)++;
            *p2++ = **p;
            (*p)++;

            if(!get_hex_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '0') {
            *p2++ = **p;
            (*p)++;

            if(!get_oct_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
                return FALSE;
            }
        }
        else { 
            parser_err_msg("require number after + or -", sname, sline_top);
            *node = 0;
            (*err_num)++;
        }
    }
    else if(**p == '0' && *(*p+1) == 'x') {
        char buf[128];
        char* p2;

        p2 = buf;

        *p2++ = **p;
        (*p)++;
        *p2++ = **p;
        (*p)++;

        if(!get_hex_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
            return FALSE;
        }
    }
    else if(**p == '0' && *(*p+1) >= '0' && *(*p+1) <= '9') {
        char buf[128];
        char* p2;

        p2 = buf;

        *p2++ = **p;
        (*p)++;

        if(!get_oct_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
            return FALSE;
        }
    }
    else if(**p >= '0' && **p <= '9') {
        char buf[128];
        char* p2;

        p2 = buf;

        if(!get_number(buf, 128, p2, node, p, sname, sline, err_num, current_namespace, klass, sline_top)) {
            return FALSE;
        }
    }
    else if(**p == '\'') {
        char c;

        (*p)++;

        if(**p == '\\') {
            (*p)++;

            switch(**p) {
                case 'n':
                    c = '\n';
                    (*p)++;
                    break;

                case 't':
                    c = '\t';
                    (*p)++;
                    break;

                case 'r':
                    c = '\r';
                    (*p)++;
                    break;

                case 'a':
                    c = '\a';
                    (*p)++;
                    break;

                case '\\':
                    c = '\\';
                    (*p)++;
                    break;

                default:
                    c = **p;
                    (*p)++;
                    break;
            }
        }
        else {
            c = **p;
            (*p)++;
        }

        if(**p != '\'') {
            parser_err_msg("close \' to make character value", sname, sline_top);
            (*err_num)++;
        }
        else {
            (*p)++;

            skip_spaces_and_lf(p, sline);

            *node = sNodeTree_create_character_value(c);
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
                parser_err_msg("close \" to make string value", sname, sline_top);
                return FALSE;
            }
            else {
                if(**p =='\n') (*sline)++;

                sBuf_append_char(&value, **p);
                (*p)++;
            }
        }

        skip_spaces_and_lf(p, sline);

        *node = sNodeTree_create_string_value(MANAGED value.mBuf, 0, 0, 0);
    }
    else if(**p == '{') {
        int sline2;
        unsigned int new_node;
        unsigned int elements_num;

        (*p)++;
        skip_spaces_and_lf(p, sline);

        sline2 = *sline;

        new_node = 0;
        elements_num = 0;

        if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
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

                    if(!node_expression_without_comma(&new_node2, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(p, sline);

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
                        skip_spaces_and_lf(p, sline);
                    }
                    else if(**p == '}') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
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

        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        memset(&type, 0, sizeof(type));
        type.mClass = cl_get_class_with_namespace("", buf, klass == NULL);   // when eval, vm runtime, and interpreter, klass == NULL

        if(type.mClass == NULL) {
            parser_err_msg_format(sname, sline_top, "there is no definition of this namespace and class name(::%s)", buf);
            (*err_num)++;

            *node = 0;
        }
        /// class method , class field, or variable definition ///
        else {
            if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass ? klass->mClass:NULL))
            {
                return FALSE;
            }

            if(!after_class_name(&type, node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
    }
    /// words ///
    else if(isalpha(**p)) {
        BOOL processed;
        char buf[128];

        if(!parse_word(buf, 128, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// reserved words ///
        if(!reserved_words(&processed, buf, node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
            return FALSE;
        }

        if(processed) {  // reserved words is processed
        }
        /// local variable ///
        else if(lv_table && get_variable_from_table(lv_table, buf)) {
            /// calling block ///
            if(**p == '(') {
                unsigned int param_node;

                param_node = 0;
                if(!get_params(p, sname, sline, err_num, &param_node, current_namespace, klass, '(', ')', method, lv_table)) {
                    return FALSE;
                }

                *node = sNodeTree_create_call_block(buf, param_node, 0, 0);
            }
            else {
                *node = sNodeTree_create_var(buf, 0, 0, 0);
            }
        }
        /// class name ///
        else {
            /// class name with namespace ///
            if(**p == ':' && *(*p+1) == ':') {
                char buf2[128];
                sCLNodeType type;

                (*p)+=2;

                if(!parse_word(buf2, 128, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                memset(&type, 0, sizeof(type));
                type.mClass = cl_get_class_with_argument_namespace_only(buf, buf2, klass == NULL);   // when eval, vm runtime, and interpreter, klass is NULL

                if(type.mClass == NULL) {
                    parser_err_msg_format(sname, sline_top, "there is no definition of this namespace and class name(%s::%s)", buf, buf2);
                    (*err_num)++;

                    *node = 0;
                }
                /// class method , class field, or variable definition ///
                else {
                    if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass ? klass->mClass:NULL))
                    {
                        return FALSE;
                    }

                    if(!after_class_name(&type, node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                        return FALSE;
                    }
                }
            }
            /// class name ///
            else {
                int generics_type_num;

                /// is this generic type ? ///
                generics_type_num = get_generics_type_num((klass ? klass->mClass:NULL), buf);

                if(generics_type_num != -1) {
                    sCLNodeType type;

                    memset(&type, 0, sizeof(type));

                    type.mClass = gAnonymousType[generics_type_num].mClass;

                    if(!after_class_name(&type, node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                        return FALSE;
                    }
                }
                else {
                    sCLNodeType type;

                    memset(&type, 0, sizeof(type));

                    type.mClass = cl_get_class_with_namespace(current_namespace, buf, klass == NULL);   // when eval, vm runtime, and interpreter, klass is NULL

                    if(type.mClass == NULL) {
                        parser_err_msg_format(sname, sline_top, "there is no definition of this class(%s::%s)", current_namespace, buf);
                        (*err_num)++;

                        *node = 0;
                        return TRUE;
                    }

                    /// class method , class field, or variable definition ///
                    if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass ? klass->mClass:NULL))
                    {
                        return FALSE;
                    }

                    if(!after_class_name(&type, node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                        return FALSE;
                    }
                }
            }
        }
    }
    else if(**p == '(') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!node_expression(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(!expect_next_character(")", err_num, p, sname, sline)) {
            return FALSE;
        }
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(*node == 0) {
            parser_err_msg("require expression as ( operand", sname, sline_top);
            (*err_num)++;
        }
    }
    else if(**p == '\n') {
        skip_spaces_and_lf(p, sline);

        return expression_node(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top);
    }
    else if(**p == ';' || **p == '}' || **p == ')' || **p == 0) {
        *node = 0;
        return TRUE;
    }
    else {
        parser_err_msg_format(sname, *sline, "invalid character (character code %d) (%c)", **p, **p);
        *node = 0;
        if(**p == '\n') (*sline)++;
        (*p)++;
        (*err_num)++;
    }

    /// postposition operator ///
    if(!postposition_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top))
    {
        return FALSE;
    }

    return TRUE;
}

// from right to left order 
static BOOL expression_monadic_operator(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    while(**p) {
        if(**p == '+' && *(*p+1) == '+') {
            (*p) +=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", sname, sline_top);
                (*err_num)++;
            }

            if(!increment_and_decrement(kOpMinusMinus, node, 0, 0, sname, sline, err_num, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '-' && *(*p+1) == '-') {
            (*p) +=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", sname, sline_top);
                (*err_num)++;
            }

            if(!increment_and_decrement(kOpPlusPlus, node, 0, 0, sname, sline, err_num, sline_top)) {
                return FALSE;
            }
            break;
        }
        else if(**p == '~') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComplement, *node, 0, 0);
            break;
        }
        else if(**p == '!') {
            (*p) ++;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpLogicalDenial, *node, 0, 0);
            break;
        }
        else {
            if(!expression_node(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_mult_div(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_monadic_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '*' && *(*p+1) != '=') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);
            if(!expression_monadic_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpMult, *node, right, 0);
        }
        else if(**p == '/' && *(*p+1) != '=') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpDiv, *node, right, 0);
        }
        else if(**p == '%' && *(*p+1) != '=') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_monadic_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
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

// from left to right order
static BOOL expression_add_sub(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_mult_div(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) != '=' && *(*p+1) != '+') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            right = 0;

            if(!expression_mult_div(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAdd, *node, right, 0);
        }
        else if(**p == '-' && *(*p+1) != '=' && *(*p+1) != '-') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_mult_div(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
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

// from left to right order
static BOOL expression_shift(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_add_sub(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '<' && *(*p+1) == '<' && *(*p+2) != '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_add_sub(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpLeftShift, *node, right, 0);
        }
        else if(**p == '>' && *(*p+1) == '>' && *(*p+2) != '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_add_sub(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpRightShift, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comparison_operator(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_shift(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '>' && *(*p+1) == '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_shift(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonGreaterEqual, *node, right, 0);
        }
        else if(**p == '<' && *(*p+1) == '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_shift(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonLesserEqual, *node, right, 0);
        }
        else if(**p == '>' && *(*p+1) != '>') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_shift(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonGreater, *node, right, 0);
        }
        else if(**p == '<' && *(*p+1) != '<') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_shift(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonLesser, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comparison_equal_operator(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_comparison_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '=' && *(*p+1) == '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_comparison_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonEqual, *node, right, 0);
        }
        else if(**p == '!' && *(*p+1) == '=') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_comparison_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonNotEqual, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_and(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_comparison_equal_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '&' && *(*p+1) != '&' && *(*p+1) != '=') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_comparison_equal_operator(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAnd, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_xor(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_and(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '^' && *(*p+1) != '=') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_and(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpXor, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_or(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_xor(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '|' && *(*p+1) != '=' && *(*p+1) != '|') {
            unsigned int right;

            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_xor(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpOr, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_and_and(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_or(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '&' && *(*p+1) == '&') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_or(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAndAnd, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_or_or(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_and_and(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '|' && *(*p+1) == '|') {
            unsigned int right;

            right = 0;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!expression_and_and(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpOrOr, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_conditional_operator(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_or_or(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '?') {
            unsigned int middle;
            unsigned int right;

            middle = 0;
            right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!node_expression(&middle, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }

            expect_next_character_with_one_forward(":", err_num, p, sname, sline);

            if(!node_expression(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(middle == 0) {
                parser_err_msg("require middle value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpConditional, *node, right, middle);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_substitution(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top);

static BOOL substitution_node(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, enum eNodeSubstitutionType substitution_type, sCLMethod* method, sVarTable* lv_table, int sline_top) 
{
    unsigned int right;

    right = 0;

    if(!expression_substitution(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }

    if(*node == 0) {
        parser_err_msg("require left value", sname, sline_top);
        (*err_num)++;
    }
    if(right == 0) {
        parser_err_msg("require right value", sname, sline_top);
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
                parser_err_msg("require varible name on left node of equal", sname, sline_top);
                (*err_num)++;
                break;
        }

        gNodes[*node].mRight = right;
        gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
    }

    return TRUE;
}

// from right to left order
static BOOL expression_substitution(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_conditional_operator(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == '+' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSPlus, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '-' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSMinus, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '*' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSMult, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '/' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSDiv, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '%' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSMod, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '<' && *(*p+1) == '<' && *(*p+2) == '=') {
            (*p)+=3;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSLShift, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '>' && *(*p+1) == '>' && *(*p+2) == '=') {
            (*p)+=3;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSRShift, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '&' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSAnd, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '^' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSXor, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '|' && *(*p+1) == '=') {
            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSOr, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else if(**p == '=') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!substitution_node(node, p, sname, sline, err_num, current_namespace, klass, kNSNone, method, lv_table, sline_top)) {
                return FALSE;
            }
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comma(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, int sline_top)
{
    if(!expression_substitution(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**p) {
        if(**p == ',') {
            unsigned int right = 0;

            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(!expression_substitution(&right, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", sname, sline_top);
                (*err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", sname, sline_top);
                (*err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComma, *node, right, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table)
{
    int sline_top;

    skip_spaces_and_lf(p, sline);

    sline_top = *sline;

    *node = 0;

    return expression_comma(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top);
}

BOOL node_expression_without_comma(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table)
{
    int sline_top;

    skip_spaces_and_lf(p, sline);

    sline_top = *sline;

    *node = 0;

    return expression_substitution(node, p, sname, sline, err_num, current_namespace, klass, method, lv_table, sline_top);
}
