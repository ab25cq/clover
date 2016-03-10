#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct sPreprocessorFunctionStruct {
    char* mName;
    sBuf mSource;
};

typedef struct sPreprocessorFunctionStruct sPreprocessorFunction;

sPreprocessorFunction gPreprocessorFunctions[CL_PREPROCESSOR_FUN_MAX];

static void preprocessor_init()
{
    memset(gPreprocessorFunctions, 0, sizeof(sPreprocessorFunction)*CL_PREPROCESSOR_FUN_MAX);
}

static void preprocessor_final()
{
    int i;
    for(i=0; i<CL_PREPROCESSOR_FUN_MAX; i++) {
        sPreprocessorFunction* func;

        func = gPreprocessorFunctions + i;

        if(func->mName) {
            MFREE(func->mName);
            MFREE(func->mSource.mBuf);
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
            func->mName = MSTRDUP(name);
            func->mSource = *fun_source;
            break;
        }
        else {
            func++;

            if(func == gPreprocessorFunctions + CL_PREPROCESSOR_FUN_MAX) {
                func = gPreprocessorFunctions;
            }
            else if(func == gPreprocessorFunctions + hash_value) {
                MFREE(fun_source->mBuf);
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
        fprintf(stderr, "popen(2) is failed on #preprocessor\n");
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
        fprintf(stderr, "pclose(2) is failed on #preprocessor\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL preprocessor(sBuf* source, sBuf* source2)
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
                        fprintf(stderr, "Clover reads out the source file before #endpreprocessor\n");
                        MFREE(command.mBuf);
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
                    MFREE(command.mBuf);
                    return FALSE;
                }

                MFREE(command.mBuf);
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
                            fprintf(stderr, "Overflow preprocessor function name\n");
                            return FALSE;
                        }
                    }

                    *p2 = 0;
                }
                else {
                    fprintf(stderr, "Clover expects alphabet character, but this is %c\n", *p);
                    return FALSE;
                }

                while(*p == ' ' || *p == '\t') p++;

                if(*p == '\n') {
                    sBuf_append_char(source2, *p);
                    p++;
                }
                else {
                    fprintf(stderr, "Clover expects a line field, but this is %c\n", *p);
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
                        fprintf(stderr, "Clover reads out the source file before #enddef\n");
                        MFREE(fun_source.mBuf);
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
                    fprintf(stderr, "Overflow preprocessor function table\n");
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
                            fprintf(stderr, "Overflow preprocessor function name\n");
                            return FALSE;
                        }
                    }

                    *p2 = 0;
                }
                else {
                    fprintf(stderr, "Clover expects alphabet character, but this is %c\n", *p);
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
                                fprintf(stderr, "Clover reads out the source before #endcall");
                                for(i=0; i<num_argments; i++) {
                                    MFREE(argments[i]);
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
                                    fprintf(stderr, "overflow preprocessor function argments length");
                                    for(i=0; i<num_argments; i++) {
                                        MFREE(argments[i]);
                                    }
                                    return FALSE;
                                }
                            }
                            else {
                                *p2++ = *p++;

                                if(p2 - argment >= CL_PREPROCESSOR_FUN_ARGMENT_LENGTH_MAX)
                                {
                                    fprintf(stderr, "overflow preprocessor function argments length");
                                    for(i=0; i<num_argments; i++) {
                                        MFREE(argments[i]);
                                    }
                                    return FALSE;
                                }
                            }
                        }

                        *p2 = 0;

                        argments[num_argments] = MSTRDUP(argment);
                        num_argments++;

                        if(num_argments > CL_PREPROCESSOR_FUN_ARGUMENTS_NUM) {
                            fprintf(stderr, "Overflow preprocessor function argment number");
                            for(i=0; i<num_argments; i++) {
                                MFREE(argments[i]);
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
                        fprintf(stderr, "Clover reads out the source file before #endcall\n");

                        for(i=0; i<num_argments; i++) {
                            MFREE(argments[i]);
                        }

                        MFREE(block.mBuf);
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
                    fprintf(stderr, "Clover can't find out the preprocessor function of this name(%s)\n", name);

                    for(i=0; i<num_argments; i++) {
                        MFREE(argments[i]);
                    }

                    MFREE(block.mBuf);
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
                if(f == NULL) {
                    fprintf(stderr, "Clover can't open %s\n", file_name);
                    exit(2);
                }
                fprintf(f, "%s", block.mBuf);
                fclose(f);

                setenv("BLOCK_FILE", file_name, 1);

                /// do call ///
                if(!call_preprocessor(&func->mSource, source2)) {
                    int i;

                    for(i=0; i<num_argments; i++) {
                        MFREE(argments[i]);
                    }

                    MFREE(block.mBuf);
                    unlink(file_name);
                    return FALSE;
                }

                for(i=0; i<num_argments; i++) {
                    MFREE(argments[i]);
                }

                MFREE(block.mBuf);
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

BOOL delete_comment(sBuf* source, sBuf* source2)
{
    char* p;
    BOOL in_string;

    p = source->mBuf;

    in_string = FALSE;

    while(*p) {
        if(*p == '"') {
            in_string = !in_string;
            sBuf_append_char(source2, *p);
            p++;
        }
        else if(!in_string && ((p == source->mBuf && *p =='/' && *(p+1) == '/')
            || ((*p =='\t' || *p == '\n' || *p == '\r' || *p ==' ') && *(p+1) == '/' && *(p+2) == '/')))
        {
            if(*p == '\n') {
                sBuf_append_char(source2, '\n');   // no delete line field for error message
            }

            if(p == source->mBuf) {
                p+=2;
            }
            else {
                p+=3;
            }

            while(1) {
                if(*p == 0) {
                    break;
                }
                else if(*p == '\n') {
                    //p++;      // no delete line field for error message
                    break;
                }
                else {
                    p++;
                }
            }
        }
        else if(!in_string && *p == '/' && *(p+1) == '*') {
            int nest;

            p+=2;
            nest = 0;
            while(1) {
                if(*p == '"') {
                    p++;
                    in_string = !in_string;
                }
                else if(*p == 0) {
                    fprintf(stderr, "there is not a comment end until source end\n");
                    return FALSE;
                }
                else if(!in_string && *p == '/' && *(p+1) == '*') {
                    p+=2;
                    nest++;
                }
                else if(!in_string && *p == '*' && *(p+1) == '/') {
                    p+=2;
                    if(nest == 0) {
                        break;
                    }

                    nest--;
                }
                else if(*p == '\n') {
                    sBuf_append_char(source2, *p);   // no delete line field for error message
                    p++;
                }
                else {
                    p++;
                }
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}

static BOOL preprocess_source(char* sname, char* output_sname)
{
    sBuf source, source2, source3;
    int f;
    FILE* f2;

    /// load source ///
    f = open(sname, O_RDONLY);

    if(f < 0) {
        fprintf(stderr, "Clover can't open %s\n", sname);
        return FALSE;
    }

    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size;

        size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    close(f);

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        return FALSE;
    }

    /// preprocessor ///
    sBuf_init(&source3);

    if(!preprocessor(&source2, &source3)) {
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        MFREE(source3.mBuf);
        return FALSE;
    }

    /// write output ///
    f2 = fopen(output_sname, "w");

    if(f2 == NULL) {
        fprintf(stderr, "Clover can't open %s\n", output_sname);
        MFREE(source.mBuf);
        MFREE(source2.mBuf);
        MFREE(source3.mBuf);
        return FALSE;
    }
    
    fprintf(f2, "%s", source3.mBuf);

    fclose(f2);
    
    MFREE(source.mBuf);
    MFREE(source2.mBuf);
    MFREE(source3.mBuf);

    return TRUE;
}

static char* extname(char* file_name)
{
    char* p;

    p = file_name + strlen(file_name);

    while(p >= file_name) {
        if(*p == '.') {
            return p + 1;
        }
        else {
            p--;
        }
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    int i;

    srandom((unsigned)time(NULL));

    setlocale(LC_ALL, "");

    preprocessor_init();

    if(!cl_init(1024, 512, argc, argv)) {
        exit(1);
    }

    if(!cl_load_fundamental_classes()) {
        fprintf(stderr, "can't load fundamental class\n");
        exit(1);
    }

    for(i=1; i<argc; i++) {
        char* extension;

        extension = extname(argv[i]);

        if(strcmp(extension, "in") == 0) {
            char output_sname[PATH_MAX];
            int len;

            len = strlen(argv[i]) - strlen(extension) -1;
            memcpy(output_sname, argv[i], len);
            output_sname[len] = 0;

            setenv("SOURCE", argv[i], 1);
            
            if(!preprocess_source(argv[i], output_sname)) {
                preprocessor_final();
                exit(2);
            }
        }
    }

    cl_final();
    preprocessor_final();
    exit(0);
}
