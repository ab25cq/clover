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

    int err_num = 0;
    if(!cl_parse(cmdline, sname, sline, &code, &constant, TRUE, &err_num)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }
    if(err_num > 0) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }
    if(!cl_main(&code, &constant, gGVTable.mVarNum)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }

    FREE(code.mCode);
    FREE(constant.mConst);

    return TRUE;
}
