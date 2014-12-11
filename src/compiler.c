#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <libgen.h>

// 1st parse(alloc classes)
// 2nd parse(get methods and fields)
// 3rd parse(do compile code)
#define PARSE_PHASE_ALLOC_CLASSES 1
#define PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES 2
#define PARSE_PHASE_ADD_METHODS_AND_FIELDS 3
#define PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS 4
#define PARSE_PHASE_DO_COMPILE_CODE 5
#define PARSE_PHASE_MAX 6

// for compile time parametor
struct sClassCompileDataStruct {
    char mRealClassName[CL_REAL_CLASS_NAME_MAX];

    unsigned char mNumDefinition;
    unsigned char mNumMethod;;
    unsigned char mNumMethodOnLoaded;
    enum eCompileType mCompileType;
};

typedef struct sClassCompileDataStruct sClassCompileData;

static sClassCompileData gCompileData[CLASS_HASH_SIZE];

static sClassCompileData* get_compile_data(sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sClassCompileData* class_compile_data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass));

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    class_compile_data = gCompileData + hash;
    while(1) {
        if(class_compile_data->mRealClassName[0] == 0) {
            return NULL;
        }
        else if(strcmp(class_compile_data->mRealClassName, real_class_name) == 0) {
            return class_compile_data;
        }
        else {
            class_compile_data++;

            if(class_compile_data == gCompileData + CLASS_HASH_SIZE) {
                class_compile_data = gCompileData;
            }
            else if(class_compile_data == gCompileData + hash) {
                return NULL;
            }
        }
    }
}

BOOL add_compile_data(sCLClass* klass, char num_definition, unsigned char num_method, enum eCompileType compile_type)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sClassCompileData* class_compile_data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass));

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    class_compile_data = gCompileData + hash;
    while(class_compile_data->mRealClassName[0] != 0) {
        class_compile_data++;

        if(class_compile_data == gCompileData + CLASS_HASH_SIZE) {
            class_compile_data = gCompileData;
        }
        else if(class_compile_data == gCompileData + hash) {
            return FALSE;
        }
    }

    xstrncpy(class_compile_data->mRealClassName, real_class_name, CL_REAL_CLASS_NAME_MAX);
    class_compile_data->mNumDefinition = num_definition;
    class_compile_data->mNumMethod = num_method;
    class_compile_data->mNumMethodOnLoaded = num_method;
    class_compile_data->mCompileType = compile_type;

    return TRUE;
}

static void clear_compile_data()
{
    int i;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        sClassCompileData* class_compile_data = gCompileData + i;

        if(class_compile_data->mRealClassName[0] != 0)
        {
            class_compile_data->mNumDefinition = 0;
            if(class_compile_data->mCompileType == kCompileTypeLoad) {
                class_compile_data->mNumMethod = class_compile_data->mNumMethodOnLoaded;
            }
            else {
                class_compile_data->mNumMethod = 0;
            }
        }
    }
}

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
            parser_err_msg_format(sname, *sline, "It arrived at the end of source before block closing\n");
            return FALSE;
        }
        else {
            (*p)++;
        }
    }

    return TRUE;
}

static BOOL parse(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num);

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
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    /// 1st parse(alloc classes) ///
    p = source2.mBuf;

    sline = 1;
    err_num = 0;

    switch(parse_phase_num) {
    case PARSE_PHASE_ALLOC_CLASSES:
    case PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES:
    case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
    case PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS:
    case PARSE_PHASE_DO_COMPILE_CODE:
        info.p = &p;
        info.sname = sname;
        info.sline = &sline;
        info.err_num = &err_num;
        info.current_namespace = current_namespace;
        if(!parse(&info, kCompileTypeInclude, parse_phase_num)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }

        if(err_num > 0) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
        break;

    default:
        compile_error("unexpected err on do_include_file\n");
        exit(1);
    }


    FREE(source.mBuf);
    FREE(source2.mBuf);

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
    sClassCompileData* compile_data;
    int method_index;
    sCLMethod* method_of_the_same_type;

    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        sCLNodeType* result_type;

        compile_data = get_compile_data(klass->mClass);
        ASSERT(compile_data != NULL);

        method_index = compile_data->mNumMethod-1;  // method index of this method
        method_of_the_same_type = get_method_with_type_params(klass, name, class_params, num_params, static_, NULL, method_index-1, block_num, bt_num_params, bt_class_params, bt_result_type, ALLOC &result_type);

        if(method_of_the_same_type) {
            if(!mixin_) {
                parser_err_msg_format(sname, *sline, "require \"mixin\" before the method definition because a method which has the same name and the same parametors on this class exists before");
                (*err_num)++;
            }

            /// check the result type of these ///
            if(!type_identity(type, result_type)) {
                parser_err_msg_format(sname, *sline, "the result type of this method(%s) is differ from the result type of the method before", name);
                (*err_num)++;
            }
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
            if(!parse_namespace_and_class_and_generics_type(ALLOC bt_result_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass: NULL, info->method, FALSE))
            {
                return FALSE;
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

                    exception_class[*exception_num] = cl_get_class_with_namespace(info->current_namespace, buf);

                    if(exception_class[*exception_num] == NULL) {
                        exception_class[*exception_num] = load_class_with_namespace_on_compile_time(info->current_namespace, buf, TRUE);

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

struct sCLNodeGenericsParamTypesStruct {
    char mName[CL_CLASS_TYPE_VARIABLE_MAX];

    sCLNodeType* mExtendsType;

    char mNumImplementsTypes;
    sCLNodeType* mImplementsTypes[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];
};

typedef struct sCLNodeGenericsParamTypesStruct sCLNodeGenericsParamTypes;

static BOOL parse_constructor(sParserInfo* info, sCLNodeType* result_type, char* name, BOOL mixin_, BOOL native_, BOOL synchronized_, BOOL generics_newable, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top, BOOL interface)
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
    if(!parse_params_with_initializer(class_params, code_params, max_stack_params, lv_num_params, &num_params, CL_METHOD_PARAM_MAX, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, info->method, lv_table, ')', sline_top))
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

            add_method(info->klass->mClass, FALSE, FALSE, FALSE, native_, synchronized_, FALSE, FALSE, generics_newable, name, result_type, TRUE, info->method);

            if(!add_param_to_method(info->klass->mClass, class_params, code_params, max_stack_params, lv_num_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name))
            {
                parser_err_msg("overflow parametor number", info->sname, *info->sline);
                return FALSE;
            }
            for(i=0; i<exception_num; i++) {
                add_exception_class(info->klass->mClass, info->method, exception_class[i]);
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

                add_method(info->klass->mClass, FALSE, FALSE, FALSE, native_, synchronized_, FALSE, FALSE, generics_newable, name, result_type, TRUE, info->method);

                if(!add_param_to_method(info->klass->mClass, class_params, code_params, max_stack_params, lv_num_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name))
                {
                    parser_err_msg("overflow parametor number", info->sname, *info->sline);
                    return FALSE;
                }
                for(i=0; i<exception_num; i++) {
                    add_exception_class(info->klass->mClass, info->method, exception_class[i]);
                }
            }
            break;

        case PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS:
            expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

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

        if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS) {
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

        if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS) {
            if(!set_alias_flag_to_method(info->klass->mClass, buf)) {
                parser_err_msg_format(info->sname, sline_top, "this is not class method or overflow alais table or not found the method(%s)",buf);
                (*info->err_num)++;
            }
        }
    }

    return TRUE;
}

static BOOL parse_method(sParserInfo* info, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL mixin_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, sCLNodeType* result_type, char* name, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top, BOOL interface)
{
    sVarTable* lv_table;
    sCLNodeType* class_params[CL_METHOD_PARAM_MAX];
    int num_params;
    sByteCode code_params[CL_METHOD_PARAM_MAX];
    int max_stack_params[CL_METHOD_PARAM_MAX];
    int lv_num_params[CL_METHOD_PARAM_MAX];
    sCLNodeType* klass2;
    sCLMethod* method_on_super_class;
    BOOL block_num;
    char block_name[CL_VARIABLE_NAME_MAX];
    sCLNodeType* bt_result_type;
    sCLNodeType* bt_class_params[CL_METHOD_PARAM_MAX];
    int bt_num_params;
    sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX];
    int exception_num;
    sCLNodeType* result_type_of_super_class_method;
    int i;

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
    if(!parse_params_with_initializer(class_params, code_params, max_stack_params, lv_num_params, &num_params, CL_METHOD_PARAM_MAX, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, info->method, lv_table, ')', sline_top))
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
    method_on_super_class = get_method_with_type_params_on_super_classes(info->klass, name, class_params, num_params, &klass2, static_, NULL, block_num, bt_num_params, bt_class_params, bt_result_type, ALLOC &result_type_of_super_class_method);

    if(method_on_super_class) {
        if(!substitution_posibility(result_type_of_super_class_method, result_type)) 
        {
            parser_err_msg_format(info->sname, *info->sline, "can't override of this method because result type of this method(%s) is differ from the result type of the method on the super class.", name);
            cl_print("Requiring type is ");
            show_node_type(result_type_of_super_class_method);
            cl_print(". But this type is ");
            show_node_type(result_type);
            cl_print("\n");

            (*info->err_num)++;
        }

        /// check the override and virtual method types ///
        if((method_on_super_class->mFlags & CL_VIRTUAL_METHOD) && !virtual_) {
            parser_err_msg_format(info->sname, *info->sline, "require \"virtual\" type to this method because the method on the super class has \"virtual\" method type");
            (*info->err_num)++;
        }
        if((method_on_super_class->mFlags & CL_ABSTRACT_METHOD) && !virtual_) {
            parser_err_msg_format(info->sname, *info->sline, "require \"virtual\" type to this method because the method on the super class has \"abstract\" method type");
            (*info->err_num)++;
        }
    }

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
            add_method(info->klass->mClass, static_, private_, protected_, native_, synchronized_, virtual_, abstract_, FALSE, name, result_type, FALSE, info->method);

            if(!add_param_to_method(info->klass->mClass, class_params, code_params, max_stack_params, lv_num_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name))
            {
                parser_err_msg("overflow parametor number", info->sname, *info->sline);
                return FALSE;
            }
            for(i=0; i<exception_num; i++) {
                add_exception_class(info->klass->mClass, info->method, exception_class[i]);
            }
        }
    }
    else {
        switch(parse_phase_num) {
            case PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS:
                expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

                if(!skip_block(info->p, info->sname, info->sline)) {
                    return FALSE;
                }
                break;

            case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
                expect_next_character_with_one_forward("{", info->err_num, info->p, info->sname, info->sline);

                if(!skip_block(info->p, info->sname, info->sline)) {
                    return FALSE;
                }

                /// add method to class definition ///
                if(*info->err_num == 0) {
                    add_method(info->klass->mClass, static_, private_, protected_, native_, synchronized_, virtual_, abstract_, FALSE, name, result_type, FALSE, info->method);

                    if(!add_param_to_method(info->klass->mClass, class_params, code_params, max_stack_params, lv_num_params, num_params, info->method, block_num, block_name, bt_result_type, bt_class_params, bt_num_params, name))
                    {
                        parser_err_msg("overflow parametor number", info->sname, *info->sline);
                        return FALSE;
                    }
                    for(i=0; i<exception_num; i++) {
                        add_exception_class(info->klass->mClass, info->method, exception_class[i]);
                    }

                    for(i=0; i<exception_num; i++) {
                        add_exception_class(info->klass->mClass, info->method, exception_class[i]);
                    }
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

static BOOL add_fields(sParserInfo* info, sClassCompileData* class_compile_data, int parse_phase_num, BOOL static_ , BOOL private_, BOOL protected, char* name, sCLNodeType* result_type, BOOL initializer)
{
    sByteCode initializer_code;
    sVarTable* lv_table;
    int max_stack;

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
                parser_err_msg_format(info->sname, *info->sline, "type error");

                cl_print("left type is ");
                show_node_type(result_type);
                cl_print(". right type is ");
                show_node_type(initializer_code_type);
                puts("");

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

    if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS) {
    }
    else if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        sCLNodeType* founded_class;
        sCLField* field;
        sCLNodeType* field_type;

        /// check special class ///
        if(info->klass->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS || is_parent_special_class(info->klass->mClass))
        {
            parser_err_msg("can't append a field to the special class or a child class of special class", info->sname, *info->sline);
            (*info->err_num)++;
        }

        /// check that the same name field exists ///
        field = get_field_including_super_classes(info->klass, name, &founded_class, static_, &field_type, NULL);
        if(field) {
            parser_err_msg_format(info->sname, *info->sline, "the same name field exists.(%s)", name);
            (*info->err_num)++;
        }

        if(*info->err_num == 0) {
            if(!add_field(info->klass->mClass, static_, private_, protected, name, result_type)) {
                parser_err_msg("overflow number fields", info->sname, *info->sline);
                return FALSE;
            }
        }
    }
    else if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
        if(initializer) {
            if(!add_field_initializer(info->klass->mClass, static_, name, MANAGED initializer_code, lv_table, max_stack)) {
                parser_err_msg("overflow number fields", info->sname, *info->sline);
                return FALSE;
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


static BOOL parse_generics_param_types(sParserInfo* info, int* generics_param_types_num, sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX], int parse_phase_num)
{
    if(**info->p == '<') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        while(1) {
            if(!parse_word(generics_param_types[*generics_param_types_num].mName, CL_CLASS_TYPE_VARIABLE_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(parse_phase_num == PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES) {
                if(!add_generics_param_type_name(info->klass->mClass, generics_param_types[*generics_param_types_num].mName))
                {
                    parser_err_msg_format(info->sname, *info->sline, "overflow generics parametor types number or there is the same name of class parametor");
                    return FALSE;
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
                        generics_param_types[*generics_param_types_num].mImplementsTypes[num_implements_types] = alloc_node_type();

                        if(!parse_namespace_and_class_and_generics_type(&generics_param_types[*generics_param_types_num].mImplementsTypes[num_implements_types], info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, parse_phase_num != PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES)) 
                        {
                            return FALSE;
                        }
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
                    if(generics_param_types[*generics_param_types_num].mExtendsType != NULL)
                    {
                        parser_err_msg_format(info->sname, *info->sline, "Can't write \"extends\" keyword twice on a generics param type");
                        (*info->err_num)++;
                    }

                    generics_param_types[*generics_param_types_num].mExtendsType = alloc_node_type();

                    if(!parse_namespace_and_class_and_generics_type(&generics_param_types[*generics_param_types_num].mExtendsType, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, parse_phase_num != PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES)) 
                    {
                        return FALSE;
                    }
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "expected \"implements\" or \"extends\" keyword");
                    (*info->err_num)++;
                }
            }

            if(**info->p == 0) {
                parser_err_msg_format(info->sname, *info->sline, "It arrived at the end of source before > closing\n");
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

static BOOL init_method(sParserInfo* info, sClassCompileData* class_compile_data, int parse_phase_num)
{
    /// temporarily entried ///
    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS)
    {
        if(!create_method(info->klass->mClass, &info->method)) {
            parser_err_msg("overflow methods number", info->sname, *info->sline);
            return FALSE;
        }

        class_compile_data->mNumMethod++;
    }
    else {
        int method_index;

        /// get method pointer and index from sClassCompileData. ///
        method_index = class_compile_data->mNumMethod++;

        /// add generics parametor types to methods ///
        info->method = info->klass->mClass->mMethods + method_index;
    }

    return TRUE;
}

static BOOL methods_and_fields_and_alias(sParserInfo* info, sClassCompileData* class_compile_data, int parse_phase_num, BOOL interface)
{
    int i;
    char buf[WORDSIZ];

    while(**info->p != '}') {
        BOOL static_;
        BOOL private_;
        BOOL protected_;
        BOOL native_;
        BOOL mixin_;
        BOOL synchronized_;
        BOOL virtual_;
        BOOL generics_newable;
        BOOL abstract_;
        char* saved_p;
        int saved_sline;
        int generics_param_types_num;
        sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX];

        /// method scope generics ///
        generics_param_types_num = 0;
        memset(generics_param_types, 0, sizeof(generics_param_types));

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
        generics_newable = FALSE;

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
            else if(strcmp(buf, "synchronized") == 0) {
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
            else if(strcmp(buf, "generics_newable") == 0) {
                generics_newable = TRUE;

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
        if(strcmp(buf, CLASS_NAME(info->klass->mClass)) == 0 && **info->p == '(') {
            char name[CL_METHOD_NAME_MAX+1];
            sCLNodeType* result_type;

            if(static_ || private_ || protected_ || virtual_ || abstract_) {
                parser_err_msg("don't append method type(\"static\" or \"private\" or \"protected\" or \"mixin\" or \"virtual\" or \"abstract\") to constructor", info->sname, *info->sline);
                (*info->err_num)++;
            }

            if(generics_param_types_num == 0) {
                if(!init_method(info, class_compile_data, parse_phase_num))
                {
                    return FALSE;
                }
            }

            if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            xstrncpy(name, "_constructor", CL_METHOD_NAME_MAX);

            result_type = clone_node_type(info->klass);

            if(!parse_constructor(info, result_type, name, mixin_, native_, synchronized_, generics_newable, class_compile_data, parse_phase_num, *info->sline, interface))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "alias") == 0) {
            if(static_ || private_ || protected_ || native_ || mixin_  || synchronized_ || abstract_ || generics_newable) {
                parser_err_msg("don't append method type(\"static\" or \"private\" or \"protected\" or \"mixin\" or \"native\" or \"synchronized\" or \"abstract\" or \"generics_newable\") to alias", info->sname, *info->sline);
                (*info->err_num)++;
            }
            if(generics_param_types_num > 0) {
                parser_err_msg("can't append generics types to alias", info->sname, *info->sline);
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

            if(!parse_namespace_and_class_and_generics_type(ALLOC &result_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, (info->klass ? info->klass->mClass:NULL), info->method, FALSE)) 
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

                if(generics_newable)
                {
                    parser_err_msg("don't append constructor type(\"generics_newable\") to a method", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(generics_param_types_num == 0) {
                    if(!init_method(info, class_compile_data, parse_phase_num))
                    {
                        return FALSE;
                    }
                }

                if(!parse_method(info, static_, private_, protected_, native_, mixin_, synchronized_, virtual_, abstract_, result_type, name, class_compile_data, parse_phase_num, *info->sline, interface))

                {
                    return FALSE;
                }
            }
            /// field without initializer ///
            else if(**info->p == ';') {
                if(native_ || mixin_  || synchronized_ || virtual_ || abstract_ || generics_newable)
                {
                    parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"synchronized\" or \"virtual\" or \"abstract_\" or \"generics_newable\") to field", info->sname, *info->sline);
                    (*info->err_num)++;
                }
                if(generics_param_types_num > 0) {
                    parser_err_msg("can't append generics types to fields", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(interface) {
                    parser_err_msg("An interface can't define fields", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(!add_fields(info, class_compile_data, parse_phase_num, static_, private_, protected_, name, result_type, FALSE))
                {
                    return FALSE;
                }
            }
            /// field with initializer ///
            else if(**info->p == '=') {
                if(native_ || mixin_  || synchronized_ || virtual_ || abstract_ || generics_newable)
                {
                    parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"synchronized\" or \"virtual\" or \"abstract\" or \"generics_newable\") to field", info->sname, *info->sline);
                    (*info->err_num)++;
                }
                if(generics_param_types_num > 0) {
                    parser_err_msg("can't append generics types to fields", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(interface) {
                    parser_err_msg("An interface can't define fields", info->sname, *info->sline);
                    (*info->err_num)++;
                }

                if(!add_fields(info, class_compile_data, parse_phase_num, static_, private_, protected_, name, result_type, TRUE))
                {
                    return FALSE;
                }
            }
        }
    }

    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    return TRUE;
}

//////////////////////////////////////////////////
// parse class
//////////////////////////////////////////////////
static BOOL skip_namespace_and_class_and_generics_type(char** p, sParserInfo* info)
{
    sCLNodeType* dummy;

    if(!parse_namespace_and_class_and_generics_type(ALLOC &dummy, p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass: NULL, info->method, TRUE) )
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL extends_and_implements(sParserInfo* info, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_)
{
    char buf[WORDSIZ];
    sCLNodeType* super_class;
    BOOL no_super_class;

    /// extends or implements ///
    super_class = NULL;
    no_super_class = TRUE;

    while(**info->p == 'e' || **info->p == 'i') {
        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(strcmp(buf, "extends") == 0) {
            if(mixin_) {
                parser_err_msg("A class can't extend another class with mixin", info->sname, *info->sline);
                (*info->err_num)++;
            }

            if(super_class == NULL) {
                if(parse_phase_num == PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES) {
                    /// get class ///
                    if(!parse_namespace_and_class_and_generics_type(&super_class, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE)) 
                    {
                        return FALSE;
                    }

                    if(super_class->mClass) {
                        if(interface && !(super_class->mClass->mFlags & CLASS_FLAGS_INTERFACE))
                        {
                            parser_err_msg("An interface should extend another interface only, can't extend another class", info->sname, *info->sline);
                            (*info->err_num)++;
                        }

                        if(abstract_ && !(super_class->mClass->mFlags & CLASS_FLAGS_ABSTRACT)) {
                            parser_err_msg("An abstract class should extend another abstract class only", info->sname, *info->sline);
                            (*info->err_num)++;
                        }
                    }

                    if(*info->err_num == 0) {
                        if(!add_super_class(info->klass->mClass, super_class)) {
                            parser_err_msg("Overflow number of super class.", info->sname, *info->sline);
                            return FALSE;
                        }
                    }
                }
                else if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS && *info->err_num == 0) 
                {
                    /// get class ///
                    if(!parse_namespace_and_class_and_generics_type(&super_class, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE)) 
                    {
                        return FALSE;
                    }

                    if(super_class->mClass && !(info->klass->mClass->mFlags & CLASS_FLAGS_ABSTRACT) && (super_class->mClass->mFlags & CLASS_FLAGS_ABSTRACT) && !check_implemented_abstract_methods(info->klass)) { 
                        parser_err_msg_format(info->sname, *info->sline, "%s is not implemented abstract methods on the super class", REAL_CLASS_NAME(info->klass->mClass));
                        (*info->err_num)++;
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

                parser_err_msg("A class can exntend one super class. Clover doesn't support multi-inheritance", info->sname, *info->sline);
                (*info->err_num)++;
            }
        }
        else if(strcmp(buf, "implements") == 0) {
            if(interface) {
                parser_err_msg("An interface can't implement an interface", info->sname, *info->sline);
                (*info->err_num)++;
            }

            while(1) {
                sCLNodeType* interface;

                if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS && *info->err_num == 0)
                {
                    if(!parse_namespace_and_class_and_generics_type(&interface, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE))
                    {
                        return FALSE;
                    }

                    /// check the implement methods on the class ///
                    if(!check_implemented_interface(info->klass, interface)) {
                        parser_err_msg_format(info->sname, *info->sline, "%s is not implemented %s interface", REAL_CLASS_NAME(info->klass->mClass), REAL_CLASS_NAME(interface->mClass));
                        (*info->err_num)++;
                    }
                    else {
                        /// add the implement info to the class ///
                        if(!add_implemented_interface(info->klass->mClass, interface)) {
                            parser_err_msg_format(info->sname, *info->sline, "overflow implemented interface");
                            (*info->err_num)++;
                            return FALSE;
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
            parser_err_msg_format(info->sname, *info->sline, "clover expected \"extends\" or \"implements\" as next word, but this is \"%s\"\n", buf);
            (*info->err_num)++;
        }
    }

    if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS && *info->err_num == 0) 
    {
        ASSERT(gObjectType->mClass != NULL);

        if(no_super_class && !mixin_ && info->klass->mClass != gObjectType->mClass) 
        {
            if(!add_super_class(info->klass->mClass, gObjectType)) {
                parser_err_msg("Overflow number of super class.", info->sname, *info->sline);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL allocate_new_class(char* class_name, sParserInfo* info, BOOL private_, BOOL mixin_, BOOL abstract_, BOOL dynamic_typing_, enum eCompileType compile_type, int parse_phase_num, BOOL interface) 
{
    /// new difinition of class ///
    if(info->klass->mClass == NULL) {
        info->klass->mClass = alloc_class_on_compile_time(info->current_namespace, class_name, private_, abstract_, interface, dynamic_typing_);

        if(!add_compile_data(info->klass->mClass, 0, 0, compile_type)) {
            return FALSE;
        }

        if(mixin_) {
            parser_err_msg_format(info->sname, *info->sline, "require base class definition for mixin");
            (*info->err_num)++;
        }
    }
    /// version up of old class ///
    else {
        sClassCompileData* class_compile_data;
        
        class_compile_data = get_compile_data(info->klass->mClass);
        ASSERT(class_compile_data != NULL);

        if(!mixin_) {
            parser_err_msg_format(info->sname, *info->sline, "require \"mixin\" keyword for new version of class");
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
        if(dynamic_typing_) {
            parser_err_msg_format(info->sname, *info->sline, "\"dynamic_typing\" should be defined on new class only");
            (*info->err_num)++;
        }
    }

    return TRUE;
}

static BOOL skip_class_definition(sParserInfo* info, int parse_phase_num, BOOL mixin_, BOOL interface, BOOL abstract_) 
{
    /// extends or implements ///
    if(!extends_and_implements(info, mixin_, parse_phase_num, interface, abstract_)) {
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
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)\n", **info->p);

        return FALSE;
    }

    return TRUE;
}

static BOOL get_definition_from_class(sParserInfo* info, sClassCompileData* class_compile_data, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements(info, mixin_, parse_phase_num, interface, abstract_)) {
        return FALSE;
    }

    if(**info->p == '{') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!methods_and_fields_and_alias(info, class_compile_data, parse_phase_num, interface)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)\n", **info->p);

        return FALSE;
    }

    return TRUE;
}

static BOOL compile_class(sParserInfo* info, sClassCompileData* class_compile_data, BOOL mixin_, int parse_phase_num, BOOL interface, BOOL abstract_)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements(info, mixin_, parse_phase_num, interface, abstract_)) {
        return FALSE;
    }

    if(**info->p == '{') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!methods_and_fields_and_alias(info, class_compile_data, parse_phase_num, interface)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "require { after class name. this is (%c)\n", **info->p);

        return FALSE;
    }

    return TRUE;
}

static void check_constructor_type_for_generics_newable(sParserInfo* info)
{
    sCLNodeType* klass;
    sCLClass* super;
    int i;

    klass = info->klass;

    for(i=0; i<klass->mClass->mNumSuperClasses; i++) {
        sCLNodeType* super_class;
        int j;
        
        super_class = ALLOC create_node_type_from_cl_type(&klass->mClass->mSuperClasses[i], klass->mClass);

        ASSERT(super_class->mClass != NULL);     // checked on load time

        for(j=0; j<super_class->mClass->mNumMethods; j++) {
            sCLMethod* method;

            method = super_class->mClass->mMethods + j;

            if(strcmp(METHOD_NAME2(super_class->mClass, method), "_constructor") == 0 && method->mFlags & CL_GENERICS_NEWABLE_CONSTRUCTOR) {
                int k;
                BOOL flg;

                flg = FALSE;

                for(k=0; k<klass->mClass->mNumMethods; k++) {
                    sCLMethod* method2;

                    method2 = klass->mClass->mMethods + k;

                    if(strcmp(METHOD_NAME2(klass->mClass, method2), "_constructor") == 0 
                        && check_the_same_parametor_of_two_methods(super_class, method, klass, method2)) 
                    {
                        flg = TRUE;
                        break;
                    }
                }

                if(!flg) {
                    parser_err_msg_format(info->sname, *info->sline, "require definition of the constructor which has the same parametor of the \"generics_newable\" constructor on the super class for %s class",REAL_CLASS_NAME(klass->mClass));
                    show_method(super_class->mClass, method);
                    (*info->err_num)++;
                    break;
                }
            }
        }
    }
}

static BOOL parse_class(sParserInfo* info, BOOL private_, BOOL mixin_, BOOL abstract_, BOOL dynamic_typing_, enum eCompileType compile_type, int parse_phase_num, BOOL interface)
{
    char class_name[WORDSIZ];
    int generics_param_types_num;
    sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX];
    int i;
    sClassCompileData* class_compile_data;

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) 
    {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    info->klass = alloc_node_type(); // allocated

    info->klass->mClass = cl_get_class_with_argument_namespace_only(info->current_namespace, class_name);

    ASSERT(info->klass->mClass != NULL || info->klass->mClass == NULL);

    /// get class type variable ///
    generics_param_types_num = 0;
    memset(generics_param_types, 0, sizeof(generics_param_types));

    if(!parse_generics_param_types(info, &generics_param_types_num, generics_param_types, parse_phase_num))
    {
        return FALSE;
    }

    info->klass->mGenericsTypesNum = generics_param_types_num;
    for(i=0; i<generics_param_types_num; i++) {
        info->klass->mGenericsTypes[i] = gAnonymousType[i];
    }

    switch(parse_phase_num) {
        case PARSE_PHASE_ALLOC_CLASSES:
            if(!allocate_new_class(class_name, info, private_, mixin_, abstract_, dynamic_typing_, compile_type, parse_phase_num, interface)) 
            {
                return FALSE;
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_))
            {
                return FALSE;
            }

            class_compile_data = get_compile_data(info->klass->mClass);
            ASSERT(class_compile_data != NULL);
            break;

        case PARSE_PHASE_ADD_GENERICS_TYPES_ADD_SUPER_CLASSES:
            ASSERT(info->klass->mClass != NULL);

            class_compile_data = get_compile_data(info->klass->mClass);
            ASSERT(class_compile_data != NULL);

            if(*info->err_num == 0) {
                for(i=0; i<generics_param_types_num; i++) {
                    if(!add_generics_param_type(info->klass->mClass, generics_param_types[i].mName, generics_param_types[i].mExtendsType, generics_param_types[i].mNumImplementsTypes, generics_param_types[i].mImplementsTypes))
                    {
                        parser_err_msg_format(info->sname, *info->sline, "not found the parametor name(%s)", generics_param_types[i].mName);
                        return FALSE;
                    }
                }
            }

            if(!skip_class_definition(info, parse_phase_num, mixin_, interface, abstract_))
            {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            ASSERT(info->klass->mClass != NULL);

            class_compile_data = get_compile_data(info->klass->mClass);
            ASSERT(class_compile_data != NULL);

            if(!get_definition_from_class(info, class_compile_data, mixin_, parse_phase_num, interface, abstract_)) {
                return FALSE;
            }
            break;

        case PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS:
            ASSERT(info->klass->mClass != NULL);

            class_compile_data = get_compile_data(info->klass->mClass);
            ASSERT(class_compile_data != NULL);

            if(!get_definition_from_class(info, class_compile_data, mixin_, parse_phase_num, interface, abstract_)) {
                return FALSE;
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            ASSERT(info->klass->mClass != NULL);

            class_compile_data = get_compile_data(info->klass->mClass);
            ASSERT(class_compile_data != NULL);

            if(!compile_class(info, class_compile_data, mixin_, parse_phase_num, interface, abstract_)) {
                return FALSE;
            }

            /// version up ///
            increase_class_version(info->klass->mClass);

            if(CLASS_VERSION(info->klass->mClass) >= CLASS_VERSION_MAX) {
                parser_err_msg_format(info->sname, *info->sline, "overflow class version of this class(%s)", REAL_CLASS_NAME(info->klass->mClass));

                return FALSE;
            }

            /// A flag is setted for writing class to file ///
            info->klass->mClass->mFlags |= CLASS_FLAGS_MODIFIED;   // for save_all_modified_class() 
            }
            break;
    }

    if(class_compile_data->mNumDefinition > NUM_DEFINITION_MAX) {
        parser_err_msg_format(info->sname, *info->sline, "overflow number of class definition(%s).", REAL_CLASS_NAME(info->klass->mClass));
        (*info->err_num)++;
    }
    class_compile_data->mNumDefinition++;  // this is for check to be able to define fields. see methods_and_fields_and_alias(...)

    if(parse_phase_num == PARSE_PHASE_ADD_ALIASES_AND_IMPLEMENTS) {
        check_constructor_type_for_generics_newable(info);
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
        parser_err_msg_format(info->sname, *info->sline, "can't meke namespace nest\n");
        (*info->err_num)++;
    }

    return TRUE;
}

static BOOL parse_namespace(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num)
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
            else {
                break;
            }
        }

        if(strcmp(buf, "namespace") == 0) {
            if(private_ || mixin_ || abstract_ || dynamic_typing_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use namespace with \"private\" or \"mixin\" or \"abstract\" or \"dynamic_typing\"");
                (*info->err_num)++;
            }

            if(!parse_namespace(info, compile_type, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, compile_type, parse_phase_num, FALSE)) 
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "interface") == 0) {
            if(abstract_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use interface with \"abstract\"");
                (*info->err_num)++;
            }

            if(!parse_class(info, private_, mixin_, FALSE, dynamic_typing_, compile_type, parse_phase_num, TRUE)) 
            {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "syntax error(%s). require \"class\" or \"interface\" or \"namespace\" keyword.\n", buf);
            return FALSE;
        }
    }
    
    /// restore namespace ///
    xstrncpy(info->current_namespace, current_namespace_before, CL_NAMESPACE_NAME_MAX);

    return TRUE;
}

static BOOL parse(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num)
{
    char buf[WORDSIZ];
    BOOL private_;
    BOOL mixin_;
    BOOL abstract_;
    BOOL dynamic_typing_;

    skip_spaces_and_lf(info->p, info->sline);

    clear_compile_data();

    while(**info->p) {
        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        private_ = FALSE;
        mixin_ = FALSE;
        abstract_ = FALSE;
        dynamic_typing_ = FALSE;

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
            else {
                break;
            }
        }

        if(strcmp(buf, "include") == 0) {
            if(private_ || mixin_ || abstract_) {
                parser_err_msg_format(info->sname, *info->sline, "can't include with \"private\" or \"mixin\" or \"abstract\"");
                (*info->err_num)++;
            }

            if(!include_file(info, parse_phase_num)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) {
            if(private_ || mixin_ || abstract_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use namespace with \"private\" or \"mixin\" or \"abstract\"");
                (*info->err_num)++;
            }

            if(!parse_namespace(info, compile_type,parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(!parse_class(info, private_, mixin_, abstract_, dynamic_typing_, compile_type, parse_phase_num, FALSE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "interface") == 0) {
            if(abstract_) {
                parser_err_msg_format(info->sname, *info->sline, "can't use interface with \"abstract\"");
                (*info->err_num)++;
            }

            if(!parse_class(info, private_, mixin_, FALSE, dynamic_typing_, compile_type, parse_phase_num, TRUE)) {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(info->sname, *info->sline, "syntax error(%s). require \"class\" or \"include\" or \"namespace\" keyword.\n", buf);
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

static BOOL compile_script(char* sname)
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
        FREE(source.mBuf);
        FREE(source2.mBuf);
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
    if(!compile_statments(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table))
    {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    if(err_num > 0) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    /// write code to a file ///
    save_code(&code, &constant, gv_table, max_stack, sname);

    FREE(source.mBuf);
    FREE(source2.mBuf);
    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
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
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    /// parse it PARSE_PHASE_MAX times
    for(i=1; i<PARSE_PHASE_MAX; i++) {
        sParserInfo info;

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
        if(!parse(&info, kCompileTypeFile, i)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }

        if(err_num > 0) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
    }

    /// save all modified class ///
    save_all_modified_class();

    return TRUE;
}

//////////////////////////////////////////////////
// loaded class on compile time
//////////////////////////////////////////////////
static char* gLoadedClassOnCompileTime = NULL;
static int gSizeLoadedClassOnCompileTime;
static int gNumLoadedClassOnCompileTime;

static void compiler_init()
{
    gSizeLoadedClassOnCompileTime = 4;
    gLoadedClassOnCompileTime = CALLOC(1, CL_REAL_CLASS_NAME_MAX*gSizeLoadedClassOnCompileTime);
    gNumLoadedClassOnCompileTime = 0;

    init_vtable();
    init_node_types();
}

static void compiler_final()
{
    free_node_types();
    final_vtable();

    if(gLoadedClassOnCompileTime) {
        FREE(gLoadedClassOnCompileTime);
    }
}

static BOOL is_already_added_on_loaded_class_table(char* real_class_name)
{
    char* p = gLoadedClassOnCompileTime;

    while(p < gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gNumLoadedClassOnCompileTime)
    {
        if(strcmp(p, real_class_name) == 0) {
            return TRUE;
        }

        p += CL_REAL_CLASS_NAME_MAX;
    }

    return FALSE;
}

void add_loaded_class_to_table(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    if(is_already_added_on_loaded_class_table(real_class_name)) {
        return;
    }

    if(gNumLoadedClassOnCompileTime == gSizeLoadedClassOnCompileTime) {
        int new_size;
        
        new_size = gSizeLoadedClassOnCompileTime * 2;
        gLoadedClassOnCompileTime = REALLOC(gLoadedClassOnCompileTime, CL_REAL_CLASS_NAME_MAX*new_size);
        memset(gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gSizeLoadedClassOnCompileTime, 0, CL_REAL_CLASS_NAME_MAX*(new_size - gSizeLoadedClassOnCompileTime));
        gSizeLoadedClassOnCompileTime = new_size;
    }

    memcpy(gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gNumLoadedClassOnCompileTime, real_class_name, strlen(real_class_name)+1);
    gNumLoadedClassOnCompileTime++;
}

char* get_loaded_class(int index)
{
    return gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*index;
}

int num_loaded_class()
{
    return gNumLoadedClassOnCompileTime;
}

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
    int option_num;
    char* basename_;

    load_fundamental_classes = TRUE;
    option_num = -1;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--no-load-fundamental-classes") == 0) {
            load_fundamental_classes = FALSE;
            option_num = i;
        }
    }

    basename_ = basename(argv[0]);

    setlocale(LC_ALL, "");

    compiler_init();

    if(!cl_init(1024, 512)) {
        exit(1);
    }

    if(load_fundamental_classes) {
        if(!load_fundamental_classes_on_compile_time()) {
            fprintf(stderr, "can't load fundamental class\n");
            exit(1);
        }
    }

    if(argc >= 2) {
        int i;
        for(i=1; i<argc; i++) {
            if(i != option_num) {
                BOOL compile_class;
                char* extname_;
                char extention[PATH_MAX];

                extname_ = extname(argv[i]);

                if(extname_ != NULL && strcmp(extname_, "clc") == 0) {
                    if(!compile_class_source(argv[i])) {
                        exit(1);
                    }
                }
                else {
                    if(!compile_script(argv[i])) {
                        exit(1);
                    }
                }
            }
        }
    }

    cl_final();
    compiler_final();

    exit(0);
}

