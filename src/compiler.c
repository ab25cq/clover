#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>

// 1st parse(alloc classes)
// 2nd parse(get methods and fields)
// 3rd parse(do compile code)
#define PARSE_PHASE_ALLOC_CLASSES 1
#define PARSE_PHASE_ADD_METHODS_AND_FIELDS 2
#define PARSE_PHASE_DO_COMPILE_CODE 3

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

static BOOL parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type, int parse_phase_num);

static BOOL do_reffer_file(char* fname, int parse_phase_num)
{
    int f;
    sBuf source;
    sBuf source2;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    char* p;
    int sline;
    int err_num;

    if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
        return TRUE;
    }

    f = open(fname, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", fname);
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

    /// 1st parse(alloc classes) ///
    *current_namespace = 0;
    p = source2.mBuf;

    sline = 1;
    err_num = 0;

    switch(parse_phase_num) {
    case PARSE_PHASE_ALLOC_CLASSES:
    case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
        if(!parse(&p, fname, &sline, &err_num, current_namespace, kCompileTypeReffer, parse_phase_num)) {
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
        compile_error("unexpected err on do_reffer_file\n");
        exit(1);
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

static BOOL reffer_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, int parse_phase_num)
{
    char file_name[PATH_MAX];
    char* p2;

    if(**p != '"') {
        parser_err_msg("require \" after reffer", sname, *sline);
        return FALSE;
    }
    else {
        (*p)++;
    }

    p2 = file_name;
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

    expect_next_character_with_one_forward(";", err_num, p, sname, sline);

    if(!do_reffer_file(file_name, parse_phase_num)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL do_include_file(char* sname, char* current_namespace, int parse_phase_num)
{
    int f;
    sBuf source;
    char buf2[WORDSIZ];
    sBuf source2;
    char* p;
    int sline;
    int err_num;

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
    case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
    case PARSE_PHASE_DO_COMPILE_CODE:
        if(!parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeInclude, parse_phase_num)) {
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

static BOOL include_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, int parse_phase_num)
{
    char file_name[PATH_MAX];
    char* p2;

    if(**p != '"') {
        parser_err_msg("require \" after include", sname, *sline);
        return FALSE;
    }
    else {
        (*p)++;
    }

    p2 = file_name;
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
                parser_err_msg("too long file name to include", sname, *sline);
                return FALSE;
            }
        }
    }
    *p2 = 0;
    
    skip_spaces_and_lf(p, sline);

    expect_next_character_with_one_forward(";", err_num, p, sname, sline);

    if(!do_include_file(file_name, current_namespace, parse_phase_num)) {
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse methods and fields
//////////////////////////////////////////////////
static void check_the_existance_of_this_method_before(sCLClass* klass, char* sname, int* sline, int* err_num, sCLNodeType class_params[], int num_params, BOOL static_, BOOL mixin_, sCLNodeType* type, char* name, int block_num, int bt_num_params, sCLNodeType* bt_class_params, sCLNodeType* bt_result_type, int parse_phase_num)
{
    sClassCompileData* compile_data;
    int method_index;
    sCLMethod* method_of_the_same_type;

    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        compile_data = get_compile_data(klass);
        ASSERT(compile_data != NULL);

        method_index = compile_data->mNumMethod;  // method index of this method
        method_of_the_same_type = get_method_with_type_params(klass, name, class_params, num_params, static_, NULL, method_index-1, block_num, bt_num_params, bt_class_params, bt_result_type);


        if(method_of_the_same_type) {
            sCLNodeType result_type;

            if(!mixin_) {
                parser_err_msg_format(sname, *sline, "require \"mixin\" before the method definition because a method which has the same name and the same parametors on this class exists before");
                (*err_num)++;
            }

            /// check the result type of these ///
            memset(&result_type, 0, sizeof(result_type));
            if(!get_result_type_of_method(klass, method_of_the_same_type, &result_type, NULL)) {
                parser_err_msg_format(sname, *sline, "the result type of this method(%s) is not found", name);
                (*err_num)++;
            }

            if(!type_identity(type, &result_type)) {
                parser_err_msg_format(sname, *sline, "the result type of this method(%s) is differ from the result type of the method before", name);
                (*err_num)++;
            }
        }
    }
}

static void check_compile_type(sCLClass* klass, char* sname, int* sline, int* err_num, int parse_phase_num)
{
    if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
        sClassCompileData* compile_data;

        compile_data = get_compile_data(klass);
        ASSERT(compile_data != NULL);

        if(compile_data->mCompileType == kCompileTypeReffer) {
            parser_err_msg_format(sname, *sline, "can't add method to the class which is reffered");
            (*err_num)++;
        }
    }
}

static BOOL parse_declaration_of_method_block(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sVarTable* lv_table, char* block_name, sCLNodeType* bt_result_type, sCLNodeType* bt_class_params, int* bt_num_params, int size_bt_class_params, int sline_top, int* block_num)
{
    char buf[WORDSIZ];
    char* rewind;
    int sline_rewind;

    *block_num = 0;

    if(isalpha(**p)) {
        rewind = *p;
        sline_rewind = *sline;

        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "with") == 0) {
            *block_num = 1;

            memset(bt_result_type, 0, sizeof(sCLNodeType));

            /// get class ///
            if(!parse_namespace_and_class_and_generics_type(bt_result_type, p, sname, sline, err_num, current_namespace, klass->mClass))
            {
                return FALSE;
            }

            /// block name ///
            if(!parse_word(block_name, CL_VARIABLE_NAME_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(!add_variable_to_table(lv_table, block_name, &gBlockType)) {
                parser_err_msg("local variable table overflow", sname, *sline);
                return FALSE;
            }

            *bt_num_params = 0;
            expect_next_character_with_one_forward("(", err_num, p, sname, sline);
            /// params ///
            if(!parse_params(bt_class_params, bt_num_params, size_bt_class_params, p, sname, sline, err_num, current_namespace, klass->mClass, NULL, ')', sline_top)) {
                return FALSE;
            }
        }
        else {
            *p = rewind;
            *sline = sline_rewind;
        }
    }

    return TRUE;
}

static BOOL parse_throws(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX], int* exception_num)
{
    char buf[WORDSIZ];
    char* rewind;
    int sline_rewind;

    *exception_num = 0;

    if(isalpha(**p)) {
        rewind = *p;
        sline_rewind = *sline;

        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, FALSE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "throws") == 0) {
            while(**p) {
                if(isalpha(**p) || **p == '_') {
                    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, FALSE)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(p, sline);

                    exception_class[*exception_num] = cl_get_class_with_namespace(current_namespace, buf);

                    if(exception_class[*exception_num] == NULL) {
                        exception_class[*exception_num] = load_class_with_namespace_on_compile_time(current_namespace, buf, TRUE);

                        if(exception_class[*exception_num]) {
                            add_dependence_class(klass->mClass, exception_class[*exception_num]);
                        }
                        else {
                            parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", current_namespace, buf);
                            (*err_num)++;
                        }
                    }

                    (*exception_num)++;

                    if(**p == ',') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
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
                parser_err_msg_format(sname, *sline, "require exception class name");
                (*err_num)++;
            }
        }
        else {
            *p = rewind;
            *sline = sline_rewind;
        }
    }

    return TRUE;
}

static BOOL parse_constructor(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* result_type, char* name, BOOL mixin_, BOOL native_, BOOL synchronized_, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top)
{
    sVarTable* lv_table;
    sCLNodeType class_params[CL_METHOD_PARAM_MAX];
    int num_params;
    BOOL block_num;
    sCLNodeType bt_result_type;
    sCLNodeType bt_class_params[CL_METHOD_PARAM_MAX];
    int bt_num_params;
    char block_name[CL_VARIABLE_NAME_MAX];
    int method_index;
    sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX];
    int exception_num;

    lv_table = init_var_table();

    /// method ///
    if(!add_variable_to_table(lv_table, "self", klass)) {
        parser_err_msg("local variable table overflow", sname, *sline);
        return FALSE;
    }

    memset(class_params, 0, sizeof(class_params));
    num_params = 0;

    /// params ///
    if(!parse_params(class_params, &num_params, CL_METHOD_PARAM_MAX, p, sname, sline, err_num, current_namespace, klass->mClass, lv_table, ')', sline_top)) {
        return FALSE;
    }

    /// check that this method which is defined on refferd class and will be compiled ///
    check_compile_type(klass->mClass, sname, sline, err_num, parse_phase_num);

    /// get method pointer and index from sClassCompileData. This is only way to do so ///
    method_index = class_compile_data->mNumMethod++;

    /// method with block ///
    if(!parse_declaration_of_method_block(p, klass, sname, sline, err_num, current_namespace, lv_table, block_name, &bt_result_type, bt_class_params, &bt_num_params, CL_METHOD_PARAM_MAX, sline_top, &block_num)) {
        return FALSE;
    }

    /// throws ///
    if(!parse_throws(p, klass, sname, sline, err_num, current_namespace, exception_class, &exception_num)) 
    {
        return FALSE;
    }

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(klass->mClass, sname, sline, err_num, class_params, num_params, FALSE, mixin_, result_type, name, block_num, bt_num_params, bt_class_params, &bt_result_type, parse_phase_num);

    /// go ///
    if(native_) {
        if(!expect_next_character(";", err_num, p, sname, sline)) {
            return FALSE;
        }

        (*p)++;
        skip_spaces_and_lf(p, sline);

        /// add method to class definition ///
        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS && *err_num == 0) {
            sCLMethod* method;
             
            if(!add_method(klass->mClass, FALSE, FALSE, native_, synchronized_, name, result_type, class_params, num_params, TRUE, &method, block_num, block_name, &bt_result_type, bt_class_params, bt_num_params)) 
            {
                parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                return FALSE;
            }
        }
    }
    else {
        switch(parse_phase_num) {
        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            expect_next_character_with_one_forward("{", err_num, p, sname, sline);

            if(!skip_block(p, sname, sline)) {
                return FALSE;
            }

            /// add method to class definition ///
            if(*err_num == 0) {
                sCLMethod* method;

                if(!add_method(klass->mClass, FALSE, FALSE, native_, synchronized_, name, result_type, class_params, num_params, TRUE, &method, block_num, block_name, &bt_result_type, bt_class_params, bt_num_params)) 
                {
                    parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                    return FALSE;
                }
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            sCLMethod* method;
             
            method = klass->mClass->mMethods + method_index;

            if(!expect_next_character("{", err_num, p, sname, sline)) {
                return FALSE;
            }

            if(!compile_method(method, klass, p, sname, sline, err_num, lv_table, TRUE, current_namespace)) {
                return FALSE;
            }
            }
            break;

        default:
            compile_error("unexpected error on parse_constructor\n");
            exit(1);
        }
    }

    skip_spaces_and_lf(p, sline);

    return TRUE;
}

static BOOL parse_alias(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, int parse_phase_num, int sline_top)
{
    char buf[WORDSIZ];

    if(**p == '*') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!expect_next_character(";", err_num, p, sname, sline)) {
            return FALSE;
        }

        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
            if(!set_alias_flag_to_all_methods(klass->mClass)) {
                parser_err_msg_format(sname, *sline, "overflow alais table");
                return FALSE;
            }
        }
    }
    else {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(!expect_next_character(";", err_num, p, sname, sline)) {
            return FALSE;
        }

        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
            if(!set_alias_flag_to_method(klass->mClass, buf)) {
                parser_err_msg_format(sname, sline_top, "overflow alais table or not found the method(%s)",buf);
                (*err_num)++;
            }
        }
    }

    return TRUE;
}


static BOOL parse_method(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL static_, BOOL private_, BOOL native_, BOOL mixin_, BOOL synchronized_, sCLNodeType* result_type, char* name, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top)
{
    sVarTable* lv_table;
    sCLNodeType class_params[CL_METHOD_PARAM_MAX];
    int num_params;
    sCLClass* klass2;
    sCLMethod* method_on_super_class;
    int method_index;
    BOOL block_num;
    char block_name[CL_VARIABLE_NAME_MAX];
    sCLNodeType bt_result_type;
    sCLNodeType bt_class_params[CL_METHOD_PARAM_MAX];
    int bt_num_params;
    sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX];
    int exception_num;

    /// definitions ///
    lv_table = init_var_table();

    if(!static_) {
        if(!add_variable_to_table(lv_table, "self", klass)) {
            parser_err_msg("local variable table overflow", sname, *sline);
            return FALSE;
        }
    }

    memset(class_params, 0, sizeof(class_params));

    num_params = 0;

    /// params ///
    if(!parse_params(class_params, &num_params, CL_METHOD_PARAM_MAX, p, sname, sline, err_num, current_namespace, klass->mClass, lv_table, ')', sline_top)) 
    {
        return FALSE;
    }

    /// check that this method which is defined on refferd class and will be compiled ///
    check_compile_type(klass->mClass, sname, sline, err_num, parse_phase_num);

    /// get method pointer and index from sClassCompileData. This is only way to do so ///
    method_index = class_compile_data->mNumMethod++;

    /// method with block ///
    if(!parse_declaration_of_method_block(p, klass, sname, sline, err_num, current_namespace, lv_table, block_name, &bt_result_type, bt_class_params, &bt_num_params, CL_METHOD_PARAM_MAX, sline_top, &block_num)) {
        return FALSE;
    }

    /// throws ///
    if(!parse_throws(p, klass, sname, sline, err_num, current_namespace, exception_class, &exception_num)) 
    {
        return FALSE;
    }

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(klass->mClass, sname, sline, err_num, class_params, num_params, FALSE, mixin_, result_type, name, block_num, bt_num_params, bt_class_params, &bt_result_type, parse_phase_num);

    /// check the existance of a method which has the same name and the same parametors on super classes ///
    method_on_super_class = get_method_with_type_params_on_super_classes(klass->mClass, name, class_params, num_params, &klass2, static_, NULL, block_num, bt_num_params, bt_class_params, &bt_result_type);

    if(method_on_super_class) {
        sCLNodeType result_type_of_method_on_super_class;

        memset(&result_type_of_method_on_super_class, 0, sizeof(result_type_of_method_on_super_class));
        if(!get_result_type_of_method(klass2, method_on_super_class, &result_type_of_method_on_super_class, NULL)) {
            parser_err_msg_format(sname, *sline, "the result type of this method(%s) is not found", name);
            (*err_num)++;
        }

        if(!type_identity(result_type, &result_type_of_method_on_super_class)) {
            parser_err_msg_format(sname, *sline, "can't override of this method because result type of this method(%s) is differ from the result type of  the method on the super class.", name);
            (*err_num)++;
        }
    }

    /// go ///
    if(native_) {
        if(!expect_next_character(";", err_num, p, sname, sline)) {
            return FALSE;
        }

        (*p)++;
        skip_spaces_and_lf(p, sline);

        /// add method to class definition ///
        if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS && *err_num == 0) 
        {
            sCLMethod* method;
            int i;

            if(!add_method(klass->mClass, static_, private_, native_, synchronized_, name, result_type, class_params, num_params, FALSE, &method, block_num, block_name, &bt_result_type, bt_class_params, bt_num_params)) 
            {
                parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                return FALSE;
            }

            for(i=0; i<exception_num; i++) {
                add_exception_class(klass->mClass, method, exception_class[i]);
            }
        }
    }
    else {
        switch(parse_phase_num) {
            case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
                expect_next_character_with_one_forward("{", err_num, p, sname, sline);

                if(!skip_block(p, sname, sline)) {
                    return FALSE;
                }

                /// add method to class definition ///
                if(*err_num == 0) {
                    sCLMethod* method;
                    int i;

                    if(!add_method(klass->mClass, static_, private_, native_, synchronized_, name, result_type, class_params, num_params, FALSE, &method, block_num, block_name, &bt_result_type, bt_class_params, bt_num_params)) 
                    {
                        parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                        return FALSE;
                    }

                    for(i=0; i<exception_num; i++) {
                        add_exception_class(klass->mClass, method, exception_class[i]);
                    }
                }
                break;

            case PARSE_PHASE_DO_COMPILE_CODE: {
                sCLMethod* method;

                method = klass->mClass->mMethods + method_index;

                if(!expect_next_character("{", err_num, p, sname, sline)) {
                    return FALSE;
                }

                if(!compile_method(method, klass, p, sname, sline, err_num, lv_table, FALSE, current_namespace)) {
                    return FALSE;
                }
                }
                break;

            default:
                compile_error("un expected error on parse_method()\n");
                exit(1);
        }

        skip_spaces_and_lf(p, sline);
    }

    return TRUE;
}

static BOOL add_fields(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sClassCompileData* class_compile_data, int parse_phase_num, BOOL static_ , BOOL private_, char* name, sCLNodeType result_type, BOOL initializer)
{
    sByteCode initializer_code;
    sVarTable* lv_table;
    int max_stack;

    lv_table = init_var_table();

    /// add field ///
    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(initializer) {
        if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
            sCLNodeType initializer_code_type;

            sByteCode_init(&initializer_code);
            if(!compile_field_initializer(&initializer_code, &initializer_code_type, klass, p, sname, sline, err_num, current_namespace, lv_table, &max_stack)) {
                sByteCode_free(&initializer_code);
                return FALSE;
            }

            /// type checking ///
            if(!substition_posibility(&result_type, &initializer_code_type)) {
                parser_err_msg_format(sname, *sline, "type error");

                cl_print("left type is ");
                show_node_type(&result_type);
                cl_print(". right type is ");
                show_node_type(&initializer_code_type);
                puts("");

                (*err_num)++;
            }
        }
        else {
            if(!skip_field_initializer(p, sname, sline, current_namespace, klass, lv_table)) {
                return FALSE;
            }
        }
    }
    else {
        memset(&initializer_code, 0, sizeof(sByteCode));
        max_stack = 0;
    }

    if(parse_phase_num == PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        sCLClass* founded_class;
        sCLField* field;

        if(!(klass->mClass->mFlags & CLASS_FLAGS_OPEN) && class_compile_data->mNumDefinition > 0) 
        {
            parser_err_msg("can't append field to non open class when the definition is multiple time.", sname, *sline);
            (*err_num)++;
        }

        /// check special or immediate value class ///
        if(klass->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS || klass->mClass->mFlags & CLASS_FLAGS_SPECIAL_CLASS) 
        {
            parser_err_msg("can't append field to special classes and immediate value class", sname, *sline);
            (*err_num)++;
        }
        if(is_parent_special_class(klass->mClass) || is_parent_immediate_value_class(klass->mClass))
        {
            parser_err_msg("can't append field to a child class of a special class or an immediate class.", sname, *sline);
            (*err_num)++;
        }

        /// check that the same name field exists ///
        field = get_field_including_super_classes(klass->mClass, name, &founded_class, static_);
        if(field) {
            parser_err_msg_format(sname, *sline, "the same name field exists.(%s)", name);
            (*err_num)++;
        }

        if(*err_num == 0) {
            if(!add_field(klass->mClass, static_, private_, name, &result_type)) {
                parser_err_msg("overflow number fields", sname, *sline);
                return FALSE;
            }
        }
    }
    else if(parse_phase_num == PARSE_PHASE_DO_COMPILE_CODE) {
        if(initializer) {
            if(!add_field_initializer(klass->mClass, static_, name, MANAGED initializer_code, lv_table, max_stack)) {
                parser_err_msg("overflow number fields", sname, *sline);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL methods_and_fields_and_alias(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sClassCompileData* class_compile_data, int parse_phase_num)
{
    int i;
    char buf[WORDSIZ];

    while(**p != '}') {
        BOOL static_;
        BOOL private_;
        BOOL native_;
        BOOL mixin_;
        BOOL synchronized_;

        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// prefix ///
        static_ = FALSE;
        private_ = FALSE;
        native_ = FALSE;
        mixin_ = FALSE;
        synchronized_ = FALSE;

        while(**p) {
            if(strcmp(buf, "native") == 0) {
                native_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "synchronized") == 0) {
                synchronized_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "static") == 0) {
                static_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "mixin") == 0) {
                mixin_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

        /// constructor ///
        if(strcmp(buf, CLASS_NAME(klass->mClass)) == 0 && **p == '(') {
            char name[CL_METHOD_NAME_MAX+1];
            sCLNodeType result_type;

            if(static_ || private_ || mixin_) {
                parser_err_msg("don't append method type(\"static\" or \"private\" or \"mixin\") to constructor", sname, *sline);
                (*err_num)++;
            }

            if(klass->mClass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
                parser_err_msg_format(sname, *sline, "immediate value class(%s) don't need constructor", CLASS_NAME(klass->mClass));
                (*err_num)++;
            }

            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            (*p)++;
            skip_spaces_and_lf(p, sline);

            xstrncpy(name, buf, CL_METHOD_NAME_MAX);

            result_type = *klass;

            if(!parse_constructor(p, klass, sname, sline, err_num, current_namespace, &result_type, name, mixin_, native_, synchronized_, class_compile_data, parse_phase_num, *sline)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "alias") == 0) {
            if(static_ || private_ || native_ || mixin_  || synchronized_) {
                parser_err_msg("don't append method type(\"static\" or \"private\" or \"mixin\" or \"native\" or \"synchronized\") to alias", sname, *sline);
                (*err_num)++;
            }

            if(!parse_alias(p, klass, sname, sline, err_num, current_namespace, parse_phase_num, *sline)) {
                return FALSE;
            }
        }
        /// non constructor ///
        else {
            sCLNodeType result_type;
            char name[CL_METHOD_NAME_MAX+1];

            memset(&result_type, 0, sizeof(result_type));

            /// a second word ///
            if(**p == ':' && *(*p+1) == ':') {
                char buf2[128];

                (*p)+=2;

                if(!parse_word(buf2, 128, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                result_type.mClass = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(result_type.mClass == NULL) {
                    result_type.mClass = load_class_with_namespace_on_compile_time(buf, buf2, TRUE);

                    if(result_type.mClass) {
                        add_dependence_class(klass->mClass, result_type.mClass);
                    }
                    else {
                        parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", buf, buf2);
                        (*err_num)++;
                    }
                }

                if(!parse_generics_types_name(p, sname, sline, err_num, &result_type.mGenericsTypesNum, result_type.mGenericsTypes, current_namespace, klass->mClass))
                {
                    return FALSE;
                }
            }
            else {
                int generics_type_num;

                /// is this generic type ? ///
                generics_type_num = get_generics_type_num(klass->mClass, buf);

                if(generics_type_num != -1) {
                    result_type.mClass = gAnonymousType[generics_type_num].mClass;
                }
                else {
                    result_type.mClass = cl_get_class_with_namespace(current_namespace, buf);

                    if(result_type.mClass == NULL) {
                        result_type.mClass = load_class_with_namespace_on_compile_time(current_namespace, buf, TRUE);

                        if(result_type.mClass) {
                            add_dependence_class(klass->mClass, result_type.mClass);
                        }
                        else {
                            parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", current_namespace, buf);
                            (*err_num)++;
                        }
                    }

                    if(!parse_generics_types_name(p, sname, sline, err_num, &result_type.mGenericsTypesNum, result_type.mGenericsTypes, current_namespace, klass->mClass))
                    {
                        return FALSE;
                    }
                }
            }

            /// name ///
            if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(strcmp(name, "operator") == 0) {
                skip_spaces_and_lf(p, sline);

                if(**p == '[') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == ']') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        if(**p == '=') {
                            (*p)++;
                            skip_spaces_and_lf(p, sline);

                            xstrncpy(name, "[]=", CL_METHOD_NAME_MAX);
                        }
                        else {
                            xstrncpy(name, "[]", CL_METHOD_NAME_MAX);
                        }
                    }
                    else {
                        parser_err_msg_format(sname, *sline, "require ] after [ on operator []");
                        (*err_num)++;
                    }
                }
                else if(**p == '+') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '+') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "++", CL_METHOD_NAME_MAX);
                    }
                    else if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "+=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "+", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '-') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '-') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "--", CL_METHOD_NAME_MAX);
                    }
                    else if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "-=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "-", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '*') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "*=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "*", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '/') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "/=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "/", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '%') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "%=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "%", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '<') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '<') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        if(**p == '=') {
                            (*p)++;
                            skip_spaces_and_lf(p, sline);

                            xstrncpy(name, "<<=", CL_METHOD_NAME_MAX);
                        }
                        else {
                            xstrncpy(name, "<<", CL_METHOD_NAME_MAX);
                        }
                    }
                    else {
                        xstrncpy(name, "<", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '>') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '>') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        if(**p == '=') {
                            xstrncpy(name, ">>=", CL_METHOD_NAME_MAX);
                        }
                        else {
                            xstrncpy(name, ">>", CL_METHOD_NAME_MAX);
                        }
                    }
                    else if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, ">=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, ">", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '&') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "&=", CL_METHOD_NAME_MAX);
                    }
                    else if(**p == '&') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "&&", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "&", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '^') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "^=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "^", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '|') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "|=", CL_METHOD_NAME_MAX);
                    }
                    else if(**p == '|') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "||", CL_METHOD_NAME_MAX);
                    }
                    else {
                        xstrncpy(name, "|", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '!') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "!=", CL_METHOD_NAME_MAX);
                    }
                    else {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "!", CL_METHOD_NAME_MAX);
                    }
                }
                else if(**p == '~') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    xstrncpy(name, "~", CL_METHOD_NAME_MAX);
                }
                else if(**p == '=') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(**p == '=') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        xstrncpy(name, "==", CL_METHOD_NAME_MAX);
                    }
                    else {
                        parser_err_msg_format(sname, *sline, "can't define operator=");
                        (*err_num)++;
                    }
                }
                else {
                    parser_err_msg_format(sname, *sline, "invalid operator method (%c)", **p);
                    (*err_num)++;
                }
            }

            if(!expect_next_character("(;=", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// method ///
            if(**p == '(') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!parse_method(p, klass, sname, sline, err_num, current_namespace, static_, private_, native_, mixin_, synchronized_, &result_type, name, class_compile_data, parse_phase_num, *sline)) {
                    return FALSE;
                }
            }
            /// field without initializer ///
            else if(**p == ';') {
                if(native_ || mixin_  || synchronized_) {
                    parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"synchronized\") to field", sname, *sline);
                    (*err_num)++;
                }

                if(!add_fields(p, klass, sname, sline, err_num, current_namespace, class_compile_data, parse_phase_num, static_, private_, name, result_type, FALSE))
                {
                    return FALSE;
                }
            }
            /// field with initializer ///
            else if(**p == '=') {
                if(native_ || mixin_  || synchronized_) {
                    parser_err_msg("don't append field type(\"mixin\" or \"native\" or \"synchronized\") to field", sname, *sline);
                    (*err_num)++;
                }

                if(!add_fields(p, klass, sname, sline, err_num, current_namespace, class_compile_data, parse_phase_num, static_, private_, name, result_type, TRUE))
                {
                    return FALSE;
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
static BOOL extends_and_implements_and_imports(sCLClass* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL mixin_, int parse_phase_num)
{
    char buf[WORDSIZ];
    sCLClass* super_class;

    /// extends or implements ///
    super_class = NULL;

    while(**p == 'e' || **p == 'i') {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "extends") == 0) {
            if(mixin_) {
                parser_err_msg("can't use \"extends\" on mixin class", sname, *sline);
                (*err_num)++;
            }

            if(super_class == NULL) {
                /// get class ///
                if(!parse_namespace_and_class(&super_class, p, sname, sline, err_num, current_namespace, klass)) {
                    return FALSE;
                }

                if(super_class && (super_class->mFlags & CLASS_FLAGS_OPEN)) {
                    parser_err_msg("can't extend from open class", sname, *sline);
                    (*err_num)++;
                }

                if(parse_phase_num == PARSE_PHASE_ALLOC_CLASSES && *err_num == 0) {
                    if(!add_super_class(klass, super_class)) {
                        parser_err_msg("Overflow number of super class.", sname, *sline);
                        return FALSE;
                    }
                }
            }
            else {
                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                parser_err_msg("A class can exntend one super class. Clover doesn't support multi-mixinance", sname, *sline);
                (*err_num)++;
            }
        }
        else if(strcmp(buf, "implements") == 0) {
        }
        else {
            parser_err_msg_format(sname, *sline, "clover expected \"extends\" or \"implements\" as next word, but this is \"%s\"\n", buf);
            (*err_num)++;
        }
    }

    if(parse_phase_num == PARSE_PHASE_ALLOC_CLASSES && *err_num == 0) 
    {
        ASSERT(gObjectType.mClass != NULL);

        if(super_class == NULL && !mixin_ && !(klass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) && klass != gObjectType.mClass) 
        {
            if(!add_super_class(klass, gObjectType.mClass)) {
                parser_err_msg("Overflow number of super class.", sname, *sline);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL alloc_class_and_get_super_class(sCLNodeType* klass, char* class_name, char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL private_, BOOL open_, BOOL mixin_, enum eCompileType compile_type, int generics_types_num, char* generics_types[CL_GENERICS_CLASS_PARAM_MAX], int parse_phase_num) 
{
    /// new difinition of class ///
    if(klass->mClass == NULL) {
        klass->mClass = alloc_class(current_namespace, class_name, private_, open_, generics_types, generics_types_num);

        if(!add_compile_data(klass->mClass, 0, 0, compile_type)) {
            return FALSE;
        }
    }
    /// version up of old class ///
    else {
        if(!mixin_) {
            parser_err_msg_format(sname, *sline, "require \"mixin\" keyword for new version of class");
            (*err_num)++;
        }
        if(private_ || open_) {
            parser_err_msg_format(sname, *sline, "\"open\" or \"private\" should be defined on new class only");
            (*err_num)++;
        }
    }

    /// extends or implements ///
    if(!extends_and_implements_and_imports(klass->mClass, p, sname, sline, err_num, current_namespace, mixin_, parse_phase_num)) {
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
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

        return FALSE;
    }

    return TRUE;
}

static BOOL get_definition_from_class(sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sClassCompileData* class_compile_data, BOOL mixin_, int parse_phase_num)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements_and_imports(klass->mClass, p, sname, sline, err_num, current_namespace, mixin_, parse_phase_num)) {
        return FALSE;
    }

    if(**p == '{') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!methods_and_fields_and_alias(p, klass, sname, sline, err_num, current_namespace, class_compile_data, parse_phase_num)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

        return FALSE;
    }

    return TRUE;
}

static BOOL compile_class(char** p, sCLNodeType* klass, char* sname, int* sline, int* err_num, char* current_namespace, sClassCompileData* class_compile_data, BOOL mixin_, int parse_phase_num)
{
    char buf[WORDSIZ];

    /// extends or implements ///
    if(!extends_and_implements_and_imports(klass->mClass, p, sname, sline, err_num, current_namespace, mixin_, parse_phase_num)) {
        return FALSE;
    }

    if(**p == '{') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        if(!methods_and_fields_and_alias(p, klass, sname, sline, err_num, current_namespace, class_compile_data, parse_phase_num)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

        return FALSE;
    }

    return TRUE;
}

static BOOL parse_generics_types_name_string(char** p, char* sname, int* sline, int* err_num, int* generics_types_num, char** generics_types)
{
    if(**p == '<') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        while(1) {
            if(!parse_word(generics_types[*generics_types_num], CL_CLASS_TYPE_VARIABLE_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);
            (*generics_types_num)++;

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

static BOOL parse_class(char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL private_, BOOL open_, BOOL mixin_, enum eCompileType compile_type, int parse_phase_num)
{
    char class_name[WORDSIZ];
    sCLNodeType klass;
    int generics_types_num;
    char* generics_types[CL_GENERICS_CLASS_PARAM_MAX];
    int i;
    sClassCompileData* class_compile_data;

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, p, sname, sline, err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    klass.mClass = cl_get_class_with_argument_namespace_only(current_namespace, class_name);

    ASSERT(klass.mClass != NULL || klass.mClass == NULL);

    /// get class type variable ///
    generics_types_num = 0;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        generics_types[i] = MALLOC(CL_CLASS_TYPE_VARIABLE_MAX);
    }

    if(!parse_generics_types_name_string(p, sname, sline, err_num, &generics_types_num, generics_types)) {
        int i;
        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            FREE(generics_types[i]);
        }
        return FALSE;
    }

    klass.mGenericsTypesNum = generics_types_num;
    for(i=0; i<generics_types_num; i++) {
        klass.mGenericsTypes[i] = gAnonymousType[i].mClass;
    }

    switch(parse_phase_num) {
        case PARSE_PHASE_ALLOC_CLASSES: {
            if(!alloc_class_and_get_super_class(&klass, class_name, p , sname, sline, err_num, current_namespace, private_, open_, mixin_, compile_type, generics_types_num, generics_types, parse_phase_num)) 
            {
                int i;

                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            class_compile_data = get_compile_data(klass.mClass);
            ASSERT(class_compile_data != NULL);
            break;

        case PARSE_PHASE_ADD_METHODS_AND_FIELDS:
            ASSERT(klass.mClass != NULL);

            class_compile_data = get_compile_data(klass.mClass);
            ASSERT(class_compile_data != NULL);

            if(!get_definition_from_class(&klass, p , sname, sline, err_num, current_namespace, class_compile_data, mixin_, parse_phase_num)) {
                int i;

                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }
            break;

        case PARSE_PHASE_DO_COMPILE_CODE: {
            ASSERT(klass.mClass != NULL);

            class_compile_data = get_compile_data(klass.mClass);
            ASSERT(class_compile_data != NULL);

            if(!compile_class(p, &klass, sname, sline, err_num, current_namespace, class_compile_data, mixin_, parse_phase_num)) {
                int i;
                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            /// version up ///
            increase_class_version(klass.mClass);

            if(CLASS_VERSION(klass.mClass) >= CLASS_VERSION_MAX) {
                int i;
                parser_err_msg_format(sname, *sline, "overflow class version of this class(%s)", REAL_CLASS_NAME(klass.mClass));

                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            /// A flag is setted for writing class to file ///
            klass.mClass->mFlags |= CLASS_FLAGS_MODIFIED;   // for save_all_modified_class() 
            }
            break;
        }
    }

    if(class_compile_data->mNumDefinition > NUM_DEFINITION_MAX) {
        parser_err_msg_format(sname, *sline, "overflow number of class definition(%s).", REAL_CLASS_NAME(klass.mClass));
        (*err_num)++;
    }
    class_compile_data->mNumDefinition++;  // this is for check to be able to define fields. see methods_and_fields_and_alias(...)

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        FREE(generics_types[i]);
    }
    return TRUE;
}

static BOOL change_namespace(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    expect_next_character_with_one_forward("{", err_num, p, sname, sline);

    if(current_namespace[0] == 0) {
        if(!append_namespace_to_curernt_namespace(current_namespace, buf)) {
            return FALSE;
        }
    }
    else {
        parser_err_msg_format(sname, *sline, "can't meke namespace nest\n");
        (*err_num)++;
    }

    return TRUE;
}

static BOOL parse_namespace(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type, int parse_phase_num)
{
    char current_namespace_before[CL_NAMESPACE_NAME_MAX + 1];
    
    /// save namespace ///
    xstrncpy(current_namespace_before, current_namespace, CL_NAMESPACE_NAME_MAX);

    /// change namespace ///
    if(!change_namespace(p, sname, sline, err_num, current_namespace)) {
        return FALSE;
    }

    /// parse namespace ///
    while(**p) {
        BOOL open_;
        BOOL private_;
        BOOL mixin_;
        char buf[WORDSIZ];

        if(**p == '}') {
            (*p)++;
            skip_spaces_and_lf(p, sline);
            break;
        }

        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        open_ = FALSE;
        private_ = FALSE;
        mixin_ = FALSE;

        while(**p) {
            if(strcmp(buf, "open") == 0) {
                open_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "mixin") == 0) {
                mixin_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

        if(strcmp(buf, "namespace") == 0) {
            if(open_ || private_ || mixin_) {
                parser_err_msg_format(sname, *sline, "can't use namespace with \"open\" or \"private\" or \"mixin\"");
                (*err_num)++;
            }

            if(!parse_namespace(p, sname, sline, err_num, current_namespace, compile_type, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(!parse_class(p, sname, sline, err_num, current_namespace, private_, open_, mixin_, compile_type, parse_phase_num)) {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(sname, *sline, "syntax error(%s). require \"class\" keyword.\n", buf);
            return FALSE;
        }
    }
    
    /// restore namespace ///
    xstrncpy(current_namespace, current_namespace_before, CL_NAMESPACE_NAME_MAX);

    return TRUE;
}

static BOOL parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type, int parse_phase_num)
{
    char buf[WORDSIZ];
    BOOL open_;
    BOOL private_;
    BOOL mixin_;

    skip_spaces_and_lf(p, sline);

    clear_compile_data();

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        open_ = FALSE;
        private_ = FALSE;
        mixin_ = FALSE;

        while(**p) {
            if(strcmp(buf, "open") == 0) {
                open_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "private") == 0) {
                private_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else if(strcmp(buf, "mixin") == 0) {
                mixin_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

        if(strcmp(buf, "reffer") == 0) {
            if(open_ || private_ || mixin_) {
                parser_err_msg_format(sname, *sline, "can't reffer with \"open\" or \"private\" or \"mixin\"");
                (*err_num)++;
            }

            if(!reffer_file(p, sname, sline, err_num, current_namespace, parse_phase_num)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "namespace") == 0) {
            if(open_ || private_ || mixin_) {
                parser_err_msg_format(sname, *sline, "can't use namespace with \"open\" or \"private\" or \"mixin\"");
                (*err_num)++;
            }

            if(!parse_namespace(p, sname, sline, err_num, current_namespace, compile_type, parse_phase_num))
            {
                return FALSE;
            }
        }
        else if(strcmp(buf, "include") == 0) {
            if(open_ || private_ || mixin_) {
                parser_err_msg_format(sname, *sline, "can't include namespace with \"open\" or \"private\" or \"mixin\"");
                (*err_num)++;
            }

            if(compile_type == kCompileTypeReffer) { // change "include" to "reffer" when compile type is reffer
                if(!reffer_file(p, sname, sline, err_num, current_namespace, parse_phase_num)) {
                    return FALSE;
                }
            }
            else {
                if(!include_file(p, sname, sline, err_num, current_namespace, parse_phase_num)) {
                    return FALSE;
                }
            }
        }
        else if(strcmp(buf, "class") == 0) {
            if(!parse_class(p, sname, sline, err_num, current_namespace, private_, open_, mixin_, compile_type, parse_phase_num)) {
                return FALSE;
            }
        }
        else {
            parser_err_msg_format(sname, *sline, "syntax error(%s). require \"class\" or \"reffer\" or \"load\" or \"include\" or \"namespace\" keyword.\n", buf);
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

    /// 1st parse(alloc classes) ///
    /// 2nd parse(get methods and fields) ///
    /// 3rd parse(do compile code) ///

    for(i=1; i<=3; i++) {
        *current_namespace = 0;

        p = source2.mBuf;

        sline = 1;
        err_num = 0;
        if(!parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeFile, i)) {
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
}

static void compiler_final()
{
    final_vtable();

    if(gLoadedClassOnCompileTime) {
        FREE(gLoadedClassOnCompileTime);
    }
}

void add_loaded_class_to_table(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

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

int main(int argc, char** argv)
{
    int i;
    BOOL load_fundamental_classes;
    BOOL compile_class;
    int option_num;
    int option_num2;

    load_fundamental_classes = TRUE;
    compile_class = FALSE;
    option_num = -1;
    option_num2 = -1;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--no-load-fundamental-classes") == 0) {
            load_fundamental_classes = FALSE;
            option_num = i;
        }
        else if(strcmp(argv[i], "--script") == 0) {
            compile_class = FALSE;
            option_num2 = i;
        }
        else if(strcmp(argv[i], "--class") == 0) {
            compile_class = TRUE;
            option_num2 = i;
        }
    }

    setlocale(LC_ALL, "");

    if(!cl_init(1024, 512)) {
        exit(1);
    }

    if(load_fundamental_classes) {
        if(!load_fundamental_classes_on_compile_time()) {
            fprintf(stderr, "can't load fundamental class\n");
            exit(1);
        }
    }

    compiler_init();

    if(argc >= 2) {
        int i;
        for(i=1; i<argc; i++) {
            if(i != option_num && i != option_num2) {
                if(compile_class) {
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

