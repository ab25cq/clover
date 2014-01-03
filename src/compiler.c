#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

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

static BOOL change_namespace(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    if(!expect_next_character(";", err_num, p, sname, sline)) {
        return FALSE;
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    xstrncpy(current_namespace, buf, CL_NAMESPACE_NAME_MAX);

    return TRUE;
}

static BOOL second_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace);

static BOOL do_reffer_file(char* sname, char* current_namespace)
{
    int f = open(sname, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "can't open %s\n", sname);
        return FALSE;
    }

    sBuf source;
    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    /// get methods and fields ///
    char* p = source.mBuf;

    int sline = 1;
    int err_num = 0;
    if(!second_parse(&p, sname, &sline, &err_num, current_namespace)) {
        return FALSE;
    }

    if(err_num > 0) {
        return FALSE;
    }

    return TRUE;
}

static BOOL reffer_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL skip)
{
    if(**p != '"') {
        parser_err_msg("require \" after reffer", sname, *sline);
        return FALSE;
    }
    else {
        (*p)++;
    }

    char file_name[PATH_MAX];
    char* p2 = file_name;
    while(1) {
        if(**p == '\0') {
            parser_err_msg("forwarded at the source end in getting file name. require \"", sname, *sline);
            return FALSE;
        }
        else if(**p == '"') {
            (*p)++;
            break;
        }
        else {
            if(**p == '\n') (*sline)++;
            *p2++ = **p;
            (*p)++;

            if(p2 - file_name >= PATH_MAX-1) {
                parser_err_msg("too long file name to reffer", sname, *sline);
                return FALSE;
            }
        }
    }
    *p2 = 0;
    
    skip_spaces_and_lf(p, sline);

    if(!expect_next_character(";", err_num, p, sname, sline)) {
        return FALSE;
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!skip) {
        if(!do_reffer_file(file_name, current_namespace)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL do_load_file(char* fname)
{
    if(!load_object_file(fname)) {
        fprintf(stderr, "object file(%s) is not found\n", fname);
        return FALSE;
    }

    return TRUE;
}

static BOOL load_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL skip)
{
    if(**p != '"') {
        parser_err_msg("require \" after load", sname, *sline);
        return FALSE;
    }
    else {
        (*p)++;
    }

    char file_name[PATH_MAX];
    char* p2 = file_name;
    while(1) {
        if(**p == '\0') {
            parser_err_msg("forwarded at the source end in getting file name. require \"", sname, *sline);
            return FALSE;
        }
        else if(**p == '"') {
            (*p)++;
            break;
        }
        else {
            if(**p == '\n') (*sline)++;
            *p2++ = **p;
            (*p)++;

            if(p2 - file_name >= PATH_MAX-1) {
                parser_err_msg("too long file name to load", sname, *sline);
                return FALSE;
            }
        }
    }
    *p2 = 0;
    
    skip_spaces_and_lf(p, sline);

    if(!expect_next_character(";", err_num, p, sname, sline)) {
        return FALSE;
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(!skip) {
        if(!do_load_file(file_name)) {
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse methods and fields
//////////////////////////////////////////////////
BOOL parse_constructor(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL compile_method_, BOOL static_, BOOL private_, BOOL native_, char* name)
{
    /// method ///
    sVarTable lv_table;
    memset(&lv_table, 0, sizeof(lv_table));

    if(!add_variable_to_table(&lv_table, "self", klass)) {
        parser_err_msg("local variable table overflow", sname, *sline);
        return FALSE;
    }

    sCLClass* class_params[CL_METHOD_PARAM_MAX];
    uint num_params = 0;

    /// params ///
    if(**p == ')') {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            /// type ///
            sCLClass* param_type;
            if(!parse_namespace_and_class(&param_type, p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }

            /// name ///
            char param_name[CL_METHOD_NAME_MAX];
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(param_type) {
                class_params[num_params] = param_type;
                num_params++;

                if(!add_variable_to_table(&lv_table, param_name, param_type)) {
                    parser_err_msg("local variable table overflow", sname, *sline);
                    return FALSE;
                }
            }
            
            if(!expect_next_character("),", err_num, p, sname, sline)) {
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
        }
    }

    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(compile_method_) {
        uint method_index = get_method_index_with_params(klass, name, class_params, num_params);

        ASSERT(method_index != -1); // must be found

        sCLMethod* method = klass->mMethods + method_index;

        if(!compile_method(method, klass, p, sname, sline, err_num, &lv_table, TRUE, current_namespace)) {
            return FALSE;
        }

        method->mNumLocals = lv_table.mVarNum;
    }
    else {
        if(!skip_block(p, sname, sline)) {
            return FALSE;
        }

        /// add method to class definition ///
        if(*err_num == 0) {
            if(!add_method(klass, static_, private_, native_, name, klass, class_params, num_params, TRUE)) {
                parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                return FALSE;
            }
        }
    }

    skip_spaces_and_lf(p, sline);

    return TRUE;
}

BOOL parse_method(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL compile_method_, BOOL static_, BOOL private_, BOOL native_, sCLClass* type_, char* name)
{
    /// definitions ///
    sVarTable lv_table;
    memset(&lv_table, 0, sizeof(lv_table));

    if(!static_) {
        if(!add_variable_to_table(&lv_table, "self", klass)) {
            parser_err_msg("local variable table overflow", sname, *sline);
            return FALSE;
        }
    }

    sCLClass* class_params[CL_METHOD_PARAM_MAX];
    uint num_params = 0;

    /// params ///
    if(**p == ')') {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            /// type ///
            sCLClass* param_type;
            if(!parse_namespace_and_class(&param_type, p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }

            /// name ///
            char param_name[CL_METHOD_NAME_MAX];
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(param_type) {
                class_params[num_params] = param_type;
                num_params++;

                if(!add_variable_to_table(&lv_table, param_name, param_type)) {
                    parser_err_msg("local variable table overflow", sname, *sline);
                    return FALSE;
                }
            }
            
            if(!expect_next_character("),", err_num, p, sname, sline)) {
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
        }
    }

    if(native_) {
        if(!expect_next_character(";", err_num, p, sname, sline)) {
            return FALSE;
        }

        (*p)++;
        skip_spaces_and_lf(p, sline);

        /// add method to class definition ///
        if(!compile_method_ && *err_num == 0) {
            if(!add_method(klass, static_, private_, native_, name, type_, class_params, num_params, FALSE)) {
                parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                return FALSE;
            }
        }
    }
    else {
        if(!expect_next_character("{", err_num, p, sname, sline)) {
            return FALSE;
        }
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(compile_method_) {
            uint method_index = get_method_index_with_params(klass, name, class_params, num_params);

            ASSERT(method_index != -1); // be sure to be found

            sCLMethod* method = klass->mMethods + method_index;

            if(!compile_method(method, klass, p, sname, sline, err_num, &lv_table, FALSE, current_namespace)) {
                return FALSE;
            }

            method->mNumLocals = lv_table.mVarNum;
        }
        else {
            if(!skip_block(p, sname, sline)) {
                return FALSE;
            }

            /// add method to class definition ///
            if(*err_num == 0) {
                if(!add_method(klass, static_, private_, native_, name, type_, class_params, num_params, FALSE)) {
                    parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                    return FALSE;
                }
            }
        }

        skip_spaces_and_lf(p, sline);
    }

    return TRUE;
}

static BOOL methods_and_fields(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL compile_method_)
{
    char buf[WORDSIZ];

    while(**p != '}') {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// prefix ///
        BOOL static_ = FALSE;
        BOOL private_ = FALSE;
        BOOL native_ = FALSE;

        while(**p) {
            if(strcmp(buf, "native") == 0) {
                native_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "static") == 0) {
                static_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

        /// constructor ///
        if(strcmp(buf, CLASS_NAME(klass)) == 0) {
            if(static_ || native_ || private_) {
                parser_err_msg("don't append method type(\"static\" or \"native\" or \"private\")  to constructor", sname, *sline);
                (*err_num)++;
            }

            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            (*p)++;
            skip_spaces_and_lf(p, sline);

            char name[CL_METHOD_NAME_MAX];
            xstrncpy(name, buf, CL_METHOD_NAME_MAX);

            if(!parse_constructor(p, klass, sname, sline, err_num, current_namespace, compile_method_, static_, private_, native_, name)) {
                return FALSE;
            }
        }
        /// non constructor ///
        else {
            sCLClass* type_;

            /// a second word ///
            if(**p == ':' && *(*p+1) == ':') {
                (*p)+=2;

                char buf2[128];

                if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                type_ = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(type_ == NULL) {
                    char buf3[128];
                    snprintf(buf3, 128, "invalid class name(%s::%s)", buf, buf2);
                    parser_err_msg(buf3, sname, *sline);
                    (*err_num)++;
                }
            }
            else {
                type_ = cl_get_class_with_namespace(current_namespace, buf);

                if(type_ == NULL) {
                    char buf2[128];
                    snprintf(buf2, 128, "invalid class name(%s::%s)", current_namespace, buf);
                    parser_err_msg(buf2, sname, *sline);
                    (*err_num)++;
                }
            }

            /// name ///
            char name[CL_METHOD_NAME_MAX];
            if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(!expect_next_character("(;", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// method ///
            if(**p == '(') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!parse_method(p, klass, sname, sline, err_num, current_namespace, compile_method_, static_, private_, native_, type_, name)) {
                    return FALSE;
                }
            }
            /// field ///
            else if(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!compile_method_ && *err_num == 0) {
                    if(!add_field(klass, static_, private_, name, type_)) {
                        parser_err_msg("overflow number fields", sname, *sline);
                        return FALSE;
                    }
                }
            }
        }
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    return TRUE;
}

//////////////////////////////////////////////////
// parse class
//////////////////////////////////////////////////
static BOOL extends_and_implements(sCLClass* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL skip)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    sCLClass* super_class = NULL;

    while(**p == 'e' || **p == 'i') {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "extends") == 0) {
            if(super_class == NULL) {
                /// get class ///
                if(!parse_namespace_and_class(&super_class, p, sname, sline, err_num, current_namespace)) {
                    return FALSE;
                }

                if(!skip) {
                    if(!add_super_class(klass, super_class)) {
                        parser_err_msg("Overflow number of super class.", sname, *sline);
                        return FALSE;
                    }
                }
                else {
                    parser_err_msg("You can't inherit a super class on additinal definition of a class", sname, *sline);
                    (*err_num)++;
                }
            }
            else {
                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                parser_err_msg("A super class has existed already. Clover doesn't support multi-inheritance", sname, *sline);
                (*err_num)++;
            }
        }
        else if(strcmp(buf, "implements") == 0) {
        }
        else {
            char buf2[128];
            snprintf(buf2, 128, "clover expected \"extends\" or \"implements\" as next word, but this is \"%s\"\n", buf);
            parser_err_msg(buf2, sname, *sline);
            (*err_num)++;
        }
    }

    return TRUE;
}

BOOL alloc_class_and_get_super_class(sCLClass* klass, char* class_name, char** p, char* sname, int* sline, int* err_num, char* current_namespace) 
{
    /// extends or implements ///
    if(!extends_and_implements(klass, p, sname, sline, err_num, current_namespace, FALSE)) {
        return FALSE;
    }

    if(**p == '{') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!skip_block(p, sname, sline)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);
    }
    else {
        char buf2[WORDSIZ];
        snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
        parser_err_msg(buf2, sname, *sline);

        return FALSE;
    }

    return TRUE;
}

static BOOL get_definition_from_class(sCLClass* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements(klass, p, sname, sline, err_num, current_namespace, TRUE)) {
        return FALSE;
    }

    if(**p == '{') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!methods_and_fields(p, klass, sname, sline, err_num, current_namespace, FALSE)) {
            return FALSE;
        }
    }
    else {
        char buf2[WORDSIZ];
        snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
        parser_err_msg(buf2, sname, *sline);

        return FALSE;
    }

    return TRUE;
}

static BOOL compile_class(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements(klass, p, sname, sline, err_num, current_namespace, TRUE)) {
        return FALSE;
    }

    if(**p == '{') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!methods_and_fields(p, klass, sname, sline, err_num, current_namespace, TRUE)) {
            return FALSE;
        }
    }
    else {
        char buf2[WORDSIZ];
        snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
        parser_err_msg(buf2, sname, *sline);

        return FALSE;
    }

    return TRUE;
}

enum eParseType { kPCGetDefinition, kPCCompile, kPCAlloc };

static BOOL parse_class(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eParseType parse_type)
{
    char class_name[WORDSIZ];

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, p, sname, sline, err_num)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    sCLClass* klass = cl_get_class_with_argument_namespace_only(current_namespace, class_name);

    switch((int)parse_type) {
        case kPCAlloc: {
            if(klass == NULL) {
                klass = alloc_class(current_namespace, class_name);
            }

            if(!alloc_class_and_get_super_class(klass, class_name, p , sname, sline, err_num, current_namespace)) {
                return FALSE;
            }
            break;

        case kPCGetDefinition:
            ASSERT(klass != NULL);

            if(!get_definition_from_class(klass, p , sname, sline, err_num, current_namespace)) {
                return FALSE;
            }
            break;

        case kPCCompile:
            ASSERT(klass != NULL);

            if(!compile_class(p, klass, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }

            klass->mFlags |= CLASS_FLAGS_MODIFIED;
            break;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// 1st parse(load and reffer file)
//////////////////////////////////////////////////
static BOOL first_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "reffer") == 0) {  // do reffer file
            if(!reffer_file(p, sname, sline, err_num, current_namespace, FALSE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) { // do change namespace
            if(!change_namespace(p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "load") == 0) { // do load file
            if(!load_file(p, sname, sline, err_num, current_namespace, FALSE)) {
                return TRUE;
            }
        }
        else if(strcmp(buf, "class") == 0) { // skip class definition
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCAlloc)) {
                return TRUE;
            }
        }
        else {
            char buf2[WORDSIZ];
            snprintf(buf2, WORDSIZ, "syntax error(%s). require \"class\" or \"reffer\" or \"load\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// 2nd parse(get definitions)
//////////////////////////////////////////////////
static BOOL second_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "reffer") == 0) {  // skip reffer
            if(!reffer_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) { // do change namespace
            if(!change_namespace(p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "load") == 0) { // skip load
            if(!load_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return TRUE;
            }
        }
        else if(strcmp(buf, "class") == 0) { // get definitions
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCGetDefinition)) {
                return TRUE;
            }
        }
        else {
            char buf2[WORDSIZ];
            snprintf(buf2, WORDSIZ, "syntax error(%s). require \"class\" or \"reffer\" or \"load\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// 3rd parse(compile class)
//////////////////////////////////////////////////
static BOOL third_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }

        skip_spaces_and_lf(p, sline);

        /// skip reffer ///
        if(strcmp(buf, "reffer") == 0) { // skip reffer
            if(!reffer_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) { // do change namespace
            if(!change_namespace(p, sname, sline, err_num, current_namespace)) {
                return FALSE;
            }
        }
        /// skip load ///
        else if(strcmp(buf, "load") == 0) { // skip load
            if(!load_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return TRUE;
            }
        }
        /// compile class ///
        else if(strcmp(buf, "class") == 0) {
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCCompile)) {
                return TRUE;
            }
        }
        else {
            char buf2[WORDSIZ];
            snprintf(buf2, WORDSIZ, "syntax error(%s). require \"class\" or \"reffer\" or \"load\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// compile
//////////////////////////////////////////////////
static BOOL delete_comment(sBuf* source, sBuf* source2)
{
    char* p = source->mBuf;

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
            p+=2;
            int nest = 0;
            while(1) {
                if(*p == 0) {
                    fprintf(stderr, "there is not a comment end until source end\n");
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

static BOOL compile(char* sname)
{
    int f = open(sname, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "can't open %s\n", sname);
        return FALSE;
    }

    sBuf source;
    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    /// delete comment ///
    sBuf source2;
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    /// 1st parse(load and reffer file. And alloc classes) ///
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    *current_namespace = 0;

    char* p = source2.mBuf;

    int sline = 1;
    int err_num = 0;
    if(!first_parse(&p, sname, &sline, &err_num, current_namespace)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    if(err_num > 0) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    /// 2nd parse(get methods and fields) ///
    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!second_parse(&p, sname, &sline, &err_num, current_namespace)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    if(err_num > 0) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    /// 3rd parse(do compile code) ///
    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    if(!third_parse(&p, sname, &sline, &err_num, current_namespace)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    if(err_num > 0) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    /// save it ///
    char ofile_name[PATH_MAX];
    snprintf(ofile_name, PATH_MAX, "%so", sname);

    if(!save_object_file(ofile_name)) {
        fprintf(stderr, "failed to write\n");
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char** argv)
{
    int i;
    BOOL load_foudamental_classes = TRUE;
    int option_num = -1;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--no-load-foundamental-classes") == 0) {
            load_foudamental_classes = FALSE;
            option_num = i;
        }
    }
    cl_init(1024, 1024, 1024, 512, load_foudamental_classes);

    if(argc >= 2) {
        int i;
        for(i=1; i<argc; i++) {
            if(option_num != i) {
                if(!compile(argv[i])) {
                    exit(1);
                }
            }
        }
    }

    cl_final();

    exit(0);
}

