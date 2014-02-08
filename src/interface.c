#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

BOOL cl_eval(char* cmdline, char* sname, int* sline)
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

    if(!cl_parse(cmdline, sname, sline, &code, &constant, TRUE, &err_num, &max_stack, current_namespace)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(err_num > 0) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(!cl_main(&code, &constant, gGVTable.mVarNum, max_stack)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

#define CL_SOURCE_LINE_MAX 256

static BOOL get_line(char* line, size_t line_size, char** p) 
{
    char* p2;

    p2 = line;
    while(**p) {
        if(**p == '\n') {
            *p2++ = **p;
            (*p)++;

            if(p2 - line >= line_size-1) {
                *line = 0;
                return FALSE;
            }

            *p2 = 0;

            return TRUE;
        }
        else {
            *p2++ = **p;
            (*p)++;

            if(p2 - line >= line_size-1) {
                *line = 0;
                return FALSE;
            }
        }
    }

    *p2 = 0;
    return TRUE;
}

BOOL cl_eval_file(char* file_name)
{
    sBuf source, source2;
    int sline;
    int f;
    char* p;
    char line[CL_SOURCE_LINE_MAX + 1];

    f = open(file_name, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "can't open %s\n", file_name);
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
        if(!get_line(line, CL_SOURCE_LINE_MAX, &p)) {
            fprintf(stderr, "overflow line length\n");
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
        if(!cl_eval(line, file_name, &sline)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

void cl_print(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

#ifdef VM_DEBUG
    vm_debug("%s", msg2);
#else
    printf("%s", msg2);
#endif
}
