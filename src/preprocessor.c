#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

static BOOL compile_csource_and_get_output(sBuf* csource, sBuf* output, int argc, char* argv[])
{
    FILE* f;
    char file_name[PATH_MAX];
    char command[512];
    int num;
    
    while(1) {
        char* home;
        num = rand() % 9999;

        home = getenv("HOME");
        assert(home != NULL);

        snprintf(file_name, PATH_MAX, "%s/.clover/tmpfiles/clover_clang%d.c", home, num);
        
        if(access(file_name, F_OK) != 0) {
            FILE* f2;
            char buf[BUFSIZ];
            int i;
            
            f = fopen(file_name, "w");
            if(f == NULL) {
                parser_err_msg_without_line("fopen(3) is failed on #clang");
                return FALSE;
            }
            
            fprintf(f, "%s", csource->mBuf);
            (void)fclose(f);
            
            snprintf(command, 512, "/bin/gcc -o $HOME/.clover/tmpfiles/clover_clang%d $HOME/.clover/tmpfiles/clover_clang%d.c; $HOME/.clover/tmpfiles/clover_clang%d", num, num, num);
            
            for(i=0; i<argc; i++) {
                xstrncat(command, " ", 512);
                xstrncat(command, argv[i], 512);
            }
            
            f2 = popen(command, "r");
            if(f2 == NULL) {
                parser_err_msg_without_line("popen(2) is failed on #clang");
                
                snprintf(command, 512, "/bin/rm -f $HOME/.clover/tmpfiles/clover_clang%d.c $HOME/.clover/tmpfiles/clover_clang%d", num, num);
                system(command);
                return FALSE;
            }
            while(1) {
                int size;
                
                size = fread(buf, 1, BUFSIZ, f2);
                sBuf_append(output, buf, size);
                
                if(size < BUFSIZ) {
                    break;
                }
            }
            (void)pclose(f2);

            snprintf(command, 512, "/bin/rm -f $HOME/.clover/tmpfiles/clover_clang%d.c $HOME/.clover/tmpfiles/clover_clang%d", num, num);
            system(command);
            
            break;
        }
    }
    
    return TRUE;
}

static BOOL call_preprocessor(sBuf* command, sBuf* output)
{
    FILE* f;
    char buf[BUFSIZ];

    f = popen(command->mBuf, "r");
    if(f == NULL) {
        parser_err_msg_without_line("popen(2) is failed on #shell");
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
    (void)pclose(f);

    return TRUE;
}

BOOL preprocessor(sBuf* source, sBuf* source2)
{
    char* p;

    p = source->mBuf;

    while(*p) {
        if(*p == '\n' && *(p+1) == '#' || p == source->mBuf && *p == '#') {
            if(*p == '\n') {
                sBuf_append_char(source2, '\n');
                p+=2;
            }
            else {
                p++;
            }

            if(memcmp(p, "clang", 5) == 0) {
                sBuf csource;
                char* argv[PREPROCESSOR_ARG_NUM_MAX];
                int argc;
                sBuf arg;
                int i;

                p+=5;

                /// get arguments ///
                while(*p == ' ' || *p == '\t') p++;

                sBuf_init(&arg);
                argc = 0;

                while(1) {
                    if(*p == '\n') {
                        if(arg.mLen > 0) {
                            argv[argc] = MANAGED arg.mBuf;
                            argc++;
                            
                            if(argc >= PREPROCESSOR_ARG_NUM_MAX) {
                                parser_err_msg_without_line("Overflow argment number");
                                for(i=0; i<argc; i++) {
                                    FREE(argv[i]);
                                }
                                return FALSE;
                            }
                            
                            sBuf_init(&arg);
                        }
                        sBuf_append_char(source2, '\n');
                        break;
                    }
                    else if(*p == ' ' || *p == '\t') {
                        p++;
                        
                        if(arg.mLen > 0) {
                            argv[argc] = MANAGED arg.mBuf;
                            argc++;
                            
                            if(argc >= PREPROCESSOR_ARG_NUM_MAX) {
                                parser_err_msg_without_line("Overflow argment number");
                                for(i=0; i<argc; i++) {
                                    FREE(argv[i]);
                                }
                                return FALSE;
                            }
                            
                            sBuf_init(&arg);
                        }
                        
                        while(*p == ' ' || *p == '\t') p++;
                    }
                    else if(*p == 0) {
                        
                        parser_err_msg_without_line("Clover read out the source file before #endclang");
                        FREE(arg.mBuf);
                        for(i=0; i<argc; i++) {
                            FREE(argv[i]);
                        }
                        return FALSE;
                    }
                    else {
                        sBuf_append_char(&arg, *p);
                        p++;
                    }
                }

                /// read c source ///
                sBuf_init(&csource);

                while(1) {
                    if(memcmp(p, "#endclang", 9) == 0) {
                        p+=9;
                        break;
                    }
                    else if(*p == 0) {
                        
                        parser_err_msg_without_line("Clover read out the source file before #endclang");
                        for(i=0; i<argc; i++) {
                            FREE(argv[i]);
                        }
                        FREE(csource.mBuf);
                        return FALSE;
                    }
                    else if(*p == '\n') {
                        sBuf_append_char(&csource, *p);
                        sBuf_append_char(source2, *p);
                        p++;
                    }
                    else {
                        sBuf_append_char(&csource, *p);
                        p++;
                    }
                }

                /// compile ///
                if(!compile_csource_and_get_output(&csource, source2, argc, argv)) {
                    FREE(csource.mBuf);
                    for(i=0; i<argc; i++) {
                        FREE(argv[i]);
                    }
                    return FALSE;
                }

                for(i=0; i<argc; i++) {
                    FREE(argv[i]);
                }
                FREE(csource.mBuf);
            }
            else if(memcmp(p, "preprocessor", 12) == 0) {
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
                        parser_err_msg_without_line("Clover reads out the source file before #endpreprocessor");
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
