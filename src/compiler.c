#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>

//////////////////////////////////////////////////
// resizable buf
//////////////////////////////////////////////////
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

static void sBuf_append(sBuf* self, void* str, size_t size)
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
// compile header
//////////////////////////////////////////////////
static BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num)
{
    buf[0] = 0;

    char* p2 = buf;

    if(isalpha(**p) || **p == '_') {
        while(isalnum(**p) || **p == '_') {
            if(p2 - buf < buf_size-1) {
                *p2++ = **p;
                (*p)++;
            }
            else {
                char buf[WORDSIZ];
                snprintf(buf, WORDSIZ, "length of word is too long");
                parser_err_msg(buf, sname, *sline);
                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(**p == 0) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "require word(alphabet or _ or number). this is the end of source");
        parser_err_msg(buf, sname, *sline);
        return FALSE;
    }

    if(buf[0] == 0) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "require word(alphabet or _ or number). this is (%c) (%d)\n", **p, **p);
        parser_err_msg(buf, sname, *sline);

        (*err_num)++;

        if(**p == '\n') (*sline)++;

        (*p)++;
    }

    return TRUE;
}

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

static BOOL get_definition_of_methods_and_fields(char** p, char* buf, sCLClass* klass, char* sname, int* sline, int* err_num)
{
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
            char name[CL_METHOD_NAME_MAX];
            xstrncpy(name, buf, CL_METHOD_NAME_MAX);

            static_ = FALSE;
            native_ = FALSE;
            private_ = FALSE;

            if(!expect_next_character("(", err_num, p, sname, sline)) {
                return FALSE;
            }

            /// method ///
            if(**p == '(') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

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
                        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);
                        
                        sCLClass* param_type = cl_get_class(buf);

                        if(param_type == NULL) {
                            char buf2[WORDSIZ];
                            snprintf(buf2, WORDSIZ, "There is no definition of this class(%s)\n", buf);
                            parser_err_msg(buf2, sname, *sline);
                            (*err_num)++;
                        }

                        /// name ///
                        char name[CL_METHOD_NAME_MAX];
                        if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);

                        if(param_type) {
                            class_params[num_params] = param_type;
                            num_params++;
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

                if(**p == '{') {
                    int sline2 = *sline;

                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    if(!skip_block(p, sname, sline)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(p, sline);
                }

                /// add method to class definition ///
                if(*err_num == 0) {
                    if(!add_method(klass, static_, private_, native_, name, klass, class_params, num_params)) {
                        parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                        return FALSE;
                    }
                }
            }
        }
        else {
            /// type ///
            sCLClass* type_ = cl_get_class(buf);

            if(type_ == NULL) {
                char buf2[WORDSIZ];
                snprintf(buf2, WORDSIZ, "There is no definition of this class(%s)\n", buf);
                parser_err_msg(buf2, sname, *sline);
                (*err_num)++;
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
                        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);
                        
                        sCLClass* param_type = cl_get_class(buf);

                        if(param_type == NULL) {
                            char buf2[WORDSIZ];
                            snprintf(buf2, WORDSIZ, "There is no definition of this class(%s)\n", buf);
                            parser_err_msg(buf2, sname, *sline);
                            (*err_num)++;
                        }

                        /// name ///
                        char name[CL_METHOD_NAME_MAX];
                        if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);

                        if(param_type) {
                            class_params[num_params] = param_type;
                            num_params++;
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

                    if(**p == ';') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
                    }

                    /// add method to class definition ///
                    if(*err_num == 0) {
                        if(!add_method(klass, static_, private_, native_, name, type_, class_params, num_params)) {
                            parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                            return FALSE;
                        }
                    }
                }
                else {
                    if(!expect_next_character("{", err_num, p, sname, sline)) {
                        return FALSE;
                    }

                    if(**p == '{') {
                        int sline2 = *sline;

                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        if(!skip_block(p, sname, sline)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);
                    }

                    /// add method to class definition ///
                    if(*err_num == 0) {
                        if(!add_method(klass, static_, private_, native_, name, type_, class_params, num_params)) {
                            parser_err_msg("overflow methods number or method parametor number", sname, *sline);
                            return FALSE;
                        }
                    }
                }
            }
            /// field ///
            else if(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(*err_num == 0) {
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

static BOOL reffer_file(char* sname);

static BOOL get_definition(char** p, char* buf, char* sname, int* sline, int* err_num)
{
    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }

        skip_spaces_and_lf(p, sline);

        if(strcmp(buf, "reffer") == 0) {
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
                        parser_err_msg("too long file name to require", sname, *sline);
                        return FALSE;
                    }
                }
            }
            *p2 = 0;
            
            skip_spaces_and_lf(p, sline);

            if(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }

            if(!reffer_file(file_name)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            /// class name ///
            if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            uchar* class_name = buf;

            sCLClass* klass = cl_get_class(class_name);

            if(klass == NULL) {
                klass = alloc_class(class_name);
            }

            if(**p == '{') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!get_definition_of_methods_and_fields(p, buf, klass, sname, sline, err_num)) {
                    return FALSE;
                }
            }
            else {
                char buf2[WORDSIZ];
                snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }
        }
        else {
            char buf2[WORDSIZ];
            snprintf(buf2, WORDSIZ, "syntax error(%s). require \"class\" or \"reffer\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL compile_class(char** p, char* buf, sCLClass* klass, char* sname, int* sline, int* err_num)
{
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
            char name[CL_METHOD_NAME_MAX];
            xstrncpy(name, buf, CL_METHOD_NAME_MAX);

            static_ = FALSE;
            native_ = FALSE;
            private_ = FALSE;

            /// method ///
            if(**p == '(') {
                sVarTable lv_table;
                memset(&lv_table, 0, sizeof(lv_table));

                if(!add_variable_to_table(&lv_table, "this", klass)) {
                    parser_err_msg("local variable table overflow", sname, *sline);
                    return FALSE;
                }

                (*p)++;
                skip_spaces_and_lf(p, sline);

                /// params ///
                if(**p == ')') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);
                }
                else {
                    while(1) {
                        /// type ///
                        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);
                        
                        sCLClass* param_type = cl_get_class(buf);

                        /// name ///
                        char name[CL_METHOD_NAME_MAX];
                        if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);

                        if(!add_variable_to_table(&lv_table, name, param_type)) {
                            parser_err_msg("local variable table overflow", sname, *sline);
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

                if(**p == '{') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);

                    uint method_index = get_method_index(klass, name);

                    ASSERT(method_index != -1); // be sure to be found

                    sCLMethod* method = klass->mMethods + method_index;

                    if(!compile_method(method, klass, p, sname, sline, err_num, &lv_table)) {
                        return FALSE;
                    }

                    method->mNumLocals = lv_table.mVarNum;

                    skip_spaces_and_lf(p, sline);
                }
            }
        }
        else {
            /// type ///
            sCLClass* type_ = cl_get_class(buf);

            /// name ///
            char name[CL_METHOD_NAME_MAX];
            if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            /// method ///
            if(**p == '(') {
                sVarTable lv_table;
                memset(&lv_table, 0, sizeof(lv_table));

                if(!static_) {
                    if(!add_variable_to_table(&lv_table, "this", klass)) {
                        parser_err_msg("local variable table overflow", sname, *sline);
                        return FALSE;
                    }
                }

                (*p)++;
                skip_spaces_and_lf(p, sline);

                /// params ///
                if(**p == ')') {
                    (*p)++;
                    skip_spaces_and_lf(p, sline);
                }
                else {
                    while(1) {
                        /// type ///
                        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);
                        
                        sCLClass* param_type = cl_get_class(buf);

                        /// name ///
                        char name[CL_METHOD_NAME_MAX];
                        if(!parse_word(name, CL_METHOD_NAME_MAX, p, sname, sline, err_num)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(p, sline);

                        if(!add_variable_to_table(&lv_table, name, param_type)) {
                            parser_err_msg("local variable table overflow", sname, *sline);
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
                    if(**p == ';') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
                    }
                }
                else {
                    if(**p == '{') {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);

                        uint method_index = get_method_index(klass, name);

                        ASSERT(method_index != -1); // be sure to be found

                        sCLMethod* method = klass->mMethods + method_index;

                        if(!compile_method(method, klass, p, sname, sline, err_num, &lv_table)) {
                            return FALSE;
                        }

                        method->mNumLocals = lv_table.mVarNum;

                        skip_spaces_and_lf(p, sline);
                    }
                }
            }
            /// field ///
            else if(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
        }
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    return TRUE;
}

static BOOL compile_file(char** p, char* buf, char* sname, int* sline, int* err_num)
{
    skip_spaces_and_lf(p, sline);

    while(**p) {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }

        skip_spaces_and_lf(p, sline);

        /// skip reffer ///
        if(strcmp(buf, "reffer") == 0) {
            (*p)++;

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
                    *p2++ = **p;
                    if(**p == '\n') (*p)++;
                    (*p)++;

                    if(p2 - file_name >= PATH_MAX-1) {
                        parser_err_msg("too long file name to require", sname, *sline);
                        return FALSE;
                    }
                }
            }
            *p2 = 0;
            
            skip_spaces_and_lf(p, sline);

            if(**p == ';') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
        }
        /// compile class ///
        else if(strcmp(buf, "class") == 0) {
            /// class name ///
            if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            sCLClass* klass = cl_get_class(buf);

            char clc_file_name[PATH_MAX];
            snprintf(clc_file_name, PATH_MAX, "%s.clc", buf);

            if(**p == '{') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!compile_class(p, buf, klass, sname, sline, err_num)) {
                    return FALSE;
                }

                /// Is this end of class definition ? ///
                int i;
                BOOL all_method_are_compiling = TRUE;
                for(i=0; i<klass->mNumMethods; i++) {
                    sCLMethod* method = klass->mMethods + i;
                    if(!(method->mHeader & CL_NATIVE_METHOD) && method->mByteCodes.mSize == 0) {
                        all_method_are_compiling = FALSE;
                        break;
                    }
                }

                if(all_method_are_compiling) {
                    if(!save_class(klass, clc_file_name)) {
                        return FALSE;
                    }
                }
            }
            else {
                char buf2[WORDSIZ];
                snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }
        }
        else {
            char buf2[WORDSIZ];
            snprintf(buf2, WORDSIZ, "syntax error(%s). require \"class\" or \"reffer\" keyword.\n", buf);
            parser_err_msg(buf2, sname, *sline);
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL reffer_file(char* sname)
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
    char buf[WORDSIZ];

    int sline = 1;
    int err_num = 0;
    if(!get_definition(&p, buf, sname, &sline, &err_num)) {
        return FALSE;
    }

    if(err_num > 0) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_definition_and_compile_file(char* sname)
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
    char buf[WORDSIZ];

    int sline = 1;
    int err_num = 0;
    if(!get_definition(&p, buf, sname, &sline, &err_num)) {
        return FALSE;
    }

    /// do code compile ///
    p = source.mBuf;

    sline = 1;
    if(!compile_file(&p, buf, sname, &sline, &err_num)) {
        return FALSE;
    }

    if(err_num > 0) {
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
                if(!get_definition_and_compile_file(argv[i])) {
                    exit(1);
                }
            }
        }
    }

    cl_final();

    exit(0);
}

