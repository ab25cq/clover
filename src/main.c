#include "clover.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>

static void version()
{
    printf("Clover version %s. (c)Daisuke Minato 2013-2015\n\n", getenv("CLOVER_VERSION"));
    printf("--version output this message\n");
}

///////////////////////////////////////////////////
// main function
///////////////////////////////////////////////////
static void usage()
{
    printf("usage clover [-c command] [--version ] [ filer initial directory or script file]\n\n");

    printf("-c : eval a command on clover\n");
    printf("--version : display clover version\n");

    exit(0);
}

#define SCRIPT_FILE_MAX 32

int main(int argc, char** argv) 
{
    char* script_file[SCRIPT_FILE_MAX];
    int num_script_file;
    int i;

    CHECKML_BEGIN
    memset(script_file, 0, sizeof(script_file));

    num_script_file = 0;

    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "--help") == 0) {
            if(!cl_init(1024, 512)) {
                exit(1);
            }
            version();
            cl_final();
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
    
    if(!cl_init(1024, 512)) {
        exit(1);
    }

    if(!cl_load_fundamental_classes()) {
        fprintf(stderr, "can't load fundamental class\n");
        exit(1);
    }

    if(!cl_call_runtime_method()) {
        fprintf(stderr, "Runtime method is faled\n");
        exit(1);
    }

    for(i=0; i<num_script_file; i++) {
        if(!cl_eval_file(script_file[i])) {
            fprintf(stderr, "script file(%s) is abort\n", script_file[i]);
            exit(1);
        }
    }

    cl_final();

    CHECKML_END

    exit(0);
}

