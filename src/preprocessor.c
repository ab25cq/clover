#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

static BOOL compile_csource_and_get_output(sBuf* csource, sBuf* output)
{
    FILE* f;
    char file_name[PATH_MAX];
    char command[128];
    int num;

    while(1) {
        num = rand() % 9999;
        snprintf(file_name, PATH_MAX, "/tmp/clover_clang%d.c", num);

        if(access(file_name, F_OK) != 0) {
            FILE* f2;
            char buf[BUFSIZ];

            f = fopen(file_name, "w");
            if(f == NULL) {
                parser_err_msg_without_line("fopen(3) is failed on #clang");
                return FALSE;
            }

            fprintf(f, "%s", csource->mBuf);
            (void)fclose(f);

            snprintf(command, 128, "/bin/gcc -o /tmp/clover_clang%d /tmp/clover_clang%d.c; /tmp/clover_clang%d", num, num, num);

            f2 = popen(command, "r");
            if(f2 == NULL) {
                parser_err_msg_without_line("popen(2) is failed on #clang");

                snprintf(command, 128, "/bin/rm -f /tmp/clover_clang%d.c", num);
                system(command);

                snprintf(command, 128, "/bin/rm -f /tmp/clover_clang%d", num);
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

            snprintf(command, 128, "/bin/rm -f /tmp/clover_clang%d.c", num);
            system(command);

            snprintf(command, 128, "/bin/rm -f /tmp/clover_clang%d", num);
            system(command);
            break;
        }
    }

    return TRUE;
}

BOOL preprocessor(sBuf* source, sBuf* source2)
{
    char* p;

    p = source->mBuf;

    while(*p) {
        if(*p == '#') {
            p++;

            if(memcmp(p, "clang", 5) == 0) {
                sBuf csource;

                p+=5;

                /// read c source ///
                sBuf_init(&csource);

                while(1) {
                    if(memcmp(p, "#endclang", 9) == 0) {
                        p+=9;
                        break;
                    }
                    else if(*p == 0) {
                        parser_err_msg_without_line("Clover read out the source file before #endclang");
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
                if(!compile_csource_and_get_output(&csource, source2)) {
                    FREE(csource.mBuf);
                    return FALSE;
                }

                FREE(csource.mBuf);
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}
