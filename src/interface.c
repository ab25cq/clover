#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static BOOL eval_statment(char** p, char* sname, int* sline, sVarTable* lv_table)
{
    sByteCode code;
    sConst constant;
    int max_stack;
    int err_num;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];

    sByteCode_init(&code);
    sConst_init(&constant);

    max_stack = 0;
    err_num = 0;
    *current_namespace = 0;

    if(!parse_statment(p, sname, sline, &code, &constant, &err_num, &max_stack, current_namespace, lv_table)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(err_num > 0) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(!cl_main(&code, &constant, lv_table->mVarNum + lv_table->mBlockVarNum, max_stack)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

BOOL cl_eval(char* cmdline, char* sname, int* sline)
{
    sBuf source, source2;
    char* p;

    sBuf_init(&source);
    sBuf_append(&source, cmdline, strlen(cmdline));

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    p = source2.mBuf;
    *sline = 1;

    while(*p) {
        if(!eval_statment(&p, sname, sline, &gGVTable)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

BOOL cl_eval_file(char* file_name)
{
    sBuf source, source2;
    int sline;
    int f;
    char* p;

    f = open(file_name, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", file_name);
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

    p = source2.mBuf;
    sline = 1;

    while(*p) {
        if(!eval_statment(&p, file_name, &sline, &gGVTable)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

sBuf* gCLPrintBuffer;

int cl_print(char* msg, ...)
{
    char msg2[CL_PRINT_BUFFER_MAX];
    int n;

    va_list args;
    va_start(args, msg);
    n = vsnprintf(msg2, CL_PRINT_BUFFER_MAX, msg, args);
    va_end(args);

    if(gCLPrintBuffer) {                            // this is hook of all clover output
        sBuf_append(gCLPrintBuffer, msg2, strlen(msg2));
    }
    else {
#ifdef VM_DEBUG
    vm_debug("%s", msg2);
#else
    printf("%s", msg2);
#endif
    }

    return n;
}

int cl_errmsg(char* msg, ...)
{
    char msg2[1024];
    int n;

    va_list args;
    va_start(args, msg);
    n = vsnprintf(msg2, 1024, msg, args);
    va_end(args);

#ifdef VM_DEBUG
    vm_debug("%s", msg2);
#else
    fprintf(stderr, "%s", msg2);
#endif

    return n;
}

void cl_puts(char* str)
{
    puts(str);
}

