#include "clover.h"
#include "common.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <libgen.h>
#include <time.h>

int gParsePhaseNum = 1;

static BOOL skip_block(char** p, char* sname, int* sline)
{
    int nest;

    nest = 1;

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
            parser_err_msg_format(sname, *sline, "It arrived at the end of source before block closing");
            return FALSE;
        }
        else {
            (*p)++;
        }
    }

    return TRUE;
}

static BOOL parse(sParserInfo* info, int parse_phase_num);

static BOOL do_include_file(char* sname, char* current_namespace, int parse_phase_num)
{
    int f;
    sBuf source;
    char buf2[WORDSIZ];
    sBuf source2;
    char* p;
    int sline;
    int err_num;
    sParserInfo info;

    memset(&info, 0, sizeof(info));

    f = open(sname, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", sname);
        return FALSE;
    }

    sBuf_init(&source);

    while(1) {
        int size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    close(f);

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        return FALSE;
    }

    /// 1st parse(alloc classes) ///
    p = source2.mBuf;

    sline = 1;
    err_num = 0;

    switch(parse_phase_num) {
    case PARSE_PHASE_ALLOC_CLASSES:
    case PARSE_PHASE_ADD_SUPER_CLASSES:
    case PARSE_PHASE_CALCULATE_SUPER_CLASSES:
    case PARSE_PHASE_ADD_GENERICS_TYPES:
    case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
    case PARSE_PHASE_COMPILE_PARAM_INITIALIZER:
    case PARSE_PHASE_DO_COMPILE_CODE:
        info.p = &p;
        info.sname = sname;
        info.sline = &sline;
        info.err_num = &err_num;
        info.current_namespace = current_namespace;
        if(!parse(&info, parse_phase_num)) {
            MFREE(source.mBuf);
            MFREE(source2.mBuf);
            return FALSE;
        }

        if(err_num > 0) {
            MFREE(source.mBuf);
            MFREE(source2.mBuf);
            return FALSE;
        }
        break;

    default:
        compile_error("unexpected err on do_include_file\n");
        exit(1);
    }


    MFREE(source.mBuf);
    MFREE(source2.mBuf);

    return TRUE;
}

static BOOL include_file(sParserInfo* info, int parse_phase_num)
{
    char file_name[PATH_MAX];
    char* p2;

    if(**info->p != '"') {
        parser_err_msg("require \" after include", info->sname, *info->sline);
        return FALSE;
    }
    else {
        (*info->p)++;
    }

    p2 = file_name;
    while(1) {
        if(**info->p == '\0') {
            parser_err_msg("forwarded at the source end in getting file name. require \"", info->sname, *info->sline);
            return FALSE;
        }
        else if(**info->p == '"') {
            (*info->p)++;
            break;
        }
        else {
            if(**info->p == '\n') (*info->sline)++;
            *p2++ = **info->p;
            (*info->p)++;

            if(p2 - file_name >= PATH_MAX-1) {
                parser_err_msg("too long file name to include", info->sname, *info->sline);
                return FALSE;
            }
        }
    }
    *p2 = 0;
    
    skip_spaces_and_lf(info->p, info->sline);

    if(!do_include_file(file_name, info->current_namespace, parse_phase_num)) {
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse methods and fields
//////////////////////////////////////////////////
static void check_the_existance_of_this_method_before(sCLNodeType* klass, char* sname, int* sline, int* err_num, sCLNodeType** class_params, int num_params, BOOL static_, BOOL mixin_, sCLNodeType* type, char* name, int block_num, int bt_num_params, sCLNodeType** bt_class_params, sCLNodeType* bt_result_type, int parse_phase_num)
{
    int method_index;
    sCLMethod* method_of_the_same_type;

    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        sCLNodeType* result_type;

        method_index = klass->mClass->mMethodIndexOfCompileTime -1;
        method_of_the_same_type = get_method_with_type_params(klass, name, class_params, num_params, static_, NULL, method_index-1, block_num, bt_num_params, bt_class_params, bt_result_type, ALLOC &result_type);

        if(method_of_the_same_type) {
            /// check the result type of these ///
            if(!type_identity(type, result_type)) {
                parser_err_msg_format(sname, *sline, "the result type of this method(%s) is differ from the result type of the method before", name);
                (*err_num)++;
            }
        }
    }
}

/// check the existance of a method which has the same name and the same parametors on super classes ///
static void check_existance_of_method_on_super_class(sCLNodeType* klass, char* sname, int* sline, int* err_num, sCLNodeType** class_params, int num_params, BOOL static_, BOOL mixin_, BOOL abstract_, BOOL virtual_, sCLNodeType* result_type, char* name, int block_num, int bt_num_params, sCLNodeType** bt_class_params, sCLNodeType* bt_result_type, int parse_phase_num, sParserInfo* info)
{
    sCLMethod* method_on_super_class;
    sCLNodeType* klass2;
    sCLNodeType* result_type_of_super_class_method;

    method_on_super_class = get_method_with_type_params_on_super_classes(info->klass, name, class_params, num_params, &klass2, static_, NULL, block_num, bt_num_params, bt_class_params, bt_result_type, ALLOC &result_type_of_super_class_method);

    if(method_on_super_class) {
        if(!substitution_posibility(result_type_of_super_class_method, result_type)) 
        {
            parser_err_msg_format(info->sname, *info->sline, "can't override of this method because result type of this method(%s) is differ from the result type of the method on the super class.", name);
            parser_err_msg_without_line("Requiring type is ");
            show_node_type_for_errmsg(result_type_of_super_class_method);
            parser_err_msg_without_line(". But this type is ");
            show_node_type_for_errmsg(result_type);
            parser_err_msg_without_line("\n");

            (*info->err_num)++;
        }

        /// check the override and virtual method types ///
        if(((method_on_super_class->mFlags & CL_ABSTRACT_METHOD) || (method_on_super_class->mFlags & CL_VIRTUAL_METHOD)) && !(virtual_ || abstract_)) {
            parser_err_msg_format(info->sname, *info->sline, "require \"virtual\" type to this method(%s) because the method on the super class has \"virtual\" method type", METHOD_NAME2(klass2->mClass, method_on_super_class));
            (*info->err_num)++;
        }
    }
}

static void check_param_initializer(sByteCode* code_params, int num_params, char* sname, int* sline, int* err_num)
{
    int i;
    BOOL existance_of_code_params;

    existance_of_code_params = FALSE;

    for(i=0; i<num_params; i++) {
        if(code_params[i].mCode == NULL) {
            if(existance_of_code_params) {
                parser_err_msg_format(sname, *sline, "invalid param initializer. param iniailizers require at tail");
                (*err_num)++;
            }
        }
        else {
            existance_of_code_params = TRUE;
        }
    }
}

static BOOL parse_declaration_of_method_block(sParserInfo* info, sVarTable* lv_table, char* block_name, sCLNodeType** bt_result_type, sCLNodeType** bt_class_params, int* bt_num_params, int size_bt_class_params, int sline_top, int* block_num)
{
    char buf[WORDSIZ];
    char* rewind;
    int sline_rewind;

    *block_num = 0;
    *bt_num_params = 0;

    if(isalpha(**info->p)) {
        rewind = *info->p;
        sline_rewind = *info->sline;

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
        {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(strcmp(buf, "with") == 0) {
            *block_num = 1;

            /// get class ///
            if(!parse_namespace_and_class_and_generics_type(ALLOC bt_result_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass: NULL, info->method, FALSE, TRUE))
            {
                return FALSE;
            }

            /// append break existance flag to result type ///
            if(type_identity(*bt_result_type,gVoidType)) {
                *bt_result_type = gBoolType;
            }
            else {
                make_block_result(bt_result_type);
            }

            /// block name ///
            if(!parse_word(block_name, CL_VARIABLE_NAME_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(!add_variable_to_table(lv_table, block_name, gBlockType)) {
                parser_err_msg("local variable table overflow", info->sname, *info->sline);
                return FALSE;
            }

            *bt_num_params = 0;
            expect_next_character_with_one_forward("(", info->err_num, info->p, info->sname, info->sline);
            /// params ///
            if(!parse_params(bt_class_params, bt_num_params, size_bt_class_params, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass->mClass, NULL, NULL, ')', sline_top)) 
            {
                return FALSE;
            }
        }
        else {
            *info->p = rewind;
            *info->sline = sline_rewind;
        }
    }

    return TRUE;
}

static BOOL parse_throws(sParserInfo* info, sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX], int* exception_num)
{
    char buf[WORDSIZ];
    char* rewind;
    int sline_rewind;

    *exception_num = 0;

    if(isalpha(**info->p)) {
        rewind = *info->p;
        sline_rewind = *info->sline;

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(strcmp(buf, "throws") == 0) {
            while(**info->p) {
                if(isalpha(**info->p) || **info->p == '_') {
                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);

                    exception_class[*exception_num] = cl_get_class_with_namespace(info->current_namespace, buf, 0);

                    if(exception_class[*exception_num] == NULL) {
                        exception_class[*exception_num] = load_class_with_namespace_on_compile_time(info->current_namespace, buf, TRUE, 0, -1);

                        if(exception_class[*exception_num]) {
                            add_dependence_class(info->klass->mClass, exception_class[*exception_num]);
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "can't resovle this class name(%s::%s)", info->current_namespace, buf);
                            (*info->err_num)++;
                        }
                    }
                    else {
                        add_dependence_class(info->klass->mClass, exception_class[*exception_num]);
                    }

                    (*exception_num)++;

                    if(**info->p == ',') {
                        (*info->p)++;
                        skip_spaces_and_lf(info->p, info->sline);
                    }
                    else {
                        break;
                    }
                }
                else {
                    break;
                }
            }

            if(*exception_num == 0) {
                parser_err_msg_format(info->sname, *info->sline, "require exception class name");
                (*info->err_num)++;
            }
        }
        else {
            *info->p = rewind;
            *info->sline = sline_rewind;
        }
    }

    return TRUE;
}

static BOOL parse_alias(sParserInfo* info, int parse_phase_num, int sline_top)
{
    char buf[WORDSIZ];

    if(**info->p == '*') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER) {
            if(!set_alias_flag_to_all_methods(info->klass->mClass)) {
                parser_err_msg_format(info->sname, *info->sline, "overflow alais table");
                return FALSE;
            }
        }
    }
    else {
        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
        {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER) {
            if(!set_alias_flag_to_method(info->klass->mClass, buf)) {
                parser_err_msg_format(info->sname, sline_top, "this is not class method or overflow alias table or not found the method(%s)",buf);
                (*info->err_num)++;
            }
        }
    }

    return TRUE;
}

static BOOL parse_constructor(sParserInfo* info, sCLNodeType* result_type, char* name, BOOL mixin_, BOOL native_, BOOL synchronized_, int parse_phase_num, int sline_top, BOOL interface)
{
    sVarTable* lv_table;
    sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
    sByteCode code_params[CL_METHOD_PARAM_MAX];
    int max_stack_params[CL_METHOD_PARAM_MAX];
    int lv_num_params[CL_METHOD_PARAM_MAX];
    int num_params;
    BOOL block_num;
    sCLNodeType* bt_result_type;
    sCLNodeType* bt_class_params[CL_METHOD_PARAM_MAX];
    int bt_num_params;
    char block_name[CL_VARIABLE_NAME_MAX];
    sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX];
    int exception_num;
    BOOL variable_arguments;

    lv_table = init_var_table();

    /// method ///
    if(!add_variable_to_table(lv_table, "self", info->klass)) {
        parser_err_msg("local variable table overflow", info->sname, *info->sline);
        return FALSE;
    }

    memset(class_params, 0, sizeof(class_params));
    memset(code_params, 0, sizeof(code_params));
    memset(max_stack_params, 0, sizeof(max_stack_params));
    memset(lv_num_params, 0, sizeof(lv_num_params));
    num_params = 0;

    /// params ///
    if(!parse_params_with_initializer(class_params, ALLOC code_params, max_stack_params, lv_num_params, &num_params, CL_METHOD_PARAM_MAX, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, info->method, lv_table, ')', sline_top, &variable_arguments, parse_phase_num != PARSE_PHASE_COMPILE_PARAM_INITIALIZER))
    {
        return FALSE;
    }

    /// check param initializer ///
    check_param_initializer(code_params, num_params, info->sname, info->sline, info->err_num);

    /// method with block ///
    if(!parse_declaration_of_method_block(info, lv_table, block_name, &bt_result_type, bt_class_params, &bt_num_params, CL_METHOD_PARAM_MAX, sline_top, &block_num)) {
        return FALSE;
    }

    /// throws ///
    if(!parse_throws(info, exception_class, &exception_num)) 
    {
        return FALSE;
    }

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(info->klass, info->sname, info->sline, info->err_num, class_params, num_params, FALSE, mixin_, result_type, name, block_num, bt_num_params, bt_class_params, bt_result_type, parse_phase_num);

    /// go ///
    if(native_ || interface) {
        if(native_ && interface) {
            parser_err_msg("An interface can't define native methods", info->sname, *info->sline);
            (*info->err_num)++;
        }

        if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        /// add method to class definition ///
        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS && *info->err_num == 0) 
        {
            int i;

            add_method(info->klass->mClass, FALSE, FALSE, FALSE, native_, synchronized_, FALSE, FALSE, name, result_type, TRUE, info->method);

            if(!add_param_to_method(info->klass->mClass, class_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name, variable_arguments))
            {
                parser_err_msg("overflow parametor number", info->sname, *info->sline);
                return FALSE;
            }
            for(i=0; i<exception_num; i++) {
                add_exception_class(info->klass->mClass, info->method, exception_class[i]);
            }
        }
        else if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER && *info->err_num == 0) 
        {
            if(!add_param_initializer_to_method(info->klass->mClass, MANAGED code_params, max_stack_params, lv_num_params, num_params, info->method))
            {
                return FALSE;
            }
        }
    }
    else {
        switch(parse_phase_num) {
        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

            if(!skip_block(info->p, info->sname, info->sline)) {
                return FALSE;
            }

            /// add method to class definition ///
            if(*info->err_num == 0) {
                int i;

                add_method(info->klass->mClass, FALSE, FALSE, FALSE, native_, synchronized_, FALSE, FALSE, name, result_type, TRUE, info->method);

                if(!add_param_to_method(info->klass->mClass, class_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name, variable_arguments))
                {
                    parser_err_msg("overflow parametor number", info->sname, *info->sline);
                    return FALSE;
                }
                for(i=0; i<exception_num; i++) {
                    add_exception_class(info->klass->mClass, info->method, exception_class[i]);
                }
            }
            break;

        case PARSE_PHASE_COMPILE_PARAM_INITIALIZER:
            expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

            if(*info->err_num == 0) {
                if(!add_param_initializer_to_method(info->klass->mClass, MANAGED code_params, max_stack_params, lv_num_params, num_params, info->method))
                {
                    return FALSE;
                }
            }

            if(!skip_block(info->p, info->sname, info->sline)) {
                return FALSE;
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }

            if(!compile_method(info->method, info->klass, info->p, info->sname, info->sline, info->err_num, lv_table, TRUE, info->current_namespace)) {
                return FALSE;
            }
            }
            break;

        default:
            compile_error("unexpected error on parse_constructor\n");
            exit(1);
        }
    }

    skip_spaces_and_lf(info->p, info->sline);

    return TRUE;
}

static BOOL parse_method(sParserInfo* info, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL mixin_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, sCLNodeType* result_type, char* name, int parse_phase_num, int sline_top, BOOL interface)
{
    sVarTable* lv_table;
    sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
    int num_params;
    sByteCode code_params[CL_METHOD_PARAM_MAX];
    int max_stack_params[CL_METHOD_PARAM_MAX];
    int lv_num_params[CL_METHOD_PARAM_MAX];
    BOOL block_num;
    char block_name[CL_VARIABLE_NAME_MAX];
    sCLNodeType* bt_result_type;
    sCLNodeType* bt_class_params[CL_METHOD_PARAM_MAX];
    int bt_num_params;
    sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX];
    int exception_num;
    int i;
    BOOL variable_arguments;

    /// definitions ///
    lv_table = init_var_table();

    if(!static_) {
        if(!add_variable_to_table(lv_table, "self", info->klass)) {
            parser_err_msg("local variable table overflow", info->sname, *info->sline);
            return FALSE;
        }
    }

    memset(class_params, 0, sizeof(class_params));
    memset(code_params, 0, sizeof(code_params));
    memset(max_stack_params, 0, sizeof(max_stack_params));
    memset(lv_num_params, 0, sizeof(lv_num_params));

    num_params = 0;

    /// params ///
    if(!parse_params_with_initializer(class_params, ALLOC code_params, max_stack_params, lv_num_params, &num_params, CL_METHOD_PARAM_MAX, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, info->method, lv_table, ')', sline_top, &variable_arguments, parse_phase_num != PARSE_PHASE_COMPILE_PARAM_INITIALIZER))
    {
        return FALSE;
    }

    /// check param initializer ///
    check_param_initializer(code_params, num_params, info->sname, info->sline, info->err_num);

    /// method with block ///
    if(!parse_declaration_of_method_block(info, lv_table, block_name, &bt_result_type, bt_class_params, &bt_num_params, CL_METHOD_PARAM_MAX, sline_top, &block_num)) {
        return FALSE;
    }

    /// throws ///
    if(!parse_throws(info, exception_class, &exception_num)) 
    {
        return FALSE;
    }

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(info->klass, info->sname, info->sline, info->err_num, class_params, num_params, static_, mixin_, result_type, name, block_num, bt_num_params, bt_class_params, bt_result_type, parse_phase_num);

    /// check the existance of a method which has the same name and the same parametors on super classes ///
    check_existance_of_method_on_super_class(info->klass, info->sname, info->sline, info->err_num, class_params, num_params, static_, mixin_, abstract_, virtual_, result_type, name, block_num, bt_num_params, bt_class_params, bt_result_type, parse_phase_num, info);

    /// go ///
    if(native_ || interface || abstract_) {
        if(native_ && interface) {
            parser_err_msg("An interface can't define native methods", info->sname, *info->sline);
            (*info->err_num)++;
        }
        if(native_ && abstract_) {
            parser_err_msg("A native and abstract method can't exist", info->sname, *info->sline);
            (*info->err_num)++;
        }
        if(interface && abstract_) {
            parser_err_msg("An interface can't define abstract method", info->sname, *info->sline);
            (*info->err_num)++;
        }

        if(abstract_ && !(info->klass->mClass->mFlags & CLASS_FLAGS_ABSTRACT)) {
            parser_err_msg("Only an abstract class can define an abstract method.", info->sname, *info->sline);
            (*info->err_num)++;
        }

        if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        /// add method to class definition ///
        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS && *info->err_num == 0) 
        {
            add_method(info->klass->mClass, static_, private_, protected_, native_, synchronized_, virtual_, abstract_, name, result_type, FALSE, info->method);

            if(!add_param_to_method(info->klass->mClass, class_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name, variable_arguments))
            {
                parser_err_msg("overflow parametor number", info->sname, *info->sline);
                return FALSE;
            }
            for(i=0; i<exception_num; i++) {
                add_exception_class(info->klass->mClass, info->method, exception_class[i]);
            }
        }
        else if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER && *info->err_num == 0) 
        {
            if(!add_param_initializer_to_method(info->klass->mClass, MANAGED code_params, max_stack_params, lv_num_params, num_params, info->method))
            {
                return FALSE;
            }
        }
    }
    else {
        switch(parse_phase_num) {
            case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
                expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

                if(!skip_block(info->p, info->sname, info->sline)) {
                    return FALSE;
                }

                /// add method to class definition ///
                if(*info->err_num == 0) {
                    add_method(info->klass->mClass, static_, private_, protected_, native_, synchronized_, virtual_, abstract_, name, result_type, FALSE, info->method);

                    if(!add_param_to_method(info->klass->mClass, class_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name, variable_arguments))
                    {
                        parser_err_msg("overflow parametor number", info->sname, *info->sline);
                        return FALSE;
                    }
                    for(i=0; i<exception_num; i++) {
                        add_exception_class(info->klass->mClass, info->method, exception_class[i]);
                    }
                }
                break;

            case PARSE_PHASE_COMPILE_PARAM_INITIALIZER:
                expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

                if(*info->err_num == 0) {
                    if(!add_param_initializer_to_method(info->klass->mClass, MANAGED code_params, max_stack_params, lv_num_params, num_params, info->method))
                    {
                        return FALSE;
                    }
                }

                if(!skip_block(info->p, info->sname, info->sline)) {
                    return FALSE;
                }
                break;


            case PARSE_PHASE_DO_COMPILE_CODE: {
                if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
                    return FALSE;
                }

                if(!compile_method(info->method, info->klass, info->p, info->sname, info->sline, info->err_num, lv_table, FALSE, info->current_namespace)) 
                {
                    return FALSE;
                }
                }
                break;

            default:
                compile_error("un expected error on parse_method()\n");
                exit(1);
        }

        skip_spaces_and_lf(info->p, info->sline);
    }

    return TRUE;
}

static BOOL add_fields(sParserInfo* info, int parse_phase_num, BOOL static_ , BOOL private_, BOOL protected, char* name, sCLNodeType* result_type, BOOL initializer)
{
    sByteCode initializer_code;
    sVarTable* lv_table;
    int max_stack;
    int sline_top;

    sline_top = *info->sline;

    lv_table = init_var_table();

    /// add field ///
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    if(initializer) {
        if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
            sCLNodeType* initializer_code_type;

            initializer_code_type = NULL;

            sByteCode_init(&initializer_code);
            if(!compile_field_initializer(&initializer_code, ALLOC &initializer_code_type, info->klass, info->p, info->sname, info->sline, info->err_num, info->current_namespace, lv_table, &max_stack)) 
            {
                sByteCode_free(&initializer_code);
                return FALSE;
            }

            /// type checking ///
            if(!substitution_posibility(result_type, initializer_code_type)) {
                parser_err_msg_format(info->sname, sline_top, "type error");

                parser_err_msg_without_line("left type is ");
                show_node_type_for_errmsg(result_type);
                parser_err_msg_without_line(". right type is ");
                show_node_type_for_errmsg(initializer_code_type);
                parser_err_msg_without_line("\n");

                (*info->err_num)++;
            }
        }
        else {
            if(!skip_field_initializer(info->p, info->sname, info->sline, info->current_namespace, info->klass, lv_table)) 
            {
                return FALSE;
            }
        }
    }
    else {
        memset(&initializer_code, 0, sizeof(sByteCode));
        max_stack = 0;
    }

    if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER) {
    }
    else if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        sCLNodeType* founded_class;
        sCLField* field;
        sCLNodeType* field_type;

        /// check native class ///
        if(info->klass->mClass->mFlags & CLASS_FLAGS_NATIVE)
        {
            parser_err_msg("Clover can't append a field to the native class or a child class of native class", info->sname, sline_top);
            (*info->err_num)++;
        }

        /// check that the same name field exists ///
        field = get_field_including_super_classes(info->klass, name, &founded_class, static_, &field_type, NULL);
        if(field) {
            parser_err_msg_format(info->sname, sline_top, "the same name field exists.(%s)", name);
            (*info->err_num)++;
        }

        if(*info->err_num == 0) {
            if(!add_field(info->klass->mClass, static_, private_, protected, name, result_type)) {
                parser_err_msg("overflow number fields", info->sname, sline_top);
                return FALSE;
            }
        }
    }
    else if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
        if(*info->err_num == 0) {
            set_field_index(info->klass->mClass, name, static_);

            if(initializer) {
                if(!add_field_initializer(info->klass->mClass, static_, name, MANAGED initializer_code, lv_table, max_stack)) {
                    parser_err_msg("overflow number fields", info->sname, sline_top);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

static void parser_operator_method_name(char* name, int name_size, sParserInfo* info)
{
    if(strcmp(name, "operator") == 0) {
        skip_spaces_and_lf(info->p, info->sline);

        if(**info->p == '[') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == ']') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(**info->p == '=') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, "[]=", name_size);
                }
                else {
                    xstrncpy(name, "[]", name_size);
                }
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "require ] after [ on operator []");
                (*info->err_num)++;
            }
        }
        else if(**info->p == '+') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '+') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(**info->p == '2') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, "++2", name_size);
                }
                else {
                    xstrncpy(name, "++", name_size);
                }
            }
            else if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "+=", name_size);
            }
            else {
                xstrncpy(name, "+", name_size);
            }
        }
        else if(**info->p == '-') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '-') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(**info->p == '2') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, "--2", name_size);
                }
                else {
                    xstrncpy(name, "--", name_size);
                }
            }
            else if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "-=", name_size);
            }
            else {
                xstrncpy(name, "-", name_size);
            }
        }
        else if(**info->p == '*') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "*=", name_size);
            }
            else {
                xstrncpy(name, "*", name_size);
            }
        }
        else if(**info->p == '/') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "/=", name_size);
            }
            else {
                xstrncpy(name, "/", name_size);
            }
        }
        else if(**info->p == '%') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "%=", name_size);
            }
            else {
                xstrncpy(name, "%", name_size);
            }
        }
        else if(**info->p == '<') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '<') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(**info->p == '=') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, "<<=", name_size);
                }
                else {
                    xstrncpy(name, "<<", name_size);
                }
            }
            else {
                if(**info->p == '=') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, "<=", name_size);
                }
                else {
                    xstrncpy(name, "<", name_size);
                }
            }
        }
        else if(**info->p == '>') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '>') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(**info->p == '=') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    xstrncpy(name, ">>=", name_size);
                }
                else {
                    xstrncpy(name, ">>", name_size);
                }
            }
            else if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, ">=", name_size);
            }
            else {
                xstrncpy(name, ">", name_size);
            }
        }
        else if(**info->p == '&') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "&=", name_size);
            }
            else if(**info->p == '&') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "&&", name_size);
            }
            else {
                xstrncpy(name, "&", name_size);
            }
        }
        else if(**info->p == '^') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "^=", name_size);
            }
            else {
                xstrncpy(name, "^", name_size);
            }
        }
        else if(**info->p == '|') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "|=", name_size);
            }
            else if(**info->p == '|') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "||", name_size);
            }
            else {
                xstrncpy(name, "|", name_size);
            }
        }
        else if(**info->p == '!') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "!=", name_size);
            }
            else {
                xstrncpy(name, "!", name_size);
            }
        }
        else if(**info->p == '~') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            xstrncpy(name, "~", name_size);
        }
        else if(**info->p == '=') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(**info->p == '=') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "==", name_size);
            }
            else if(**info->p == '~') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "=~", name_size);
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "can't define operator=");
                (*info->err_num)++;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "invalid operator method (%c)", **info->p);
            (*info->err_num)++;
        }
    }
}

struct sCLNodeGenericsParamTypesStruct {
    char mName[CL_CLASS_TYPE_VARIABLE_MAX];

    sCLNodeType* mExtendsType;

    char mNumImplementsTypes;
    sCLNodeType* mImplementsTypes[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];
};

typedef struct sCLNodeGenericsParamTypesStruct sCLNodeGenericsParamTypes;

static BOOL parse_generics_param_types(sParserInfo* info, int* generics_param_types_num, sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX], int parse_phase_num, BOOL get_param_number_only, BOOL mixin_)
{
    if(**info->p == '<') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        while(1) {
            if(!parse_word(generics_param_types[*generics_param_types_num].mName, CL_CLASS_TYPE_VARIABLE_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(!get_param_number_only) {
                if(parse_phase_num == PARSE_PHASE_ADD_SUPER_CLASSES) {
                    if(!mixin_) {
                        if(!add_generics_param_type_name(info->klass->mClass, generics_param_types[*generics_param_types_num].mName))
                        {
                            parser_err_msg_format(info->sname, *info->sline, "overflow generics parametor types number or there is the same name of class parametor");
                            return FALSE;
                        }
                    }
                }
            }

            while(isalpha(**info->p)) {
                char buf[WORDSIZ];
                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);

                if(strcmp(buf, "implements") == 0) {
                    int num_implements_types = generics_param_types[*generics_param_types_num].mNumImplementsTypes;

                    while(1) {
                        sCLNodeType* node_type;

                        node_type = alloc_node_type();

                        if(!parse_namespace_and_class_and_generics_type(&node_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, parse_phase_num != PARSE_PHASE_ADD_GENERICS_TYPES || get_param_number_only, TRUE)) 
                        {
                            return FALSE;
                        }

                        if(node_type->mClass && !(node_type->mClass->mFlags & CLASS_FLAGS_INTERFACE))
                        {
                            parser_err_msg_format(info->sname, *info->sline, "A generics parametor class can't implement none interface class");
                            (*info->err_num)++;
                        }

                        generics_param_types[*generics_param_types_num].mImplementsTypes[num_implements_types] = node_type;

                        generics_param_types[*generics_param_types_num].mNumImplementsTypes++;
                        num_implements_types++;

                        if(generics_param_types[*generics_param_types_num].mNumImplementsTypes >= CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX)
                        {
                            parser_err_msg_format(info->sname, *info->sline, "overflow number of implements");
                            return FALSE;
                        }

                        if(**info->p == '&') {
                            (*info->p)++;
                            skip_spaces_and_lf(info->p, info->sline);
                        }
                        else {
                            break;
                        }
                    }
                }
                else if(strcmp(buf, "extends") == 0) {
                    sCLNodeType* node_type;

                    if(generics_param_types[*generics_param_types_num].mExtendsType != NULL)
                    {
                        parser_err_msg_format(info->sname, *info->sline, "Can't write \"extends\" keyword twice on a generics param type");
                        (*info->err_num)++;
                    }

                    if(!parse_namespace_and_class_and_generics_type(&node_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, parse_phase_num != PARSE_PHASE_ADD_GENERICS_TYPES || get_param_number_only, TRUE)) 
                    {
                        return FALSE;
                    }

                    if(node_type->mClass && node_type->mClass->mFlags & CLASS_FLAGS_INTERFACE)
                    {
                        parser_err_msg_format(info->sname, *info->sline, "A generics parametor class can't extend from interface");
                        (*info->err_num)++;
                    }

                    generics_param_types[*generics_param_types_num].mExtendsType = node_type;
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "expected \"implements\" or \"extends\" keyword");
                    (*info->err_num)++;
                }
            }

            if(**info->p == 0) {
                parser_err_msg_format(info->sname, *info->sline, "It arrived at the end of source before > closing");
                return FALSE;
            }
            else if(**info->p == '>') {
                (*generics_param_types_num)++;

                if(*generics_param_types_num > CL_GENERICS_CLASS_PARAM_MAX) {
                    parser_err_msg_format(info->sname, *info->sline, "overflow generics class param max");
                    return FALSE;
                }
                break;
            }
            else if(**info->p == ',') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);
                (*generics_param_types_num)++;

                if(*generics_param_types_num > CL_GENERICS_CLASS_PARAM_MAX) {
                    parser_err_msg_format(info->sname, *info->sline, "overflow generics class param max");
                    return FALSE;
                }
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "expected > or , character. But this is (%c)", **info->p);
                (*info->err_num)++;
                (*info->p)++;
            }
        }

        expect_next_character_with_one_forward(">", info->err_num, info->p, info->sname, info->sline);
    }
    else {
        *generics_param_types_num = 0;
    }

    return TRUE;
}

static BOOL methods_and_fields_and_alias(sParserInfo* info, int parse_phase_num, BOOL interface);

static BOOL include_module(sCLModule* module, sParserInfo* info, int parse_phase_num, BOOL interface)
{
    sParserInfo new_info;
    int sline;
    char buf[BUFSIZ];
    char* module_body;
    char* module_name;

    module_body = get_module_body(module);

    if(module_body == NULL) {
        return FALSE;
    }
    module_name = module->mName;

    memset(&new_info, 0, sizeof(sParserInfo));

    new_info.p = &module_body;
    snprintf(buf, BUFSIZ, "%s(which is included on %s %d)", module_name, info->sname, *info->sline);
    new_info.sname = buf;
    sline = 0;
    new_info.sline = &sline;
    new_info.err_num = info->err_num;
    new_info.current_namespace = info->current_namespace;
    new_info.klass = info->klass;
    new_info.method = info->method;

    if(!methods_and_fields_and_alias(&new_info, parse_phase_num, interface))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL automatically_include_module_from_class(sParserInfo* info, int parse_phase_num, BOOL interface, int mixin_version)
{
    int i,j;
    sCLClass* klass;

    klass = info->klass->mClass;

    if(mixin_version == -1 && !(klass->mFlags & CLASS_FLAGS_INCLUDED_MODULE)) {
        for(i=0; i<klass->mNumSuperClasses; i++) {
            sCLNodeType* super_class;
            
            super_class = ALLOC create_node_type_from_cl_type(&klass->mSuperClasses[i], klass);

            MASSERT(super_class->mClass != NULL);     // checked on load time

            for(j=0; j<super_class->mClass->mNumIncludedModules; j++) {
                sCLModule* module;

                module = get_module_from_cl_type(super_class->mClass, &super_class->mClass->mIncludedModules[j]);

                if(module == NULL) {
                    parser_err_msg_format(info->sname, *info->sline, "Clover can't find the module named %s", CONS_str(&super_class->mClass->mConstPool, super_class->mClass->mIncludedModules[j].mClassNameOffset));
                    (*info->err_num)++;
                }

                if(!include_module(module, info, parse_phase_num, interface))
                {
                    return FALSE;
                }
            }
        }

        for(i=0; i<klass->mNumIncludedModules; i++) {
            sCLModule* module;

            module = get_module_from_cl_type(klass, &klass->mIncludedModules[i]);

            if(module == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't find the module named %s", CONS_str(&klass->mConstPool, klass->mIncludedModules[i].mClassNameOffset));
                (*info->err_num)++;
            }

            if(!include_module(module, info, parse_phase_num, interface))
            {
                return FALSE;
            }
        }

        klass->mFlags |= CLASS_FLAGS_INCLUDED_MODULE;
    }

    return TRUE;
}

static BOOL init_method(sParserInfo* info, int parse_phase_num)
{
    /// temporarily entried ///
    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS)
    {
        if(!create_method(info->klass->mClass, &info->method)) {
            parser_err_msg("overflow methods number", info->sname, *info->sline);
            return FALSE;
        }

        info->klass->mClass->mMethodIndexOfCompileTime++;
    }
    else {
        int method_index;

        /// get method pointer and index from Class data. ///
        method_index = info->klass->mClass->mMethodIndexOfCompileTime++;

        /// add generics parametor types to methods ///
        info->method = info->klass->mClass->mMethods + method_index;
    }

    return TRUE;
}

static BOOL methods_and_fields_and_alias(sParserInfo* info, int parse_phase_num, BOOL interface)
{
    int i;
    char buf[WORDSIZ];

    if(**info->p == '}') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);
    }
    else {
        while(**info->p) {
            BOOL static_;
            BOOL private_;
            BOOL protected_;
            BOOL native_;
            BOOL mixin_;
            BOOL synchronized_;
            BOOL virtual_;
            BOOL abstract_;
            char* saved_p;
            int saved_sline;

            saved_p = *info->p;
            saved_sline = *info->sline;

            /// prefix ///
            static_ = FALSE;
            private_ = FALSE;
            protected_ = FALSE;
            native_ = FALSE;
            mixin_ = FALSE;
            synchronized_ = FALSE;
            virtual_ = FALSE;
            abstract_ = FALSE;

            if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
            {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            while(**info->p) {
                if(strcmp(buf, "native") == 0) {
                    native_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                    {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "nosynchronized") == 0) {
                    synchronized_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "static") == 0) {
                    static_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "virtual") == 0) {
                    virtual_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "private") == 0) {
                    private_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "protected") == 0) {
                    protected_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "mixin") == 0) {
                    mixin_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(strcmp(buf, "abstract") == 0) {
                    abstract_ = TRUE;

                    saved_p = *info->p;
                    saved_sline = *info->sline;

                    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else {
                    break;
                }
            }

            /// constructor ///
            if((strcmp(buf, "Self") == 0 || strcmp(buf, CLASS_NAME(info->klass->mClass)) == 0 ) && **info->p == '(') {
                char name[CL_METHOD_NAME_MAX+1];
                sCLNodeType* result_type;

                if(static_ || private_ || protected_ || virtual_ || abstract_) {
                    parser_err_msg("don't append method type(\"static\" or \"private\" or \"protected\" or \"mixin\" or \"virtual\" or \"abstract\") to constructor", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(!init_method(info, parse_phase_num))
                {
                    return FALSE;
                }

                if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
                    return FALSE;
                }

                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                xstrncpy(name, "_constructor", CL_METHOD_NAME_MAX);

                result_type = clone_node_type(info->klass);

                if(!parse_constructor(info, result_type, name, mixin_, native_, synchronized_, parse_phase_num, *info->sline, interface))
                {
                    return FALSE;
                }
            }
            else if(strcmp(buf, "include") == 0) {
                sCLModule* module;

                if(static_ || private_ || protected_ || native_ || mixin_  || synchronized_ || abstract_)
                {
                    parser_err_msg("don't append method type(\"static\" or \"private\" or \"protected\" or \"mixin\" or \"native\" or \"nosynchronized\" or \"abstract\") to include", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(!parse_module_name(&module, info->p, info->sname, info->sline, info->err_num, info->current_namespace, FALSE))
                {
                    return FALSE;
                }

                expect_next_character_with_one_forward(";", info->err_num, info->p, info->sname, info->sline);
                skip_spaces_and_lf(info->p, info->sline);

                if(module != NULL) {
                    if(!include_module(module, info, parse_phase_num, interface))
                    {
                        return FALSE;
                    }
                }
            }
            else if(strcmp(buf, "alias") == 0) {
                if(static_ || private_ || protected_ || native_ || mixin_  || synchronized_ || abstract_) {
                    parser_err_msg("don't append method type(\"static\" or \"private\" or \"protected\" or \"mixin\" or \"native\" or \"nosynchronized\" or \"abstract\") to alias", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(interface) {
                    parser_err_msg_format(info->sname, *info->sline, "An interface can't define aliases");
                    (*info->err_num)++;
                }

                if(!parse_alias(info, parse_phase_num, *info->sline)) {
                    return FALSE;
                }
            }
            /// method or field ///
            else {
                sCLNodeType* result_type;
                char name[CL_METHOD_NAME_MAX+1];

                *info->p = saved_p;                 // rewind
                *info->sline = saved_sline;

                if(!parse_namespace_and_class_and_generics_type(ALLOC &result_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, (info->klass ? info->klass->mClass:NULL), info->method, FALSE, TRUE)) 
                {
                    return FALSE;
                }

                /// name ///
                if(!parse_word(name, CL_METHOD_NAME_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);

                parser_operator_method_name(name, CL_METHOD_NAME_MAX, info);

                if(!expect_next_character("(;=", info->err_num, info->p, info->sname, info->sline)) 
                {
                    return FALSE;
                }

                /// method ///
                if(**info->p == '(') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    if(!init_method(info, parse_phase_num))
                    {
                        return FALSE;
                    }

                    if(!parse_method(info, static_, private_, protected_, native_, mixin_, synchronized_, virtual_, abstract_, result_type, name, parse_phase_num, *info->sline, interface))

                    {
                        return FALSE;
                    }
                }
                /// field without initializer ///
                else if(**info->p == ';') {
                    if(native_ || mixin_  || synchronized_ || virtual_ || abstract_)
                    {
                        parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"nosynchronized\" or \"virtual\" or \"abstract_\") to field", info->sname, *info->sline);
                        (*info->err_num)++;
                    }

                    if(interface) {
                        parser_err_msg("An interface can't define fields", info->sname, *info->sline);
                        (*info->err_num)++;
                    }

                    if(result_type->mClass != NULL) {
                        if(!add_fields(info, parse_phase_num, static_, private_, protected_, name, result_type, FALSE))
                        {
                            return FALSE;
                        }
                    }
                }
                /// field with initializer ///
                else if(**info->p == '=') {
                    if(native_ || mixin_  || synchronized_ || virtual_ || abstract_)
                    {
                        parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"nosynchronized\" or \"virtual\" or \"abstract_\") to field", info->sname, *info->sline);
                        (*info->err_num)++;
                    }

                    if(interface) {
                        parser_err_msg("An interface can't define fields", info->sname, *info->sline);
                        (*info->err_num)++;
                    }

                    if(result_type->mClass != NULL) {
                        if(!add_fields(info, parse_phase_num, static_, private_, protected_, name, result_type, TRUE))
                        {
                            return FALSE;
                        }
                    }
                }
            }

            if(**info->p == '}') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);
                break;
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse class
//////////////////////////////////////////////////
static BOOL skip_namespace_and_class_and_generics_type(char** p, sParserInfo* info)
{
    sCLNodeType* dummy;

    if(!parse_namespace_and_class_and_generics_type(ALLOC &dummy, p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass: NULL, info->method, TRUE, TRUE) )
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL automatically_super_class_addition(sParserInfo* info, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_, BOOL enum_, BOOL no_super_class, int sline_top)
{
    if(info->klass->mClass == gObjectType->mClass) {
        return TRUE;
    }
    if(*info->err_num > 0 || !no_super_class || mixin_) {
        return TRUE;
    }
    if(info->klass->mClass->mFlags & CLASS_FLAGS_INTERFACE) {
        return TRUE;
    }

    /// Enum ///
    if(enum_) {
        sCLNodeType* enum_type;

        enum_type = alloc_node_type();

        enum_type->mGenericsTypesNum = 0;
        enum_type->mClass = cl_get_class("Enum");

        MASSERT(enum_type->mClass != NULL);

        if(!add_super_class(info->klass->mClass, enum_type)) {
            parser_err_msg("Overflow number of super class.", info->sname, sline_top);
            return FALSE;
        }
    }
    /// Native Class ///
    else if(info->klass->mClass->mFlags & CLASS_FLAGS_NATIVE) {
        sCLNodeType* native_class_type;

        native_class_type = alloc_node_type();

        native_class_type->mGenericsTypesNum = 0;
        native_class_type->mClass = cl_get_class("NativeClass");

        MASSERT(native_class_type->mClass != NULL);

        if(!add_super_class(info->klass->mClass, native_class_type)) {
            parser_err_msg("Overflow number of super class.", info->sname, sline_top);
            return FALSE;
        }
    }
    /// User Object ///
    else if(!(info->klass->mClass->mFlags & CLASS_FLAGS_GENERICS_PARAM)) {
        sCLNodeType* user_class_type;

        user_class_type = alloc_node_type();

        user_class_type->mGenericsTypesNum = 0;
        user_class_type->mClass = cl_get_class("UserClass");

        MASSERT(user_class_type->mClass != NULL);

        if(!add_super_class(info->klass->mClass, user_class_type)) {
            parser_err_msg("Overflow number of super class.", info->sname, sline_top);
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL extends_and_implements_and_includes(sParserInfo* info, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_, BOOL enum_, BOOL native_)
{
    char buf[WORDSIZ];
    sCLNodeType* super_class;
    BOOL no_super_class;
    int sline_top;

    sline_top = *info->sline;

    /// extends or implements ///
    super_class = NULL;
    no_super_class = info->klass->mClass->mSuperClass.mClassNameOffset == 0;

    while(**info->p == 'e' || **info->p == 'i') {
        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(strcmp(buf, "extends") == 0) {
            if(mixin_) {
                parser_err_msg("A class can't extend another class with mixin", info->sname, sline_top);
                (*info->err_num)++;
            }

            if(super_class == NULL) {
                if(parse_phase_num == PARSE_PHASE_ADD_SUPER_CLASSES) 
                {
                    /// get class ///
                    if(!parse_namespace_and_class_and_generics_type(&super_class, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE, TRUE)) 
                    {
                        return FALSE;
                    }

                    if(super_class->mClass) {
                        if(interface && !(super_class->mClass->mFlags & CLASS_FLAGS_INTERFACE))
                        {
                            parser_err_msg("An interface should extend another interface only, can't extend another class", info->sname, sline_top);
                            (*info->err_num)++;
                        }

                        if(abstract_ && !(super_class->mClass->mFlags & CLASS_FLAGS_ABSTRACT)) 
                        {
                            parser_err_msg("An abstract class should extend another abstract class only", info->sname, sline_top);
                            (*info->err_num)++;
                        }

                        if(super_class->mClass->mFlags & CLASS_FLAGS_FINAL) {
                            parser_err_msg_format(info->sname, sline_top, "A class can't extend final class(%s)", REAL_CLASS_NAME(super_class->mClass));
                            (*info->err_num)++;
                        }
                    }

                    if(*info->err_num == 0) {
                        if(!add_super_class(info->klass->mClass, super_class)) {
                            parser_err_msg("Overflow number of super class.", info->sname, sline_top);
                            return FALSE;
                        }
                    }
                }
                else {
                    if(!skip_namespace_and_class_and_generics_type(info->p, info)) {
                        return FALSE;
                    }
                }

                no_super_class = FALSE;
            }
            else {
                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);

                parser_err_msg("A class can exntend one super class. Clover doesn't support multi-inheritance", info->sname, sline_top);
                (*info->err_num)++;
            }
        }
        else if(strcmp(buf, "includes") == 0) {
            sCLModule* module;

            if(interface) {
                parser_err_msg("An interface can't include a module", info->sname, sline_top);
                (*info->err_num)++;
            }

            while(1) {
                if(!parse_module_name(&module, info->p, info->sname, info->sline, info->err_num, info->current_namespace, TRUE))
                {
                    return FALSE;
                }

                if(parse_phase_num == PARSE_PHASE_ADD_GENERICS_TYPES && *info->err_num == 0)
                {
                    if(module == NULL) {
                        parser_err_msg_format(info->sname, sline_top, "Clover can't found the module");
                        (*info->err_num)++;
                    }
                    /// add the including module info to the class ///
                    else {
                        if(!add_included_module(info->klass->mClass, module)) {
                            parser_err_msg_format(info->sname, sline_top, "overflow included module");
                            (*info->err_num)++;
                            return FALSE;
                        }
                    }
                }

                if(**info->p != ',') {
                    break;
                }

                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);
            }

        }
        else if(strcmp(buf, "implements") == 0) {
            if(interface) {
                parser_err_msg("An interface can't implement an interface", info->sname, sline_top);
                (*info->err_num)++;
            }

            while(1) {
                sCLNodeType* interface;

                if(parse_phase_num == PARSE_PHASE_ADD_GENERICS_TYPES && *info->err_num == 0)
                {
                    if(!parse_namespace_and_class_and_generics_type(&interface, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE, TRUE))
                    {
                        return FALSE;
                    }

                    /// add the implement info to the class ///
                    if(interface->mClass != NULL) {
                        if(!(interface->mClass->mFlags & CLASS_FLAGS_INTERFACE))
                        {
                            parser_err_msg_format(info->sname, sline_top, "A class can't implement non interface class");
                            (*info->err_num)++;
                        }

                        if(!add_implemented_interface(info->klass->mClass, interface)) {
                            parser_err_msg_format(info->sname, sline_top, "overflow implemented interface");
                            (*info->err_num)++;
                            return FALSE;
                        }
                    }
                }
                else if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER && *info->err_num == 0)
                {
                    if(!parse_namespace_and_class_and_generics_type(&interface, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE, TRUE))
                    {
                        return FALSE;
                    }

                    /// check the implement methods on the class ///
                    if(interface->mClass) {
                        char* not_implemented_method_name;

                        if(!check_method_for_implemented_interface_without_super_class(info->klass, interface, &not_implemented_method_name)) {
                            parser_err_msg_format(info->sname, sline_top, "%s is not implemented %s interface", REAL_CLASS_NAME(info->klass->mClass), REAL_CLASS_NAME(interface->mClass));
                            parser_err_msg_format(info->sname, sline_top, "%s doesn't have method named %s", REAL_CLASS_NAME(info->klass->mClass), not_implemented_method_name);
                            (*info->err_num)++;
                        }
                    }
                }
                else {
                    if(!skip_namespace_and_class_and_generics_type(info->p, info)) {
                        return FALSE;
                    }
                }

                if(**info->p != ',') {
                    break;
                }

                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);
            }
        }
        else {
            parser_err_msg_format(info->sname, sline_top, "clover expected \"extends\" or \"implements\" as next word, but this is \"%s\"", buf);
            (*info->err_num)++;
        }
    }

    /// automatically adding super class ///
    if(parse_phase_num == PARSE_PHASE_ADD_SUPER_CLASSES) {
        if(!automatically_super_class_addition(info, mixin_, parse_phase_num, interface, abstract_, enum_, no_super_class, sline_top)) 
        {
            return FALSE;
        }
    }

    /// make super class list from temporary super class data ///
    if(parse_phase_num == PARSE_PHASE_CALCULATE_SUPER_CLASSES) {
        if(!make_super_class_list(info->klass->mClass)) {
            parser_err_msg_format(info->sname, sline_top, "Overflow super class table");
            (*info->err_num)++;
        }
    }

    return TRUE;
}

static BOOL allocate_new_class(char* class_name, sParserInfo* info, BOOL private_, BOOL mixin_, BOOL abstract_, BOOL dynamic_typing_, BOOL final_, BOOL native_, int parse_phase_num, BOOL interface, BOOL struct_, BOOL enum_, int parametor_num, int mixin_version)
{
    /// new definition of class ///
    if(info->klass->mClass == NULL) {
        if(mixin_) {
            if(mixin_version <= 1) {
                parser_err_msg_format(info->sname, *info->sline, "invalid mixin version");
                (*info->err_num)++;
            }

            info->klass->mClass = load_class_with_namespace_on_compile_time(info->current_namespace, class_name, TRUE, parametor_num, mixin_version-1);

            if(info->klass->mClass == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't load %s::%s class version %d", info->current_namespace, class_name, mixin_version-1);
                (*info->err_num)++;
            }

            if(!set_class_version(info->klass->mClass, mixin_version))
            {
                parser_err_msg_format(info->sname, *info->sline, "class version overflow");
                (*info->err_num)++;
            }
        }
        else {
            info->klass->mClass = alloc_class_on_compile_time(info->current_namespace, class_name, private_, abstract_, interface, dynamic_typing_, final_, native_, struct_, enum_, parametor_num);

            if(!set_class_version(info->klass->mClass, 1))
            {
                parser_err_msg_format(info->sname, *info->sline, "class version overflow");
                (*info->err_num)++;
            }
        }
    }
    /// the class has already been loaded ///
    else {
        /// The same class exists on this source ///
        if(info->klass->mClass->mFlags & CLASS_FLAGS_MODIFIED) {
            if(mixin_) {
                parser_err_msg_format(info->sname, *info->sline, "The same class exists on this source. Don't append mixin keyword for the class");
                (*info->err_num)++;
            }

            if(dynamic_typing_) {
                parser_err_msg_format(info->sname, *info->sline, "\"dynamic_typing\" should be defined on new class only");
                (*info->err_num)++;
            }
            if(private_) {
                parser_err_msg_format(info->sname, *info->sline, "\"private\" should be defined on new class only");
                (*info->err_num)++;
            }
            if(interface) {
                parser_err_msg_format(info->sname, *info->sline, "\"interface\" can't define multitime");
                (*info->err_num)++;
            }
        }
        else {
            /// reload the mixin class ///
            if(mixin_) {
                MASSERT(mixin_version != -1);

                unload_class(info->current_namespace, class_name, parametor_num);

                info->klass->mClass = load_class_with_namespace_on_compile_time(info->current_namespace, class_name, TRUE, parametor_num, mixin_version-1);

                if(info->klass->mClass == NULL) {
                    parser_err_msg_format(info->sname, *info->sline, "Clover can't load %s::%s class version %d", info->current_namespace, class_name, mixin_version-1);
                    (*info->err_num)++;
                }

                if(!set_class_version(info->klass->mClass, mixin_version))
                {
                    parser_err_msg_format(info->sname, *info->sline, "class version overflow");
                    (*info->err_num)++;
                }

                if(dynamic_typing_) {
                    parser_err_msg_format(info->sname, *info->sline, "\"dynamic_typing\" should be defined on new class only");
                    (*info->err_num)++;
                }
                if(private_) {
                    parser_err_msg_format(info->sname, *info->sline, "\"private\" should be defined on new class only");
                    (*info->err_num)++;
                }
                if(interface) {
                    parser_err_msg_format(info->sname, *info->sline, "\"interface\" can't define multitime");
                    (*info->err_num)++;
                }
            }
            /// unload the class and allocate new class ///
            else {
                unload_class(info->current_namespace, class_name, parametor_num);

                info->klass->mClass = alloc_class_on_compile_time(info->current_namespace, class_name, private_, abstract_, interface, dynamic_typing_, final_, native_, struct_, enum_, parametor_num);

                if(!set_class_version(info->klass->mClass, 1))
                {
                    parser_err_msg_format(info->sname, *info->sline, "class version overflow");
                    (*info->err_num)++;
                }
            }
        }
    }

    /// A flag is setted for writing class to file ///
    info->klass->mClass->mFlags |= CLASS_FLAGS_MODIFIED;   // for save_all_modified_class() 

    return TRUE;
}

static BOOL skip_class_definition(sParserInfo* info, int parse_phase_num, BOOL mixin_, BOOL interface, BOOL abstract_, BOOL enum_, BOOL native_) 
{
    /// extends or implements ///
    if(!extends_and_implements_and_includes(info, mixin_, parse_phase_num, interface, abstract_, enum_, native_)) {
        return FALSE;
    }

    if(**info->p == '{') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!skip_block(info->p, info->sname, info->sline)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)", **info->p);

        return FALSE;
    }

    return TRUE;
}

static BOOL get_definition_from_class(sParserInfo* info, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_, BOOL native_)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements_and_includes(info, mixin_, parse_phase_num, interface, abstract_, FALSE, native_)) {
        return FALSE;
    }

    if(**info->p == '{') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!methods_and_fields_and_alias(info, parse_phase_num, interface)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)", **info->p);

        return FALSE;
    }

    return TRUE;
}

static BOOL compile_class(sParserInfo* info, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_, BOOL native_)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements_and_includes(info, mixin_, parse_phase_num, interface, abstract_, FALSE, native_)) {
        return FALSE;
    }

    if(**info->p == '{') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!methods_and_fields_and_alias(info, parse_phase_num, interface)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)", **info->p);

        return FALSE;
    }

    return TRUE;
}

static void check_struct_implements_iclonable(sParserInfo* info, int sline)
{
    sCLNodeType* klass;
    int i;
    BOOL flg;

    klass = info->klass;

    if(klass->mClass->mFlags & CLASS_FLAGS_STRUCT && !(klass->mClass->mFlags & CLASS_FLAGS_ABSTRACT)) 
    {
        if(get_clone_method(klass->mClass) == NULL) {
            parser_err_msg_format(info->sname, sline, "A struct class requires to implement clone method");
            (*info->err_num)++;
        }
    }
}

static void check_abstract_methods(sParserInfo* info, int sline_saved)
{
    char* not_implemented_method_name;

    if(!check_implemented_abstract_methods(info->klass, &not_implemented_method_name)) 
    {
        parser_err_msg_format(info->sname, sline_saved, "%s is not implemented abstract methods(%s) of the super class", REAL_CLASS_NAME(info->klass->mClass), not_implemented_method_name);
        (*info->err_num)++;
    }
}

static BOOL parse_class(sParserInfo* info, BOOL private_, BOOL mixin_, BOOL abstract_, BOOL dynamic_typing_, BOOL final_, BOOL struct_, BOOL native_, int parse_phase_num, BOOL interface)
{
    char class_name[WORDSIZ];
    int generics_param_types_num;
    sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX];
    int i;
    char* p_saved;
    int sline_saved;
    int mixin_version;

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
    {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    info->klass = alloc_node_type(); // allocated
    
    /// parse forward to get generics parametor num ///
    p_saved = *info->p;
    sline_saved = *info->sline;

    generics_param_types_num = 0;
    memset(generics_param_types, 0, sizeof(generics_param_types));

    if(!parse_generics_param_types(info, &generics_param_types_num, generics_param_types, parse_phase_num, TRUE, mixin_))
    {
        return FALSE;
    }
    
    *info->p = p_saved;                 // rewind
    *info->sline = sline_saved;

    info->klass->mClass = cl_get_class_with_argument_namespace_only(info->current_namespace, class_name, generics_param_types_num);

    MASSERT(info->klass->mClass != NULL || info->klass->mClass == NULL);

    /// get class type variable ///
    generics_param_types_num = 0;
    memset(generics_param_types, 0, sizeof(generics_param_types));

    if(!parse_generics_param_types(info, &generics_param_types_num, generics_param_types, parse_phase_num, FALSE, mixin_))
    {
        return FALSE;
    }

    info->klass->mGenericsTypesNum = generics_param_types_num;
    for(i=0; i<generics_param_types_num; i++) {
        info->klass->mGenericsTypes[i] = gGParamTypes[i];
    }

    /// get version ///
    if(mixin_) {
        mixin_version = -1;

        if(memcmp(*info->p, "version", 7) == 0) {
            (*info->p) += 7;
            skip_spaces_and_lf(info->p, info->sline);

            if(isdigit(**info->p)) {
                mixin_version = 0;
                while(isdigit(**info->p)) {
                    mixin_version = mixin_version * 10 + **info->p - '0';
                    (*info->p)++;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else {
                parser_err_msg_format(info->sname, *info->sline, "Mixin class requires the mixin version");
                (*info->err_num)++;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "Mixin class requires the mixin version");
            (*info->err_num)++;
        }
    }
    else {
        mixin_version = -1;
    }

    switch(parse_phase_num) {
        case PARSE_PHASE_ALLOC_CLASSES:
            if(!allocate_new_class(class_name, info, private_, mixin_, abstract_, dynamic_typing_, final_, native_, parse_phase_num, interface, struct_, FALSE, generics_param_types_num, mixin_version)) 
            {
                return FALSE;
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_, FALSE, native_))
            {
                return FALSE;
            }

            break;

        case PARSE_PHASE_ADD_SUPER_CLASSES:
            MASSERT(info->klass->mClass != NULL);

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_, FALSE, native_))
            {
                return FALSE;
            }

            break;

        case PARSE_PHASE_CALCULATE_SUPER_CLASSES:
            MASSERT(info->klass->mClass != NULL);

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_, FALSE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_GENERICS_TYPES:
            MASSERT(info->klass->mClass != NULL);

            if(*info->err_num == 0) {
                for(i=0; i<generics_param_types_num; i++) {
                    if(!add_generics_param_type(info->klass->mClass, generics_param_types[i].mName, generics_param_types[i].mExtendsType, generics_param_types[i].mNumImplementsTypes, generics_param_types[i].mImplementsTypes))
                    {
                        parser_err_msg_format(info->sname, *info->sline, "not found the parametor name(%s)", generics_param_types[i].mName);
                        return FALSE;
                    }
                }
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_, FALSE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, interface, mixin_version))
            {
                return FALSE;
            }

            if(!get_definition_from_class(info, mixin_, parse_phase_num, interface, abstract_, native_)) {
                return FALSE;
            }

            break;

        case PARSE_PHASE_COMPILE_PARAM_INITIALIZER:
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, interface, mixin_version))
            {
                return FALSE;
            }

            if(!get_definition_from_class(info, mixin_, parse_phase_num, interface, abstract_, native_)) {
                return FALSE;
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, interface, mixin_version))
            {
                return FALSE;
            }

            if(!compile_class(info, mixin_, parse_phase_num, interface, abstract_, native_)) {
                return FALSE;
            }

            }
            break;
    }

    if(parse_phase_num == PARSE_PHASE_COMPILE_PARAM_INITIALIZER) {
        check_struct_implements_iclonable(info, sline_saved);
        check_abstract_methods(info, sline_saved);
    }

    return TRUE;
}

static BOOL change_namespace(sParserInfo* info)
{
    char buf[WORDSIZ];

    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

    if(info->current_namespace[0] == 0) {
        if(!append_namespace_to_curernt_namespace(info->current_namespace, buf)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "can't meke namespace nest");
        (*info->err_num)++;
    }

    return TRUE;
}

static BOOL parse_module(sParserInfo* info, int parse_phase_num)
{
    char buf[CL_CLASS_NAME_MAX+1];
    int block_num;
    sCLModule* module;

    /// name ///
    if(!parse_word(buf, CL_CLASS_NAME_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) 
    {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

    if(parse_phase_num == PARSE_PHASE_ALLOC_CLASSES) {
        module = create_module(info->current_namespace, buf);
        
        if(module == NULL) {
            unload_module(info->current_namespace, buf);

            module = create_module(info->current_namespace, buf);

            if(module == NULL) {
                parser_err_msg_format(info->sname, *info->sline, "overflow the module table or the same name module exists(%s::%s)", info->current_namespace, buf);
                return FALSE;
            }
        }

        this_module_is_modified(module);

        block_num = 0;

        while(**info->p) {
            if(**info->p == '{') {
                block_num++;
                append_character_to_module(module, **info->p);
                (*info->p)++;
            }
            else if(**info->p == '}') {
                if(block_num == 0) {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);
                    break;
                }
                else {
                    block_num--;
                    append_character_to_module(module, **info->p);
                    (*info->p)++;
                }
            }
            else if(**info->p == '\n') {
                append_character_to_module(module, **info->p);
                (*info->p)++;
                (*info->sline)++;
            }
            else {
                append_character_to_module(module, **info->p);
                (*info->p)++;
            }
        }
    }
    else {
        if(!skip_block(info->p, info->sname, info->sline)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);
    }

    return TRUE;
}

static BOOL parse_enum_element(sParserInfo* info, BOOL mixin_, BOOL parse_phase_num, BOOL native_)
{
    /// extends or implements ///
    if(!extends_and_implements_and_includes(info, mixin_, parse_phase_num, FALSE, FALSE, TRUE, native_)) {
        return FALSE;
    }

    expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

    while(**info->p) {
        sCLField* field;
        char element_name[WORDSIZ];
        sCLNodeType* founded_class;
        sCLNodeType* field_type;

        /// element name ///
        if(!parse_word(element_name, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
        {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
            /// check that the same name field exists ///
            field = get_field_including_super_classes(info->klass, element_name, &founded_class, TRUE, &field_type, NULL);
            if(field) {
                parser_err_msg_format(info->sname, *info->sline, "the same name field exists.(%s)", element_name);
                (*info->err_num)++;
            }

            if(*info->err_num == 0) {
                if(!add_field(info->klass->mClass, TRUE, FALSE, FALSE, element_name, info->klass)) 
                {
                    parser_err_msg("overflow number fields", info->sname, *info->sline);
                    return FALSE;
                }
            }
        }
        else if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
            if(*info->err_num == 0) {
                sCLNodeType* initializer_code_type;
                sByteCode initializer_code;
                sVarTable* lv_table;
                int max_stack;
                char created_source[256];
                char* p;
                int field_index;
                char** p2;
                int sline;
                char* sname;

                set_field_index(info->klass->mClass, element_name, TRUE);

                field_index = get_field_index_including_super_classes(info->klass->mClass, element_name, TRUE);

                MASSERT(field_index != -1);

                initializer_code_type = NULL;
                lv_table = init_var_table();

                if(substitution_posibility(gIntType, info->klass)) {
                    if(NAMESPACE_NAME(info->klass->mClass)[0] == 0) {
                        snprintf(created_source, 256, "new %s(%d);", CLASS_NAME(info->klass->mClass), field_index);
                    }
                    else {
                        snprintf(created_source, 256, "new %s::%s(%d);", NAMESPACE_NAME(info->klass->mClass), CLASS_NAME(info->klass->mClass), field_index);
                    }
                }
                else if(substitution_posibility(gByteType, info->klass)) {
                    if(NAMESPACE_NAME(info->klass->mClass)[0] == 0) {
                        snprintf(created_source, 256, "new %s(%dy);", CLASS_NAME(info->klass->mClass), field_index);
                    }
                    else {
                        snprintf(created_source, 256, "new %s::%s(%dy);", NAMESPACE_NAME(info->klass->mClass), CLASS_NAME(info->klass->mClass), field_index);
                    }
                }
                else if(substitution_posibility(gShortType, info->klass)) {
                    if(NAMESPACE_NAME(info->klass->mClass)[0] == 0) {
                        snprintf(created_source, 256, "new %s(%ds);", CLASS_NAME(info->klass->mClass), field_index);
                    }
                    else {
                        snprintf(created_source, 256, "new %s::%s(%ds);", NAMESPACE_NAME(info->klass->mClass), CLASS_NAME(info->klass->mClass), field_index);
                    }
                }
                else if(substitution_posibility(gUIntType, info->klass)) {
                    if(NAMESPACE_NAME(info->klass->mClass)[0] == 0) {
                        snprintf(created_source, 256, "new %s(%du);", CLASS_NAME(info->klass->mClass), field_index);
                    }
                    else {
                        snprintf(created_source, 256, "new %s::%s(%du);", NAMESPACE_NAME(info->klass->mClass), CLASS_NAME(info->klass->mClass), field_index);
                    }
                }
                else if(substitution_posibility(gLongType, info->klass)) {
                    if(NAMESPACE_NAME(info->klass->mClass)[0] == 0) {
                        snprintf(created_source, 256, "new %s(%dl);", CLASS_NAME(info->klass->mClass), field_index);
                    }
                    else {
                        snprintf(created_source, 256, "new %s::%s(%dl);", NAMESPACE_NAME(info->klass->mClass), CLASS_NAME(info->klass->mClass), field_index);
                    }
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "Invalid enum element class.Enum should extend from int or byte or short or uint or long.");
                    (*info->err_num)++;
                }

                p = created_source;
                p2 = &p;

                sline = *info->sline;
                sname = info->sname;

                sByteCode_init(&initializer_code);
                if(!compile_field_initializer(&initializer_code, ALLOC &initializer_code_type, info->klass, p2, sname, &sline, info->err_num, info->current_namespace, lv_table, &max_stack)) 
                {
                    sByteCode_free(&initializer_code);
                    return FALSE;
                }

                if(!add_field_initializer(info->klass->mClass, TRUE, element_name, MANAGED initializer_code, lv_table, max_stack)) 
                {
                    parser_err_msg("overflow number fields", info->sname, *info->sline);
                    return FALSE;
                }
            }
        }

        if(**info->p == ',') {
           (*info->p)++;
           skip_spaces_and_lf(info->p, info->sline);
        }
        else if(**info->p == '}') {
           (*info->p)++;
           skip_spaces_and_lf(info->p, info->sline);
           break;
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "next character should be , or }. This is (%c).", **info->p);
            (*info->err_num)++;
        }
    }

    return TRUE;
}

static BOOL parse_enum(sParserInfo* info, BOOL private_, BOOL mixin_, BOOL native_, int parse_phase_num)
{
    char class_name[WORDSIZ];
    int mixin_version;

    mixin_version = -1;

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
    {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    info->klass = alloc_node_type(); // allocated
    
    info->klass->mClass = cl_get_class_with_argument_namespace_only(info->current_namespace, class_name, 0);

    MASSERT(info->klass->mClass != NULL || info->klass->mClass == NULL);

    switch(parse_phase_num) {
        case PARSE_PHASE_ALLOC_CLASSES:
            if(!allocate_new_class(class_name, info, private_, mixin_, FALSE, FALSE, FALSE, native_, parse_phase_num, FALSE, FALSE, TRUE, 0, mixin_version))
            {
                return FALSE;
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, FALSE, FALSE, TRUE, native_))
            {
                return FALSE;
            }

            break;

        case PARSE_PHASE_ADD_SUPER_CLASSES:
            MASSERT(info->klass->mClass != NULL);

            if(!skip_class_definition(info, parse_phase_num, mixin_, FALSE, FALSE, TRUE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_CALCULATE_SUPER_CLASSES:
            MASSERT(info->klass->mClass != NULL);

            if(!skip_class_definition(info, parse_phase_num, mixin_, FALSE, FALSE, TRUE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_GENERICS_TYPES:
            MASSERT(info->klass->mClass != NULL);

            if(!skip_class_definition(info, parse_phase_num, mixin_, FALSE, FALSE, TRUE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, FALSE, mixin_version))
            {
                return FALSE;
            }

            if(!parse_enum_element(info, mixin_, parse_phase_num, native_)) 
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_COMPILE_PARAM_INITIALIZER:
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, FALSE, mixin_version))
            {
                return FALSE;
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, FALSE, FALSE, TRUE, native_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            MASSERT(info->klass->mClass != NULL);

            /// incldue class modules ///
            if(!automatically_include_module_from_class(info, parse_phase_num, FALSE, mixin_version))
            {
                return FALSE;
            }

            if(!parse_enum_element(info, mixin_, parse_phase_num, native_)) 
            {
                return FALSE;
            }

            }
            break;
    }

    return TRUE;
}

static BOOL parse_namespace(sParserInfo* info, int parse_phase_num)
{
    char current_namespace_before[CL_NAMESPACE_NAME_MAX + 1];
    
    /// save namespace ///
    xstrncpy(current_namespace_before, info->current_namespace, CL_NAMESPACE_NAME_MAX);

    /// change namespace ///
    if(!change_namespace(info)) {
        return FALSE;
    }

    /// parse namespace ///
    while(**info->p) {
        BOOL private_;
        BOOL mixin_;
        BOOL abstract_;
        BOOL dynamic_typing_;
        BOOL final_;
        BOOL native_;
        char buf[WORDSIZ];

        if(**info->p == '}') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);
            break;
        }

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        private_ = FALSE;
        mixin_ = FALSE;
        abstract_ = FALSE;
        dynamic_typing_ = FALSE;
        final_ = FALSE;
        native_ = FALSE;

        while(**info->p) {
            if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "mixin") == 0) {
                mixin_ = TRUE;

                skip_spaces_and_lf(info->p, info->sline);

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "native") == 0) {
                native_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "abstract") == 0) {
                abstract_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "dynamic_typing") == 0) {
                dynamic_typing_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "final") == 0) {
                final_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
                {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else {
                break;
            }
        }

        if(strcmp(buf, "namespace") == 0) {
            if(private_ || mixin_ || abstract_ || dynamic_typing_ || final_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use namespace with \"private\" or \"mixin\" or \"abstract\" or \"dynamic_typing\" or \"final\" or \"native\"");
                (*info->err_num)++;
            }

            if(!parse_namespace(info, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "module") == 0) {
            if(private_ || mixin_ || abstract_ || dynamic_typing_ || final_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use module with \"private\" or \"mixin\" or \"abstract\" or \"dynamic_typing\" or \"final\" or \"native\"");
                (*info->err_num)++;
            }

            if(!parse_module(info, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(private_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use class with \"private\"");
                (*info->err_num)++;
            }
            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, final_, FALSE, native_, parse_phase_num, FALSE))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "struct") == 0) {
            if(private_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use struct with \"private\"");
                (*info->err_num)++;
            }
            if(native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use struct with \"native\"");
                (*info->err_num)++;
            }

            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, final_, TRUE, native_, parse_phase_num, FALSE))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "enum") == 0) {
            if(abstract_ || dynamic_typing_ || final_)
            {
                parser_err_msg_format(info->sname, *info->sline, "can't use enum with \"abstract\" or \"dynamic_typing\" or \"final\"");
                (*info->err_num)++;
            }

            if(!parse_enum(info, private_, mixin_, native_, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "interface") == 0) {
            if(abstract_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use interface with \"abstract\" or \"native\"");
                (*info->err_num)++;
            }

            if(!parse_class(info, private_, mixin_, FALSE, dynamic_typing_, final_, FALSE, FALSE, parse_phase_num, TRUE))
            {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "syntax error(%s). require \"class\" or \"interface\" or \"namespace\" or \"module\" or \"native\" keyword.", buf);
            return FALSE;
        }
    }
    
    /// restore namespace ///
    xstrncpy(info->current_namespace, current_namespace_before, CL_NAMESPACE_NAME_MAX);

    return TRUE;
}

static BOOL parse(sParserInfo* info, int parse_phase_num)
{
    char buf[WORDSIZ];
    BOOL private_;
    BOOL mixin_;
    BOOL abstract_;
    BOOL dynamic_typing_;
    BOOL final_;
    BOOL native_;

    skip_spaces_and_lf(info->p, info->sline);

    clear_method_index_of_compile_time();

    while(**info->p) {
        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        private_ = FALSE;
        mixin_ = FALSE;
        abstract_ = FALSE;
        dynamic_typing_ = FALSE;
        final_ = FALSE;
        native_ = FALSE;

        while(**info->p) {
            if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "mixin") == 0) {
                mixin_ = TRUE;

                skip_spaces_and_lf(info->p, info->sline);

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "native") == 0) {
                native_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "abstract") == 0) {
                abstract_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "dynamic_typing") == 0) {
                dynamic_typing_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else if(strcmp(buf, "final") == 0) {
                final_ = TRUE;

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);
            }
            else {
                break;
            }
        }

        if(strcmp(buf, "include") == 0) {
            if(private_ || mixin_ || abstract_ || final_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't include with \"private\" or \"mixin\" or \"abstract\" or \"final\" or \"native\"");
                (*info->err_num)++;
            }

            if(!include_file(info, parse_phase_num)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) {
            if(private_ || mixin_ || abstract_ || final_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use namespace with \"private\" or \"mixin\" or \"abstract\" or \"final\" or \"native\"");
                (*info->err_num)++;
            }

            if(!parse_namespace(info, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "module") == 0) {
            if(private_ || mixin_ || abstract_ || final_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use module with \"private\" or \"mixin\" or \"abstract\" or \"final\" or \"native\"");
                (*info->err_num)++;
            }

            if(!parse_module(info, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(private_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use class with \"private\"");
                (*info->err_num)++;
            }
            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, final_, FALSE, native_, parse_phase_num, FALSE))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "struct") == 0) {
            if(private_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use struct with \"private\"");
                (*info->err_num)++;
            }
            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, final_, TRUE, native_, parse_phase_num, FALSE)) 
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "interface") == 0) {
            if(abstract_ || native_) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use \"abstract\" or \"native\" prefix for interface");
                (*info->err_num)++;
            }

            if(!parse_class(info, private_, mixin_, FALSE, dynamic_typing_, final_, FALSE, FALSE, parse_phase_num, TRUE)) 
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "enum") == 0) {
            if(abstract_ || dynamic_typing_ || final_)
            {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't use enum with \"abstract\" or \"dynamic_typing\" or \"final\"");
                (*info->err_num)++;
            }

            if(!parse_enum(info, private_, mixin_, native_, parse_phase_num))
            {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "syntax error(%s). require \"class\" or \"include\" or \"namespace\" or \"module\" or \"native\" keyword.", buf);
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// compile
//////////////////////////////////////////////////
static BOOL save_code(sByteCode* code, sConst* constant, sVarTable* gv_table, int max_stack, char* sname)
{
    int f;
    char output_file_name[PATH_MAX];
    char* p;
    int len;
    char magic_number[16];
    int gv_var_num;
    int i;
    int n;

    /// make output file name ///
    xstrncpy(output_file_name, sname, PATH_MAX-3);
    xstrncat(output_file_name, ".o", PATH_MAX);

    /// write code ///
    f = open(output_file_name, O_WRONLY|O_TRUNC|O_CREAT, 0644);

    magic_number[0] = 'C';
    magic_number[1] = 'L';
    magic_number[2] = 'O';
    magic_number[3] = 'V';
    magic_number[4] = 'E';
    magic_number[5] = 'R';
    magic_number[6] = 11;
    magic_number[7] = 3;
    magic_number[8] = 55;
    magic_number[9] = 12;

    if(write(f, magic_number, 10) < 0) {
        close(f);
        return FALSE;
    }

    n = num_loaded_class();

    if(write(f, &n, sizeof(int)) < 0) {
        close(f);
        return FALSE;
    }

    for(i=0; i<num_loaded_class(); i++) {
        char* loaded_class;

        loaded_class = get_loaded_class(i);

        n = strlen(loaded_class) + 1;

        if(write(f, &n, sizeof(int)) < 0) {
            close(f);
            return FALSE;
        }

        if(write(f, loaded_class, n) < 0) {
            close(f);
            return FALSE;
        }
    }

    if(write(f, &code->mLen, sizeof(int)) < 0) {
        close(f);
        return FALSE;
    }

    if(write(f, code->mCode, sizeof(int)*code->mLen) < 0) {
        close(f);
        return FALSE;
    }

    if(write(f, &constant->mLen, sizeof(int)) < 0) {
        close(f);
        return FALSE;
    }

    if(write(f, constant->mConst, constant->mLen) < 0) {
        close(f);
        return FALSE;
    }

    gv_var_num = gv_table->mVarNum + gv_table->mMaxBlockVarNum;
    if(write(f, &gv_var_num, sizeof(int)) < 0) {
        close(f);
        return FALSE;
    }

    if(write(f, &max_stack, sizeof(int)) < 0) {
        close(f);
        return FALSE;
    }

    close(f);

    return TRUE;
}

static void init_included_modules_flags()
{
    int i;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                klass->mFlags &= ~CLASS_FLAGS_INCLUDED_MODULE;
                
                klass = klass->mNextClass;
            }
        }
    }
}

static BOOL compile_class_source(char* sname)
{
    int f;
    sBuf source;
    sBuf source2;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    char* p;
    int sline;
    int err_num;
    int i;

    f = open(sname, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", sname);
        return FALSE;
    }

    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size;

        size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    close(f);

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        return FALSE;
    }

    /// parse it PARSE_PHASE_MAX times
    for(i=1; i<PARSE_PHASE_MAX; i++) {
        sParserInfo info;

        gParsePhaseNum = i;

        init_included_modules_flags();

        memset(&info, 0, sizeof(info));

        *current_namespace = 0;

        p = source2.mBuf;

        sline = 1;
        err_num = 0;

        info.p = &p;
        info.sname = sname;
        info.sline = &sline;
        info.err_num = &err_num;
        info.current_namespace = current_namespace;
        if(!parse(&info, i)) {
            MFREE(source.mBuf);
            MFREE(source2.mBuf);
            return FALSE;
        }

        if(err_num > 0) {
            MFREE(source.mBuf);
            MFREE(source2.mBuf);
            return FALSE;
        }
    }

    /// save all modified classes ///
    save_all_modified_classes();

    /// save all modified modules ///
    save_all_modified_modules();

    MFREE(source.mBuf);
    MFREE(source2.mBuf);

    return TRUE;
}

static BOOL compile_script(char* sname, BOOL output_value)
{
    int f;
    sBuf source, source2;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    char* p;
    int sline;
    int err_num;
    int i;
    sByteCode code;
    sConst constant;
    int max_stack;
    sVarTable* gv_table;

    f = open(sname, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", sname);
        return FALSE;
    }

    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size;

        size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    close(f);

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        return FALSE;
    }

    /// do compile ///
    sByteCode_init(&code);
    sConst_init(&constant);
    gv_table = init_var_table();

    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!compile_statments(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table, output_value))
    {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    if(err_num > 0) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    /// write code to a file ///
    save_code(&code, &constant, gv_table, max_stack, sname);

    MFREE(source.mBuf);
    MFREE(source2.mBuf);
    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

//////////////////////////////////////////////////
// loaded class on compile time
//////////////////////////////////////////////////
static char* extname(char* file_name)
{
    char* p;

    p = file_name + strlen(file_name);

    while(p >= file_name) {
        if(*p == '.') {
            return p + 1;
        }
        else {
            p--;
        }
    }

    return NULL;
}

int main(int argc, char** argv)
{
    int i;
    BOOL load_fundamental_classes;
    BOOL output_value;
    BOOL no_delete_tmp_files;
    int option_num;
    int option_num2;
    int option_num3;
    char* basename_;
    int source_num;

    srandom((unsigned)time(NULL));

    load_fundamental_classes = TRUE;
    output_value = FALSE;
    no_delete_tmp_files = FALSE;
    option_num = -1;
    option_num2 = -1;
    option_num3 = -1;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--no-load-fundamental-classes") == 0) {
            load_fundamental_classes = FALSE;
            option_num = i;
        }
        else if(strcmp(argv[i], "--output-value") == 0) {
            output_value = TRUE;
            option_num2 = i;
        }
        else if(strcmp(argv[i], "--no-delete-tmp-files") == 0) {
            no_delete_tmp_files = TRUE;
            option_num3 = i;
        }
    }

    basename_ = basename(argv[0]);

    setlocale(LC_ALL, "");

    cl_compiler_init();

    if(!cl_init(1024, 512, argc, argv)) {
        exit(1);
    }

    if(no_delete_tmp_files) {
        setenv("CLOVER_NO_DELETE_TMP_FILES", "1", 1);
    }

    if(load_fundamental_classes) {
        if(!load_fundamental_classes_on_compile_time()) {
            fprintf(stderr, "can't load fundamental class\n");
            exit(1);
        }
    }

    source_num = 0;

    if(argc >= 2) {
        int i;
        for(i=1; i<argc; i++) {
            if(i != option_num && i != option_num2 && i != option_num3) {
                BOOL compile_class;
                char* extname_;
                char extention[PATH_MAX];

                if(source_num > 0) {
                    fprintf(stderr, "cclover can't compile one source file.\n");
                    exit(2);
                }

                extname_ = extname(argv[i]);

                if(extname_ != NULL && strcmp(extname_, "clc") == 0) {
                    setenv("SOURCE", argv[i], 1);

                    if(!compile_class_source(argv[i])) {
                        exit(1);
                    }
                }
                else {
                    setenv("SOURCE", argv[i], 1);

                    if(!compile_script(argv[i], output_value)) {
                        exit(1);
                    }
                }

                source_num++;
            }
        }
    }

    cl_final();
    cl_compiler_final();

    exit(0);
}

