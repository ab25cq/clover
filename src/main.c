#include "clover.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

static void set_env_vars()
{
    setenv("CLOVER_VERSION", "0.0.1", 1);
    setenv("CLOVER_DATAROOTDIR", DATAROOTDIR, 1);
}

static void version()
{
    set_env_vars();
    printf("Clover version %s. (c)Grou Yamanaka 2013-2013\n\n", getenv("CLOVER_VERSION"));
    printf("--create-clc Craete foundamental class\n");
    printf("--version output this message\n");
}

int main(int argc, char** argv) 
{
    CHECKML_BEGIN();

    char* script_file = NULL;

    int i;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "--help") == 0) {
            version();
            exit(0);
        }
        else {
            script_file = argv[i];
        }
    }

    setlocale(LC_ALL, "");

    set_env_vars();
    cl_init(1024, 1024, 1024, 512, TRUE);
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

