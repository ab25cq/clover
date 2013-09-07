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

    uint global_var_num;

    if(!cl_parse(cmdline, sname, sline, &code, &constant, &global_var_num, TRUE)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }
    if(!cl_main(&code, &constant, global_var_num)) {
        FREE(code.mCode);
        FREE(constant.mConst);
        return FALSE;
    }

    FREE(code.mCode);
    FREE(constant.mConst);

    return TRUE;
}
