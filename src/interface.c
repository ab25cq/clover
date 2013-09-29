#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>

BOOL cl_eval(char* cmdline, char* sname, int* sline)
{
    sByteCode code;
    sByteCode_init(&code);

    sConst constant;
    sConst_init(&constant);

    int max_stack = 0;
    int err_num = 0;
    if(!cl_parse(cmdline, sname, sline, &code, &constant, TRUE, &err_num, &max_stack)) {
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
