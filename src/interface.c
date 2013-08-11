#include "clover.h"
#include <stdlib.h>
#include <stdio.h>

BOOL cl_eval(char* cmdline, char* sname, int* sline)
{
    sByteCode code;
    code.mSize = 1024;
    code.mLen = 0;
    code.mCode = MALLOC(sizeof(uchar)*code.mSize);

    sConst constant;
    constant.mSize = 1024;
    constant.mLen = 0;
    constant.mConst = MALLOC(sizeof(uchar)*constant.mSize);

    uint local_var_num;

    if(!cl_parse(cmdline, sname, sline, &code, &constant, &local_var_num, TRUE)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }
    if(!cl_main(&code, &constant)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }

    FREE(code.mCode);
    FREE(constant.mConst);

    return TRUE;
}
