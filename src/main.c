#include "clover.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>

static void set_env_vars()
{
    setenv("CLOVER_VERSION", "0.0.1", 1);
    setenv("CLOVER_DATAROOTDIR", DATAROOTDIR, 1);
}

static void version()
{
    set_env_vars();
    cl_print("Clover version %s. (c)Daisuke Minato 2013-2014\n\n", getenv("CLOVER_VERSION"));
    cl_print("--version output this message\n");
}

static void sig_int(int s)
{
    atexit_fun();
}

static void set_signal()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_int;
    if(sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("SITERM sigaction");
        exit(1);
    }
    if(sigaction(SIGINT, &sa, NULL) < 0) {
        perror("SIGINT sigaction");
        exit(1);
    }
fflush(stdout);
}

///////////////////////////////////////////////////
// main function
///////////////////////////////////////////////////
static void usage()
{
    printf("usage mfiler4 [-c command] [--version ] [ filer initial directory or script file]\n\n");

    printf("-c : eval a command on mfiler4\n");
    printf("--version : display mfiler4 version\n");

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
    if(!cl_init(1024, 512)) {
        exit(1);
    }

    if(!cl_load_fundamental_classes()) {
        fprintf(stderr, "can't load fundamental class\n");
        exit(1);
    }

    set_signal();

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

