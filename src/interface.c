#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>

/*
// result: (FALSE) there are errors (TRUE) success
// when result is success, output and err_output is allocaled as string
// if there are compile errors, a flag named compile_error is setted on TRUE
BOOL cl_compile(char** files, int num_files, BOOL* compile_error, ALLOC char** output, ALLOC char** err_output)
{
    pid_t pid;
    int nextout, nexterr;
    int pipeoutfds[2] = { -1, -1 };
    int pipeerrfds[2] = { -1 , -1};

    *compile_error = FALSE;

    if(pipe(pipeoutfds) < 0) {
        perror("pipe");
        return FALSE;
    }
    nextout = pipeoutfds[1];
    if(pipe(pipeerrfds) < 0) {
        perror("pipe");
        return FALSE;
    }
    nexterr = pipeerrfds[1];

    /// fork ///
    pid = fork();
    if(pid < 0) {
        perror("fork");
        return FALSE;
    }

    /// a child process ///
    if(pid == 0) {
        char** argv;
        int i;
        char buf[128];

        // a child process has a own process group
        pid = getpid();
        if(setpgid(pid,pid) < 0) {
            perror("setpgid(child)");
            return FALSE;
        }

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(child)");
            return FALSE;
        }
        sigttou_block(0);

        /// set environment variables ////
        snprintf(buf, 128, "%d", xgetmaxx());
        setenv("COLUMNS", buf, 1);

        snprintf(buf, 128, "%d", xgetmaxy());
        setenv("LINES", buf, 1);

        if(dup2(nextout, 1) < 0) {
            perror("dup2");
            return FALSE;
        }
        if(close(nextout) < 0) { return FALSE; }
        if(close(pipeoutfds[0]) < 0) { return FALSE; }

        if(dup2(nexterr, 2) < 0) {
            perror("dup2");
            return FALSE;
        }
        if(close(nexterr) < 0) { return FALSE; }
        if(close(pipeerrfds[0]) < 0) { return FALSE; }

        argv = malloc(sizeof(char*)*(num_files+3));

        argv[0] = "cclover";
        argv[1] = "--script-file";

        for(i=0; i<num_files; i++) {
            argv[i+2] = files[i];
        }

        argv[i+2] = NULL;

        execvp("cclover", argv);
        fprintf(stderr, "exec('%s') error\n", argv[0]);
        exit(127);
    }
    /// the parent process 
    else {
        int status;
        fd_set mask, read_ok;
        sBuf output_buf, err_output_buf;

        close(pipeoutfds[1]);
        close(pipeerrfds[1]);

        (void)setpgid(pid, pid);

        sigttou_block(1);
        if(tcsetpgrp(0, pid) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp(parent)");
            return FALSE;
        }
        sigttou_block(0);

        /// read output and error output ///
        sBuf_init(&output_buf);
        sBuf_init(&err_output_buf);

        FD_ZERO(&mask);
        FD_SET(pipeoutfds[0], &mask);
        FD_SET(pipeerrfds[0], &mask);

        while(1) {
            char buf[1024];
            int size;
            int size2;
            struct timeval tv = { 0, 1000 * 1000 / 100 };

            read_ok = mask;
                    
            if(select((pipeoutfds[0] > pipeerrfds[0] ? pipeoutfds[0] + 1:pipeerrfds[0] + 1), &read_ok, NULL, NULL, &tv) > 0) {
                if(FD_ISSET(pipeoutfds[0], &read_ok)) {
                    size = read(pipeoutfds[0], buf, 1024);
                    
                    if(size < 0 || size == 0) {
                        close(pipeoutfds[0]);
                    }

                    sBuf_append(&output_buf, buf, size);
                }
                if(FD_ISSET(pipeerrfds[0], &read_ok)) {
                    size = read(pipeerrfds[0], buf, 1024);
                    
                    if(size < 0 || size == 0) {
                        close(pipeoutfds[0]);
                    }

                    sBuf_append(&err_output_buf, buf, size);
                }
            }
            else {
                pid_t pid2;

                // wait everytime
                pid2 = waitpid(pid, &status, WUNTRACED|WNOHANG);

                if(pid2 == pid) {
                    /// last check ///
                    if(select((pipeoutfds[0] > pipeerrfds[0] ? pipeoutfds[0] + 1:pipeerrfds[0] + 1), &read_ok, NULL, NULL, &tv) > 0) {
                        if(FD_ISSET(pipeoutfds[0], &read_ok)) {
                            size = read(pipeoutfds[0], buf, 1024);
                            
                            if(size < 0 || size == 0) {
                                close(pipeoutfds[0]);
                            }

                            sBuf_append(&output_buf, buf, size);
                        }
                        if(FD_ISSET(pipeerrfds[0], &read_ok)) {
                            size = read(pipeerrfds[0], buf, 1024);
                            
                            if(size < 0 || size == 0) {
                                close(pipeoutfds[0]);
                            }

                            sBuf_append(&err_output_buf, buf, size);
                        }
                    }

                    break;
                }
            }
        }

        (void)close(pipeoutfds[0]);
        (void)close(pipeerrfds[0]);

        sigttou_block(1);
        if(tcsetpgrp(0, getpgid(0)) < 0) {
            sigttou_block(0);
            perror("tcsetpgrp");
            FREE(output_buf.mBuf);
            FREE(err_output_buf.mBuf);

            return FALSE;
        }
        sigttou_block(0);

        /// exited normally ///
        if(WIFEXITED(status)) {
            if(WEXITSTATUS(status) != 0) {
                *compile_error = TRUE;
                *output = ALLOC output_buf.mBuf;
                *err_output = ALLOC err_output_buf.mBuf;

                return TRUE;
            }
        }
        else if(WIFSTOPPED(status)) {
            fprintf(stderr, "%s", output_buf.mBuf);
            fprintf(stderr, "%s", err_output_buf.mBuf);
            fprintf(stderr, "signal interrupt. stopped. signal %d is gotten. \n", WTERMSIG(status));

            kill(pid, SIGKILL);
            pid = waitpid(pid, &status, WUNTRACED);

            FREE(output_buf.mBuf);
            FREE(err_output_buf.mBuf);

            return FALSE;
        }
        else if(WIFSIGNALED(status)) {
            fprintf(stderr, "%s", output_buf.mBuf);
            fprintf(stderr, "%s", err_output_buf.mBuf);
            fprintf(stderr, "signal interrupt. signal %d is gotten.\n", WTERMSIG(status));

            FREE(output_buf.mBuf);
            FREE(err_output_buf.mBuf);

            return FALSE;
        }

        *output = ALLOC output_buf.mBuf;
        *err_output = ALLOC err_output_buf.mBuf;
    }

    return TRUE;
}
*/

static BOOL load_code(sByteCode* code, sConst* constant, int* gv_var_num, int* max_stack, char* fname)
{
    int f;
    char buf[BUFSIZ];
    int int_buf[BUFSIZ];
    char c;
    int len;
    int n;
    int i;

    f = open(fname, O_RDONLY);

    /// magic number ///
    if(read(f, buf, 6) != 6) {
        close(f);
        return FALSE;
    }

    buf[6] = 0;

    if(strcmp(buf, "CLOVER") != 0) {
        close(f);
        return FALSE;
    }

    if(read(f, &c, 1) != 1 || c != 11) {
        close(f);
        return FALSE;
    }

    if(read(f, &c, 1) != 1 || c != 3) {
        close(f);
        return FALSE;
    }

    if(read(f, &c, 1) != 1 || c != 55) {
        close(f);
        return FALSE;
    }

    if(read(f, &c, 1) != 1 || c != 12) {
        close(f);
        return FALSE;
    }

    /// loaded class on compile time ///
    if(read(f, &n, sizeof(int)) != sizeof(int)) {
        close(f);
        return FALSE;
    }

    for(i=0; i<n; i++) {
        char* loaded_class;
        int len;
        char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
        sCLClass* klass;

        if(read(f, &len, sizeof(int)) != sizeof(int)) {
            close(f);
            return FALSE;
        }

        if(read(f, real_class_name, len) != len) {
            close(f);
            return FALSE;
        }

        /// load class ///
        klass = load_class_from_classpath(real_class_name, TRUE); // load before running Virtual Machine for Thread 

        if(klass == NULL) {
            fprintf(stderr, "can't load %s\n", real_class_name);
            close(f);
            return FALSE;
        }
    }

    /// byte code ///
    if(read(f, &len, sizeof(int)) != sizeof(int)) {
        close(f);
        return FALSE;
    }

    n = len;
    while(1) {
        int size;

        if(n < BUFSIZ) {
            size = read(f, int_buf, sizeof(int)*n);
        }
        else {
            size = read(f, int_buf, sizeof(int)*BUFSIZ);
        }

        if(size < 0 || size == 0) {
            break;
        }

        append_buf_to_bytecodes(code, int_buf, size/sizeof(int));
        n -= (size/sizeof(int));

        if(n <= 0) {
            break;
        }
    }

    /// constant ///
    if(read(f, &len, sizeof(int)) != sizeof(int)) {
        close(f);
        return FALSE;
    }

    n = len;
    while(1) {
        int size;

        if(n < BUFSIZ) {
            size = read(f, buf, n);
        }
        else {
            size = read(f, buf, BUFSIZ);
        }

        if(size < 0 || size == 0) {
            break;
        }

        append_buf_to_constant_pool(constant, buf, size);
        n -= size;

        if(n <= 0) {
            break;
        }
    }

    if(read(f, gv_var_num, sizeof(int)) != sizeof(int)) {
        close(f);
        return FALSE;
    }

    if(read(f, max_stack, sizeof(int)) != sizeof(int)) {
        close(f);
        return FALSE;
    }

    close(f);

    return TRUE;
}

BOOL cl_eval_file(char* file_name)
{
    sByteCode code;
    sConst constant;
    sVarTable gv_table;
    int max_stack;
    int gv_var_num;
    char compiled_file_name[PATH_MAX];

    /// make compiled file name ///
    xstrncpy(compiled_file_name, file_name, PATH_MAX-3);
    xstrncat(compiled_file_name, ".o", PATH_MAX);

    sByteCode_init(&code);
    sConst_init(&constant);

    if(!load_code(&code, &constant, &gv_var_num, &max_stack, compiled_file_name)) {
        fprintf(stderr, "load script file (%s) failure.\n", compiled_file_name);
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    if(!cl_main(&code, &constant, gv_var_num, max_stack, CL_STACK_SIZE)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

sBuf* gCLPrintBuffer;

int cl_print(char* msg, ...)
{
    char* msg2;
    int n;

    va_list args;
    va_start(args, msg);
    n = vasprintf(ALLOC &msg2, msg, args);
    va_end(args);

    if(gCLPrintBuffer) {                            // this is hook of all clover output
        sBuf_append(gCLPrintBuffer, msg2, n);
    }
    else {
        //printf("%s", msg2);
        //fflush(stdout);
        write(1, msg2, strlen(msg2));
    }

    free(msg2);

    return n;
}

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_class_field(sCLClass* klass, char* field_name, sCLClass* field_class, MVALUE* result)
{
    sCLField* field;
    CLObject object;
    CLObject type_object;
    sCLClass* klass2;

    field = get_field(klass, field_name, TRUE);

    if(field == NULL || (field->mFlags & CL_STATIC_FIELD) == 0) {
        return FALSE;
    }

    if(field_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
        *result = field->uValue.mStaticField;
    }
    else {
        object = get_object_from_mvalue(field->uValue.mStaticField);
        type_object = CLOBJECT_HEADER(object)->mType;
        klass2 = CLTYPEOBJECT(type_object)->mClass;

        /// type checking ///
        if(object && substitution_posibility_of_class(field_class, klass2)) {
            (*result).mObjectValue = object;
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_array_element(CLObject array, int index, sCLClass* element_class, MVALUE* result)
{
    if(index < 0 || index >= CLARRAY(array)->mLen) {
        return FALSE;
    }

    if(element_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
        *result = CLARRAY_ITEMS(array, index);
    }
    else {
        CLObject object;
        CLObject type;
        sCLClass* klass;

        object = get_object_from_mvalue(CLARRAY_ITEMS(array, index));
        type = CLOBJECT_HEADER(object)->mType;
        klass = CLTYPEOBJECT(type)->mClass;

        /// type checking ///
        if(object && substitution_posibility_of_class(element_class, klass)) {
            (*result).mObjectValue = object;
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}
