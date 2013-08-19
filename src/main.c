#include "clover.h"
#include <stdio.h>
#include <stdlib.h>

static void set_env_vars()
{
    setenv("CLOVER_VERSION", "0.0.1", 1);
}

int main(int argc, char** argv) 
{
    CHECKML_BEGIN();

    set_env_vars();
    cl_init(1024, 1024, 1024, 512);
    cl_editline_init();

    while(1) {
        char* line = ALLOC editline("clover : ", NULL);

        if(line == NULL) {
            break;
        }

        int sline = 1;
        (void)cl_eval(line, "cmdline", &sline);

        if(line) { FREE(line); }
    }

    cl_editline_final();
    cl_final();

    CHECKML_END();

    exit(0);
}

