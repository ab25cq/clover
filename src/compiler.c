#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>

enum eCompileType { kCompileTypeReffer, kCompileTypeInclude, kCompileTypeFile };

// for compile time parametor
typedef struct {
    char mRealClassName[CL_REAL_CLASS_NAME_MAX];

    unsigned char mNumDefinition;
    unsigned char mNumMethod;;
    enum eCompileType mCompileType;
} sCompileData;

static sCompileData gCompileData[CLASS_HASH_SIZE];

static sCompileData* get_compile_data(sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sCompileData* data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass));

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    data = gCompileData + hash;
    while(1) {
        if(data->mRealClassName[0] == 0) {
            return NULL;
        }
        else if(strcmp(data->mRealClassName, real_class_name) == 0) {
            return data;
        }
        else {
            data++;

            if(data == gCompileData + CLASS_HASH_SIZE) {
                data = gCompileData;
            }
            else if(data == gCompileData + hash) {
                return NULL;
            }
        }
    }
}

static BOOL add_compile_data(sCLClass* klass, char num_definition, unsigned char num_method, enum eCompileType compile_type)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sCompileData* data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass));

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    data = gCompileData + hash;
    while(data->mRealClassName[0] != 0) {
        data++;

        if(data == gCompileData + CLASS_HASH_SIZE) {
            data = gCompileData;
        }
        else if(data == gCompileData + hash) {
            return FALSE;
        }
    }

    xstrncpy(data->mRealClassName, real_class_name, CL_REAL_CLASS_NAME_MAX);
    data->mNumDefinition = num_definition;
    data->mNumMethod = num_method;
    data->mCompileType = compile_type;

    return TRUE;
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

static BOOL change_namespace(char** p, char* sname, int* sline, int* err_num, char* current_namespace)
{
    char buf[WORDSIZ];

    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    expect_next_character_with_one_forward(";", err_num, p, sname, sline);

    xstrncpy(current_namespace, buf, CL_NAMESPACE_NAME_MAX);

    return TRUE;
}

static BOOL delete_comment(sBuf* source, sBuf* source2);
static BOOL first_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type);
static BOOL second_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type);
static BOOL third_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type);

static BOOL do_reffer_file(char* fname)
{
    int f;
    sBuf source;
    sBuf source2;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    char* p;
    int sline;
    int err_num;

    f = open(fname, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "can't open %s\n", fname);
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

    /// 1st parse(include and reffer file. And alloc classes) ///
    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!first_parse(&p, fname, &sline, &err_num, current_namespace, kCompileTypeReffer)) {
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
    if(!second_parse(&p, fname, &sline, &err_num, current_namespace, kCompileTypeReffer)) {
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

    return TRUE;
}

static BOOL reffer_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL skip)
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

    if(!skip) {
        if(!do_reffer_file(file_name)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL do_include_file(char* sname, char* current_namespace)
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
        fprintf(stderr, "can't open %s\n", sname);
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

    /// 1st parse(include and reffer file. And alloc classes) ///
    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!first_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeInclude)) {
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
    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!second_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeInclude)) {
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
    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!third_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeInclude)) {
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

    return TRUE;
}

static BOOL include_file(char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL skip)
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

    if(!skip) {
        if(!do_include_file(file_name, current_namespace)) {
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse methods and fields
//////////////////////////////////////////////////
static void check_the_existance_of_this_method_before(sCLClass* klass, char* sname, int* sline, int* err_num, sCLNodeType class_params[], unsigned int num_params, BOOL static_, BOOL inherit_, sCLNodeType* type, char* name)
{
    sCompileData* compile_data;
    int method_index;
    sCLMethod* method_of_the_same_type;

    compile_data = get_compile_data(klass);
    ASSERT(compile_data != NULL);

    method_index = compile_data->mNumMethod;  // method index of this method
    method_of_the_same_type = get_method_with_type_params(klass, name, class_params, num_params, static_, NULL, method_index-1);

    if(method_of_the_same_type) {
        sCLNodeType result_type;

        if(!inherit_) {
            parser_err_msg_format(sname, *sline, "require \"inherit\" before the method definition because a method which has the same name and the same parametors on this class exists before");
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

static void check_compile_type(sCLClass* klass, char* sname, int* sline, int* err_num, BOOL compile_method_)
{
    if(compile_method_) {
        sCompileData* compile_data;

        compile_data = get_compile_data(klass);
        ASSERT(compile_data != NULL);

        if(compile_data->mCompileType == kCompileTypeReffer) {
            parser_err_msg_format(sname, *sline, "can't add method to the class which is reffered");
            (*err_num)++;
        }
    }
}

static BOOL parse_params(sCLNodeType* class_params, unsigned int* num_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sVarTable* lv_table)
{
    if(**p == ')') {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            sCLNodeType param_type;
            char param_name[CL_METHOD_NAME_MAX+1];

            memset(&param_type, 0, sizeof(param_type));

            /// class and generics types ///
            if(!parse_namespace_and_class_and_generics_type(&param_type, p, sname, sline, err_num, current_namespace, klass)) {
                return FALSE;
            }

            /// name ///
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(param_type.mClass) {
                class_params[*num_params] = param_type;
                (*num_params)++;

                if(!add_variable_to_table(lv_table, param_name, &param_type)) {
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

    return TRUE;
}

static BOOL parse_constructor(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL compile_method_, sCLNodeType* type, char* name, BOOL inherit_, BOOL static_)
{
    sVarTable lv_table;
    sCLNodeType self_type;
    sCLNodeType class_params[CL_METHOD_PARAM_MAX];
    unsigned int num_params;

    /// method ///
    memset(&lv_table, 0, sizeof(lv_table));

    memset(&self_type, 0, sizeof(sCLNodeType));
    self_type.mClass = klass;

    if(!add_variable_to_table(&lv_table, "self", &self_type)) {
        parser_err_msg("local variable table overflow", sname, *sline);
        return FALSE;
    }

    memset(class_params, 0, sizeof(class_params));
    num_params = 0;

    /// params ///
    if(!parse_params(class_params, &num_params, p, sname, sline, err_num, current_namespace, klass, &lv_table)) {
        return FALSE;
    }

    /// check that this method which is defined on refferd class and will be compiled ///
    check_compile_type(klass, sname, sline, err_num, compile_method_);

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(klass, sname, sline, err_num, class_params, num_params, static_, inherit_, type, name);

    if(!expect_next_character("{", err_num, p, sname, sline)) {
        return FALSE;
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    if(compile_method_) {
        sCompileData* data;
        int method_index;
        sCLMethod* method;

        data = get_compile_data(klass);
        ASSERT(data != NULL);

        method_index = data->mNumMethod++;

        method = klass->mMethods + method_index;

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
            if(!add_method(klass, FALSE, FALSE, FALSE, name, type, class_params, num_params, TRUE)) {
                parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                return FALSE;
            }
        }
    }

    skip_spaces_and_lf(p, sline);

    return TRUE;
}

static BOOL parse_method(char** p, sCLClass* klass, char* sname, int* sline, int* err_num, char* current_namespace, BOOL compile_method_, BOOL static_, BOOL private_, BOOL native_, BOOL inherit_, sCLNodeType* type, char* name)
{
    sVarTable lv_table;
    sCLNodeType class_params[CL_METHOD_PARAM_MAX];
    unsigned int num_params;
    sCLClass* klass2;
    sCLMethod* method_on_super_class;

    /// definitions ///
    memset(&lv_table, 0, sizeof(lv_table));

    if(!static_) {
        sCLNodeType self_type;

        memset(&self_type, 0, sizeof(sCLNodeType));
        self_type.mClass = klass;

        if(!add_variable_to_table(&lv_table, "self", &self_type)) {
            parser_err_msg("local variable table overflow", sname, *sline);
            return FALSE;
        }
    }

    memset(class_params, 0, sizeof(class_params));

    num_params = 0;

    /// params ///
    if(!parse_params(class_params, &num_params, p, sname, sline, err_num, current_namespace, klass, &lv_table)) {
        return FALSE;
    }

    /// check that this method which is defined on refferd class and will be compiled ///
    check_compile_type(klass, sname, sline, err_num, compile_method_);

    /// check the existance of a method which has the same name and the same parametors on this class ///
    check_the_existance_of_this_method_before(klass, sname, sline, err_num, class_params, num_params, static_, inherit_, type, name);

    /// check the existance of a method which has the same name and the same parametors on super classes ///
    method_on_super_class = get_method_with_type_params_on_super_classes(klass, name, class_params, num_params, &klass2, static_, NULL);

    if(method_on_super_class) {
        sCLNodeType result_type_of_method_on_super_class;

        memset(&result_type_of_method_on_super_class, 0, sizeof(result_type_of_method_on_super_class));
        if(!get_result_type_of_method(klass2, method_on_super_class, &result_type_of_method_on_super_class, NULL)) {
            parser_err_msg_format(sname, *sline, "the result type of this method(%s) is not found", name);
            (*err_num)++;
        }

        if(!type_identity(type, &result_type_of_method_on_super_class)) {
            parser_err_msg_format(sname, *sline, "can't override of this method because result type of this method(%s) is differ from the result type of  the method on the super class.", name);
            (*err_num)++;
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
            if(!add_method(klass, static_, private_, native_, name, type, class_params, num_params, FALSE)) {
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
            sCompileData* data;
            int method_index;
            sCLMethod* method;

            data = get_compile_data(klass);
            ASSERT(data != NULL);
            method_index = data->mNumMethod++;

            method = klass->mMethods + method_index;

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
                if(!add_method(klass, static_, private_, native_, name, type, class_params, num_params, FALSE)) {
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
        BOOL static_;
        BOOL private_;
        BOOL native_;
        BOOL inherit_;

        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// prefix ///
        static_ = FALSE;
        private_ = FALSE;
        native_ = FALSE;
        inherit_ = FALSE;

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
            else if(strcmp(buf, "inherit") == 0) {
                inherit_ = TRUE;

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
            char name[CL_METHOD_NAME_MAX+1];
            sCLNodeType type;

            if(static_ || native_ || private_ || inherit_) {
                parser_err_msg("don't append method type(\"static\" or \"native\" or \"private\" or \"inherit\")  to constructor", sname, *sline);
                (*err_num)++;
            }

            if(klass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
                parser_err_msg_format(sname, *sline, "immediate value class(%s) don't need constructor", CLASS_NAME(klass));
                (*err_num)++;
            }

            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            (*p)++;
            skip_spaces_and_lf(p, sline);

            xstrncpy(name, buf, CL_METHOD_NAME_MAX);

            memset(&type, 0, sizeof(type));
            type.mClass = klass;
            type.mGenericsTypesNum = 0;

            if(!parse_constructor(p, klass, sname, sline, err_num, current_namespace, compile_method_, &type, name, inherit_, static_)) {
                return FALSE;
            }
        }
        /// non constructor ///
        else {
            sCLNodeType type;
            char name[CL_METHOD_NAME_MAX+1];

            memset(&type, 0, sizeof(type));

            /// a second word ///
            if(**p == ':' && *(*p+1) == ':') {
                char buf2[128];

                (*p)+=2;

                if(!parse_word(buf2, 128, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                type.mClass = cl_get_class_with_argument_namespace_only(buf, buf2);

                if(type.mClass == NULL) {
                    parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", buf, buf2);
                    (*err_num)++;
                }

                if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass))
                {
                    return FALSE;
                }
            }
            else {
                int generics_type_num;

                /// is this generic type ? ///
                generics_type_num = get_generics_type_num(klass, buf);

                if(generics_type_num != -1) {
                    type.mClass = gAnonymousType[generics_type_num].mClass;
                }
                else {
                    type.mClass = cl_get_class_with_namespace(current_namespace, buf);

                    if(type.mClass == NULL) {
                        parser_err_msg_format(sname, *sline, "invalid class name(%s::%s)", current_namespace, buf);
                        (*err_num)++;
                    }

                    if(!parse_generics_types_name(p, sname, sline, err_num, &type.mGenericsTypesNum, type.mGenericsTypes, current_namespace, klass))
                    {
                        return FALSE;
                    }
                }
            }

            /// name ///
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

                if(!parse_method(p, klass, sname, sline, err_num, current_namespace, compile_method_, static_, private_, native_, inherit_, &type, name)) {
                    return FALSE;
                }
            }
            /// field ///
            else if(**p == ';') {
                if(native_ || inherit_) {
                    parser_err_msg("don't append type(\"native\" or \"inherit\")  to a field", sname, *sline);
                    (*err_num)++;
                }

                /// add field ///
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!compile_method_) {
                    sCompileData* data;
                    sCLClass* founded_class;
                    sCLField* field;

                    /// check that this is a open class ///
                    data = get_compile_data(klass);
                    ASSERT(data != NULL);

                    if(!compile_method_ && !(klass->mFlags & CLASS_FLAGS_OPEN) && data->mNumDefinition > 0) {
                        parser_err_msg("don't append field to non open class when the definition is multiple time.", sname, *sline);
                        (*err_num)++;
                    }

                    /// check that the same name field exists ///
                    field = get_field_including_super_classes(klass, name, &founded_class);
                    if(field) {
                        parser_err_msg_format(sname, *sline, "the same name field exists.(%s)", name);
                        (*err_num)++;
                    }

                    if(*err_num == 0) {
                        if(!add_field(klass, static_, private_, name, &type)) {
                            parser_err_msg("overflow number fields", sname, *sline);
                            return FALSE;
                        }
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
    sCLClass* super_class;

    /// extends or implements ///
    super_class = NULL;

    while(**p == 'e' || **p == 'i') {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "extends") == 0) {
            if(super_class == NULL) {
                /// get class ///
                if(!parse_namespace_and_class(&super_class, p, sname, sline, err_num, current_namespace, klass)) {
                    return FALSE;
                }

                if((klass->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS)) {
                    parser_err_msg("immediate value class can't extend from other class", sname, *sline);
                    (*err_num)++;
                }

                if((super_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS)) {
                    parser_err_msg("can't extend from immediate value class", sname, *sline);
                    (*err_num)++;
                }

                if(super_class && (super_class->mFlags & CLASS_FLAGS_OPEN)) {
                    parser_err_msg("can't extend from open class", sname, *sline);
                    (*err_num)++;
                }

                if(!skip && *err_num == 0) {
                    if(!add_super_class(klass, super_class)) {
                        parser_err_msg("Overflow number of super class.", sname, *sline);
                        return FALSE;
                    }
                }
            }
            else {
                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);

                parser_err_msg("A class can exntend one super class. Clover doesn't support multi-inheritance", sname, *sline);
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

    return TRUE;
}

static BOOL alloc_class_and_get_super_class(sCLClass* klass, char* class_name, char** p, char* sname, int* sline, int* err_num, char* current_namespace) 
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
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

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
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

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
        parser_err_msg_format(sname, *sline, "require { after class name. this is (%c)\n", **p);

        return FALSE;
    }

    return TRUE;
}

enum eParseType { kPCGetDefinition, kPCCompile, kPCAlloc };

static BOOL parse_generics_types_name_string(char** p, char* sname, int* sline, int* err_num, int* generics_types_num, char** generics_types)
{
    if(**p == '<') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        while(1) {
            if(!parse_word(generics_types[*generics_types_num], CL_CLASS_TYPE_VARIABLE_MAX, p, sname, sline, err_num)) {
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

static BOOL parse_class(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eParseType parse_type, BOOL private_, BOOL open_,  BOOL inherit_, enum eCompileType compile_type)
{
    char class_name[WORDSIZ];
    sCLClass* klass;
    int generics_types_num;
    char* generics_types[CL_GENERICS_CLASS_PARAM_MAX];
    int i;
    sCompileData* data;

    /// class name ///
    if(!parse_word(class_name, WORDSIZ, p, sname, sline, err_num)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    klass = cl_get_class_with_argument_namespace_only(current_namespace, class_name);

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

    switch((int)parse_type) {
        case kPCAlloc: {
            if(klass == NULL) {
                klass = alloc_class(current_namespace, class_name, private_, open_, generics_types, generics_types_num);

                if(!add_compile_data(klass, 0, 0, compile_type)) {
                    int i;

                    parser_err_msg("number of class data overflow", sname, *sline);
                    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                        FREE(generics_types[i]);
                    }
                    return FALSE;
                }
            }
            else {
                if(!inherit_) {
                    parser_err_msg_format(sname, *sline, "require \"inherit\" keyword for new version of class");
                    (*err_num)++;
                }
                if(private_ || open_) {
                    parser_err_msg_format(sname, *sline, "\"open\" or \"private\" should be defined on new class only");
                    (*err_num)++;
                }
            }

            if(!alloc_class_and_get_super_class(klass, class_name, p , sname, sline, err_num, current_namespace)) {
                int i;

                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            if(!check_super_class_offsets(klass)) {  // from klass.c
                parser_err_msg_format(sname, *sline, "invalid super class on %s", REAL_CLASS_NAME(klass));
                (*err_num)++;
            }
            break;

        case kPCGetDefinition:
            ASSERT(klass != NULL);

            if(!get_definition_from_class(klass, p , sname, sline, err_num, current_namespace)) {
                int i;

                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            data = get_compile_data(klass);
            ASSERT(data != NULL);

            if(data->mNumDefinition > NUM_DEFINITION_MAX) {
                parser_err_msg_format(sname, *sline, "overflow number of class definition(%s).", REAL_CLASS_NAME(klass));
                (*err_num)++;
            }
            data->mNumDefinition++;  // this is for check to be able to define fields. see methods_and_fields(...)
            break;

        case kPCCompile: {
            ASSERT(klass != NULL);

            if(!compile_class(p, klass, sname, sline, err_num, current_namespace)) {
                int i;
                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }

            increase_class_version(klass);

            if(CLASS_VERSION(klass) >= CLASS_VERSION_MAX) {
                parser_err_msg_format(sname, *sline, "overflow class version of this class(%s)", REAL_CLASS_NAME(klass));
                return FALSE;
            }

            klass->mFlags |= CLASS_FLAGS_MODIFIED;

/*
            if(!save_class(klass)) {
                int i;
                for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
                    FREE(generics_types[i]);
                }
                return FALSE;
            }
*/

            }
            break;
        }
    }

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        FREE(generics_types[i]);
    }
    return TRUE;
}

//////////////////////////////////////////////////
// 1st parse(include and reffer file)
//////////////////////////////////////////////////
static BOOL first_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type)
{
    char buf[WORDSIZ];
    BOOL open_;
    BOOL private_;
    BOOL inherit_;

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        open_ = FALSE;
        private_ = FALSE;
        inherit_ = FALSE;

        while(**p) {
            if(strcmp(buf, "open") == 0) {
                open_ = TRUE;

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
            else if(strcmp(buf, "inherit") == 0) {
                inherit_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

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
        else if(strcmp(buf, "include") == 0) { // do include file
            if(compile_type == kCompileTypeReffer) { // chage "include" to "reffer" from "reffer"
                if(!reffer_file(p, sname, sline, err_num, current_namespace, FALSE)) {
                    return FALSE;
                }
            }
            else {
                if(!include_file(p, sname, sline, err_num, current_namespace, FALSE)) {
                    return FALSE;
                }
            }
        }
        else if(strcmp(buf, "class") == 0) { // skip class definition
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCAlloc, private_, open_, inherit_, compile_type)) {
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
// 2nd parse(get definitions)
//////////////////////////////////////////////////
static BOOL second_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type)
{
    char buf[WORDSIZ];
    BOOL open_;
    BOOL private_;
    BOOL inherit_;

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        open_ = FALSE;
        private_ = FALSE;
        inherit_ = FALSE;

        while(**p) {
            if(strcmp(buf, "open") == 0) {
                open_ = TRUE;

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
            else if(strcmp(buf, "inherit") == 0) {
                inherit_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

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
        else if(strcmp(buf, "include") == 0) { // skip include file
            if(!include_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) { // get definitions
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCGetDefinition, private_, open_, inherit_, compile_type)) {
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
// 3rd parse(compile class)
//////////////////////////////////////////////////
static BOOL third_parse(char** p, char* sname, int* sline, int* err_num, char* current_namespace, enum eCompileType compile_type)
{
    char buf[WORDSIZ];
    BOOL open_;
    BOOL private_;
    BOOL inherit_;

    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }

        skip_spaces_and_lf(p, sline);

        open_ = FALSE;
        private_ = FALSE;
        inherit_ = FALSE;

        while(**p) {
            if(strcmp(buf, "open") == 0) {
                open_ = TRUE;

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
            else if(strcmp(buf, "inherit") == 0) {
                inherit_ = TRUE;

                if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                    return FALSE;
                }
                skip_spaces_and_lf(p, sline);
            }
            else {
                break;
            }
        }

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
        else if(strcmp(buf, "include") == 0) { // skip include file
            if(!include_file(p, sname, sline, err_num, current_namespace, TRUE)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {   // do compile class
            if(!parse_class(p, sname, sline, err_num, current_namespace, kPCCompile, private_, open_, inherit_, compile_type)) {
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
static BOOL delete_comment(sBuf* source, sBuf* source2)
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
    int f;
    sBuf source;
    sBuf source2;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];
    char* p;
    int sline;
    int err_num;

    f = open(sname, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "can't open %s\n", sname);
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

    /// 1st parse(include and reffer file. And alloc classes) ///
    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    if(!first_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeFile)) {
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
    if(!second_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeFile)) {
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
    err_num = 0;
    if(!third_parse(&p, sname, &sline, &err_num, current_namespace, kCompileTypeFile)) {
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

    /// save all modified class ///
    save_all_modified_class();

    return TRUE;
}

int main(int argc, char** argv)
{
    int i;
    BOOL load_foudamental_classes;
    int option_num;

    load_foudamental_classes = TRUE;
    option_num = -1;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--no-load-foundamental-classes") == 0) {
            load_foudamental_classes = FALSE;
            option_num = i;
        }
    }

    setlocale(LC_ALL, "");

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

