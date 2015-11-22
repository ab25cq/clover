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

int gParsePhaseNum = PARSE_PHASE_ADD_METHODS_AND_FIELDS;

static void cl_parser_init()
{
}

static void cl_parser_final()
{
}

static BOOL parser_get_type(char* sname) 
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
    sCLNodeType* type_;
    BOOL output_value;

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

    /// do compile ///
    sByteCode_init(&code);
    sConst_init(&constant);
    gv_table = init_var_table();

    *current_namespace = 0;

    p = source2.mBuf;

    sline = 1;
    err_num = 0;
    output_value = FALSE;
    type_ = NULL;
    if(!get_type_from_statment(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table, output_value, &type_))
    {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    if(type_) {
        show_node_type(type_);
        puts("");
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);
    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}
static void output_var_table(sVarTable* table)
{
    sVarTable* p;

    p = table;
    while(p) {
        int i;

        for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
            sVar* var;
            
            var = &p->mLocalVariables[i];

            if(var->mName[0] != 0) {
                printf("%s\n", var->mName);
            }
        }

        p = p->mParent;
    }
}

static BOOL parser_get_variable_names(char* sname) 
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
    sCLNodeType* type_;
    BOOL output_value;

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

    /// do compile ///
    sByteCode_init(&code);
    sConst_init(&constant);
    gv_table = init_var_table();

    *current_namespace = 0;

    p = source2.mBuf;

    gParserVarTable = NULL;
    sline = 1;
    err_num = 0;
    output_value = FALSE;
    type_ = NULL;
    if(!get_type_from_statment(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table, output_value, &type_))
    {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    if(gParserVarTable == NULL) {
        output_var_table(gv_table);
    }
    else {
        output_var_table(gParserVarTable);
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);
    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

static BOOL parser_get_class_type(char* sname) 
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
    sCLNodeType* type_;
    BOOL output_value;

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

    /// do compile ///
    sByteCode_init(&code);
    sConst_init(&constant);
    gv_table = init_var_table();

    *current_namespace = 0;

    p = source2.mBuf;

    gParserGetClassType = NULL;
    sline = 1;
    err_num = 0;
    output_value = FALSE;
    type_ = NULL;
    (void)get_type_from_statment(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table, output_value, &type_);

    if(gParserGetClassType && gParserGetClassType->mClass) {
        show_node_type(gParserGetClassType);
        puts("");
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);
    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

static BOOL parser_inputing_path(char* sname) 
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
    sCLNodeType* type_;
    BOOL output_value;

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

    /// do compile ///
    sByteCode_init(&code);
    sConst_init(&constant);
    gv_table = init_var_table();

    *current_namespace = 0;

    p = source2.mBuf;

    gParserInputingPath = FALSE;
    sline = 1;
    err_num = 0;
    output_value = FALSE;
    type_ = NULL;
    (void)get_type_from_statment(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, gv_table, output_value, &type_);

    if(gParserInputingPath) {
        puts("true");
    }
    else {
        puts("false");
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);
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
    char** argv2;
    int argc2;
    BOOL no_output;

    argv2 = CALLOC(1, sizeof(char*)*argc);
    argc2 = 0;
    no_output = FALSE;

    for(i=0; i<argc; i++) {
        if(strcmp(argv[i], "--no-output") == 0) {
            no_output = TRUE;
        }
        else {
            argv2[argc2] = STRDUP(argv[i]);
            argc2++;
        }
    }

    srandom((unsigned)time(NULL));
    setlocale(LC_ALL, "");

    cl_compiler_init();

    if(!cl_init(1024, 512)) {
        for(i=0; i<argc2; i++) {
            FREE(argv2[i]);
        }
        FREE(argv2);
        exit(1);
    }

    if(!load_fundamental_classes_on_compile_time()) {
        fprintf(stderr, "can't load fundamental class\n");
        for(i=0; i<argc2; i++) {
            FREE(argv2[i]);
        }
        FREE(argv2);
        exit(1);
    }

    if(no_output) {
        gParserOutput = FALSE;
    }

    if(argc2 > 2) {
        if(strcmp(argv2[1], "get_type") == 0) {
            char* file_name;

            file_name = argv2[2];

            if(!parser_get_type(file_name)) {
                if(!no_output) {
                    fprintf(stderr, "parser error\n");
                }
                for(i=0; i<argc2; i++) {
                    FREE(argv2[i]);
                }
                FREE(argv2);
                exit(2);
            }
        }
        else if(strcmp(argv2[1], "get_class_type") == 0) {
            char* file_name;

            file_name = argv2[2];

            if(!parser_get_class_type(file_name)) {
                if(!no_output) {
                    fprintf(stderr, "parser error\n");
                }
                for(i=0; i<argc2; i++) {
                    FREE(argv2[i]);
                }
                FREE(argv2);
                exit(2);
            }
        }
        else if(strcmp(argv2[1], "inputing_path") == 0) {
            char* file_name;

            file_name = argv2[2];

            if(!parser_inputing_path(file_name)) {
                if(!no_output) {
                    fprintf(stderr, "parser error\n");
                }
                for(i=0; i<argc2; i++) {
                    FREE(argv2[i]);
                }
                FREE(argv2);
                exit(2);
            }
        }
        else if(strcmp(argv2[1], "get_variable_names") == 0) {
            char* file_name;

            file_name = argv2[2];

            if(!parser_get_variable_names(file_name)) {
                if(!no_output) {
                    fprintf(stderr, "parser error\n");
                }
                for(i=0; i<argc2; i++) {
                    FREE(argv2[i]);
                }
                FREE(argv2);
                exit(2);
            }
        }
    }

    cl_final();
    cl_compiler_final();

    for(i=0; i<argc2; i++) {
        FREE(argv2[i]);
    }
    FREE(argv2);

    exit(0);
}

