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
        snprintf(buf, WORDSIZ, "require word(alphabet or _ or number). this is (%c)\n", **p);
        parser_err_msg(buf, sname, *sline);

        (*err_num)++;

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

// characters is null-terminated
static BOOL expect_next_character(uchar* characters, int* err_num, char** p, char* sname, int* sline)
{
    BOOL err = FALSE;
    while(1) {
        if(**p == '0') {
            char buf[WORDSIZ];
            snprintf(buf, WORDSIZ, "clover has expected that next characters are '%s', but it arrived at source end", characters);
            parser_err_msg(buf, sname, *sline);
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
            (*p)++;
        }
    }

    if(err) {
        char buf[WORDSIZ];
        snprintf(buf, WORDSIZ, "clover has expected that next characters are '%s', but there are some characters before them", characters);
        parser_err_msg(buf, sname, *sline);
        (*err_num)++;
    }

    return TRUE;
}

static BOOL method_and_field_definition(char** p, char* buf, sCLClass* klass, char* sname, int* sline, int* err_num)
{
    while(**p != '}') {
        if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        /// prefix ///
        BOOL static_ = FALSE;
        BOOL private_ = FALSE;

        while(**p) {
            if(strcmp(buf, "static") == 0) {
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

            uint class_params[CL_METHOD_PARAM_MAX];
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
                        class_params[num_params] = klass->mConstPool.mLen;
                        sConst_append_str(&klass->mConstPool, CLASS_NAME(param_type));
                        num_params++;

                        if(num_params >= CL_METHOD_PARAM_MAX) {
                            parser_err_msg("overflow method parametor number", sname, *sline);
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
            if(type_) {
                sCLMethod* method = klass->mMethods + klass->mNumMethods;
                method->mHeader = (static_ ? CL_STATIC_METHOD:0) | (private_ ? CL_PRIVATE_METHOD:0);

                method->mNameOffset = klass->mConstPool.mLen;
                sConst_append_str(&klass->mConstPool, name);

                method->mPathOffset = klass->mConstPool.mLen;

                const int path_max = CL_METHOD_NAME_MAX + CL_CLASS_NAME_MAX;
                char buf[path_max];
                snprintf(buf, path_max, "%s.%s", CLASS_NAME(klass), name);
                sConst_append_str(&klass->mConstPool, buf);

                method->mResultType = klass->mConstPool.mLen;
                sConst_append_str(&klass->mConstPool, CLASS_NAME(type_));

                method->mParamTypes = CALLOC(1, sizeof(uint)*num_params);

                int i;
                for(i=0; i<num_params; i++) {
                    method->mParamTypes[i] = class_params[i];
                }
                method->mNumParams = num_params;

                klass->mNumMethods++;
                
                if(klass->mNumMethods >= CL_METHODS_MAX) {
                    parser_err_msg("overflow number methods", sname, *sline);
                    return FALSE;
                }
            }
        }
        /// field ///
        else if(**p == ';') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            if(type_) {
                sCLField* field = klass->mFields + klass->mNumFields;

                field->mHeader = (static_ ? CL_STATIC_FIELD:0) | (private_ ? CL_PRIVATE_FIELD:0);

                field->mNameOffset = klass->mConstPool.mLen;
                sConst_append_str(&klass->mConstPool, name);    // field name

                field->mClassNameOffset = klass->mConstPool.mLen;
                sConst_append_str(&klass->mConstPool, CLASS_NAME(type_));  // class name

                klass->mNumFields++;
                
                if(klass->mNumFields >= CL_FIELDS_MAX) {
                    parser_err_msg("overflow number methods", sname, *sline);
                    return FALSE;
                }
            }
        }
    }

    (*p)++;
    skip_spaces_and_lf(p, sline);

    return TRUE;
}

static BOOL parse_class(char* sname, int* sline);

static BOOL class_definition(char** p, char* buf, char* sname, int* sline, int* err_num)
{
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

            int sline = 1;
            if(!parse_class(file_name, &sline)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "class") == 0) {
            /// class name ///
            if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            sCLClass* klass = alloc_class_part(sizeof(sCLClass));
            klass->mConstPool.mSize = 1024;
            klass->mConstPool.mConst = CALLOC(1, sizeof(uchar)*klass->mConstPool.mSize);

            klass->mMethods = CALLOC(1, sizeof(sCLMethod)*CL_METHODS_MAX);
            klass->mFields = CALLOC(1, sizeof(sCLField)*CL_FIELDS_MAX);

            klass->mClassNameOffset = klass->mConstPool.mLen;
            sConst_append_str(&klass->mConstPool, buf);  // class name

            if(**p == '{') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!method_and_field_definition(p, buf, klass, sname, sline, err_num)) {
                    return FALSE;
                }
            }
            else {
                char buf2[WORDSIZ];
                snprintf(buf2, WORDSIZ, "require { after class name. this is (%c)\n", **p);
                parser_err_msg(buf2, sname, *sline);
                return FALSE;
            }

            /// added to class table ///
            uchar* class_name  = CONS_str(klass->mConstPool, klass->mClassNameOffset);
            const int hash = get_hash(class_name) % CLASS_HASH_SIZE;
            klass->mNextClass = gClassHashList[hash];
            gClassHashList[hash] = klass;
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

// source is null-terminated
static BOOL parse_class(char* sname, int* sline)
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

    skip_spaces_and_lf(&p, sline);

    int err_num = 0;
    if(!class_definition(&p, buf, sname, sline, &err_num)) {
        return FALSE;
    }

    if(err_num > 0) {
        return FALSE;
    }

/*
    /// do code compile ///
    if(!class_definition2(&p, buf, &klass, sname, sline)) {
        return FALSE;
    }
*/

    return TRUE;
}

int main(int argc, char** argv)
{
    cl_init(1024, 1024, 1024, 512, TRUE);
    if(argc >= 2) {
        int i;
        for(i=1; i<argc; i++) {
            int sline = 1;
            if(!parse_class(argv[i], &sline)) {
                exit(1);
            }
        }
sCLClass* klass = cl_get_class("MyClass");
show_class(klass);
klass = cl_get_class("MyClass2");
show_class(klass);
    }

    cl_final();

    exit(0);
}

