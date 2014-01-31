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

#define SCRIPT_FILE_MAX 32

int main(int argc, char** argv) 
{
    CHECKML_BEGIN();

    char* script_file[SCRIPT_FILE_MAX];
    memset(script_file, 0, sizeof(script_file));

    int num_script_file = 0;

    int i;
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "--help") == 0) {
            version();
            exit(0);
        }
        else {
            script_file[num_script_file++] = argv[i];

            if(num_script_file >= SCRIPT_FILE_MAX) {
                fprintf(stderr, "overflow script file number\n");
                exit(1);
            }
        }
    }

    setlocale(LC_ALL, "");

    set_env_vars();
    cl_init(1024, 1024, 1024, 512, TRUE);

    for(i=0; i<num_script_file; i++) {
        if(!cl_eval_file(script_file[i])) {
            fprintf(stderr, "script file(%s) is abort\n", script_file[i]);
            exit(1);
        }
    }

    cl_final();

    CHECKML_END();

    exit(0);
}

