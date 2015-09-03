#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>

struct sPreprocessorFunctionStruct {
    char* mName;
    sBuf mSource;
};

typedef struct sPreprocessorFunctionStruct sPreprocessorFunction;

sPreprocessorFunction gPreprocessorFunctions[CL_PREPROCESSOR_FUN_MAX];

void preprocessor_init()
{
    memset(gPreprocessorFunctions, 0, sizeof(sPreprocessorFunction)*CL_PREPROCESSOR_FUN_MAX);
}

void preprocessor_final()
{
    int i;
    for(i=0; i<CL_PREPROCESSOR_FUN_MAX; i++) {
        sPreprocessorFunction* func;

        func = gPreprocessorFunctions + i;

        if(func->mName) {
            FREE(func->mName);
            FREE(func->mSource.mBuf);
        }
    }
}

static BOOL add_preprocessor(char* name, MANAGED sBuf* fun_source)
{
    unsigned int hash_value;
    sPreprocessorFunction* func;

    hash_value = get_hash(name) % CL_PREPROCESSOR_FUN_MAX;

    func = gPreprocessorFunctions + hash_value;

    while(1) {
        if(func->mName == NULL) {
            func->mName = STRDUP(name);
            func->mSource = *fun_source;
            break;
        }
        else {
            func++;

            if(func == gPreprocessorFunctions + CL_PREPROCESSOR_FUN_MAX) {
                func = gPreprocessorFunctions;
            }
            else if(func == gPreprocessorFunctions + hash_value) {
                FREE(fun_source->mBuf);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static sPreprocessorFunction* get_func(char* name)
{
    unsigned int hash_value;
    sPreprocessorFunction* func;

    hash_value = get_hash(name) % CL_PREPROCESSOR_FUN_MAX;
    func = gPreprocessorFunctions + hash_value;

    while(1) {
        if(func->mName && name && strcmp(func->mName, name) == 0) {
            break;
        }
        else {
            func++;

            if(func == gPreprocessorFunctions + CL_PREPROCESSOR_FUN_MAX) {
                func = gPreprocessorFunctions;
            }
            else if(func == gPreprocessorFunctions + hash_value) {
                return NULL;
            }
        }
    }

    return func;
}

static BOOL call_preprocessor(sBuf* command, sBuf* output)
{
    FILE* f;
    char buf[BUFSIZ];

    f = popen(command->mBuf, "r");
    if(f == NULL) {
        parser_err_msg_without_line("popen(2) is failed on #preprocessor\n");
        return FALSE;
    }
    while(1) {
        int size;

        size = fread(buf, 1, BUFSIZ, f);
        sBuf_append(output, buf, size);

        if(size < BUFSIZ) {
            break;
        }
    }
    if(pclose(f) < 0) {
        parser_err_msg_without_line("pclose(2) is failed on #preprocessor\n");
        return FALSE;
    }

    return TRUE;
}

BOOL preprocessor(sBuf* source, sBuf* source2)
{
    char* p;
    BOOL dquort;

    p = source->mBuf;

    dquort = FALSE;

    while(*p) {
        if(*p == '"') {
            dquort = !dquort;
            sBuf_append_char(source2, *p);
            p++;
        }
        else if(!dquort && ((*p == '\n' && *(p+1) == '#') || (p == source->mBuf && *p == '#'))) 
        {
            if(*p == '\n') {
                sBuf_append_char(source2, '\n');
                p+=2;
            }
            else {
                p++;
            }

            if(memcmp(p, "preprocessor", 12) == 0) {
                sBuf command;

                p+=12;

                /// read one line ///
                sBuf_init(&command);

                while(1) {
                    if(memcmp(p, "#endpreprocessor", 16) == 0) {
                        p+=16;
                        break;
                    }
                    else if(*p == 0) {
                        parser_err_msg_without_line("Clover reads out the source file before #endpreprocessor\n");
                        FREE(command.mBuf);
                        return FALSE;
                    }
                    else if(*p == '\n') {
                        sBuf_append_char(source2, *p);
                        sBuf_append_char(&command, *p);
                        p++;
                    }
                    else {
                        sBuf_append_char(&command, *p);
                        p++;
                    }
                }

                /// compile ///
                if(!call_preprocessor(&command, source2)) {
                    FREE(command.mBuf);
                    return FALSE;
                }

                FREE(command.mBuf);
            }
            else if(memcmp(p, "def", 3) == 0) {
                sBuf fun_source;
                char name[CL_PREPROCESSOR_FUN_NAME_MAX+1];
                char* p2;

                p+=3;
                while(*p == ' ' || *p == '\t') p++;

                p2 = name;

                if(isalpha(*p)) {
                    while(isalnum(*p) || *p == '_') {
                        *p2++ = *p++;

                        if(p2 - name >= CL_PREPROCESSOR_FUN_NAME_MAX) {
                            parser_err_msg_without_line("Overflow preprocessor function name\n");
                            return FALSE;
                        }
                    }

                    *p2 = 0;
                }
                else {
                    parser_err_msg_without_line("Clover expects alphabet character, but this is %c\n", *p);
                    return FALSE;
                }

                while(*p == ' ' || *p == '\t') p++;

                if(*p == '\n') {
                    sBuf_append_char(source2, *p);
                    p++;
                }
                else {
                    parser_err_msg_without_line("Clover expects a line field, but this is %c\n", *p);
                    return FALSE;
                }

                /// read function source ///
                sBuf_init(&fun_source);

                while(1) {
                    if(memcmp(p, "#enddef", 7) == 0) {
                        p+=7;
                        break;
                    }
                    else if(*p == 0) {
                        parser_err_msg_without_line("Clover reads out the source file before #enddef\n");
                        FREE(fun_source.mBuf);
                        return FALSE;
                    }
                    else if(*p == '\n') {
                        sBuf_append_char(source2, *p);
                        sBuf_append_char(&fun_source, *p);
                        p++;
                    }
                    else {
                        sBuf_append_char(&fun_source, *p);
                        p++;
                    }
                }

                /// add preprocessor function to the table ///
                if(!add_preprocessor(name, MANAGED &fun_source)) {
                    parser_err_msg_without_line("Overflow preprocessor function table\n");
                    return FALSE;
                }
            }
            else if(memcmp(p, "call", 4) == 0) {
                char name[CL_PREPROCESSOR_FUN_NAME_MAX+1];
                char* p2;
                char* argments[CL_PREPROCESSOR_FUN_ARGUMENTS_NUM+1];
                int num_argments;
                sBuf block;
                sPreprocessorFunction* func;
                int i;
                FILE* f;
                char file_name[PATH_MAX];
                char* home;

                p+=4;

                while(*p == ' ' || *p == '\t') { p++; }

                /// get name ///
                p2 = name;
                if(isalpha(*p)) {
                    while(isalnum(*p) || *p == '_') {
                        *p2++ = *p++;
                        if(p2 - name >= CL_PREPROCESSOR_FUN_NAME_MAX) {
                            parser_err_msg_without_line("Overflow preprocessor function name\n");
                            return FALSE;
                        }
                    }

                    *p2 = 0;
                }
                else {
                    parser_err_msg_without_line("Clover expects alphabet character, but this is %c\n", *p);
                    return FALSE;
                }

                while(*p == ' ' || *p == '\t') { p++; }

                /// get argments ///
                memset(argments, 0, sizeof(argments));
                num_argments = 0;

                while(1) {
                    char* p2;
                    char argment[CL_PREPROCESSOR_FUN_ARGMENT_LENGTH_MAX+1];

                    while(*p == ' ' || *p == '\t') { p++; }

                    if(*p == '\n') {
                        sBuf_append_char(source2, *p);
                        p++;
                        break;
                    }
                    else { // *p != ' ' && *p != '\t' && *p != '\n'
                        p2 = argment;

                        while(1) {
                            if(*p == 0) {
                                parser_err_msg_without_line("Clover reads out the source before #endcall");
                                for(i=0; i<num_argments; i++) {
                                    FREE(argments[i]);
                                }
                                return FALSE;
                            }
                            else if(*p == ' ' || *p == '\t' || *p == '\n') {
                                break;
                            }
                            else if(*p == '\\') {
                                p++;
                                if(*p == '\n') {
                                    sBuf_append_char(source2, *p);
                                }
                                *p2++ = *p++;

                                if(p2 - argment >= CL_PREPROCESSOR_FUN_ARGMENT_LENGTH_MAX)
                                {
                                    parser_err_msg_without_line("overflow preprocessor function argments length");
                                    for(i=0; i<num_argments; i++) {
                                        FREE(argments[i]);
                                    }
                                    return FALSE;
                                }
                            }
                            else {
                                *p2++ = *p++;

                                if(p2 - argment >= CL_PREPROCESSOR_FUN_ARGMENT_LENGTH_MAX)
                                {
                                    parser_err_msg_without_line("overflow preprocessor function argments length");
                                    for(i=0; i<num_argments; i++) {
                                        FREE(argments[i]);
                                    }
                                    return FALSE;
                                }
                            }
                        }

                        *p2 = 0;

                        argments[num_argments] = STRDUP(argment);
                        num_argments++;

                        if(num_argments > CL_PREPROCESSOR_FUN_ARGUMENTS_NUM) {
                            parser_err_msg_without_line("Overflow preprocessor function argment number");
                            for(i=0; i<num_argments; i++) {
                                FREE(argments[i]);
                            }
                            return FALSE;
                        }
                    }
                }

                while(*p == ' ' || *p == '\t') p++;

                /// get block ///
                sBuf_init(&block);

                while(1) {
                    if(memcmp(p, "#endcall", 8) == 0) {
                        p+=8;
                        break;
                    }
                    else if(*p == 0) {
                        parser_err_msg_without_line("Clover reads out the source file before #endcall\n");

                        for(i=0; i<num_argments; i++) {
                            FREE(argments[i]);
                        }

                        FREE(block.mBuf);
                        return FALSE;
                    }
                    else if(*p == '\n') {
                        sBuf_append_char(source2, *p);
                        sBuf_append_char(&block, *p);
                        p++;
                    }
                    else {
                        sBuf_append_char(&block, *p);
                        p++;
                    }
                }

                /// run the preprocessor function ///
                func = get_func(name);

                if(func == NULL) {
                    parser_err_msg_without_line("Clover can't find out the preprocessor function of this name(%s)\n", name);

                    for(i=0; i<num_argments; i++) {
                        FREE(argments[i]);
                    }

                    FREE(block.mBuf);
                    return FALSE;
                }

                /// set argment and block to environment variable and file ///
                for(i=0; i<num_argments; i++) {
                    char env_name[32];

                    snprintf(env_name, 32, "PARAM%d", i);

                    setenv(env_name, argments[i], 1);
                }

                home = getenv("HOME");

                assert(home != NULL);

                while(1) {
                    int value;

                    value = rand() % 9999;
                    snprintf(file_name, PATH_MAX, "%s/.clover/tmpfiles/preprocessor_block%d", home, value);
                    if(access(file_name, F_OK) != 0) {
                        break;
                    }
                }

                f = fopen(file_name, "w");
                fprintf(f, "%s", block.mBuf);
                fclose(f);

                setenv("BLOCK_FILE", file_name, 1);

                /// do call ///
                if(!call_preprocessor(&func->mSource, source2)) {
                    int i;

                    for(i=0; i<num_argments; i++) {
                        FREE(argments[i]);
                    }

                    FREE(block.mBuf);
                    unlink(file_name);
                    return FALSE;
                }

                for(i=0; i<num_argments; i++) {
                    FREE(argments[i]);
                }

                FREE(block.mBuf);
                unlink(file_name);
            }
            else {
                sBuf_append_char(source2, '#');
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}
