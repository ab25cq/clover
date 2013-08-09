#include "clover.h"
#include <stdio.h>
#include <stdlib.h>

static void set_env_vars()
{
    setenv("MCLOVER_VERSION", "0.0.1", 1);
}

int main(int argc, char** argv) 
{
    CHECKML_BEGIN();

    set_env_vars();
    cl_init();

//    clover_editline_init();
//    clover_editline_history_init();

    char* source = "1 + 2";
    int sline = 1;

    if(!cl_parse(source, "Hello World", &sline)) {
        exit(1);
    }

//    clover_editline_final();
    cl_final();

    CHECKML_END();

    exit(0);
}

